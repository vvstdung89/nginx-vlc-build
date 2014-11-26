
#include "ngx_libvlc_call.h"
#include "ngx_libvlc_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

pthread_t leak_memory_collect;

void* doGarbageCollect(void *arg)
{
	callback_media_data *data = (callback_media_data *) arg;
    sleep(5);

	// stop playing
  libvlc_media_player_stop((libvlc_media_player_t *) *data->mp);

	//free the media_player
	libvlc_media_player_release((libvlc_media_player_t *) *data->mp);

	libvlc_release((libvlc_instance_t *) *data->inst);

  free(data);
  return NULL;
}

//callback when media end reach
void hls_callback(const libvlc_event_t* event, void* ptr){
	callback_media_data *data = (callback_media_data *) ptr;
	switch ( event->type )	{
		case libvlc_MediaPlayerEndReached:
			//create a thread which dealy 5s before free up everything
      pthread_create(&(leak_memory_collect), NULL, &doGarbageCollect, data);
    break;

	}
}

/*
* Function used to convert a uri to x.264 hls format
* Take input link, hls_transcode_option 
* Return the streaming link (res)
*/
int ngx_libvlc_hls_convert(char* uri, hls_transcode_option* option, ngx_str_t res) {

  //declare a libvlc instance, media , and media player instance
	libvlc_instance_t *inst;
  libvlc_media_player_t *mp;
  libvlc_media_t *m;

  //callback_media_data used to store the above instance (for garabage collection thread)
	callback_media_data *cdata = (callback_media_data*) malloc( sizeof(callback_media_data*) );

  //output option
  char *smem_options = malloc( 1000*sizeof(char*) );
  //index file location
  char *index_location = malloc( 500*sizeof(char*) ); 
  //index url (streaming link)
  char *index_url =  malloc( 500*sizeof(char*) );
  //libvlc log file
	char *log_file = malloc( 100*sizeof(char*) );
  //ts file location
  char *ts_location = malloc( 500*sizeof(char*) ); 
  //tmp parameter to store some folder location
	char *store_folder = malloc( 100*sizeof(char*) );



  //intialize data with \0'
  memset(smem_options, '\0',1000);
  memset(index_location, '\0',500);
  memset(index_url, '\0',500);
  memset(ts_location, '\0',500);
  memset(log_file, '\0',100);
  memset(store_folder, '\0',100);

  //check if the location to store data file is created, if not create that folder
	struct stat st;
	sprintf(store_folder, "%s/%s",option->prefix_location,option->request_id);
	if (stat(store_folder, &st) == -1) {
		    mkdir(store_folder, 0777);
	}
  //check if log folder is created
  memset(store_folder, '\0',100);
  sprintf(store_folder, "/app/logs/libvlc");
  if (stat(store_folder, &st) == -1) {
        mkdir(store_folder, 0777);
  }

  //construction parameter    
	sprintf(index_location, "%s/%s/list.m3u8", option->prefix_location,option->request_id);
	sprintf(index_url, "%s/%s/data-#########.ts", option->prefix_url,option->request_id);
	sprintf(ts_location, "%s/%s/data-#########.ts", option->prefix_location,option->request_id);
  sprintf(log_file,"/app/logs/libvlc/%s.log",option->request_id);

  sprintf(smem_options
  , "#transcode { vcodec=h264,vb=256,scale=1,acodec=none,venc=x264{aud,profile=baseline,level=30,keyint=15, ref=1, preset=fast }}:std{access=livehttp{seglen=5, numsegs=20, delsegs=true,index=%s,index-url=%s},mux=ts{use-key-frames},dst=%s}"
  , index_location //index file location-> access through web service ex: /var/www/html/hls/f8ee546866a511e4a73f22000b0f0a0c/list.m3u8
  , index_url //http://127.0.0.1/f8ee546866a511e4a73f22000b0f0a0c/data-#########.ts
  , ts_location  //ts file location-> access through web service ex: /var/www/html/hls/f8ee546866a511e4a73f22000b0f0a0c/data-#########.ts
  );

  const char * const vlc_args[] = {
            "-I", "dummy", // Don't use any interface
            "--ignore-config", // Don't use VLC's config
            "vlc://quit", // 
            "-vvv", // Be verbose,
            "--file-logging", //enable logfile
            "--logfile", log_file, 
            "--sout", smem_options // Stream to memory
  };
 
  //create vlc instance with args
  inst = libvlc_new( sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
    
  // create a new item
  m = libvlc_media_new_location(inst, uri);

  // create a media play playing environment
  mp = libvlc_media_player_new_from_media(m);

  //event manager 
  libvlc_event_manager_t* p_em = libvlc_media_player_event_manager( mp );
  //store media related instance
  cdata->mp=&mp;
  cdata->m=&m;
  cdata->inst=&inst;
  //register to event end reach
  libvlc_event_attach(p_em, libvlc_MediaPlayerEndReached, hls_callback, (void *)cdata );

  // no need to keep the media now
  libvlc_media_release(m);

  // play the media_player
  libvlc_media_player_play(mp);

  // wait for index file (m3u8) present
  int counter = 0;
  while (counter++ <= 60) { // Time out set to 60 seconds
  	sleep(1);
  	//Check file present
    if (doesFileExist(index_location)){
    	
      //return streaming link
    	sprintf((char*)res.data,"%s/%s/list.m3u8",option->prefix_url,option->request_id);

      //free up memory
      free(smem_options);
      free(index_location);
      free(index_url);
      free(ts_location);
      free(log_file);
      free(store_folder);


    	return 0;
    }
  }

    // stop playing
    libvlc_media_player_stop(mp);

    // free the media_player
    libvlc_media_player_release(mp);

    //free libvlc
    libvlc_release(inst);

    //return time out string
    sprintf((char*)res.data,"Request time out");

    //free up memory
    free(smem_options);
    free(index_location);
    free(index_url);
    free(ts_location);
    free(log_file);
    free(store_folder);
	return -1;
}
