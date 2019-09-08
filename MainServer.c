
#include "MainServer.h"
#include "ListCache.h"
#include "HttpPool.h"

#include <pthread.h>

DListNode *oldhead = NULL;

int nFindChar(char *pszIngs, char cValue)
{
    int iNum = 0;
    char szStr[1024] = {0};
    char szIngs[1024] = {0};
    int iI = 0;
    int iJ = 0;
    strcpy(szStr, pszIngs);

    while ((szIngs[iJ]  = szStr[iI++]) != '\0')
    {
        while (szIngs[iJ]  != '\0')
        {
            if (szIngs[iJ++] == cValue)
            {
                iNum++;
            }
        }
    }

    return iNum;
}

int nPrintFileInfo(char *szPathname, char *szBuf, int iDepth)
{
    struct stat filestat;
    int iCount = 0;
    int iNum = 0;
    char *szP = NULL;

    if (stat(szPathname, &filestat) == -1)
    {
        printf("cannot access the file %s", szPathname);
        return -1;
    }

    iCount = nFindChar(szPathname, '/');

    if (iCount > 10)
    {
        printf("the path is too long  error\n");
    }

    // printf("iCount:%d\n", iCount);
    szP = rindex(szPathname, '/');
    sprintf(szBuf + strlen(szBuf), "<li><a style=\"text-decoration:none;\" href=\"%s\">", szPathname + 1);

    for (iNum = 0; iNum < iCount; ++iNum)
    {
        strcat(szBuf, "&emsp;&emsp;");
    }

    strcat(szBuf, "|----");
    sprintf(szBuf + strlen(szBuf), "%s</a>", szP);

    if ((filestat.st_mode & S_IFMT) == S_IFDIR)
    {
        if (RC_SUCC != nDirOrder(szBuf, szPathname, iDepth))
        {
            printf("depth:%d", iDepth);
        }
    }

    // printf("test test :%s %8ld\n", szPathname, filestat.st_size);
    return RC_SUCC;
}

int nDirOrder(char *szBuf, char *szPhysicalPath, int iDepth)
{
    DIR *dfd;
    char szName[MAX_PATH] = {0};
    struct dirent *dp;
    ++iDepth;

    if ((dfd = opendir(szPhysicalPath)) == NULL)
    {
        printf("dir_order: can't open %s\n %s", szPhysicalPath, strerror(errno));
        return RC_FAIL;
    }

    while ((dp = readdir(dfd)) != NULL)
    {
        if (strncmp(dp->d_name, ".", 1) == 0)
        {
            continue;                            /* 跳过当前目录和上一层目录以及隐藏文件*/
        }

        if (strlen(szPhysicalPath) + strlen(dp->d_name) + 2 > sizeof(szName))
        {
            printf("dir_order: name %s %s too long\n", szPhysicalPath, dp->d_name);
        }
        else
        {
            memset(szName, 0, sizeof(szName));
            strcat(szName, szPhysicalPath);

            if (strcmp(szPhysicalPath, "./"))
            {
                strcat(szName, "/");
            }

            strcat(szName, dp->d_name);

            if (RC_SUCC != nPrintFileInfo(szName, szBuf, iDepth))
            {
                printf("nPrintFileInfo ERROR\n");
                return RC_FAIL;
            }
        }
    }

    closedir(dfd);
    return RC_SUCC;
}

/**
 * nIsDir - check file is directory
 *
 */
static int nIsDir(const char *filename)
{
    struct stat stBuf;

    if (stat(filename, &stBuf) < 0)
    {
        return -1;
    }

    if (S_ISDIR(stBuf.st_mode))
    {
        return 1;
    }

    return 0;
}

/**
 * Send http header
 *
 */
static int nSendHeaders(int nClientSock, int iStatus, char *szTitle, char *szExtraHeader, char *szMimeType, off_t iLength, time_t mod)
{
    char szBuf[BUFFER_SIZE] = {0};
    char szBufAll[REQUEST_MAX_SIZE] = {0};
    unsigned int iWriteNum = 0;
    /* Make http head information */
    sprintf(szBuf, "%s %d %s\r\n", "HTTP/1.0", iStatus, szTitle);
    strcat(szBufAll, szBuf);
#if 0

    if (extra_header != NULL)
    {
        memset(buf, 0, strlen(buf));
        sprintf(buf, "%s\r\n", extra_header);
        strcat(szBufAll, buf);
    }

#endif

    if (szMimeType != (char *)0)
    {
        memset(szBuf, 0, strlen(szBuf));
        sprintf(szBuf, "Content-Type: %s\r\n", szMimeType);
        strcat(szBufAll, szBuf);
    }

    if (iLength >= 0)
    {
        memset(szBuf, 0, strlen(szBuf));
        sprintf(szBuf, "Content-length: %lld\r\n\r\n", (int64_t)iLength);
        strcat(szBufAll, szBuf);
    }

    /* Write http header to client socket */
    iWriteNum = strlen(szBufAll);
    printf("iWriteNum:%d\n", iWriteNum);

    if (write(nClientSock, szBufAll, iWriteNum) < 0)
    {
        printf("write error\n");
    }

    printf("head buf:(%s)\n", szBufAll);
    return RC_SUCC;
}

static int nSendError(int nClientSock, int status, char *title, char *extra_header, char *text)
{
    char buf[BUFFER_SIZE] = {0};
    char szBufAll[REQUEST_MAX_SIZE] = {0};
    /* Send http header */
    nSendHeaders(nClientSock, status, title, extra_header, "text/html", -1, -1);
    /* Send html page */
    memset(buf, 0, strlen(buf));
    sprintf(buf, "<html>\n<head>\n<title>%d %s - %s</title>\n</head>\n<body>\n<h2>%d %s</h2>\n", status, title, SERVER_NAME, status, title);
    strcat(szBufAll, buf);
    memset(buf, 0, strlen(buf));
    sprintf(buf, "%s\n", text);
    strcat(szBufAll, buf);
    memset(buf, 0, strlen(buf));
    sprintf(buf, "\n<br /><br /><hr />\n<address>%s</address>\n</body>\n</html>\n", SERVER_NAME);
    strcat(szBufAll, buf);

    /* Write client socket */
    if (write(nClientSock, szBufAll, strlen(szBufAll) < strlen(szBufAll)))
    {
        printf("nSendError html if fail\n");
        return RC_FAIL;
    }

    return RC_SUCC;
}

int nSendHead(int iConn, int iSize)
{
    char szBuf[1024] = "HTTP/1.1 200 OK\r\n";
    char szBuf2[1024] = {0};
    char szBuf3[1024] = "Content-Type: text/html\r\n\r\n";
    sprintf(szBuf2, "Content-length: %d\r\n", iSize);

    if (send(iConn, szBuf, strlen(szBuf), 0) < 0)
    {
        printf("nSendHead if fail\n");
        return RC_FAIL;
    }

    if (send(iConn, szBuf2, strlen(szBuf2), 0) < 0)
    {
        printf("nSendHead if fail\n");
        return RC_FAIL;
    }

    if (send(iConn, szBuf3, strlen(szBuf3), 0) < 0)
    {
        printf("nSendHead if fail\n");
        return RC_FAIL;
    }

    return RC_SUCC;
}

/**
 * Get MIME type header
 *
 */
static void vMimeContentType(const char *name, char *ret)
{
    char *dot = NULL;
    char *buf = NULL;
    dot = strrchr(name, '.');

    if (NULL == dot)        // 其他类型的文件按text/plain处理
    {
        strcpy(ret, "text/plain");
        return;
    }

    /* Text */
    if (strcmp(dot, ".txt") == 0)
    {
        buf = "text/plain";
    }
    else if (strcmp(dot, ".css") == 0)
    {
        buf = "text/css";
    }
    else if (strcmp(dot, ".js") == 0)
    {
        buf = "text/javascript";
    }
    else if (strcmp(dot, ".xml") == 0 || strcmp(dot, ".xsl") == 0)
    {
        buf = "text/xml";
    }
    else if (strcmp(dot, ".xhtm") == 0 || strcmp(dot, ".xhtml") == 0 || strcmp(dot, ".xht") == 0)
    {
        buf = "application/xhtml+xml";
    }
    else if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0 || strcmp(dot, ".shtml") == 0 || strcmp(dot, ".hts") == 0)
    {
        buf = "text/html";
        /* Images */
    }
    else if (strcmp(dot, ".gif") == 0)
    {
        buf = "image/gif";
    }
    else if (strcmp(dot, ".png") == 0)
    {
        buf = "image/png";
    }
    else if (strcmp(dot, ".bmp") == 0)
    {
        buf = "application/x-MS-bmp";
    }
    else if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0 || strcmp(dot, ".jpe") == 0 || strcmp(dot, ".jpz") == 0)
    {
        buf = "image/jpeg";
        /* Audio & Video */
    }
    else if (strcmp(dot, ".wav") == 0)
    {
        buf = "audio/wav";
    }
    else if (strcmp(dot, ".wma") == 0)
    {
        buf = "audio/x-ms-wma";
    }
    else if (strcmp(dot, ".wmv") == 0)
    {
        buf = "audio/x-ms-wmv";
    }
    else if (strcmp(dot, ".au") == 0 || strcmp(dot, ".snd") == 0)
    {
        buf = "audio/basic";
    }
    else if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
    {
        buf = "audio/midi";
    }
    else if (strcmp(dot, ".mp3") == 0 || strcmp(dot, ".mp2") == 0)
    {
        buf = "audio/x-mpeg";
    }
    else if (strcmp(dot, ".rm") == 0  || strcmp(dot, ".rmvb") == 0 || strcmp(dot, ".rmm") == 0)
    {
        buf = "audio/x-pn-realaudio";
    }
    else if (strcmp(dot, ".avi") == 0)
    {
        buf = "video/x-msvideo";
    }
    else if (strcmp(dot, ".3gp") == 0)
    {
        buf = "video/3gpp";
    }
    else if (strcmp(dot, ".mov") == 0)
    {
        buf = "video/quicktime";
    }
    else if (strcmp(dot, ".wmx") == 0)
    {
        buf = "video/x-ms-wmx";
    }
    else if (strcmp(dot, ".asf") == 0  || strcmp(dot, ".asx") == 0)
    {
        buf = "video/x-ms-asf";
    }
    else if (strcmp(dot, ".mp4") == 0 || strcmp(dot, ".mpg4") == 0)
    {
        buf = "video/mp4";
    }
    else if (strcmp(dot, ".mpe") == 0  || strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpg") == 0 || strcmp(dot, ".mpga") == 0)
    {
        buf = "video/mpeg";
        /* Documents */
    }
    else if (strcmp(dot, ".pdf") == 0)
    {
        buf = "application/pdf";
    }
    else if (strcmp(dot, ".rtf") == 0)
    {
        buf = "application/rtf";
    }
    else if (strcmp(dot, ".doc") == 0  || strcmp(dot, ".dot") == 0)
    {
        buf = "application/msword";
    }
    else if (strcmp(dot, ".xls") == 0  || strcmp(dot, ".xla") == 0)
    {
        buf = "application/msexcel";
    }
    else if (strcmp(dot, ".hlp") == 0  || strcmp(dot, ".chm") == 0)
    {
        buf = "application/mshelp";
    }
    else if (strcmp(dot, ".swf") == 0  || strcmp(dot, ".swfl") == 0 || strcmp(dot, ".cab") == 0)
    {
        buf = "application/x-shockwave-flash";
    }
    else if (strcmp(dot, ".ppt") == 0  || strcmp(dot, ".ppz") == 0 || strcmp(dot, ".pps") == 0 || strcmp(dot, ".pot") == 0)
    {
        buf = "application/mspowerpoint";
        /* Binary & Packages */
    }
    else if (strcmp(dot, ".zip") == 0)
    {
        buf = "application/zip";
    }
    else if (strcmp(dot, ".rar") == 0)
    {
        buf = "application/x-rar-compressed";
    }
    else if (strcmp(dot, ".gz") == 0)
    {
        buf = "application/x-gzip";
    }
    else if (strcmp(dot, ".jar") == 0)
    {
        buf = "application/java-archive";
    }
    else if (strcmp(dot, ".tgz") == 0  || strcmp(dot, ".tar") == 0)
    {
        buf = "application/x-tar";
    }
    else
    {
        buf = "application/octet-stream";
    }

    strcpy(ret, buf);
}

static long filesize(const char *szFilename)
{
    struct stat buf;

    if (!stat(szFilename, &buf))
    {
        return buf.st_size;
    }

    return RC_SUCC;
}

static int nSendFile(int iClientSock, char *filename, char *pathInfo)
{
    char buf[128] = {0};
    char contents[8193] = {0};
    char mime_type[64] = {0};
    int fd = -1;
    int iNum = 0;
    size_t lFileSize = 0;
    /* Get mime type & send http header information */
    vMimeContentType(filename, mime_type);
    nSendHeaders(iClientSock, 200, "OK", NULL, mime_type, filesize(filename), 0);

    /* Open file */
    if ((fd = open(filename, O_RDONLY)) == -1)
    {
        memset(buf, '\0', sizeof(buf));
        sprintf(buf, "Something unexpected went wrong read file %s.", pathInfo);
        nSendError(iClientSock, 500, "Internal Server Error", "", buf);
        return RC_FAIL;
    }

    /* Read static file write to client socket (File size less 8192 bytes) */
    lFileSize = filesize(filename);

    while ((iNum = read(fd, contents, 8192))  > 0)
    {
        write(iClientSock, contents, iNum);
        memset(contents, '\0', sizeof(contents));
    }

    /* Close file descriptor */
    close(fd);
    return RC_SUCC;
}

static int nIsFile(const char *szFilename)
{
    struct stat buf;

    if (stat(szFilename, &buf)   < 0)
    {
        return -1;
    }

    if (S_ISREG(buf.st_mode))
    {
        return 1;
    }

    return 0;
}

static int nProcRequest(int iClientSock, struct sockaddr_in stClientAddr, struct st_request_info stRequestInfo)
{
    int iCount = 0;
    int iDepth = 0;
    char szBuf[8096000] = {0};
    char szArg[4096] = {0};
    char szArg2[1024] = {0};
    char szPathSrc[1024] = ".";
    // printf("111111stRequestInfo.szPhysicalPath:%s\n", stRequestInfo.szPhysicalPath);

    if (access(stRequestInfo.szPhysicalPath, R_OK) != 0)
    {
        memset(szBuf, 0, sizeof(szBuf));
        sprintf(szBuf, "File %s is protected.", stRequestInfo.szPathInfo);

        if (RC_SUCC != nSendError(iClientSock, 403, "Forbidden", "", szBuf))
        {
            printf("send error fail\n");
        }

        return RC_FAIL;
    }

    if (nIsDir(stRequestInfo.szPhysicalPath) == 1)
    {
        strcpy(szArg, "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\"><html><title>Directory listing for /</title><body><h2>Directory listing for /</h2><hr><ul>");
        iCount = strlen(szArg);
        memset(szBuf, 0, sizeof(szBuf));
        strcat(szPathSrc, stRequestInfo.szPathInfo);

        if (0 != nDirOrder(szBuf, szPathSrc, iDepth))
        {
            printf("nDirOrder() error\n");
        }

        strcpy(szArg2,  "</ul><hr></body></html>");
        iCount = iCount + strlen(szBuf) + strlen("</ul><hr></body></html>");

        if (RC_SUCC != nSendHead(iClientSock, iCount))
        {
            printf("send head fail\n");
            return RC_FAIL;
        }

        send(iClientSock, szArg, strlen(szArg), 0);
        send(iClientSock, szBuf, strlen(szBuf), 0);
        send(iClientSock, szArg2, strlen(szArg2), 0);
        printf("send buf:{%s}\n", szBuf);
    }
    else if (nIsFile(stRequestInfo.szPhysicalPath) == 1)
    {
        if (RC_SUCC != nSendFile(iClientSock,  stRequestInfo.szPhysicalPath, stRequestInfo.szPathInfo))
        {
            printf("nSendFile is null\n");
        }
    }
    else
    {
        printf("error ...............\n");
        return RC_FAIL;
    }

    return RC_SUCC;
}

static int nParseRequest(int nClientSock, struct sockaddr_in stClient_Addr, char *szReq)
{
    char szCwd[1024] = {0};
    char szPathSrc[512] = {0};
    char szPath[256] = {0};
    char *szP = NULL;
    char *szPtr = NULL;
    char *szPtrBegin = NULL;
    char *szPtrEnd  =  NULL;
    char pszFirstLine[1024] = {0};
    struct st_request_info stRequestInfo;
    memset(&stRequestInfo, 0, sizeof(stRequestInfo));
    printf("request[%s]\n", szReq);
    szPtr = strstr(szReq, "\r\n");
    snprintf(pszFirstLine, sizeof(pszFirstLine), "%.*s", szPtr - szReq, szReq);

    if (strncmp(pszFirstLine, "GET", 3))                // method of http
    {
        return RC_FAIL;
    }

    szPtrBegin = index(pszFirstLine, ' ');
    szPtrEnd = index(szPtrBegin + 1, ' ');

    if (szPtrBegin == NULL || szPtrEnd == NULL)
    {
        printf("szPtr is null\n");
        return RC_FAIL;
    }

    snprintf(szPathSrc, sizeof(szPathSrc), "%.*s", szPtrEnd - szPtrBegin - 1, szPtrBegin + 1);
    stRequestInfo.szPathInfo       = szPathSrc;
    getcwd(szCwd, sizeof(szCwd));
    strcat(szCwd, szPathSrc);
    stRequestInfo.szPhysicalPath   = szCwd;
    printf("stRequestInfo.szPathInfo:(%s)\n", stRequestInfo.szPathInfo);
    printf("stRequestIfo.szPhysicalPath:(%s)\n"    , stRequestInfo.szPhysicalPath);

    if (RC_SUCC != nProcRequest(nClientSock, stClient_Addr, stRequestInfo))
    {
        printf("nProcRequest() erro\n");
        return RC_FAIL;
    }

    return 0;
}

long  lGetTickTime()
{
    struct timeval tval;
    uint64_t tick;
    gettimeofday(&tval, NULL);
    // tick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
    tick = tval.tv_sec;
    return tick;
}

int nHttpRecv(int iSocket, char *pszRecv, int iRead, int iTime)
{
    int iByte = 0;
    int iRecv = 0;
    // uint64_t utime = nGetTickTime();
    errno = 0;

    while (iRead > iRecv)
    {
        iByte = recv(iSocket, pszRecv + iRecv, iRead - iRecv, MSG_DONTWAIT);

        if (iByte < 0)
        {
            //  表示没数据了
            if (EAGAIN == errno || EWOULDBLOCK == errno)
            {
                /*
                   if ((nGetTickTime() - utime) >= (iTime ))
                   {
                       printf("获取描述符(%d)数据超时, (%d)(%s)\n", iSocket, errno,
                              strerror(errno));
                       return iRecv;
                   }
                */
                continue;
            }

            //  Connection reset by peer
            if (errno == ECONNRESET || ENETRESET == errno || ENETDOWN == errno ||
                EINTR == errno)
            {
                return RC_FAIL;
            }
            else
            {
                printf("获取描述符(%d)(%d)数据失败, (%d)(%d)(%s)\n", iByte, iRead, iTime,
                       errno, strerror(errno));
                return RC_FAIL;
            }
        }
        else if (NULL != strstr(pszRecv, "\r\n\r\n"))    // 默认无实体的HTTP报文
        {
            return iRecv;
        }
        else if (iByte == 0)    //  认为是关闭了连接
        {
            return iRecv;
        }

        iRecv += iByte;
    }

    return iRecv;
}


int nHandleClient(TConn *pstConn, struct sockaddr_in stClientAddr)
{
    int iLen = 0;
	int iRet = 0;
/*	for (iLen = 0; ; iLen += iRet)
	{
		iRet = read(pstConn->iSocketfd, pstConn->szBuf + pstConn->iReadLen, sizeof(pstConn->szBuf ) - pstConn->iReadLen);
		if (errno == EAGAIN) 
		{
                 return -2;
        }
        else
        {
              fprintf(stdout, "nRet:%d,strerror:%s,errno:%d,EAGAIN:%d\n", iRet, strerror(errno),errno,EAGAIN);
	    }
        perror("read");

		pstConn->iReadLen += iRet;
	}
*/

    if (nHttpRecv(pstConn->iSocketfd, pstConn->szBuf, REQUEST_MAX_SIZE, 10) <  0)
    {
        if (RC_SUCC != nSendError(pstConn->iSocketfd, 500, "Internal Server Error", "", "Client request not success."))
        {
            printf("send error fail\n");
        }

        return RC_FAIL;
    }
    if (RC_SUCC != nParseRequest(pstConn->iSocketfd, stClientAddr, pstConn->szBuf))
    {
        if (RC_SUCC != nSendError(pstConn->iSocketfd, 500, "Internal Server Error", "", "Client request not success."))
        {
            printf("send error fail\n");
        }

        return RC_FAIL;
    }

    return RC_SUCC;
}

int nMakeSocketNonBlocking(int iSockfd)
{
    int flags = -1;
    int ret = 0;
    flags = fcntl(iSockfd, F_GETFL, 0);

    if (flags == -1)
    {
        perror("fcntl");
        return RC_FAIL;
    }

    flags |= O_NONBLOCK;
    ret = fcntl(iSockfd, F_SETFL, flags);

    if (ret == -1)
    {
        perror("fcntl");
        return RC_FAIL;
    }

    return RC_SUCC;
}

int nSocketCreate()
{
    int iRet = 0;
    int iSockFd = -1;
    struct sockaddr_in stMyAddr;                    /*本方地址信息结构体，下面有具体的属性赋值*/
    iSockFd = socket(AF_INET, SOCK_STREAM, 0);    /*建立socket  */

    if (iSockFd == -1)
    {
        printf("socket failed:%d", errno);
        return -1;
    }

    stMyAddr.sin_family = AF_INET;                  /*该属性表示接收本机或其他机器传输*/
    stMyAddr.sin_port = htons(PORT);                /*端口号*/
    stMyAddr.sin_addr.s_addr = htonl(INADDR_ANY);   /*IP，括号内容表示本机IP*/
    bzero(&(stMyAddr.sin_zero), 8);                 /*将其他属性置0*/

    if (bind(iSockFd, (struct sockaddr *)&stMyAddr, sizeof(struct sockaddr)) < 0)
    {
        /*绑定地址结构体和socket*/
        perror("bind error");
        exit(1);
    }

    if (RC_SUCC != listen(iSockFd, BACKLOG))                                                       /*开启监听 ，第二个参数是最大监听数*/
    {
        perror("listen");
        exit(1);
    }

    iRet = nMakeSocketNonBlocking(iSockFd);

    if (iRet == -1)
    {
        close(iSockFd);
        perror("fcntl");
        return RC_FAIL;
    }

    printf("sockfd:%d\n", iSockFd);
    return iSockFd;
}
int nFdAdd(int iEpfd, int iListfd, struct epoll_event *stEvent, int flag)
{
    int iRet = -1;
    /*EPOLLIN ：表示对应的文件描述符可以读
      EPOLLET ：将EPOLL设为边缘触发(Edge Triggered)模式
    */
    stEvent->events = EPOLLIN | EPOLLET;

    if (0 == flag)
    {
        stEvent->data.fd = iListfd;
    }

    iRet = epoll_ctl(iEpfd, EPOLL_CTL_ADD, iListfd, stEvent);

    if (-1 == iRet)
    {
        perror("ctl");
        return RC_FAIL;
    }

    return RC_SUCC;
}
int nFdDel(int iEpfd, int iFd)
{
    struct epoll_event ev;
    int iRet = -1;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = iFd;
    iRet = epoll_ctl(iEpfd, EPOLL_CTL_DEL, iFd, &ev);

    if (-1 == iRet)
    {
        perror("ctl");
        return RC_FAIL;
    }

    close(iFd);
    return RC_SUCC;
}
DListNode *nCreatHead(DListNode *phead)
{
    DListNode *tmp = malloc(sizeof(DListNode));

    if (NULL == tmp)
    {
        printf("malloc is fail\n");
        exit(1);
    }

    tmp->iSockfd = -2;
    tmp->lCurrTime = -100;
    tmp->next = NULL;
    return tmp;
}
DListNode *delList(DListNode *phead, int iConn)
{
    DListNode *find = phead;
    DListNode *tmp = phead;
    printf("del fdLIST %d\n", iConn);

    while (find != NULL)
    {
        if (find->iSockfd == iConn)
        {
            tmp = find->next;
            free(find);
            return phead;
        }

        tmp = find;
        find = find->next;
    }

    return phead;
}

int nAnalysisBuf(int iEpfd, struct epoll_event *retevent, int iListfd)
{
    TConn *pstConn = NULL;
    int iConn = -1;
    int  iLen = 0;
    int iRet = -1;
    int  i = 0;
    struct sockaddr_in stTheirAddr;                 /*对方地址信息*/
    bzero(&stTheirAddr, sizeof(stTheirAddr));
    oldhead = nCreatHead(oldhead);

    while (1)
    {
        if (search(oldhead, pstConn, lGetTickTime()))
        {
            printf("nfdDEl 924 \n");
            // nFdDel(iEpfd, iConn);
        }

        iRet = epoll_wait(iEpfd, retevent, 1024, 400);

        if (-1 == iRet)
        {
            perror("epoll wait");
            free(retevent);
            return RC_FAIL;
        }

        for (i = 0; i < iRet; ++i)
        {
            if (retevent[i].data.fd == iListfd)
            {
                while (1)
                {
                    iConn = accept(iListfd, (struct sockaddr *)&stTheirAddr, &iLen);

                    if (-1 == iConn)
                    {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))     // has been no date!!!
                        {
                            break;
                        }
                        else
                        {
                            perror("accept");
                            break;
                        }
                    }

                    if (RC_SUCC != nMakeSocketNonBlocking(iConn))
                    {
                        close(iConn);
                        perror("fcntl");
                        return RC_FAIL;
                    }

                    struct epoll_event stEvent;

                    pstConn = calloc(1, sizeof(TConn));

                    if (NULL == pstConn)
                    {
                        printf("pstConn is null\n");
                        close(iConn);
                        return RC_FAIL;
                    }

                    printf("iconn:%d\n", iConn);
                    pstConn->iSocketfd = iConn;
                    stEvent.data.ptr = pstConn;

                    if (RC_SUCC != nFdAdd(iEpfd, iConn, &stEvent, 1))
                    {
                        perror("epoll_ctl");
                        break;
                    }
                    oldhead = nAddCache(oldhead, pstConn, lGetTickTime(), iConn);
                }
            }
            else if (retevent[i].events & EPOLLIN)
            {
                pstConn = retevent[i].data.ptr;
                iConn = pstConn->iSocketfd;
				
                printf("retevent[i].data.fd:%d\n", iConn);
				
                iRet = nHandleClient(pstConn, stTheirAddr);
				
                if (-2 == iRet )
                {
                    continue;
                }

                printf("end1 delete\n");
            }
            // printf("end delete\n");
            // nFdDel(iEpfd, iConn);
        }
    }
}

int main(void)
{
    int iListfd = -1;
    int iEpfd = -1;
    struct epoll_event retevent[1024] = { 0 };
    struct epoll_event stEvent;

    if ((iListfd = nSocketCreate()) < 0)
    {
        perror("Error creating socket");
        return RC_FAIL;
    }

    iEpfd = epoll_create(1024);

    if (RC_FAIL == iEpfd)
    {
        close(iEpfd);
        close(iListfd);
        perror("epoll_create fail\n");
        return RC_FAIL;
    }

    if (RC_FAIL == nFdAdd(iEpfd, iListfd, &stEvent, 0))
    {
        printf("Fd_add error\n");
        close(iEpfd);
        close(iListfd);
        return RC_FAIL;
    }

    if (RC_SUCC != nAnalysisBuf(iEpfd, retevent, iListfd))
    {
        printf("nAnalysisBuf fail\n");
    }

    close(iEpfd);
    close(iListfd);
    return RC_SUCC;
}

