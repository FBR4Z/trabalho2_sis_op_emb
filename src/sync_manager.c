#include "sync_manager.h"

// ============================================================
// SEMÁFOROS POSIX NOMEADOS
// ============================================================

sem_t* create_semaphore(const char *name, int initial_value) {
    // Remove semáforo antigo se existir
    sem_unlink(name);
    
    sem_t *sem = sem_open(name, O_CREAT | O_EXCL, 0644, initial_value);
    if (sem == SEM_FAILED) {
        perror("sem_open (create)");
        return NULL;
    }
    
    return sem;
}

sem_t* open_semaphore(const char *name) {
    sem_t *sem = sem_open(name, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return NULL;
    }
    return sem;
}

void close_semaphore(sem_t *sem) {
    if (sem && sem != SEM_FAILED) {
        sem_close(sem);
    }
}

void unlink_semaphore(const char *name) {
    sem_unlink(name);
}

void sem_acquire(sem_t *sem) {
    if (sem_wait(sem) == -1) {
        perror("sem_wait");
    }
}

void sem_release(sem_t *sem) {
    if (sem_post(sem) == -1) {
        perror("sem_post");
    }
}

// ============================================================
// MUTEX PARA MEMÓRIA COMPARTILHADA
// ============================================================

int init_shared_mutex(pthread_mutex_t *mutex, pthread_mutexattr_t *attr) {
    // Configura mutex para funcionar entre processos
    if (pthread_mutexattr_init(attr) != 0) {
        perror("pthread_mutexattr_init");
        return -1;
    }
    
    if (pthread_mutexattr_setpshared(attr, PTHREAD_PROCESS_SHARED) != 0) {
        perror("pthread_mutexattr_setpshared");
        pthread_mutexattr_destroy(attr);
        return -1;
    }
    
    if (pthread_mutex_init(mutex, attr) != 0) {
        perror("pthread_mutex_init");
        pthread_mutexattr_destroy(attr);
        return -1;
    }
    
    return 0;
}

void destroy_mutex(pthread_mutex_t *mutex, pthread_mutexattr_t *attr) {
    pthread_mutex_destroy(mutex);
    pthread_mutexattr_destroy(attr);
}

void mutex_lock(pthread_mutex_t *mutex) {
    pthread_mutex_lock(mutex);
}

void mutex_unlock(pthread_mutex_t *mutex) {
    pthread_mutex_unlock(mutex);
}

// ============================================================
// VARIÁVEIS DE CONDIÇÃO PARA MEMÓRIA COMPARTILHADA
// ============================================================

int init_shared_cond(pthread_cond_t *cond, pthread_condattr_t *attr) {
    // Configura cond para funcionar entre processos
    if (pthread_condattr_init(attr) != 0) {
        perror("pthread_condattr_init");
        return -1;
    }
    
    if (pthread_condattr_setpshared(attr, PTHREAD_PROCESS_SHARED) != 0) {
        perror("pthread_condattr_setpshared");
        pthread_condattr_destroy(attr);
        return -1;
    }
    
    if (pthread_cond_init(cond, attr) != 0) {
        perror("pthread_cond_init");
        pthread_condattr_destroy(attr);
        return -1;
    }
    
    return 0;
}

void destroy_cond(pthread_cond_t *cond, pthread_condattr_t *attr) {
    pthread_cond_destroy(cond);
    pthread_condattr_destroy(attr);
}

void cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    pthread_cond_wait(cond, mutex);
}

void cond_signal(pthread_cond_t *cond) {
    pthread_cond_signal(cond);
}

void cond_broadcast(pthread_cond_t *cond) {
    pthread_cond_broadcast(cond);
}

// ============================================================
// LIMPEZA
// ============================================================

void cleanup_sync(sem_t *sem) {
    close_semaphore(sem);
    unlink_semaphore(SEM_IO_NAME);
}
