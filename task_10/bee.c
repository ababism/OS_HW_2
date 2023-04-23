#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>

#define PIPE_BEE_NAME "pipe_bee.fifo"
#define PIPE_POT_NAME "pipe_pot.fifo"
#define PIPE_BEAR_NAME "pipe_bear.fifo"

int pipe_bee;
int pipe_pot;
int pipe_bear;

// Определим имя для семафора
#define SEM_NAME "/mutex-semaphore"
sem_t *sem;   // указатель на семафор читателей

int isTerminated = 0;

// Код выполняющийся при завершении
void terminationCode(int sig) {
    isTerminated = 1;
}

int main(int argc, char *argv[]) {
    int honey_lim = atoi(argv[1]);
//    int bees_amount = atoi(argv[2]);
    int amount_made = atoi(argv[2]);
    // подключаем pipe
    if ((pipe_bee = open(PIPE_BEE_NAME, O_WRONLY)) < 0) {
        printf("Can\'t open bee oFIFO\n");
        exit(-1);
    }
    if ((pipe_pot = open(PIPE_POT_NAME, O_RDONLY)) < 0) {
        printf("Can\'t open pot iFIFO\n");
        exit(-1);
    }
    if ((pipe_bear = open(PIPE_BEAR_NAME, O_WRONLY)) < 0) {
        printf("Can\'t open pot iFIFO\n");
        exit(-1);
    }
    // Создаем именованный семафор
    if ((sem = sem_open(SEM_NAME, O_CREAT, 0666, 1)) == 0) {
        perror("sem_open: Can not create mutex semaphore");
        exit(-1);
    }
    // будет считывать сигналы прерывания с терминала
    signal(SIGINT, terminationCode);
    signal(SIGTERM, terminationCode);
    // bee process
    // Присоединение сегмента к адресному пространству процесса
    int answer = 0;
    while (isTerminated == 0) {
        // Критическая секция
        if (sem_wait(sem) == -1) {
            perror("sem_wait: Incorrect wait of mutex to honey");
            exit(-1);
        };
        if (write(pipe_bee, &amount_made, sizeof(int)) < 0) {
            printf("Can\'t write string from FIFO\n");
            exit(-1);
        }
        if (read(pipe_pot, &answer, sizeof(int)) < 0) {
            printf("Can\'t read string from FIFO\n");
            exit(-1);
        }
        printf("bee %d is added %d sip of honey, now: %d!\n", getpid(), amount_made, answer);
        if (answer == honey_lim) {
            printf("bee %d is waking up bear!\n", getpid());
            int bear_sig = 1;
            // TODO написать медведю
            if (write(pipe_bear, &bear_sig, sizeof(int)) < 0) {
                printf("Can\'t write string from FIFO\n");
                exit(-1);
            }
        }
        if (answer == -1) {
            isTerminated = 1;
//            if (sem_post(sem) == -1) {
//                perror("sem_post: Incorrect post of  mutex to honey");
//                exit(-1);
//            };
//            break;
        }
        if (sem_post(sem) == -1) {
            perror("sem_post: Incorrect post of  mutex to honey");
            exit(-1);
        };
        sleep(2);
    }
    if (sem_close(sem) == -1) {
        perror("sem_close: Incorrect close of mutex semaphore");
        exit(-1);
    };
//    if (sem_unlink(SEM_NAME) == -1) {
//        perror("sem_unlink: Incorrect unlink of mutex semaphore");
//    };
    close(pipe_bee);
    close(pipe_pot);
    close(pipe_bear);
    return 0;
    if (sem_close(sem) == -1) {
        perror("sem_close: Incorrect close of mutex semaphore");
        exit(-1);
    };
    if (sem_unlink(SEM_NAME) == -1) {
        perror("sem_unlink: Incorrect unlink of mutex semaphore");
    };
}