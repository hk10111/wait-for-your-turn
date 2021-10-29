#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "children.c"

typedef void (*child_type)(void *shared_status);

pid_t start_child(void *shared_status, child_type child) {
  pid_t pid = fork();
  if (pid == 0) {
    child(shared_status);
    exit(0);
  }
  return pid;
}

int all_children_done(pid_t *children) {
  int status;
  for (int i = 0; i < 3; ++i) {
    status = waitpid(children[i], NULL, WNOHANG);
    if (status == 0)
      return 0;
  }
  return 1;
}

void schedule(int child, void *shared_status) {
  *((int *) shared_status) = child;
}

void schedule_rr(pid_t *children, void *shared_status) {
  int i = 0, status = 0;
  while (1) {
    if (all_children_done(children) == 1)
      break;

    // Only schedule a child that is still running
    if (waitpid(children[i], NULL, WNOHANG) == 0) {
      schedule(i, shared_status);
      printf("Child %d started.\n", i);
      sleep(QUANTUM);
    }

    i = (i + 1) % 3;
  }
  printf("Done.\n");
}

int main(int argc, char *argv[]) {
  void *shared_status = mmap(NULL,
                             sizeof(int),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS,
                             -1,
                             0);
  schedule(-1, shared_status);

  pid_t children[3];

  children[0] = start_child(shared_status, child_zero);
  children[1] = start_child(shared_status, child_one);
  children[2] = start_child(shared_status, child_two);

  schedule_rr(children, shared_status);

  return 0;
}

