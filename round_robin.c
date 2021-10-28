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
long sum_p1 = 0;
int process1(int,int,void*);
void process2(FILE *fp,void* shmem,int *,int n2);
FILE* process3(FILE *fp,long *sum_p3,void *shmem,int n3,int*);
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

    //Creating pipes
    int fd1[2],fd3[2];
    pipe(fd1);
    pipe(fd3);


    //Creating a shared memory
    void* shmem = create_shared_memory(128);
    char parent_message[]="0";
    memcpy(shmem,parent_message, sizeof(parent_message));
    int process_queue[4]={0,1,1,1};

    pid1 = fork();
    if (pid1 != 0)
    {
        //close the input end of both pipes
        close(fd1[1]);

        pid2 = fork();

        if (pid2 != 0)
        {
            pid3 = fork();

            if (pid3!= 0)
            {
              // Master Process
              close(fd3[1]);
              int returnStatus;
              while(1)
              {
                int flag=0;
                printf("\nStage ");
                for(int i=1;i<4;i++)
                {
                  printf("%d ",process_queue[i]);
                  if(process_queue[i]!=0)
                  {
                    flag=1;
                  }
                }
                printf("\n");
                if(!flag)
                {
                  break;
                }
                for(int i=1;i<4;i++)
                {
                  if(process_queue[i]==0)
                  continue;
                  time_t time_start=clock();
                  parent_message[0]=(char)(i+'0');
                  memcpy(shmem,parent_message, sizeof(parent_message));
                  while((double)((clock()-time_start)/CLOCKS_PER_SEC)<TIME_QT&&atoi(shmem)==i)
                  {
                    ;
                  }

                  if(atoi(shmem)==0)
                  {
                    process_queue[i]=0;
                  }
                }
              }

              long result_p1=0L;
              read(fd1[0], &result_p1, sizeof(result_p1));
              close(fd1[0]);
              printf("\nTHIS IS THE MASTER PROCESS, SUM FROM PROCESS 1 =  %lu\n",result_p1);
              //waitpid(pid1, &returnStatus, 0);

              long result_p3=0L;
              read(fd3[0], &result_p3, sizeof(result_p3));
              close(fd3[0]);
              printf("\nTHIS IS THE MASTER PROCESS, SUM FROM PROCESS 3 =  %lu\n",result_p3);
              //waitpid(pid3, &returnStatus, 0);
              exit(0);

              }
              else
              {
                //Child process 3
                char child_message[]="0";
                long sum_p3=0;
                close(fd3[0]);
                FILE* fp=fopen("random.txt","r");
                int lines_summed=0;
                while(lines_summed<n3)
                {
                  if(atoi(shmem)==3)
                  fp=process3(fp,&sum_p3,shmem,n3,&lines_summed);
                }
                printf("\nChild process 3 sum = %lu",sum_p3);
                write(fd3[1],&sum_p3,sizeof(sum_p3));
                close(fd3[1]);
                process_queue[3]=0;
                printf("\nProcess 3 over");
                memcpy(shmem,child_message, sizeof(child_message));


              }
            }
        else
        {
            //printf("\nchild process 2 ");
            char child_message[]="0";
            FILE *fp1=fopen("random.txt","r");
            int lines_printed=0;
            printf("\nChild 2 printing");
            printf("\nShared = %d",atoi(shmem));
            while(lines_printed<n2)
            {
              if(atoi(shmem)==2)
              {
                printf("\nYES");
                process2(fp1,shmem,&lines_printed,n2);
              }
            }
            fclose(fp1);
            process_queue[2]=0;
            printf("\nProcess 2 over");
            memcpy(shmem,child_message, sizeof(child_message));

        }
    }
    else
    {
        //close the output end of pipe
        close(fd1[0]);
        char child_message[]="0";
        //printf("\nchild 1  %d\nmessage = %s\n",n1,(char*)shmem);
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
        write(fd1[1],&sum_p1,sizeof(sum_p1));
        close(fd1[1]);
        process_queue[1]=0;
        printf("\nProcess 1 over");
        memcpy(shmem,child_message, sizeof(child_message));


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

void process2(FILE *fp,void* shmem,int *lines_printed,int n2)
{
    printf("\nProcess 2\n");
    char line[500];
    while (fgets(line, sizeof(line), fp)&&*lines_printed<n2)
    {
        printf("%s", line);
        *lines_printed=*lines_printed+1;
        if(Check_state(shmem,2)==0)
        break;
        /*create 2nd thread check for time quantum and break if over*/
    }
}

FILE* process3(FILE *fp,long *sum_p3,void *shmem,int n3,int *lines_summed)
{
    char line[500];
    while (fgets(line, sizeof(line), fp)&&*lines_summed<n3)
    {
        printf("\n%d %s",*lines_summed,line);
        *lines_summed=*lines_summed+1;
        *sum_p3 =*sum_p3+ atoi(line);
        if(Check_state(shmem,3)==0)
        break;
        /*create 2nd thread check for time quantum and break if over*/
    }
    return fp;
}
