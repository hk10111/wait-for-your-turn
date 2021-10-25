#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#define SIZE 10000
/*
Arguments format
argc : number of arguments
argv : n1,n2,n3
*/
int arr_num[SIZE];
long sum_p1 = 0;
int main(int argc, char *argv[])
{
    int n1 = atoi(argv[0]), n2 = atoi(argv[1]), n3 = atoi(argv[2]);
    int pid1 = 0, pid2 = 0, pid3 = 0;
    pid1 = fork();
    if (pid1 != 0)
    {
        sleep(5);
        pid2 = fork();
        if (pid2 != 0)
        {
            sleep(5);
            pid3 = fork();
            if (pid3 == 0)
            {
                printf("\n child 3 ");
                while (true)
                    sleep()
            }
        }
        else
        {
            printf("\n child 2 ");
            for (int i = 0; i < n1; i++)
            {
                arr_num[i] = rand();
            }
            while (true)
                sleep();
        }
    }
    else
    {
        printf("\n child 1 ");
        while (true)
            sleep();
    }
}

int process1(int start, int end)
{
    int i = 0;
    for (i = start; i < end; i++)
    {
        sum_p1 += arr_num[i];
        /*
         create 2nd thread Check for time quantum and break if over
        */
    }
    return i;
}

int process2(FILE *fp)
{
    char line[500];
    while (fgets(line, sizeof(line), fp))
    {
        printf("%s", line);
        /*create 2nd thread check for time quantum and break if over*/
    }
    return 0;
}

int process3(FILE *fp, int start)
{
    char line[500];
    while (fgets(line, sizeof(line), file))
    {
        sum_p3 += atoi(line);
        /*create 2nd thread check for time quantum and break if over*/
    }
    return sum_p3;
}
