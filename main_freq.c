#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "children.c"

typedef void (*child_type)(void *shared_status,int fd[2],int n[3]);

pid_t start_child(void *shared_status, child_type child, int fd[2], int n[3]) {
  pipe(fd);
  pid_t pid = fork();
  if (pid == 0) {
    child(shared_status,fd,n);
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

  int fd0[2], fd1[2], fd2[2];

  pipe(fd0);
  pipe(fd1);
  pipe(fd2);

  int n[3]={0,0,0};
  for(int i=0; i<3; i++)
  {
    n[i]=atoi(argv[i+1]);
  }

  children[0] = start_child(shared_status, child_zero, fd0, n);
  children[1] = start_child(shared_status, child_one, fd1, n);
  children[2] = start_child(shared_status, child_two, fd2, n);

  schedule_rr(children, shared_status);

  long result1 = 0;
  read(fd0[0], &result1, sizeof(result1));
  printf("Result from process 1 = %lu\n", result1);

  long result3 = 0;
  read(fd2[0], &result3, sizeof(result3));
  printf("Result from process 3 = %lu\n",result3);

  return 0;
}
