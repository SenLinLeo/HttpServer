#ifndef _sort_h_
#define _sort_h_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

typedef struct _DListNode
{

    int iSockfd;
	long lReadLen;
    long lCurrTime;
    struct _DListNode *next;
    // struct _DListNode *prev;
    TConn* pstConn;
} DListNode;

extern DListNode *oldhead;
DListNode *del(DListNode *phead);
 //DListNode *search(DListNode *phead, long lCurrTime, int iSock);

DListNode *nAddCache(DListNode *phead, TConn *pstConn, long lSrcTime, int iSock);
void show(DListNode *phead);
DListNode*  search( DListNode* phead, TConn *pstConn, long lNewTime);
void destory(DListNode *phead);


#endif
