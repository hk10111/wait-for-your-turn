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

int arr_num[SIZE];
long sum_p1 = 0,sum_p3=0;
int process1(int,int,void*);

void* create_shared_memory(size_t size)
{
  int protection = PROT_READ | PROT_WRITE;
  int visibility = MAP_SHARED | MAP_ANONYMOUS;
  return mmap(NULL, size, protection, visibility, -1, 0);
}

int main(int argc, char *argv[])
{
    int n1 = atoi(argv[1]), n2 = atoi(argv[2]), n3 = atoi(argv[3]);
    int pid1 = 0, pid2 = 0, pid3 = 0;

    int fd[2];
    pipe(fd);
    void* shmem = create_shared_memory(128);
    char parent_message[]="0";
    memcpy(shmem,parent_message, sizeof(parent_message));

    pid1 = fork();
    if (pid1 != 0)
    {
        close(fd[1]);
        pid2 = fork();
        if (pid2 != 0)
        {
            pid3 = fork();
            if (pid3 == 0)
            {
              exit(0);
            }
            else
            {
              int returnStatus;
              sleep(2);
              parent_message[0]='1';
              memcpy(shmem,parent_message, sizeof(parent_message));
              }
            }
        else
        {
            FILE *fp=fopen("random.txt","r");
            exit(0);
        }
    }
    else
    {
        close(fd[0]);
        time_t t;
        srand((unsigned) time(&t));
        for (int i = 0; i < n1; i++)
        {
            arr_num[i] = rand()%10000;
        }
        int track=0;
        while(track<n1)
        {
          if(atoi(shmem)==1)
          {
            track=process1(track,n1,shmem);
          }
        }
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
    }
    return i;
}

FILE* process2(FILE *fp)
{
    char line[500];
    while (fgets(line, sizeof(line), fp))
    {
        printf("%s", line);
        if(Check_state(shmem,2)==0)
        break;
    }
    return fp;
}

int process3(FILE *fp, int start)
{
    char line[500];
    while (fgets(line, sizeof(line), fp))
    {
        sum_p3 += atoi(line);
        if(Check_state(shmem,3)==0)
        break;
    }
    return sum_p3;
}
