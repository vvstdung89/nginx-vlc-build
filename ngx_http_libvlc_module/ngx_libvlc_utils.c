
#include "ngx_libvlc_utils.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
int
run_getenv (const char * name,char* result)
{
 	char* value = getenv (name);
    
    if (! value) {
        printf ("'%s' is not set.\n", name);
        return -1;
    }
    else {
        printf ("%s = %s size %d %d\n", name, value,(int)strlen(value),(int)sizeof(result));
        memset(result, '\0', 100);
        strcpy(result,value);
        return 0;
    }

}

int
doesFileExist(const char *filename) {
    struct stat st;
    int result = stat(filename, &st);
    return result == 0;
}


int
generateGUID(char* GUID){
	struct timeval tv;
	// struct timezone tz;
	gettimeofday (&tv,0);
	srand ((unsigned int)tv.tv_sec);
	int t = 0;
	char *szTemp = "xxxxxxxxxxxx4xxxyxxxxxxxxxxxxxxx";
	char *szHex = "0123456789ABCDEF-";
	int nLen = strlen (szTemp);

	for (t=0; t<nLen+1; t++)
	{
	    int r = rand () % 16;
	    char c = ' ';   

	    switch (szTemp[t])
	    {
	        case 'x' : { c = szHex [r]; } break;
	        case 'y' : { c = szHex [(r & 0x03) | 0x08]; } break;
	        case '-' : { c = '-'; } break;
	        case '4' : { c = '4'; } break;
	    }

	    GUID[t] = ( t < nLen ) ? c : 0x00;
	}
	
	// printf ("%s\r\n", GUID);
	return 0;
}