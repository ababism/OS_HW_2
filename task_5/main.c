#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SH_INT "shared-honey"

int isTerminated = 0;

// Код выполняющийся при завершении
void terminationCode(int sig) {
    isTerminated = 1;
}

int main(int argc, char *argv[]) {
    sem_t sem;
    int honey_lim = atoi(argv[1]);
    int bees_amount = atoi(argv[2]);
    int sh_mem;
    int *amount_ptr;
    //создаю разделяемую память
    if ((sh_mem = shm_open(SH_INT, O_CREAT | O_EXCL | O_RDWR, 0666)) == -1) {
        perror("shm_open: Shared memory is already open");
        return 1;
    }
    printf("Shared honey is open: path = %s, adr = 0x%x\n", SH_INT, sh_mem);
    if (ftruncate(sh_mem, sizeof(int)) == -1) {
        perror("ftruncate: memory sizing error");
        return 1;
    }

    // подключаем память
    amount_ptr = mmap(0, sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, sh_mem, 0);
    *amount_ptr = 0;

    // Создаем семафор
    sem_init(&sem, 1, 1); // первое значение 1 значит, что используем семафор между процессами!

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
            amount_ptr = mmap(0, sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, sh_mem, 0);
            while (isTerminated == 0) {
                sem_wait(&sem);
                if (*amount_ptr < honey_lim) {
                    *amount_ptr += 1;
                    printf("bee %d is added 1 sip of honey, now: %d!\n", getpid(), *amount_ptr);
                    if (*amount_ptr == honey_lim) {
                        printf("bee %d is waking up bear!\n", getpid());
                    }
                }
                sem_post(&sem);
                sleep(2);
            }
            // Открепление сегмента разделяемой памяти
            close(sh_mem);
            sem_destroy(&sem);
            exit(0);
        }
    }
    // Код процесса родителя (медведя)
    while (isTerminated == 0) {
        sem_wait(&sem);
        if (*amount_ptr == honey_lim) {
            *amount_ptr = 0;
            printf("bear ate all honey!\n");
        }
        sem_post(&sem);
    }
    close(sh_mem);
    if (shm_unlink(SH_INT) == -1) {
        perror("shm_unlink");
    }
    sem_destroy(&sem);
    return 0;
}