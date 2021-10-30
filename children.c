#include <stdio.h>
#include <unistd.h>

#define QUANTUM 1 

int check_status(void *shared_status) {
  return *((int *) shared_status);
}

void child_zero(void *shared_status) {
  int i = 0, sum = 0;
  while (i < 10) {
    if (check_status(shared_status) == 0) {
      sum += i;
      i += 1;
    } else {
      printf("Child 0 suspended at i = %d.\n", i);
      sleep(QUANTUM);
    }
  }
  printf("Child 0 sum = %d.\n", sum);
}

void child_one(void *shared_status) {
  int i = 0, sum = 0;
  while (i < 50) {
    if (check_status(shared_status) == 0) {
      sum += i;
      i += 1;
    } else {
      printf("Child 1 suspended at i = %d.\n", i);
      sleep(QUANTUM);
    }
  }
  printf("Child 1 sum = %d.\n", sum);
}

void child_two(void *shared_status) {
  int i = 0, sum = 0;
  while (i < 100) {
    if (check_status(shared_status) == 0) {
      sum += i;
      i += 1;
    } else {
      printf("Child 2 suspended at i = %d.\n", i);
      sleep(QUANTUM);
    }
  }
  printf("Child 2 sum = %d.\n", sum);
}



