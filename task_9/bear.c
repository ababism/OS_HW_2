#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define PIPE_BEAR_NAME "pipe_bear.fifo"

int pipe_bear;

int isTerminated = 0;

// Код выполняющийся при завершении
void terminationCode(int sig) {
    isTerminated = 1;
}

int main(int argc, char *argv[]) {
//    int honey_lim = atoi(argv[1]);
//    int bees_amount = atoi(argv[2]);
    // Задаем сигнал пробуждения меда
    int sig = 0;
    mknod(PIPE_BEAR_NAME, S_IFIFO | 0666, 0);
    if ((pipe_bear = open(PIPE_BEAR_NAME, O_RDONLY)) < 0) {
        printf("Can\'t open bear iFIFO\n");
        exit(-1);
    }
    // будет считывать сигналы прерывания с терминала
    signal(SIGINT, terminationCode);
    signal(SIGTERM, terminationCode);
    // Код процесса родителя (медведя)
    while (isTerminated == 0) {
        if (read(pipe_bear, &sig, sizeof(int)) < 0) {
            printf("Can\'t read string from FIFO\n");
            if (close(pipe_bear) < 0) {
                printf("child: Can\'t close pot_pipe\n");
                exit(-1);
            }
            exit(-1);
        }
        if (sig == 1) {
            printf("Bear ate honey!\n");
        } else {
            isTerminated = 1;
        }
    }
    if (close(pipe_bear) < 0) {
        printf("child: Can\'t close pot_pipe\n");
        exit(-1);
    }
    return 0;
}