#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#define QUANTUM_NSEC 1e6 /* Keep < 1e9 to prevent overflow */
#define QUANTUM_SEC 1

#define ALIVE 0
#define DEAD 1

typedef struct {
  /* Make children write their status here
   * because checking process status was bad */
  int status[3];

  int current;
  pthread_mutex_t mutex;
  pthread_cond_t cond;

} shared_t;

typedef void (*child_t)(shared_t *shr);

/**** Children ********************************************************/

void wait_for_your_turn(shared_t *shr, int ID, struct timespec *wait) {
  struct timespec start, end;
  clock_gettime(CLOCK_REALTIME, &start);

  pthread_mutex_lock(&shr->mutex);
  while (shr->current != ID)
    pthread_cond_wait(&shr->cond, &shr->mutex);
  pthread_mutex_unlock(&shr->mutex);

  clock_gettime(CLOCK_REALTIME, &end);
  wait->tv_sec += end.tv_sec - start.tv_sec;
  wait->tv_nsec += end.tv_nsec - start.tv_nsec;
}

void child_zero(shared_t *shr) {
  int ID = 0;

  struct timespec wait = {0, 0};

  int i = 0, sum = 0;
  while (i < 100) {
    if (shr->current == ID) {
      sum += i;
      i += 1;
    } else {
      wait_for_your_turn(shr, ID, &wait);
    }
  }
  // printf("Child 0 sum = %d.\n", sum);

  printf("Child %d waiting time = %ld s %ld nsec.\n",
      ID,
      wait.tv_sec,
      wait.tv_nsec);

  shr->status[ID] = DEAD;
  pthread_cond_broadcast(&shr->cond);
}

void child_one(shared_t *shr) {
  int ID = 1;

  struct timespec wait = {0, 0};

  int i = 0, sum = 0;
  while (i < 1000) {
    if (shr->current == ID) {
      sum += i;
      i += 1;
    } else {
      wait_for_your_turn(shr, ID, &wait);
    }
  }
  // printf("Child 0 sum = %d.\n", sum);

  printf("Child %d waiting time = %ld s %ld nsec.\n",
      ID,
      wait.tv_sec,
      wait.tv_nsec);

  shr->status[ID] = DEAD;
  pthread_cond_broadcast(&shr->cond);
}

void child_two(shared_t *shr) {
  int ID = 2;

  struct timespec wait = {0, 0};

  int i = 0, sum = 0;
  while (i < 100000) {
    if (shr->current == ID) {
      sum += i;
      i += 1;
    } else {
      wait_for_your_turn(shr, ID, &wait);
    }
  }
  // printf("Child 0 sum = %d.\n", sum);

  printf("Child %d waiting time = %ld s %ld nsec.\n",
      ID,
      wait.tv_sec,
      wait.tv_nsec);

  shr->status[ID] = DEAD;
  pthread_cond_broadcast(&shr->cond);
}

/**********************************************************************/

void start_child(shared_t *shr, child_t child, int id) {
  pid_t pid = fork();
  if (pid == 0) {
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    child(shr);

    clock_gettime(CLOCK_REALTIME, &end);
    printf("TAT for child %d: %ld s %ld nsec\n",
        id,
        end.tv_sec - start.tv_sec,
        end.tv_nsec - start.tv_nsec);

    exit(0);
  }
}

int all_children_dead(shared_t *shr) {
  for (int i = 0; i < 3; ++i) {
    if (shr->status[i] == ALIVE)
      return 0;
  }
  return 1;
}

void schedule_rr(shared_t *shr) {
  struct timespec time;
  int current = 0;

  while (1) {
    if (all_children_dead(shr) == 1)
      break;

    if (shr->status[current] == ALIVE) {
      pthread_mutex_lock(&shr->mutex);
      shr->current = current;
      pthread_cond_broadcast(&shr->cond);
      printf("Child %d started.\n", current);
      pthread_mutex_unlock(&shr->mutex);

      clock_gettime(CLOCK_REALTIME, &time);
      time.tv_sec += QUANTUM_SEC;
      time.tv_nsec += QUANTUM_NSEC;

      pthread_mutex_lock(&shr->mutex);
      while (shr->status[current] == ALIVE) {
        int err = pthread_cond_timedwait(&shr->cond, &shr->mutex, &time);
        if (err == ETIMEDOUT) {
          printf("Timed out!\n");
          break;
        }
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

  memset(shr->status, ALIVE, 3 * sizeof(int));

  pthread_mutexattr_t mutex_attr;
  pthread_mutexattr_init(&mutex_attr);
  pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);

  pthread_mutex_init(&shr->mutex, &mutex_attr);

  pthread_condattr_t cond_attr;
  pthread_condattr_init(&cond_attr);
  pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);

  pthread_cond_init(&shr->cond, &cond_attr);

  start_child(shr, child_zero, 0);
  start_child(shr, child_one, 1);
  start_child(shr, child_two, 2);

  schedule_rr(shr);

  pthread_mutex_destroy(&shr->mutex);
  pthread_mutexattr_destroy(&mutex_attr);

  pthread_cond_destroy(&shr->cond);
  pthread_condattr_destroy(&cond_attr);

  munmap(shr, sizeof(shared_t));

  return 0;
}

