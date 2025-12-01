#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include "common.h"

// Semáforos
sem_t* create_semaphore(const char *name, int initial_value);
sem_t* open_semaphore(const char *name);
void close_semaphore(sem_t *sem);
void unlink_semaphore(const char *name);
void sem_acquire(sem_t *sem);
void sem_release(sem_t *sem);

// Mutex (inicialização para memória compartilhada)
int init_shared_mutex(pthread_mutex_t *mutex, pthread_mutexattr_t *attr);
void destroy_mutex(pthread_mutex_t *mutex, pthread_mutexattr_t *attr);
void mutex_lock(pthread_mutex_t *mutex);
void mutex_unlock(pthread_mutex_t *mutex);

// Variáveis de condição (para memória compartilhada)
int init_shared_cond(pthread_cond_t *cond, pthread_condattr_t *attr);
void destroy_cond(pthread_cond_t *cond, pthread_condattr_t *attr);
void cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
void cond_signal(pthread_cond_t *cond);
void cond_broadcast(pthread_cond_t *cond);

// Limpeza
void cleanup_sync(sem_t *sem);

#endif // SYNC_MANAGER_H
