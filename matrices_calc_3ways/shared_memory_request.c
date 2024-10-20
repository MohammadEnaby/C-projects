#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define MAX_LEN 128
#define SHM_SIZE 2048
typedef struct {
    sem_t sem_writer;
    sem_t sem_reader;
    pthread_mutex_t mutex;
    char operation[MAX_LEN];
    int num_of_matrices;
    char matrices[MAX_LEN][MAX_LEN];
    int num_of_operations;
} shared_Data;

int create_shared_memory() {
    key_t SHM_KEY = ftok("/tmp", 'A');
    if (SHM_KEY == -1) {
        perror("ftok() failed");
        exit(EXIT_FAILURE);
    }

    int shm_id = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0600);
    if (shm_id < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    return shm_id;
}
void *attach_shared_memory(int shm_id) {
    void *shm_addr = shmat(shm_id, NULL, 0);
    if (shm_addr == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    return shm_addr;
}

bool is_operation(const char *input) {
    return strcmp(input, "ADD") == 0 || strcmp(input, "SUB") == 0 || strcmp(input, "MUL") == 0 ||
           strcmp(input, "AND") == 0 || strcmp(input, "OR") == 0 ||
           strcmp(input, "NOT") == 0 || strcmp(input, "TRANSPOSE") == 0;
}

int main() {
    int shm_id = create_shared_memory();
    void *shm_addr = attach_shared_memory(shm_id);
    shared_Data *shared_data;
    shared_data = (shared_Data *) shm_addr;
    sem_init(&shared_data->sem_writer, 1, 1);  // Initialize writer semaphore to 1
    sem_init(&shared_data->sem_reader, 1, 0);  // Initialize reader semaphore to 0
    pthread_mutex_init(&shared_data->mutex, NULL);
    shared_data->num_of_operations=0;
    while (1) {
        sem_wait(&shared_data->sem_writer);
        pthread_mutex_lock(&shared_data->mutex);

        char input[MAX_LEN];
        fgets(input, MAX_LEN, stdin); // Read for the first time to malloc memory and know the type of the matrices
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "END") == 0) {
            strcpy(shared_data->operation,"END");
            shmctl(shm_id,IPC_RMID,NULL);
            break;
        }

        shared_data->num_of_matrices = 0;

        if (!is_operation(input)) {
            shared_data->num_of_matrices++;
            strncpy(shared_data->matrices[0], input, MAX_LEN);
        }

        while (fgets(input, MAX_LEN, stdin) != NULL) { // Read and write them to the shm matrices until detecting an operation
            input[strcspn(input, "\n")] = '\0';
            if (is_operation(input)) break;
            if (strcmp(input, "END") == 0) {
                shmctl(shm_id,IPC_RMID,NULL);
                break;
            }
            shared_data->num_of_matrices++;
            strncpy(shared_data->matrices[1], input, MAX_LEN);
        }

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "END") == 0) {
            strcpy(shared_data->operation,"END");
            shared_data->num_of_operations=0;
            break;
        }

        strcpy(shared_data->operation, input);

        shared_data->num_of_operations++;

        pthread_mutex_unlock(&shared_data->mutex);

        sem_post(&shared_data->sem_reader);
    }
    sem_destroy(&shared_data->sem_reader);
    sem_destroy(&shared_data->sem_writer);
    pthread_mutex_destroy(&shared_data->mutex);
    shmdt(shared_data);
    return 0;
}

