#include "ListCache.h"
#include "MainServer.h"


DListNode *del(DListNode *phead)
{
    DListNode *find = phead;
    phead = phead->next;
    free(find);
    return phead;
}


void destory(DListNode *phead)
{
    DListNode *tmp = phead;

    while (phead)
    {
        tmp = phead;
        phead = phead->next;
        free(tmp);
    }
}
#if 0
DListNode * *linked_sort(DListNode * **phead)
{
    DListNode * *find = *phead;
    DListNode * *min = *phead;
    DListNode * *pre = NULL;

    if ((*phead) == NULL)
    {
        return *phead;
    }

    while (find->next != NULL)
    {
        if (find->next->id < min->id)
        {
            pre = find;
            min = find->next;
        }

        find = find->next;
    }

    newhead = create(newhead, min->id);
    *phead = del(*phead, min->id);
    find_min(phead);
}
#endif

DListNode *nAddCache(DListNode *phead, TConn *pstConn, long lSrcTime, int iSock)
{
    DListNode *tmp = malloc(sizeof(DListNode));

    if (NULL == tmp)
    {
        printf("malloc is fail\n");
        exit(1);
    }

    tmp->iSockfd = iSock;
    tmp->pstConn = pstConn;
    tmp->lCurrTime = lSrcTime;
    tmp->next = NULL;
    DListNode *find = phead;
    printf("add[%p]Sockfd:[%d]time[%ld]\n", phead, iSock, lSrcTime);

    if (phead == NULL)
    {
        printf("head\n");
        return tmp;
    }
    else
    {
        while (find->next != NULL)
        {
            find = find->next;
        }

        find->next = tmp;
    }

    return phead;
}

DListNode  *search(DListNode *phead, TConn *pstConn, long lNewTime)
{
    // printf("已经进入链表搜索 %d\n", lNewTime);
    DListNode *find = phead->next;
    DListNode *tmp = NULL;
    int flag = 0;

    if (phead == NULL)
    {
        printf("head is null\n");
        return NULL;
    }

    tmp = phead;

    while (find != NULL)
    {
        if (((lNewTime - find->lCurrTime) > 10) && (find -> lCurrTime > 0) && (lNewTime > 0))
        {
            tmp->next = find->next;
            printf("删除节点:%d\n", find->iSockfd);
            close(find->iSockfd);
            free(find);
            flag = 1;
            break;
        }

        tmp = find;
        find = find->next;
    }

    if (flag == 1)
    {
        printf("resch out\n");
        return phead;
    }

    return NULL;
}





