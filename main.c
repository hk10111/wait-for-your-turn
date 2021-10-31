#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#define QUANTUM 1

typedef struct {
  int current;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} shared_t;

typedef void (*child_t)(shared_t *shr);

/**** Children ********************************************************/

void child_zero(shared_t *shr) {
  int i = 0, sum = 0;
  while (i < 10) {
    if (shr->current == 0) {
      sum += i;
      i += 1;
    } else {
      pthread_mutex_lock(&shr->mutex);
      while (shr->current != 0)
        pthread_cond_wait(&shr->cond, &shr->mutex);
      pthread_mutex_unlock(&shr->mutex);
    }
  }
  printf("Child 0 sum = %d.\n", sum);
  pthread_cond_broadcast(&shr->cond);
}

void child_one(shared_t *shr) {
  int i = 0, sum = 0;
  while (i < 50) {
    if (shr->current == 1) {
      sum += i;
      i += 1;
    } else {
      pthread_mutex_lock(&shr->mutex);
      while (shr->current != 1)
        pthread_cond_wait(&shr->cond, &shr->mutex);
      pthread_mutex_unlock(&shr->mutex);
    }
  }
  printf("Child 1 sum = %d.\n", sum);
  pthread_cond_broadcast(&shr->cond);
}

void child_two(shared_t *shr) {
  int i = 0, sum = 0;
  while (i < 100) {
    if (shr->current == 2) {
      sum += i;
      i += 1;
    } else {
      pthread_mutex_lock(&shr->mutex);
      while (shr->current != 2)
        pthread_cond_wait(&shr->cond, &shr->mutex);
      pthread_mutex_unlock(&shr->mutex);
    }
  }
  printf("Child 2 sum = %d.\n", sum);
  pthread_cond_broadcast(&shr->cond);
}

/**********************************************************************/

pid_t start_child(shared_t *shr, child_t child) {
  pid_t pid = fork();
  if (pid == 0) {
    child(shr);
    exit(0);
  }
  return pid;
}

int is_alive(pid_t pid) {
  return waitpid(pid, NULL, WNOHANG) == 0;
}

int all_children_dead(pid_t *children) {
  for (int i = 0; i < 3; ++i) {
    if (is_alive(children[i]))
      return 0;
  }
  return 1;
}

void schedule_rr(pid_t *children, shared_t *shr) {
  struct timespec time;
  int current = 0;

  while (1) {
    if (all_children_dead(children) == 1)
      break;

    if (is_alive(children[current])) {
      pthread_mutex_lock(&shr->mutex);
      shr->current = current;
      pthread_cond_broadcast(&shr->cond);
      printf("Child %d started.\n", current);
      pthread_mutex_unlock(&shr->mutex);

      clock_gettime(CLOCK_REALTIME, &time);
      time.tv_sec += QUANTUM; 

      pthread_mutex_lock(&shr->mutex);
      while (is_alive(children[current])) {
        int err = pthread_cond_timedwait(&shr->cond, &shr->mutex, &time);
        if (err == ETIMEDOUT)
          break;
      }
      pthread_mutex_unlock(&shr->mutex);
    }
    current = (current + 1) % 3;
  }

  printf("Done.\n");
}

int main() {
  shared_t *shr = mmap(NULL,
                       sizeof(shared_t),
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS,
                       -1, 0);

  pthread_mutexattr_t mutex_attr;
  pthread_mutexattr_init(&mutex_attr);
  pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);

  pthread_mutex_init(&shr->mutex, &mutex_attr);

  pthread_condattr_t cond_attr;
  pthread_condattr_init(&cond_attr);
  pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);

  pthread_cond_init(&shr->cond, &cond_attr);

  pid_t children[3];

  children[0] = start_child(shr, child_zero);
  children[1] = start_child(shr, child_one);
  children[2] = start_child(shr, child_two);

  schedule_rr(children, shr);

  pthread_mutex_destroy(&shr->mutex);
  pthread_mutexattr_destroy(&mutex_attr);

  pthread_cond_destroy(&shr->cond);
  pthread_condattr_destroy(&cond_attr);

  munmap(shr, sizeof(shared_t));

  return 0;
}

