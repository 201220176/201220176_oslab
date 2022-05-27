#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{

    int fd[2];
    char buffer[1024];
    //copy the FCB of STDOUT
    int stdoutcopy = dup(STDOUT_FILENO);
    //set the pipe and get the file descriptors
    pipe(fd);
    //copy the write fd to STDOUT
    dup2(fd[1], STDOUT_FILENO);
    //system
    system("echo \"Hello, shell!\"");
    //read from the read port
    int read_size=read(fd[0],buffer,1024);
    //reopen the STDOUT
    dup2(stdoutcopy, STDOUT_FILENO);
    //test the result
    printf("the father process read %d bytes from the child process:",read_size);
    printf("%s\n", buffer);
}