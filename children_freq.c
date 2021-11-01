#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#define QUANTUM 1
#define BILLION  1000000000L

int check_status(void *shared_status)
{
  return *((int *) shared_status);
}

struct process_var
{
  void* shared_status;
  int active;
  int turn;
  int process_id;
};
typedef struct process_var process_var;

void monitor_comms(void* thread_ptr)
{
  process_var* thread_var;
  thread_var = (process_var*)thread_ptr;
  printf("\nComms Thread started\n");
  while(1)
  {
    if(check_status(thread_var->shared_status)==thread_var->process_id)
    {
      thread_var->turn=1;
    }
    else
    {
      thread_var->turn=0;
    }

    if(thread_var->active == 0)
    {
      break;
    }
  }
  printf("\nMonitor Thread ended\n");
}


void child_zero(void *shared_status, int fd[2], int n[3])
{
  struct timespec start, finish;
  clock_gettime( CLOCK_REALTIME, &start );

  struct timespec burst_start, burst_finish;

  int i = 0, enabled = 0, turn = 0, active = 1;
  long sum = 0L;
  close(fd[0]);
  double time_used = 0.0;

  pthread_t thread_monitor;
  process_var *pvar;

  pvar->shared_status = shared_status;
  pvar->turn = turn;
  pvar->active = active;
  pvar->process_id = 0;

  pthread_create(&thread_monitor, NULL, monitor_comms, (void *)pvar);

  while (i < n[0])
  {
    if (pvar->turn==1)
    {
        sum += i;
        i += 1;

        if(!enabled)
        {
          enabled=1;
          clock_gettime( CLOCK_REALTIME, &burst_start );
        }

    }
    else
    {
        if(enabled)
        {
          enabled=0;
          clock_gettime( CLOCK_REALTIME, &burst_finish);
          time_used = time_used + ((double)(burst_finish.tv_sec - burst_start.tv_sec)+( burst_finish.tv_nsec - burst_start.tv_nsec )/(1.0*BILLION));
        }
        printf("Child 0 suspended at i = %d.\n", i);
        sleep(QUANTUM);
    }
  }
  pvar->active=0;
  pthread_cancel(thread_monitor);

  printf("\nChild 0 sum = %lu.\n", sum);
  write(fd[1], &sum, sizeof(sum));
  clock_gettime( CLOCK_REALTIME, &finish );

  printf("\nChild 0 summary : Turnaround time = %f seconds\n",((double)(finish.tv_sec - start.tv_sec)+( finish.tv_nsec - start.tv_nsec )/ BILLION));
  printf("Time used = %f\n",time_used);
  close(fd[1]);

}

void child_one(void *shared_status, int fd[2], int n[3])
{
  return;
  struct timespec start, finish,burst_start,burst_finish;
  clock_gettime( CLOCK_REALTIME, &start );

  int i = 0,enabled=0;
  char line[500];
  FILE* fp=fopen("random1.txt","r");
  close(fd[1]);
  close(fd[0]);
  double time_used=0.0;


  while (i < n[1]) {
    if (check_status(shared_status) == 1)
    {
      if(fgets(line, sizeof(line), fp))
      {
        printf( "%d %s\n", i, line );
        i += 1;
      }
      if(!enabled)
      {
        enabled=1;
        clock_gettime( CLOCK_REALTIME, &burst_start );
      }
    }
    else
    {
      if(enabled)
      {
        enabled=0;
        clock_gettime( CLOCK_REALTIME, &burst_finish);
        time_used = time_used + ((double)(burst_finish.tv_sec - burst_start.tv_sec)+( burst_finish.tv_nsec - burst_start.tv_nsec )/(1.0*BILLION));
      }
      printf("Child 1 suspended at i = %d.\n", i);
      sleep(QUANTUM);
    }
  }

  fclose(fp);
  clock_gettime( CLOCK_REALTIME, &finish );

  printf("\nChild 1 summary : Turnaround time = %f seconds\n",((double)(finish.tv_sec - start.tv_sec)+( finish.tv_nsec - start.tv_nsec )/ BILLION));
  printf("Time used = %f\n",time_used);

}

void child_two(void *shared_status,int fd[2],int n[3]) {
  return;
  struct timespec start, finish,burst_start,burst_finish;
  clock_gettime( CLOCK_REALTIME, &start );

  int i = 0, enabled=0;
  long sum = 0L;
  FILE* fp=fopen("random1.txt","r");
  close(fd[0]);
  double time_used=0.0;

  while (i < n[2]){
    if (check_status(shared_status) == 2)
    {
      char line[500];
      int number=atoi(fgets(line, sizeof(line), fp));
      sum += number/1000;
      i += 1;

      if(!enabled)
      {
        enabled=1;
        clock_gettime( CLOCK_REALTIME, &burst_start );
      }
    }
    else
    {
      if(enabled)
      {
        enabled=0;
        clock_gettime( CLOCK_REALTIME, &burst_finish);
        time_used = time_used + ((double)(burst_finish.tv_sec - burst_start.tv_sec)+( burst_finish.tv_nsec - burst_start.tv_nsec )/(1.0*BILLION));
      }
      printf("\nChild 2 suspended at i = %d.\n", i);
      sleep(QUANTUM);
    }
  }

  fclose(fp);
  printf("Child 2 sum = %lu.\n", sum);
  write(fd[1], &sum, sizeof(sum));
  close(fd[1]);

  clock_gettime( CLOCK_REALTIME, &finish );

  printf("\nChild 2 summary : Turnaround time = %f seconds\n",((double)(finish.tv_sec - start.tv_sec)+( finish.tv_nsec - start.tv_nsec )/ BILLION));
  printf("Time used = %f\n",time_used);
}
