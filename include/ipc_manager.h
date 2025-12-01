#ifndef IPC_MANAGER_H
#define IPC_MANAGER_H

#include "common.h"

// Fila de mensagens
mqd_t create_message_queue(const char *name);
mqd_t open_message_queue(const char *name);
int send_task(mqd_t mq, const char *filename, int task_id);
int send_terminate(mqd_t mq);
int receive_task(mqd_t mq, task_message_t *msg);
void close_message_queue(mqd_t mq);
void unlink_message_queue(const char *name);

// Memória compartilhada
shared_stats_t* create_shared_memory(const char *name, int *shm_fd);
shared_stats_t* open_shared_memory(const char *name, int *shm_fd);
void close_shared_memory(shared_stats_t *stats, int shm_fd);
void unlink_shared_memory(const char *name);

// Pipes
int create_pipe(int pipefd[2]);

// Limpeza geral
void cleanup_ipc_coordinator(mqd_t mq, shared_stats_t *stats, int shm_fd);
void cleanup_ipc_worker(mqd_t mq, shared_stats_t *stats, int shm_fd);

#endif // IPC_MANAGER_H
