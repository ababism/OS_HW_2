#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>

#define SHM_ADR "/shared"
#define SHM_KEY 'K'

#define SHM_SIZE 1024

// Семафор UNIX SYSTEM V
int sem;

// Определим состояния для семафора
struct sembuf lock = {0, -1, 0};
struct sembuf unlock = {0, 1, 0};

int isTerminated = 0;

// Код выполняющийся при завершении
void terminationCode(int sig) {
    isTerminated = 1;
}

int main(int argc, char *argv[]) {
    int honey_lim = atoi(argv[1]);
    int bees_amount = atoi(argv[2]);
    key_t key;
    int sh_mem;
    int *shared_int;
    // Генерация ключа
    if ((key = ftok(SHM_ADR, SHM_KEY)) == -1)
        perror("ftok");
    // Создание сегмента разделяемой памяти
    if ((sh_mem = shmget(key, sizeof(int), 0660 | IPC_CREAT)) == -1)
        perror("shmget");
    // Присоединение сегмента к адресному пространству процесса
    if ((shared_int = (int *) shmat(sh_mem, NULL, 0)) == (int *) -1)
        perror("shmat");
    *shared_int = 0;

    // Создаем два семафора
    if ((sem = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget");
        exit(1);
    }
    if (semctl(sem, 0, SETVAL, 1) == -1) {
        perror("semctl");
        exit(1);
    }
    // будет считывать сигналы прерывания с терминала
    signal(SIGINT, terminationCode);
    signal(SIGTERM, terminationCode);

    for (int i = 0; i < bees_amount; i++) {
        //Новый процесс пчелы
        pid_t bee_pid = fork();
        if (bee_pid == -1) {
            perror("fork: can't fork new bee");
            exit(1);
        } else if (bee_pid == 0) {  // bee process
            // Присоединение сегмента к адресному пространству процесса
            if ((shared_int = (int *) shmat(sh_mem, NULL, 0)) == (int *) -1)
                perror("shmat");
            while (isTerminated == 0) {
                semop(sem, &lock, 1);
                if (*shared_int < honey_lim) {
                    *shared_int += 1;
                    printf("bee %d is added 1 sip of honey, now: %d!\n", getpid(), *shared_int);
                    if (*shared_int == honey_lim) {
                        printf("bee %d is waking up bear!\n", getpid());
                    }
                }
                semop(sem, &unlock, 1);
                sleep(2);
            }
            // Открепление сегмента разделяемой памяти
            if (shmdt(shared_int) == -1)
                perror("shmdt");
            semctl(sem, 0, IPC_RMID, NULL);
            semctl(sem, 0, IPC_RMID, NULL);
            exit(0);
        }

    }
    // код родителя (медведя)
    while (isTerminated == 0) {
        semop(sem, &lock, 1);
        if (*shared_int == honey_lim) {
            *shared_int = 0;
            printf("bear %d ate all honey!\n", getpid());
        }
        semop(sem, &unlock, 1);
        sleep(1);
    }
    // Открепление сегмента разделяемой памяти
    if (shmdt(shared_int) == -1) {
        perror("shmdt");
        exit(1);
    }
    /* Уничтожение сегмента разделяемой памяти */
    if (shmctl(sh_mem, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }
    semctl(sem, 0, IPC_RMID, NULL);
    semctl(sem, 0, IPC_RMID, NULL);
    return 0;
}