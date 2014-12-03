#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "string.h"
#include "ngx_libvlc_call.h"
#include "ngx_libvlc_utils.h"

static char *ngx_http_libvlc(ngx_conf_t *cf, void *post, void *data);

static ngx_conf_post_handler_pt ngx_http_libvlc_p = ngx_http_libvlc;

/*
 * The structure will holds the value of the 
 * module directive libvlc
 */
typedef struct {
    ngx_str_t   name;
} ngx_http_libvlc_loc_conf_t;

/* The function which initializes memory for the module configuration structure       
 */
static void *
ngx_http_libvlc_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_libvlc_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_libvlc_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }
    
    return conf;
}

/* 
 * The command array or array, which holds one subarray for each module 
 * directive along with a function which validates the value of the 
 * directive and also initializes the main handler of this module
 */
static ngx_command_t ngx_http_libvlc_commands[] = {
    { 
      ngx_string("transcode"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_libvlc_loc_conf_t, name),
      &ngx_http_libvlc_p },
 
    ngx_null_command
};
 
 
static ngx_str_t trancode_link_respond;
 
/*
 * The module context has hooks , here we have a hook for creating
 * location configuration
 */
static ngx_http_module_t ngx_http_libvlc_module_ctx = {
    NULL,                          /* preconfiguration */
    NULL,                          /* postconfiguration */
 
    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */
 
    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */
 
    ngx_http_libvlc_create_loc_conf, /* create location configuration */
    NULL                           /* merge location configuration */
};
 

/*
 * The module which binds the context and commands 
 * 
 */
ngx_module_t ngx_http_libvlc_module = {
    NGX_MODULE_V1,
    &ngx_http_libvlc_module_ctx,    /* module context */
    ngx_http_libvlc_commands,       /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};
 
/*
 * Main handler function of the module. 
 */
static ngx_int_t
ngx_http_libvlc_handler(ngx_http_request_t *r)
{
    printf("Enter libvlc handler ...\n");
    ngx_int_t    rc;
 	
 	

    /* we response to 'GET' and 'HEAD' requests only */
    if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
        return NGX_HTTP_NOT_ALLOWED;
    }
 
    /* discard request body, since we don't need it here */
    rc = ngx_http_discard_request_body(r);
 
    if (rc != NGX_OK) {
        return rc;
    }
 
   
       
    //the buffer to store request url (input video)
    char *uri = malloc(500*sizeof(char*));
    

    //the buffer to store the output link
    trancode_link_respond.data = malloc(100*sizeof(char*));

    //transcode option which will be the parameter for libvlc_call
    hls_transcode_option*   transcode_option  = (hls_transcode_option*) malloc(sizeof(hls_transcode_option));

    //request_id: unique id for each request
    transcode_option->request_id = (char *)malloc(100*sizeof(char *));

    //prefix_location: the prefix location to store m3u8 file. ex: /var/www/html/hls
	transcode_option->prefix_location = (char*) malloc(100*sizeof(char*));

    //prefix_url: the prefix url for user to streaming. ex: http://www.example.com/hls
	transcode_option->prefix_url = (char*) malloc(100*sizeof(char*));

    //initial all data with \0'
    memset(transcode_option->request_id, '\0',100);
    memset(transcode_option->prefix_location, '\0',100);
    memset(transcode_option->prefix_url, '\0', 100);
    memset(uri, '\0',500);
    memset(trancode_link_respond.data, '\0',100);

    //generateGGUI using hasing algorithm
	generateGUID(transcode_option->request_id);

    //retrieve transcode option data from environment
	run_getenv("PREFIX_LOCATION", transcode_option->prefix_location);
	run_getenv("PREFIX_URL", transcode_option->prefix_url);
	
    
    //Check if the parameter is start with http link
    if (strncmp((char* )r->uri.data, "/transcode/hls/http:/", 21) == 0){ //trancode request match http
    	
        //construct the URL
        strcpy(uri,"http://");
    	memcpy(uri+7, r->uri.data+21,r->uri.len-21);
    	
    	//start to call transcode function with: input url, transcode option 
        //return value will be trancode_link_respond
    	ngx_libvlc_hls_convert(uri, transcode_option, trancode_link_respond) ;

    } else { //the request is not in correct format
    	sprintf((char *)trancode_link_respond.data,"The request is not valid. Please use format http://path/video.file");
    }

    trancode_link_respond.len = ngx_strlen(trancode_link_respond.data);
        
    r->headers_out.status = NGX_HTTP_MOVED_PERMANENTLY; 

    r->headers_out.location = ngx_list_push(&r->headers_out.headers);
    if (r->headers_out.location == NULL) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
    }
    r->headers_out.location->hash = 1;
    ngx_str_set(&r->headers_out.location->key, "Location");
    r->headers_out.location->value.len = trancode_link_respond.len; 
    r->headers_out.location->value.data = (u_char *) trancode_link_respond.data;

    free(uri);
    free(transcode_option->request_id);
    free(transcode_option->prefix_location);
    free(transcode_option->prefix_url);
    free(transcode_option);

    /* send the headers of your response */
    return NGX_HTTP_MOVED_PERMANENTLY;
}
 
/*
 * Function for the directive trasncode , it validates its value
 * and copies it to a static variable to be printed later
 */
static char *
ngx_http_libvlc(ngx_conf_t *cf, void *post, void *data)
{
    ngx_http_core_loc_conf_t *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_libvlc_handler;

    // ngx_str_t  *name = data; // i.e., first field of ngx_http_libvlc_loc_conf_t
    
    // if (ngx_strcmp(name->data, "") == 0) {
    //     return NGX_CONF_ERROR;
    // }
    // printf("%s",name->data);
    // hello_string.data = name->data;
    // hello_string.len = ngx_strlen(hello_string.data);

    return NGX_CONF_OK;
}