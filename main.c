#include"pthreadpool.h"
void* cp_file(void* arg);
int cp_dir(char* src, char* dest,pthread_pool*pool);

void* cp_file(void* arg)
{
	char* file1 = ((char**)arg)[0];
	char* file2 = ((char**)arg)[1];
	int fd1, fd2;
	int ret, r;
	char buf[512];
	//��һ���ļ��򿪵ķ�ʽ,Ӧ���� :O_RDONLY
	fd1 = open(file1, O_RDONLY);
	if (fd1 == -1)
	{
		perror("open src failed ");
		return NULL;
	}
	//�ڶ����ļ��򿪵ķ�ʽ, O_WRONLY | O_CREAT | O_TRUNC
	fd2 = open(file2, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd2 == -1)
	{
		perror("open dest failed");
		return NULL;
	}
	ret = lseek(fd2, 0, SEEK_SET);
	if (ret == -1)
	{
		perror("lseek fd2 error");
		close(fd1);
		close(fd2);
	}
	//��������
	while (1)
	{
		//read(fd1,...);
		//write(fd2,...);
		//��fd1�ж�ȡn���ֽڵ����� ==> buf
		r = read(fd1, buf, 512);
		//��buf�ж�ȡ����x���ֽڵ�����,д�뵽fd2
		if (r > 0)
		{
			ret = write(fd2, buf, r);
			if (ret == -1)
			{
				perror("read fd1 error");
				close(fd1);
				close(fd2);
			}
		}
		else if (r == 0)
		{
			break;
		}
		else
		{
			perror("read fd1 error");
			close(fd1);
			close(fd2);
		}
	}
	close(fd1);
	close(fd2);
}
int cp_dir(char* src, char* dest,pthread_pool*pool)
{
	int i = 0;
	DIR* dir = NULL;
	struct dirent* dirp = NULL;
	dir = opendir(src);
	if (dir == NULL)
	{
		perror("opendir error");
		return -1;
	}
	while (dirp = readdir(dir))
	{
		//printf("%p\n", dirp);
		if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
		{
			continue;
		}
		char srcp[512];
		char destp[512];
		char running[512];
		snprintf(srcp,sizeof(srcp), "%s/%s",src,dirp->d_name);
		//printf("%s\n", dirp->d_name);
		//printf("%s\n", srcp);
		snprintf(destp,sizeof(destp),"%s/%s",dest,dirp->d_name);
		//snprintf(running,sizeof(running),"%s/%s",srcp,dirp->d_name);
		struct stat st;
		int ret = stat(srcp, &st);
		if (ret == -1)
		{
			//printf("%d\n", ret);
			perror("stat error");
			return -1;
		}
		if (S_ISREG(st.st_mode))
		{
			//i++;
			char** arg = (char**)malloc(1024);
			arg[0] = strdup(srcp);
			arg[1] = strdup(destp);
			/*pthread_pool* pool = (pthread_pool*)malloc(sizeof(*pool));
			Pool_Init(pool, i);*/
			add_task(pool, cp_file, arg);
			//Pool_Destroy(pool);
		}
		else if (S_ISDIR(st.st_mode))
		{
			mkdir(destp, 777);//����Ŀ¼
			cp_dir(srcp, destp,pool);
		}
	}
	closedir(dir);
}


int main(int argc,char* argv[])
{
	pthread_pool* pool = (pthread_pool*)malloc(sizeof(*pool));
	Pool_Init(pool, 4);
	cp_dir(argv[1], argv[2],pool);

	Pool_wait(pool);
	Pool_Destroy(pool);
	free(pool);
	exit(0);

	return 0;
}