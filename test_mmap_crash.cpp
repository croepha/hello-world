/*

This is to serve a proof that memory that is MAP_SHARED to a
 file on the filesystem will be persisted on crash
 
# clang -Ofast test_mmap_crash.cpp -o build/test_mmap_crash.exec
# build/test_mmap_crash.exec
Segmentation fault
# cat /tmp/mmap_crash_test_data
wrote some data!!!

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

const char * path = "/tmp/mmap_crash_test_data";
int size = 1024;

int main () {
    unlink(path);
    auto fd = open(path, O_CREAT | O_RDWR, 0644);
    ftruncate(fd, size);
    auto data = (char *)mmap(0, size, PROT_WRITE | PROT_WRITE, MAP_SHARED, fd, 0);
    snprintf(data, size, "wrote some data!!!\n");
    //kill(getpid(), SIGKILL);
    kill(getpid(), SIGSEGV);
    printf("never get here\n");
}


