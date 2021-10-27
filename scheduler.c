#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/mman.h>
#include <time.h>
#define SIZE 1000000
#define TIME_QT 0.001
/*
Arguments format
argc : number of arguments
argv : n1,n2,n3
*/
int arr_num[SIZE];
long sum_p1 = 0,sum_p3=0;
int process1(int,int,void*);
FILE* process2(FILE *fp,void* shmem);
int process3(FILE *fp, int start,void *shmem);
//int n1,n2,n3;
void* create_shared_memory(size_t size)
{
  // Our memory buffer will be readable and writable:
  int protection = PROT_READ | PROT_WRITE;

  // The buffer will be shared (meaning other processes can access it), but
  // anonymous (meaning third-party processes cannot obtain an address for it),
  // so only this process and its children will be able to use it:
  int visibility = MAP_SHARED | MAP_ANONYMOUS;

  // The remaining parameters to `mmap()` are not important for this use case,
  // but the manpage for `mmap` explains their purpose.
  return mmap(NULL, size, protection, visibility, -1, 0);
}

int main(int argc, char *argv[])
{
    int n1 = atoi(argv[1]), n2 = atoi(argv[2]), n3 = atoi(argv[3]);
    int pid1 = 0, pid2 = 0, pid3 = 0;

    int fd[2];
    pipe(fd);
    //creating a shared memory
    void* shmem = create_shared_memory(128);
    char parent_message[]="0";
    memcpy(shmem,parent_message, sizeof(parent_message));

    pid1 = fork();
    if (pid1 != 0)
    {
        //close the input end
        close(fd[1]);
        pid2 = fork();
        if (pid2 != 0)
        {
            pid3 = fork();
            if (pid3 == 0)
            {
              //  printf("\nchild 3 ");
              exit(0);
            }
            else
            {
              int returnStatus;
              sleep(2);
              parent_message[0]='1';
              memcpy(shmem,parent_message, sizeof(parent_message));

              sleep(0.001);
              printf("\nFirst burst over");
              parent_message[0]='0';
              memcpy(shmem,parent_message, sizeof(parent_message));
              sleep(0.1);
              parent_message[0]='1';
              memcpy(shmem,parent_message, sizeof(parent_message));
              printf("\nSecond burst starts");
              waitpid(pid1, &returnStatus, 0);
              long result_p1;
              read(fd[0], &result_p1, sizeof(result_p1));
              printf("\nMASTER PROCESS SUM FROM PROCESS 1 =  %lu\n",result_p1);
              /*
              clock_t init,t;
              init = clock();
              while(1)
              {
                t = clock() - init;
                double time_elapsed=((double)t)/CLOCKS_PER_SEC;
                if(time_elapsed>TIME_QT)
                {
                  parent_message=(parent_message+1)%2;
                  memcpy(shmem, &parent_message, sizeof(parent_message));
                  init=t;
                }
                if(time_elapsed>2)
                {
                  break;
                  /*
                  parent_message=1;
                  memcpy(shmem, &parent_message, sizeof(parent_message));
                }*/
              }
            }
        else
        {
            FILE *fp=fopen("random.txt","r");

            exit(0);
            //printf("\nchild 2 ");
        }
    }
    else
    {
        //close the output end of pipe
        close(fd[0]);
        printf("\nchild 1  %d\nmessage = %s\n",n1,shmem);
        time_t t;
        srand((unsigned) time(&t));
        for (int i = 0; i < n1; i++)
        {
            arr_num[i] = rand()%10000;
            //printf("\n%d %d",i,arr_num[i]);
        }
        int track=0;
        while(track<n1)
        {
          if(atoi(shmem)==1)
          {
            printf("\nProcess 1 is active\n");
            track=process1(track,n1,shmem);
            printf("\nSUM intermediate = %lu\n",sum_p1);
          }
        }
        write(fd[1],&sum_p1,sizeof(sum_p1));
        close(fd[1]);
    }
    return 0;
}
int Check_state(void* shmem,int i)
{
  if(atoi(shmem)!=i)
  return 0;
  else
  return 1;
}
int process1(int start, int end,void* shmem)
{
    int i = 0;
    for (i = start; i < end;i++)
    {
        if(Check_state(shmem,1)==0)
        break;
        sum_p1 += arr_num[i];
        //printf("\nSUM = %lu",sum_p1);

        //pthread_t thread_id;
        //pthread_create(&thread_id, NULL,Check_state, (void *)i);
        /*
         create 2nd thread Check for time quantum and break if over
        */
    }
    return i;
}

FILE* process2(FILE *fp,void* shmem)
{
    char line[500];
    while (fgets(line, sizeof(line), fp))
    {
        printf("%s", line);
        if(Check_state(shmem,2)==0)
        break;
        /*create 2nd thread check for time quantum and break if over*/
    }
    return fp;
}

int process3(FILE *fp, int start,void *shmem)
{
    char line[500];
    while (fgets(line, sizeof(line), fp))
    {
        sum_p3 += atoi(line);
        if(Check_state(shmem,3)==0)
        break;
        /*create 2nd thread check for time quantum and break if over*/
    }
    return sum_p3;
}
