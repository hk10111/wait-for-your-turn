#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

void* create_shared_memory(size_t size) {
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

#include <string.h>
#include <unistd.h>

int main() {
  char parent_message[] = "hello";  // parent process will write this message
  char child_message[] = "goodbye"; // child process will then write this one
  int parent=0;
  void* shmem = create_shared_memory(128);

  memcpy(shmem, &parent, sizeof(parent));

  int pid = fork();

  if (pid == 0) {
    printf("Child read: %d\n", atoi(shmem));
    memcpy(shmem, child_message, sizeof(child_message));
    printf("Child wrote: %s\n", shmem);

  } else {
    printf("Parent read: %s\n", shmem);
    sleep(1);
    printf("After 1s, parent read: %s\n", shmem);
  }
}
