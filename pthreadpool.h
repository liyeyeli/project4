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

//任务结点的类型
//任务链表上的任务结点,只需要描述好一个任务就可以了
//线程会不断的从任务队列上获取任务
typedef struct task
{
	void* (*do_task)(void* arg);//线程的函数指针,指向要完成的那个任务
	void* arg;
	struct task* next;//指向下一个任务结点
}TASK;
typedef struct pthread_pool
{
	//有多个线程,就会有多个tid
	pthread_t* tids;
	//线程池中正在服役的线程(正在执行任务的线程数)
	unsigned int active_threads;
	//任务队列(链表),所有的线程都需要从任务队列上获取任务执行
	struct task* task_list;//指向第一个需要被执行的任务
	struct task* task_rear;//指向最后一个需要被执行的任务
	//互斥锁,用来保护这个任务 "任务队列"要互斥访问
	pthread_mutex_t lock;//互斥锁
	//线程的条件变量,表示"任务队列"是否有任务
	pthread_cond_t cond;//条件变量
	//记录程序的状态 退出 不退出
	bool shutdown;//FALSE 不退出 /TRUE 退出
	//线程池最多能够容纳多少个线程
	unsigned int max_threads;
	//线程池中任务链表当前的任务数量
	unsigned int cur_tasks;
	//线程池中任务队列最大的任务数量
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
