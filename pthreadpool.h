#ifndef __PTHREADPOOL_H__
#define __PTHREADPOOL_H__

#define MAX_ACTIVE_PTHREADS 50
#define MAX_TASKS 100

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<stdbool.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<dirent.h>
#include<string.h>

//�����������
//���������ϵ�������,ֻ��Ҫ������һ������Ϳ�����
//�̻߳᲻�ϵĴ���������ϻ�ȡ����
typedef struct task
{
	void* (*do_task)(void* arg);//�̵߳ĺ���ָ��,ָ��Ҫ��ɵ��Ǹ�����
	void* arg;
	struct task* next;//ָ����һ��������
}TASK;
typedef struct pthread_pool
{
	//�ж���߳�,�ͻ��ж��tid
	pthread_t* tids;
	//�̳߳������ڷ��۵��߳�(����ִ��������߳���)
	unsigned int active_threads;
	//�������(����),���е��̶߳���Ҫ����������ϻ�ȡ����ִ��
	struct task* task_list;//ָ���һ����Ҫ��ִ�е�����
	struct task* task_rear;//ָ�����һ����Ҫ��ִ�е�����
	//������,��������������� "�������"Ҫ�������
	pthread_mutex_t lock;//������
	//�̵߳���������,��ʾ"�������"�Ƿ�������
	pthread_cond_t cond;//��������
	//��¼�����״̬ �˳� ���˳�
	bool shutdown;//FALSE ���˳� /TRUE �˳�
	//�̳߳�����ܹ����ɶ��ٸ��߳�
	unsigned int max_threads;
	//�̳߳�����������ǰ����������
	unsigned int cur_tasks;
	//�̳߳����������������������
	unsigned int max_waiting_task;
	//...
}pthread_pool;

int Pool_Init(pthread_pool* pool, unsigned int thread_num);
void Clean_Func(void* arg);
void* Routine(void* arg);
int Pool_Destroy(pthread_pool* pool);
void add_task(pthread_pool* pool, void* (*fun_task)(void* arg), void* fun_arg);
int add_thread(pthread_pool* pool, unsigned int add_thread_num);
int remove_thread(pthread_pool* pool, unsigned int remove_thread_num);
void Pool_wait(pthread_pool* pool);
#endif
