#ifndef _poolth_h_
#define _poolth_h_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>

/*
*�̳߳����������к͵ȴ���������һ��CThread_worker
*�������������������������һ������ṹ
*/
typedef struct worker
{ 
    /*�ص���������������ʱ����ô˺�����ע��Ҳ��������������ʽ*/
    void *(*process) (void *arg);
    void *arg;/*�ص������Ĳ���*/
    struct worker *next;

} CThread_worker;
/*�̳߳ؽṹ*/
typedef struct
{
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;
	/*����ṹ���̳߳������еȴ�����*/
    CThread_worker *queue_head;
    /*�Ƿ������̳߳�*/
    int shutdown;
    pthread_t *threadid;
    /*�̳߳�������Ļ�߳���Ŀ*/
    int max_thread_num;
    /*��ǰ�ȴ����е�������Ŀ*/
    int cur_queue_size;

} CThread_pool;

extern int pool_add_worker (void *(*process) (void *arg), void *arg);
extern void *thread_routine (void *arg);
extern void pool_init (int max_thread_num);
extern int pool_add_worker (void *(*process) (void *arg), void *arg);
extern int pool_destroy ();
extern void * thread_routine (void *arg);

//  extern pthread_mutex_t mutex;



#endif
