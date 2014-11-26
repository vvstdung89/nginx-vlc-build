#ifndef NGX_LIBVLC_UTILS_H
#define NGX_LIBVLC_UTILS_H


extern int doesFileExist(const char *filename);

extern int run_getenv(const char * name, char* value);

extern int generateGUID(char* guid);


#endif 