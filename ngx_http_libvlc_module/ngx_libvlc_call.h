#ifndef NGX_LIBVLC_CALL_H
#define NGX_LIBVLC_CALL_H

#include <vlc/vlc.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
	char* request_id;
	char* prefix_location;
	char* prefix_url;
} hls_transcode_option;


typedef struct {
   libvlc_media_player_t **mp;
   libvlc_media_t **m;
   libvlc_instance_t **inst;
} callback_media_data;

extern int ngx_libvlc_hls_convert(char* uri, hls_transcode_option* option , ngx_str_t res);

#endif 