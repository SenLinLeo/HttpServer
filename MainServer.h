#ifndef _server_h_
#define _server_h_

#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <netdb.h>
#include <dirent.h>
#include <time.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

// #define _GNU_SOURCE
#include <getopt.h>



/* constant define */
#define SERVER_NAME             "httpd_lisenlin/1.0.0"
#define VERSION                 "1.0.0 http_server"

#define BUFFER_SIZE             8192
#define REQUEST_MAX_SIZE        10240

/* configure constant */
#define 	IS_DEBUG               	 	1                    /* Is open debug mode */
#define 	PORT       	                8099                    /* Server listen port */
#define 	DEFAULT_MAX_CLIENT      	100                  /* Max connection requests */
#define 	DEFAULT_DOCUMENT_ROOT   	"./"                 /* Web document root directory */
#define 	DEFAULT_DIRECTORY_INDEX 	"index.html"         /* Directory default index file name */
#define 	DEFAULT_LOG_PATH        	"/tmp/tmhttpd.log"   /* Access log path */
#define     BACKLOG                   	20                   /*×î´ó¼àÌýÊý*/
#define     MAX_PATH                 	512
#define     RC_SUCC                   	 0
#define     RC_FAIL                  	-1



/* data struct define */
struct st_request_info
{
    char *szMethod;
    char *szPathInfo;
    char *szQuery;
    char *szProtocal;
    char *szPath;
    char *szFile;
    char *szPhysicalPath;
};

typedef struct __connection
{
    int  iSocketfd;
    size_t iReadLen;
    size_t iSendLen;
    int iCmdFlag;
    int iDataFlag;
	char szBuf[1024];

} TConn;

#endif
