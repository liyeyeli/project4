#include"pthreadpool.h"


/*
Pool_Init : 初始化一个线程池
初始化你指定的那一个线程池,线程池中能够有多少个初始线程
@pool : 指向你要初始化的那一个线程池
@thread_num : 你要初始化的线程池 中开始的线程数目
返回值 :
成功 返回 0
失败 返回 -1
*/
int Pool_Init(pthread_pool* pool, unsigned int thread_num)
{
	//初始化结构体
	//初始化互斥锁
	int r = pthread_mutex_init(&(pool->lock), NULL);
	if (r != 0)
	{
		perror("pthread_mutex_init error");
		return -1;
	}
	//初始化条件变量
	int r1 = pthread_cond_init(&(pool->cond), NULL);
	if (r1 != 0)
	{
		perror("pthread_cond_init error");
		return -1;
	}
	//创建一个任务节点，并使task->list指向他
	pool->task_list = (TASK*)malloc(sizeof(TASK));
	pool->task_list->next = NULL;
	//TASK* t = (TASK*)malloc(sizeof(*t));
	//pool->task_list = t;
	//开辟一个数组空间保存tid
	pool->tids = (pthread_t*)malloc(sizeof(*(pool->tids)));
	//初始化线程池中正在服役的线程数
	pool->active_threads = thread_num;
	//初始化线程池中最多容纳多少个线程
	pool->max_threads = MAX_ACTIVE_PTHREADS;
	//初始化线程池中任务队列最大任务数量
	pool->max_waiting_task = MAX_TASKS;
	//初始化线程池中任务链表当前任务数量
	pool->cur_tasks = 0;
	//程序的状态
	pool->shutdown = false;
	// 需要创建thread_num个线程, 将线程的id保存到tids指向的数组中去
	//还需要让 创建好的线程 去执行 任务调配函数
	int i = 0;
	while (i < thread_num)
	{
		//TASK* t1 = (TASK*)malloc(sizeof(*t1));
		//int r = pthread_create(&(pool->tids[i]), NULL,t1->do_task, t1->arg);
		int r = pthread_create(&(pool->tids[i]), NULL,Routine, (void*)pool);
		if (r != 0)
		{
			perror("pthread_create error");
			return -1;
		}
		//pthread_mutex_lock(&(pool->lock));
		//if (pool->shutdown == false && pool->cur_tasks == 0)
		//{
		//	pthread_cond_wait(&(pool->cond), &(pool->lock));
		//}
		//else
		//{
		//	Routine((void*)pool);
		//}
		//pthread_mutex_unlock(&(pool->lock));
		i++;
	}
	printf("[%lu] : [%s]==> tids[%d] : [%lu] is createsuncess!!!\n",pthread_self(),__FUNCTION__,i,pool->tids[i-1]);
	return 0;
}
//线程退出时指定的清理函数
void Clean_Func(void* arg)
{
	//当线程带锁意外退出时,解锁
	pthread_mutex_unlock((pthread_mutex_t*)arg);
}

void* Routine(void* arg)
{
	//arg 表示线程池的指针
	pthread_pool* pool = (pthread_pool*)arg;
	TASK* tasked = (TASK*)malloc(sizeof(*tasked));//存放得到的任务节点
	//pool->task_list代表线程池中所有线程一起的链表
	//第一个节点没有赋值
	//当线程池中没有结束任务,不断的从任务队列中,取下任务结点,交给线程执行

	while (1)
	{
		//共享资源
		//获取互斥锁,上锁(如果是线程意外情况退出,要先解锁)
		pthread_cleanup_push(Clean_Func, (void*)&pool->lock);
		pthread_mutex_lock(&(pool->lock));

		//1.任务队列中没有任务,但是线程池没有结束(pool->shutdown == false)
		//需要等待任务(条件变量)
		while (pool->cur_tasks == 0 && pool->shutdown == false)
		{
			printf("3\n");
			pthread_cond_wait(&(pool->cond), &(pool->lock));
			printf("4\n");
		}
		//2.任务队列中没有任务,但是线程池结束(pool->shutdown == true)
		//此时可以结束此线程(解锁,再退出)
		if (pool->cur_tasks == 0 && pool->shutdown == true)
		{
			pthread_mutex_unlock(&(pool->lock));
			pthread_exit(NULL);
		}
		//3.有任务,并且获取到了任务结点,更新线程池的任务链表
		//取任务链表的第二个结点执行,第一个结点没有任务数据
		
		tasked = pool->task_list->next;
		pool->task_list->next = tasked->next;
		pool->cur_tasks--;
		pthread_mutex_unlock(&(pool->lock));
		pthread_cleanup_pop(0);

		// 假设p指向任务结点
		//任务结点中包含了 函数执行 和 参数
		//调用线程函数

		//任务没有执行完,设置线程属性为不可别取消
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		//执行任务函数
		(tasked->do_task)(tasked->arg);
		//任务执行完,线程可以被取消
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		//释放任务结点
		free(tasked);//结点空间
		/*if (tasked != NULL && tasked->do_task != NULL)
		{
			tasked->do_task(tasked->arg);
			free(tasked);
		}*/
		//pthread_cond_broadcast(&(pool->cond));
		
	}
	
}

/*
Pool_Destroy : 销毁一个线程池
销毁线程池之前要保证所有的任务都已经完成.
@pool : 要销毁的线程池
返回值 :
无
*/
int Pool_Destroy(pthread_pool* pool)
{
	//释放所有的空间 等待任务执行完毕
	if (pool == NULL)
	{
		return -1;
	}
	pool->shutdown = 1;
	//唤醒所有的线程
	for (int i = 0; i < pool->active_threads; i++)
	{
		pthread_cond_signal(&(pool->cond));
	}
	
	//利用join的函数回收每一个线程资源
	for (int i = 0; i < pool->active_threads; i++)
	{
		pthread_join(pool->tids[i], NULL);
	}
	//销毁线程互斥锁
	pthread_mutex_destroy(&(pool->lock));
	//销毁条件变量
	pthread_cond_destroy(&(pool->cond));
	//free 掉所有的malloc过来的空间
	TASK* t = (TASK*)malloc(sizeof(*t));
	while (pool->task_list)
	{
		t = pool->task_list;
		pool->task_list = t->next;
		free(t);
	}
	if (pool->tids)
	{
		free(pool->tids);
	}
	free(pool);
	pool = NULL;
	return 0;
}
/*
add_task : 给任务链表添加任务结点
@pool : 线程池指针
@fun_task : 你添加的任务是谁 ==> 线程函数指针
@fun_arg : 你要执行的那个任务的参数
返回值 :
无
*/
void add_task(pthread_pool* pool, void* (*fun_task)(void* arg), void*fun_arg)
{
	//printf("%p\n", pool);
	//1.创建一个新的任务结点,保存当前的 要添加的一个任务
	TASK* t = (TASK*)malloc(sizeof(*t));
	//2.给任务结点赋值(三个成员变量赋值)
	t->do_task = fun_task;
	t->arg = fun_arg;
	t->next = NULL;
	//	printf("3\n");
	//3.把任务结点加入到 线程池对应的 那一个任务队列中去(共享资源)
	//3.1 考虑 任务数量已经达到上限
	//3.2 如果可以添加,那你就添加这个任务
	pthread_mutex_lock(&(pool->lock));
	while (pool->cur_tasks == pool->max_waiting_task && pool->shutdown == 0)
	{
		pthread_cond_wait(&(pool->cond),&(pool->lock));
	}
	if (pool->shutdown == 1)
	{
		pthread_mutex_unlock(&(pool->lock));
		return;
	}
	if (pool->cur_tasks < pool->max_waiting_task && pool->shutdown == 0)
	{
		while (pool->task_list->next)
		{
			pool->task_list = pool->task_list->next;
		}
		pool->task_list->next = t;
		pool->cur_tasks++;
		pthread_mutex_unlock(&(pool->lock));
	}
	//4.添加成功,唤醒线程
	pthread_cond_signal(&(pool->cond));
	//printf("1\n");
}

//利用pthread_create创建线程,并且把线程id保存到线程池 tids数组中去
//让每一个新创建的线程去执行任务调配函数
/*
add_thread : 往线程池中新增几个线程
@pool : 线程池指针
@add_thread_num : 表示你要添加多少个线程
返回值 :
返回 添加之后,正在服役的线程数
返回 -1,失败
*/
int add_thread(pthread_pool* pool, unsigned int add_thread_num)
{
	for (int i = pool->active_threads; i < (pool->active_threads + add_thread_num); i++)
	{
		int r = pthread_create(&(pool->tids[i]),NULL,Routine,(void*)pool);
		if (r != 0)
		{
			perror("pthread_create error");
			return -1;
		}
	}
	pool->active_threads= pool->active_threads + add_thread_num;
	return pool->active_threads;
}
/*****remove_thread***** /
//利用pthread_cancel 去取消部分线程
//利用 pthread_join 回收被取消线程的资源
/*
remove_thread : 销毁部分线程
@pool : 线程池指针
@remove_thread_num : 销毁线程数目
返回值 :
成功 返回销毁之后,线程池中 活跃的线程数(包括工作的和休眠的)
失败 返回-1
*/
int remove_thread(pthread_pool * pool, unsigned int remove_thread_num)
{
	for (int i = pool->active_threads - 1; i > (pool->active_threads - 1 - remove_thread_num); i--)
	{
		int r = pthread_cancel(pool->tids[i]);
		if (r != 0)
		{
			perror("pthread_cancel error");
			return -1;
		}
	}
	pool->active_threads = pool->active_threads - remove_thread_num;
	return pool->active_threads;
}

void Pool_wait(pthread_pool* pool)
{
	pthread_mutex_lock(&(pool->lock));

	while (pool->cur_tasks > 0 || pool->active_threads > 0)
	{
		pthread_cond_wait(&(pool->cond), &(pool->lock));
	}
	pthread_mutex_unlock(&(pool->lock));
}