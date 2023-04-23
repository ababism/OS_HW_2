#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PIPE_BEE_NAME "bee.fifo"
#define PIPE_POT_NAME "pot.fifo"
//#define SEM_PATH "/bee_sem"
//#define SEM_KEY 'S'
int pipe_bee;
int pipe_pot;

int isTerminated = 0;

// Код выполняющийся при завершении
void terminationCode(int sig) {
    isTerminated = 1;
}

int main(int argc, char *argv[]) {
    int honey_lim = atoi(argv[1]);
//    int bees_amount = atoi(argv[2]);
//    key_t key;
//    key = ftok(SEM_PATH, SEM_KEY);
    // Задаем количество меда
    int amount = 0;
    int new_honey = 0;
    mkfifo(PIPE_BEE_NAME, 0666);
    if ((pipe_bee = open(PIPE_BEE_NAME, O_RDONLY)) < 0) {
        printf("Can\'t open bee iFIFO\n");
        exit(-1);
    }
//    mknod(PIPE_BEE_NAME, S_IFIFO | 0666, 0);
//    if ((pipe_bee = open(PIPE_BEE_NAME, O_RDONLY)) < 0) {
//        printf("Can\'t open bee iFIFO\n");
//        exit(-1);
//    }
    mkfifo(PIPE_POT_NAME, 0666);
    if ((pipe_pot = open(PIPE_POT_NAME, O_WRONLY)) < 0) {
        printf("Can\'t open pot oFIFO\n");
        exit(-1);
    };
//    mknod(PIPE_POT_NAME, S_IFIFO | 0666, 0);
//    if ((pipe_pot = open(PIPE_POT_NAME, O_WRONLY)) < 0) {
//        printf("Can\'t open pot oFIFO\n");
//        exit(-1);
//    }
    // будет считывать сигналы прерывания с терминала
    signal(SIGINT, terminationCode);
    signal(SIGTERM, terminationCode);

    // Код процесса родителя (медведя)
    while (isTerminated == 0) {
        if (read(pipe_bee, &new_honey, sizeof(int)) < 0) {
            printf("Can\'t read string from FIFO\n");
            exit(-1);
        }
        amount += new_honey;
        if (amount > honey_lim) {
            amount = honey_lim;
        }
        if (write(pipe_pot, &amount, sizeof(int)) < 0) {
            printf("Can\'t write string from FIFO\n");
            exit(-1);
        }
        if (amount == honey_lim) {
            amount = 0;
            printf("pot is full!\n");
        }
    }
//    if (sem_unlink(SEM_NAME) == -1) {
//        perror("sem_unlink: Incorrect unlink of mutex semaphore");
//    };
    if (close(pipe_pot) < 0) {
        printf("child: Can\'t close pot_pipe\n");
        exit(-1);
    }
    if (close(pipe_bee) < 0) {
        printf("child: Can\'t close pot_pipe\n");
        exit(-1);
    }
    return 0;
}