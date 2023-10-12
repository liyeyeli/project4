#include"pthreadpool.h"


/*
Pool_Init : ��ʼ��һ���̳߳�
��ʼ����ָ������һ���̳߳�,�̳߳����ܹ��ж��ٸ���ʼ�߳�
@pool : ָ����Ҫ��ʼ������һ���̳߳�
@thread_num : ��Ҫ��ʼ�����̳߳� �п�ʼ���߳���Ŀ
����ֵ :
�ɹ� ���� 0
ʧ�� ���� -1
*/
int Pool_Init(pthread_pool* pool, unsigned int thread_num)
{
	//��ʼ���ṹ��
	//��ʼ��������
	int r = pthread_mutex_init(&(pool->lock), NULL);
	if (r != 0)
	{
		perror("pthread_mutex_init error");
		return -1;
	}
	//��ʼ����������
	int r1 = pthread_cond_init(&(pool->cond), NULL);
	if (r1 != 0)
	{
		perror("pthread_cond_init error");
		return -1;
	}
	//����һ������ڵ㣬��ʹtask->listָ����
	pool->task_list = (TASK*)malloc(sizeof(TASK));
	pool->task_list->next = NULL;
	//TASK* t = (TASK*)malloc(sizeof(*t));
	//pool->task_list = t;
	//����һ������ռ䱣��tid
	pool->tids = (pthread_t*)malloc(sizeof(*(pool->tids)));
	//��ʼ���̳߳������ڷ��۵��߳���
	pool->active_threads = thread_num;
	//��ʼ���̳߳���������ɶ��ٸ��߳�
	pool->max_threads = MAX_ACTIVE_PTHREADS;
	//��ʼ���̳߳���������������������
	pool->max_waiting_task = MAX_TASKS;
	//��ʼ���̳߳�����������ǰ��������
	pool->cur_tasks = 0;
	//�����״̬
	pool->shutdown = false;
	// ��Ҫ����thread_num���߳�, ���̵߳�id���浽tidsָ���������ȥ
	//����Ҫ�� �����õ��߳� ȥִ�� ������亯��
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
//�߳��˳�ʱָ����������
void Clean_Func(void* arg)
{
	//���̴߳��������˳�ʱ,����
	pthread_mutex_unlock((pthread_mutex_t*)arg);
}

void* Routine(void* arg)
{
	//arg ��ʾ�̳߳ص�ָ��
	pthread_pool* pool = (pthread_pool*)arg;
	TASK* tasked = (TASK*)malloc(sizeof(*tasked));//��ŵõ�������ڵ�
	//pool->task_list�����̳߳��������߳�һ�������
	//��һ���ڵ�û�и�ֵ
	//���̳߳���û�н�������,���ϵĴ����������,ȡ��������,�����߳�ִ��

	while (1)
	{
		//������Դ
		//��ȡ������,����(������߳���������˳�,Ҫ�Ƚ���)
		pthread_cleanup_push(Clean_Func, (void*)&pool->lock);
		pthread_mutex_lock(&(pool->lock));

		//1.���������û������,�����̳߳�û�н���(pool->shutdown == false)
		//��Ҫ�ȴ�����(��������)
		while (pool->cur_tasks == 0 && pool->shutdown == false)
		{
			printf("3\n");
			pthread_cond_wait(&(pool->cond), &(pool->lock));
			printf("4\n");
		}
		//2.���������û������,�����̳߳ؽ���(pool->shutdown == true)
		//��ʱ���Խ������߳�(����,���˳�)
		if (pool->cur_tasks == 0 && pool->shutdown == true)
		{
			pthread_mutex_unlock(&(pool->lock));
			pthread_exit(NULL);
		}
		//3.������,���һ�ȡ����������,�����̳߳ص���������
		//ȡ��������ĵڶ������ִ��,��һ�����û����������
		
		tasked = pool->task_list->next;
		pool->task_list->next = tasked->next;
		pool->cur_tasks--;
		pthread_mutex_unlock(&(pool->lock));
		pthread_cleanup_pop(0);

		// ����pָ��������
		//�������а����� ����ִ�� �� ����
		//�����̺߳���

		//����û��ִ����,�����߳�����Ϊ���ɱ�ȡ��
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		//ִ��������
		(tasked->do_task)(tasked->arg);
		//����ִ����,�߳̿��Ա�ȡ��
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		//�ͷ�������
		free(tasked);//���ռ�
		/*if (tasked != NULL && tasked->do_task != NULL)
		{
			tasked->do_task(tasked->arg);
			free(tasked);
		}*/
		//pthread_cond_broadcast(&(pool->cond));
		
	}
	
}

/*
Pool_Destroy : ����һ���̳߳�
�����̳߳�֮ǰҪ��֤���е������Ѿ����.
@pool : Ҫ���ٵ��̳߳�
����ֵ :
��
*/
int Pool_Destroy(pthread_pool* pool)
{
	//�ͷ����еĿռ� �ȴ�����ִ�����
	if (pool == NULL)
	{
		return -1;
	}
	pool->shutdown = 1;
	//�������е��߳�
	for (int i = 0; i < pool->active_threads; i++)
	{
		pthread_cond_signal(&(pool->cond));
	}
	
	//����join�ĺ�������ÿһ���߳���Դ
	for (int i = 0; i < pool->active_threads; i++)
	{
		pthread_join(pool->tids[i], NULL);
	}
	//�����̻߳�����
	pthread_mutex_destroy(&(pool->lock));
	//������������
	pthread_cond_destroy(&(pool->cond));
	//free �����е�malloc�����Ŀռ�
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
add_task : �������������������
@pool : �̳߳�ָ��
@fun_task : ����ӵ�������˭ ==> �̺߳���ָ��
@fun_arg : ��Ҫִ�е��Ǹ�����Ĳ���
����ֵ :
��
*/
void add_task(pthread_pool* pool, void* (*fun_task)(void* arg), void*fun_arg)
{
	//printf("%p\n", pool);
	//1.����һ���µ�������,���浱ǰ�� Ҫ��ӵ�һ������
	TASK* t = (TASK*)malloc(sizeof(*t));
	//2.�������㸳ֵ(������Ա������ֵ)
	t->do_task = fun_task;
	t->arg = fun_arg;
	t->next = NULL;
	//	printf("3\n");
	//3.����������뵽 �̳߳ض�Ӧ�� ��һ�����������ȥ(������Դ)
	//3.1 ���� ���������Ѿ��ﵽ����
	//3.2 ����������,���������������
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
	//4.��ӳɹ�,�����߳�
	pthread_cond_signal(&(pool->cond));
	//printf("1\n");
}

//����pthread_create�����߳�,���Ұ��߳�id���浽�̳߳� tids������ȥ
//��ÿһ���´������߳�ȥִ��������亯��
/*
add_thread : ���̳߳������������߳�
@pool : �̳߳�ָ��
@add_thread_num : ��ʾ��Ҫ��Ӷ��ٸ��߳�
����ֵ :
���� ���֮��,���ڷ��۵��߳���
���� -1,ʧ��
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
//����pthread_cancel ȥȡ�������߳�
//���� pthread_join ���ձ�ȡ���̵߳���Դ
/*
remove_thread : ���ٲ����߳�
@pool : �̳߳�ָ��
@remove_thread_num : �����߳���Ŀ
����ֵ :
�ɹ� ��������֮��,�̳߳��� ��Ծ���߳���(���������ĺ����ߵ�)
ʧ�� ����-1
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