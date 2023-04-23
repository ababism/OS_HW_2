#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#define SH_INT "shared-amount"

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
    int sh_mem;
    int *amount_ptr;
    printf("bear appeared!\n");
    // Создаем разделяемую память
    if ((sh_mem = shm_open(SH_INT, O_EXCL | O_RDWR, 0666)) == -1) {
        perror("shm_open: Shared memory");
        return 1;
    }
    printf("Shared honey was found by bear: path = %s, adr = 0x%x\n", SH_INT, sh_mem);
//    if (ftruncate(sh_mem, sizeof(int)) == -1) {
//        perror("ftruncate: memory sizing error");
//        return 1;
//    }
    // подключаем память
    amount_ptr = mmap(0, sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, sh_mem, 0);
    *amount_ptr = 0;

    // Создаем именованный семафор
    if ((sem = sem_open(SEM_NAME, O_CREAT, 0666, 1)) == 0) {
        perror("sem_open: Can not create mutex semaphore");
        exit(-1);
    }

    // будет считывать сигналы прерывания с терминала
    signal(SIGINT, terminationCode);
    signal(SIGTERM, terminationCode);

    // Код процесса родителя (медведя)
    while (isTerminated == 0) {
        if (sem_wait(sem) == -1) {
            perror("sem_wait: Incorrect wait of bear mutex to honey");
            exit(-1);
        };
        if (*amount_ptr == -1) {
            isTerminated = 1;
        }
        if (*amount_ptr == honey_lim) {
            *amount_ptr = 0;
            printf("bear ate all honey!\n");
        }
        if (sem_post(sem) == -1) {
            perror("sem_post: Incorrect post of bear mutex to honey");
            exit(-1);
        }
        sleep(1);
    }
    printf("\nbear left!\n");
    close(sh_mem);
    if (shm_unlink(SH_INT) == -1) {
        perror("shm_unlink: mem already unlinked by bees");
    }
    if (sem_close(sem) == -1) {
        perror("sem_close: Incorrect close of mutex semaphore or already unlinked by bees");
        exit(-1);
    };
    if (sem_unlink(SEM_NAME) == -1) {
        perror("sem_unlink: Incorrect unlink of mutex semaphore or already unlinked by bees");
    };
    return 0;
}