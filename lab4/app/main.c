#include "lib.h"
#include "types.h"
#define N 5





int uEntry(void) {


	// 测试scanf	
	int dec = 0;
	int hex = 0;
	char str[6];
	char cha = 0;
	int ret = 0;
	while(1){
		printf("Input:\" Test %%c Test %%6s %%d %%x\"\n");
		ret = scanf(" Test %c Test %6s %d %x", &cha, str, &dec, &hex);
		printf("Ret: %d; %c, %s, %d, %x.\n", ret, cha, str, dec, hex);
		if (ret == 4)
			break;
	}
	
	// 测试信号量
	int i = 4;
	sem_t sem;
	printf("Father Process: Semaphore Initializing.\n");
	ret = sem_init(&sem, 0);
	if (ret == -1) {
		printf("Father Process: Semaphore Initializing Failed.\n");
		exit();
	}

	ret = fork();
	if (ret == 0) {
		while( i != 0) {
			i --;
			printf("Child Process: Semaphore Waiting.\n");
			sem_wait(&sem);
			printf("Child Process: In Critical Area.\n");
		}
		printf("Child Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		exit();
	}
	else if (ret != -1) {
		while( i != 0) {
			i --;
			printf("Father Process: Sleeping.\n");
			sleep(128);
			printf("Father Process: Semaphore Posting.\n");
			sem_post(&sem);
		}
		printf("Father Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		exit();
	}

	

	// For lab4.3
	// TODO: You need to design and test the philosopher problem.
	// Note that you can create your own functions.
	// Requirements are demonstrated in the guide.
	
	//哲学家
	/*
	int j=0;
	int ret;
	sem_t forks[5];
	sem_t mutex;
	sem_init(&mutex, 1);
	for (int i=0;i<5;i++)
    	sem_init(&forks[i], 1);

	for(;j<4;++j)
	{
		ret = fork();
		if(ret == 0)
			break;
		else if(ret < 0)
		{
			printf("fork error\n");
			exit(1);
		}
	}

	while(1)
	{
		printf("Philosopher %d: think\n", j);
		sleep(128);
       	sem_wait(&mutex);
		sleep(128);				//
        sem_wait(&forks[j]);
		sleep(128);				//
        sem_wait(&forks[(j+1)%5]);
		sleep(128);				//
        sem_post(&mutex);
		sleep(128);				//
        printf("Philosopher %d: eat\n", j);
		sleep(128);
        sem_post(&forks[j]);
		sleep(128);				//
        sem_post(&forks[(j+1)%5]);
		sleep(128);				//
	}
	*/



	
	



/*
	//生产者消费者问题
sem_t mutex;
sem_t empty;
sem_t full;
sem_init(&empty,5);     	 //可以使用的空缓冲区数
sem_init(&full,0);  			 //缓冲区内可以使用的产品数
sem_init(&mutex,1);      	//互斥信号量
int in=0;								//放入缓冲区指针
int out=0;                         	//取出缓冲区指针

	int j=0;
	int ret;

	for(;j<4;++j)
	{
		ret = fork();
		if(ret == 0)
			break;
		else if(ret < 0)
		{
			printf("fork error\n");
			exit(1);
		}
	}

	while(1)
	{
		if(j==4)//consumer
		{
			sem_wait(&full);
			sem_wait(&mutex);
			out=(out+1)%5;
			sem_post(&mutex);
			sem_post(&empty);
			printf("Consumer : consume\n");
			sleep(128);	

		}
		else//producer
		{
			sem_wait(&empty);
			sem_wait(&mutex);
			in=(in+1)%5;  
			printf("Producer %d: produce\n", j);
			sleep(128);	
			sem_post(&mutex); 
			sem_post(&full);
		}
	}
	*/

	//读者写者问题
	

	exit(0);
	return 0;
}
