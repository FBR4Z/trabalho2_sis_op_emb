#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>

// Configurações do sistema
#define NUM_WORKERS         2
#define NUM_THREADS         3
#define MAX_FILENAME        256
#define MAX_PATH            512
#define MAX_IMAGES          100
#define MAX_MSG_SIZE        512
#define MAX_QUEUE_MSGS      10

// Nomes dos recursos IPC
#define QUEUE_NAME          "/img_queue"
#define SHM_NAME            "/img_stats"
#define SEM_IO_NAME         "/img_io_sem"

// Diretórios
#define INPUT_DIR           "images"
#define OUTPUT_DIR          "output"

// Tipos de filtro
#define FILTER_GRAYSCALE    0
#define FILTER_BLUR         1
#define FILTER_RESIZE       2

// Códigos de mensagem
#define MSG_TASK            1
#define MSG_TERMINATE       2

// Estrutura para estatísticas na memória compartilhada
typedef struct {
    pthread_mutex_t mutex;
    pthread_mutexattr_t mutex_attr;
    pthread_cond_t cond_finished;
    pthread_condattr_t cond_attr;
    int total_images;
    int processed_images;
    int failed_images;
    double total_processing_time;
    int workers_active;
    int workers_done;
    char current_files[NUM_WORKERS][MAX_FILENAME];
} shared_stats_t;

// Estrutura de mensagem para fila
typedef struct {
    long msg_type;
    char filename[MAX_FILENAME];
    int task_id;
} task_message_t;

// Argumentos para threads de filtro
typedef struct {
    unsigned char *image_data;
    int width;
    int height;
    int channels;
    char input_file[MAX_FILENAME];
    char output_file[MAX_PATH];
    int filter_type;
    int thread_id;
    int worker_id;
    int success;
} thread_args_t;

// Contexto do worker
typedef struct {
    int worker_id;
    mqd_t msg_queue;
    shared_stats_t *stats;
    sem_t *io_sem;
    int pipe_fd;
} worker_context_t;

// Macros de log
#define LOG_SETUP(fmt, ...)     printf("[SETUP] " fmt "\n", ##__VA_ARGS__)
#define LOG_COORD(fmt, ...)     printf("[COORDENADOR] " fmt "\n", ##__VA_ARGS__)
#define LOG_WORKER(id, fmt, ...)  printf("[WORKER %d] " fmt "\n", id, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)     fprintf(stderr, "[ERRO] " fmt "\n", ##__VA_ARGS__)

// Funções utilitárias
static inline double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

static inline void get_basename(const char *path, char *basename) {
    const char *last_slash = strrchr(path, '/');
    if (last_slash) {
        strcpy(basename, last_slash + 1);
    } else {
        strcpy(basename, path);
    }
}

static inline void remove_extension(char *filename) {
    char *dot = strrchr(filename, '.');
    if (dot) *dot = '\0';
}

#endif // COMMON_H
