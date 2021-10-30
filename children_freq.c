#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#define QUANTUM 2

int check_status(void *shared_status) {
  return *((int *) shared_status);
}

double find_time(time_t start,time_t end)
{
  return (double)((end-start)/CLOCKS_PER_SEC);
}

void child_zero(void *shared_status, int fd[2], int n[3]) {
  time_t start=clock();
  time_t burst_start,burst_end,end;

  int i = 0;
  long sum = 0L;
  close(fd[0]);
  double time_used =0;
  int enabled=0;

  while (i < n[0]) {
    if (check_status(shared_status) == 0)
    {
        sum += i;
        i += 1;

        if(!enabled)
        {
          enabled=1;
          burst_start=clock();
        }
    }
    else
    {
        enabled=0;
        burst_end=clock();
        time_used+=find_time(burst_start,burst_end);
        printf("Child 0 suspended at i = %d.\n", i);
        sleep(QUANTUM);
    }
  }
  printf("\nChild 0 sum = %lu.\n", sum);
  write(fd[1], &sum, sizeof(sum));
  end=clock();


  double turnaround_time = find_time(start,end);
  double time_wait=turnaround_time-time_used;

  printf("\nProcess 1 \nTime used =%f seconds\nwaiting time = %f seconds\nTurnaround time = %f seconds\n",time_used,time_wait,turnaround_time);
  close(fd[1]);

}

void child_one(void *shared_status, int fd[2], int n[3]) {
  time_t start,burst_start,burst_end,end;
  start=clock();

  double time_used =0.0;

  int i = 0,enabled=0;
  char line[500];
  FILE* fp=fopen("random.txt","r");
  close(fd[1]);
  close(fd[0]);
  while (i < n[1]) {
    if (check_status(shared_status) == 1) {

      if(!enabled)
      {
        enabled=1;
        burst_start=clock();
      }

      if(fgets(line, sizeof(line), fp))
      {
        printf( "%d %s\n", i, line );
        i += 1;
      }
    } else {
      enabled=0;
      burst_end=clock();
      time_used+=find_time(burst_start,burst_end);
      printf("Child 1 suspended at i = %d.\n", i);
      sleep(QUANTUM);
    }
  }

  fclose(fp);
  end=clock();
  double turnaround_time = find_time(start,end);
  double time_wait=turnaround_time-time_used;

  printf("\nProcess 2\nTime used = %f seconds\nwaiting time = %f seconds\nTurnaround time = %f seconds\n",time_used,time_wait,turnaround_time);


}

void child_two(void *shared_status,int fd[2],int n[3]) {
  time_t start,burst_start,burst_end,end;
  start=clock();

  double time_used =0.0;

  int i = 0,enabled=0;
  long sum = 0L;
  FILE* fp=fopen("random1.txt","r");
  close(fd[0]);

  while (i < n[2]){
    if (check_status(shared_status) == 2) {

      char line[500];
      int number=atoi(fgets(line, sizeof(line), fp));
      sum += number;
      i += 1;

      if(!enabled)
      {
        enabled=1;
        burst_start=clock();
      }

      }
      else {
      enabled=0;
      burst_end=clock();
      time_used+=find_time(burst_start,burst_end);
      printf("Child 2 suspended at i = %d.\n", i);
      sleep(QUANTUM);
    }
  }

  end=clock();
  fclose(fp);
  printf("Child 2 sum = %lu.\n", sum);
  write(fd[1], &sum, sizeof(sum));
  close(fd[1]);

  end=clock();
  double turnaround_time = find_time(start,end);
  double time_wait=turnaround_time-time_used;

  printf("\nProcess 3 \ntime used = %f seconds\nwaiting time = %f seconds\nTurnaround time = %f seconds\n",time_used,time_wait,turnaround_time);
}
