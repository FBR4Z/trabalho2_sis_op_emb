#include "worker.h"
#include "filters.h"
#include "ipc_manager.h"
#include "sync_manager.h"

// Envia log para o coordenador via pipe
void send_log(int pipe_fd, int worker_id, const char *message) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[WORKER %d] %s\n", worker_id, message);
    write(pipe_fd, buffer, strlen(buffer));
}

// Atualiza estatísticas na memória compartilhada
void update_stats(shared_stats_t *stats, int success, double elapsed_time) {
    mutex_lock(&stats->mutex);
    
    if (success) {
        stats->processed_images++;
    } else {
        stats->failed_images++;
    }
    stats->total_processing_time += elapsed_time;
    
    // Sinaliza que uma imagem foi processada
    cond_signal(&stats->cond_finished);
    
    mutex_unlock(&stats->mutex);
}

// Processa uma imagem: carrega, cria threads para filtros, salva
int process_image(worker_context_t *ctx, const char *filename) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    char input_path[MAX_PATH];
    snprintf(input_path, sizeof(input_path), "%s/%s", INPUT_DIR, filename);
    
    // Adquire semáforo para I/O (leitura)
    sem_acquire(ctx->io_sem);
    
    // Carrega imagem
    int width, height, channels;
    unsigned char *image = load_image(input_path, &width, &height, &channels);
    
    sem_release(ctx->io_sem);
    
    if (!image) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Falha ao carregar: %s", filename);
        send_log(ctx->pipe_fd, ctx->worker_id, log_msg);
        update_stats(ctx->stats, 0, 0);
        return -1;
    }
    
    LOG_WORKER(ctx->worker_id, "Processando: %s (%dx%d)", filename, width, height);
    
    // Prepara nome base para saída
    char basename[MAX_FILENAME];
    get_basename(filename, basename);
    remove_extension(basename);
    
    // Configura argumentos para as 3 threads
    pthread_t threads[NUM_THREADS];
    thread_args_t args[NUM_THREADS];
    
    const char *filter_names[] = {"grayscale", "blur", "resize"};
    void* (*filter_funcs[])(void*) = {thread_grayscale, thread_blur, thread_resize};
    
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].image_data = image;
        args[i].width = width;
        args[i].height = height;
        args[i].channels = channels;
        args[i].filter_type = i;
        args[i].thread_id = i;
        args[i].worker_id = ctx->worker_id;
        args[i].success = 0;
        
        strncpy(args[i].input_file, filename, MAX_FILENAME - 1);
        snprintf(args[i].output_file, sizeof(args[i].output_file),
                 "%s/%s_%s.jpg", OUTPUT_DIR, basename, filter_names[i]);
    }
    
    // Cria as 3 threads de filtro
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, filter_funcs[i], &args[i]) != 0) {
            LOG_ERROR("Worker %d: Falha ao criar thread %d", ctx->worker_id, i);
        }
    }
    
    // Aguarda todas as threads terminarem
    int all_success = 1;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        
        if (args[i].success) {
            LOG_WORKER(ctx->worker_id, "  Thread %d: %s ✓", i, filter_names[i]);
        } else {
            LOG_WORKER(ctx->worker_id, "  Thread %d: %s ✗", i, filter_names[i]);
            all_success = 0;
        }
    }
    
    // Libera imagem original
    free_image(image);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = get_time_diff(start, end);
    
    LOG_WORKER(ctx->worker_id, "Concluído: %s (%.2fs)", filename, elapsed);
    
    // Envia log via pipe
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Processado: %s em %.2fs", filename, elapsed);
    send_log(ctx->pipe_fd, ctx->worker_id, log_msg);
    
    // Atualiza estatísticas
    update_stats(ctx->stats, all_success, elapsed);
    
    return all_success ? 0 : -1;
}

// Função principal do worker
void worker_main(int worker_id, int pipe_fd) {
    LOG_WORKER(worker_id, "PID %d iniciado", getpid());
    
    // Conecta aos recursos IPC
    mqd_t mq = open_message_queue(QUEUE_NAME);
    if (mq == (mqd_t)-1) {
        LOG_ERROR("Worker %d: Falha ao abrir fila de mensagens", worker_id);
        exit(1);
    }
    
    int shm_fd;
    shared_stats_t *stats = open_shared_memory(SHM_NAME, &shm_fd);
    if (!stats) {
        LOG_ERROR("Worker %d: Falha ao abrir memória compartilhada", worker_id);
        close_message_queue(mq);
        exit(1);
    }
    
    sem_t *io_sem = open_semaphore(SEM_IO_NAME);
    if (!io_sem) {
        LOG_ERROR("Worker %d: Falha ao abrir semáforo", worker_id);
        cleanup_ipc_worker(mq, stats, shm_fd);
        exit(1);
    }
    
    // Contexto do worker
    worker_context_t ctx = {
        .worker_id = worker_id,
        .msg_queue = mq,
        .stats = stats,
        .io_sem = io_sem,
        .pipe_fd = pipe_fd
    };
    
    // Marca como ativo
    mutex_lock(&stats->mutex);
    stats->workers_active++;
    strncpy(stats->current_files[worker_id], "idle", MAX_FILENAME);
    mutex_unlock(&stats->mutex);
    
    // Loop consumidor: recebe tarefas da fila
    task_message_t msg;
    while (1) {
        if (receive_task(mq, &msg) == -1) {
            continue;
        }
        
        // Mensagem de término
        if (msg.msg_type == MSG_TERMINATE) {
            LOG_WORKER(worker_id, "Recebido sinal de término");
            break;
        }
        
        // Atualiza arquivo atual
        mutex_lock(&stats->mutex);
        strncpy(stats->current_files[worker_id], msg.filename, MAX_FILENAME);
        mutex_unlock(&stats->mutex);
        
        // Processa a imagem
        process_image(&ctx, msg.filename);
        
        // Volta para idle
        mutex_lock(&stats->mutex);
        strncpy(stats->current_files[worker_id], "idle", MAX_FILENAME);
        mutex_unlock(&stats->mutex);
    }
    
    // Marca como inativo
    mutex_lock(&stats->mutex);
    stats->workers_active--;
    stats->workers_done++;
    cond_signal(&stats->cond_finished);
    mutex_unlock(&stats->mutex);
    
    // Limpeza
    close_semaphore(io_sem);
    cleanup_ipc_worker(mq, stats, shm_fd);
    close(pipe_fd);
    
    LOG_WORKER(worker_id, "Finalizado");
    exit(0);
}
