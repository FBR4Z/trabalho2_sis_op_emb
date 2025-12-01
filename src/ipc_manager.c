#include "ipc_manager.h"

// ============================================================
// FILA DE MENSAGENS POSIX
// ============================================================

mqd_t create_message_queue(const char *name) {
    // Remove fila antiga se existir
    mq_unlink(name);
    
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = MAX_QUEUE_MSGS,
        .mq_msgsize = sizeof(task_message_t),
        .mq_curmsgs = 0
    };
    
    mqd_t mq = mq_open(name, O_CREAT | O_RDWR, 0644, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open (create)");
        return (mqd_t)-1;
    }
    
    return mq;
}

mqd_t open_message_queue(const char *name) {
    mqd_t mq = mq_open(name, O_RDWR);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return (mqd_t)-1;
    }
    return mq;
}

int send_task(mqd_t mq, const char *filename, int task_id) {
    task_message_t msg = {
        .msg_type = MSG_TASK,
        .task_id = task_id
    };
    strncpy(msg.filename, filename, MAX_FILENAME - 1);
    msg.filename[MAX_FILENAME - 1] = '\0';
    
    if (mq_send(mq, (char*)&msg, sizeof(msg), 0) == -1) {
        perror("mq_send");
        return -1;
    }
    return 0;
}

int send_terminate(mqd_t mq) {
    task_message_t msg = {
        .msg_type = MSG_TERMINATE,
        .task_id = -1
    };
    msg.filename[0] = '\0';
    
    if (mq_send(mq, (char*)&msg, sizeof(msg), 0) == -1) {
        perror("mq_send (terminate)");
        return -1;
    }
    return 0;
}

int receive_task(mqd_t mq, task_message_t *msg) {
    ssize_t bytes = mq_receive(mq, (char*)msg, sizeof(task_message_t), NULL);
    if (bytes == -1) {
        if (errno != EAGAIN) {
            perror("mq_receive");
        }
        return -1;
    }
    return 0;
}

void close_message_queue(mqd_t mq) {
    if (mq != (mqd_t)-1) {
        mq_close(mq);
    }
}

void unlink_message_queue(const char *name) {
    mq_unlink(name);
}

// ============================================================
// MEMÓRIA COMPARTILHADA POSIX
// ============================================================

shared_stats_t* create_shared_memory(const char *name, int *shm_fd) {
    // Remove shm antigo se existir
    shm_unlink(name);
    
    *shm_fd = shm_open(name, O_CREAT | O_RDWR, 0644);
    if (*shm_fd == -1) {
        perror("shm_open (create)");
        return NULL;
    }
    
    if (ftruncate(*shm_fd, sizeof(shared_stats_t)) == -1) {
        perror("ftruncate");
        close(*shm_fd);
        shm_unlink(name);
        return NULL;
    }
    
    shared_stats_t *stats = mmap(NULL, sizeof(shared_stats_t),
                                  PROT_READ | PROT_WRITE, MAP_SHARED,
                                  *shm_fd, 0);
    if (stats == MAP_FAILED) {
        perror("mmap");
        close(*shm_fd);
        shm_unlink(name);
        return NULL;
    }
    
    // Inicializa valores
    memset(stats, 0, sizeof(shared_stats_t));
    
    return stats;
}

shared_stats_t* open_shared_memory(const char *name, int *shm_fd) {
    *shm_fd = shm_open(name, O_RDWR, 0644);
    if (*shm_fd == -1) {
        perror("shm_open");
        return NULL;
    }
    
    shared_stats_t *stats = mmap(NULL, sizeof(shared_stats_t),
                                  PROT_READ | PROT_WRITE, MAP_SHARED,
                                  *shm_fd, 0);
    if (stats == MAP_FAILED) {
        perror("mmap");
        close(*shm_fd);
        return NULL;
    }
    
    return stats;
}

void close_shared_memory(shared_stats_t *stats, int shm_fd) {
    if (stats && stats != MAP_FAILED) {
        munmap(stats, sizeof(shared_stats_t));
    }
    if (shm_fd != -1) {
        close(shm_fd);
    }
}

void unlink_shared_memory(const char *name) {
    shm_unlink(name);
}

// ============================================================
// PIPES
// ============================================================

int create_pipe(int pipefd[2]) {
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }
    return 0;
}

// ============================================================
// LIMPEZA GERAL
// ============================================================

void cleanup_ipc_coordinator(mqd_t mq, shared_stats_t *stats, int shm_fd) {
    close_message_queue(mq);
    unlink_message_queue(QUEUE_NAME);
    close_shared_memory(stats, shm_fd);
    unlink_shared_memory(SHM_NAME);
}

void cleanup_ipc_worker(mqd_t mq, shared_stats_t *stats, int shm_fd) {
    close_message_queue(mq);
    close_shared_memory(stats, shm_fd);
}
