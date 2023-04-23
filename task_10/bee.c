#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>

#define PIPE_BEE_NAME "bee.fifo"
#define PIPE_POT_NAME "pot.fifo"
#define PIPE_BEAR_NAME "bear.fifo"

#define SEM_PATH "/bee_sem"
#define SEM_KEY 'S'
// Семафор UNIX SYSTEM V
int sem;
// Определим состояния для семафора
struct sembuf lock = {0, -1, 0};
struct sembuf unlock = {0, 1, 0};

int pipe_bee;
int pipe_pot;
int pipe_bear;

int isTerminated = 0;

// Код выполняющийся при завершении
void terminationCode(int sig) {
    isTerminated = 1;
}

int main(int argc, char *argv[]) {
    int honey_lim = atoi(argv[1]);
//    int bees_amount = atoi(argv[2]);
    int amount_made = atoi(argv[2]);
    key_t key;
    key = ftok(SEM_PATH, SEM_KEY);

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
    if ((sem = semget(key, 1, 0660 | IPC_CREAT)) == -1) {
        perror ("semget: Can not create bee mutex semaphore");
        exit (1);
    }
    if (semctl(sem, 0, SETVAL, 1) == -1) {
        perror("semctl");
        exit(1);
    }
    // будет считывать сигналы прерывания с терминала
    signal(SIGINT, terminationCode);
    signal(SIGTERM, terminationCode);
    // bee process
    // Присоединение сегмента к адресному пространству процесса
    int answer = 0;
    while (isTerminated == 0) {
        // Критическая секция
        semop(sem, &lock, 1);
//        if (semop(sem, &lock, 1) == -1) {
//            perror("sem_wait: Incorrect wait of mutex to honey");
//            exit(-1);
//        };
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
        semop(sem, &unlock, 1);
//        if (semop(sem, &unlock, 1) == -1) {
//            perror("sem_post: Incorrect post of  mutex to honey");
//            exit(-1);
//        };
        sleep(2);
    }
    close(pipe_bee);
    close(pipe_pot);
    close(pipe_bear);
    if (semctl(sem, 0, IPC_RMID) == -1) {
        perror ("semctl IPC_RMID: can't close sem");
        exit (1);
    }
    return 0;

}