#include "common.h"
#include "ipc_manager.h"
#include "sync_manager.h"
#include "worker.h"

// Lista de imagens encontradas
static char image_files[MAX_IMAGES][MAX_FILENAME];
static int num_images = 0;

// PIDs dos workers
static pid_t worker_pids[NUM_WORKERS];

// Pipe para receber logs dos workers
static int log_pipe[2];

// Recursos IPC globais para cleanup
static mqd_t g_mq = (mqd_t)-1;
static shared_stats_t *g_stats = NULL;
static int g_shm_fd = -1;
static sem_t *g_io_sem = NULL;

// Handler para SIGINT
void signal_handler(int sig) {
    (void)sig;
    printf("\n[COORDENADOR] Interrompido. Limpando recursos...\n");
    
    // Envia sinal de término para workers
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (worker_pids[i] > 0) {
            kill(worker_pids[i], SIGTERM);
        }
    }
    
    // Limpeza
    if (g_io_sem) cleanup_sync(g_io_sem);
    if (g_mq != (mqd_t)-1) cleanup_ipc_coordinator(g_mq, g_stats, g_shm_fd);
    
    exit(1);
}

// Lê lista de imagens do diretório
int scan_images_directory(void) {
    DIR *dir = opendir(INPUT_DIR);
    if (!dir) {
        LOG_ERROR("Não foi possível abrir diretório: %s", INPUT_DIR);
        return -1;
    }
    
    struct dirent *entry;
    num_images = 0;
    
    while ((entry = readdir(dir)) != NULL && num_images < MAX_IMAGES) {
        // Ignora . e ..
        if (entry->d_name[0] == '.') continue;
        
        // Verifica extensão
        const char *ext = strrchr(entry->d_name, '.');
        if (!ext) continue;
        
        if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0 ||
            strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".bmp") == 0) {
            strncpy(image_files[num_images], entry->d_name, MAX_FILENAME - 1);
            image_files[num_images][MAX_FILENAME - 1] = '\0';
            num_images++;
        }
    }
    
    closedir(dir);
    return num_images;
}

// Imprime barra de progresso
void print_progress(int current, int total) {
    const int bar_width = 20;
    float progress = (float)current / total;
    int filled = (int)(progress * bar_width);
    
    printf("\r[PROGRESSO] [");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) printf("█");
        else printf("░");
    }
    printf("] %3d%% (%d/%d)", (int)(progress * 100), current, total);
    fflush(stdout);
}

// Imprime cabeçalho
void print_header(void) {
    printf("\n");
    printf("════════════════════════════════════════════════════════════\n");
    printf("   SISTEMA DE PROCESSAMENTO PARALELO DE IMAGENS\n");
    printf("════════════════════════════════════════════════════════════\n");
}

// Imprime estatísticas finais
void print_statistics(shared_stats_t *stats) {
    printf("\n\n");
    printf("════════════════════════════════════════════════════════════\n");
    printf("                    ESTATÍSTICAS FINAIS\n");
    printf("════════════════════════════════════════════════════════════\n");
    printf("  Total de imagens:      %d\n", stats->total_images);
    printf("  Processadas:           %d\n", stats->processed_images);
    printf("  Falhas:                %d\n", stats->failed_images);
    printf("  Tempo total:           %.2fs\n", stats->total_processing_time);
    if (stats->processed_images > 0) {
        printf("  Tempo médio/imagem:    %.2fs\n", 
               stats->total_processing_time / stats->processed_images);
    }
    printf("════════════════════════════════════════════════════════════\n");
    printf("  Resultados salvos em: %s/\n", OUTPUT_DIR);
    printf("════════════════════════════════════════════════════════════\n\n");
}

// Thread para ler logs do pipe
void* log_reader_thread(void *arg) {
    (void)arg;
    char buffer[512];
    ssize_t n;
    
    while ((n = read(log_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        // Logs já vêm formatados dos workers
        // (não imprimimos aqui para não poluir a saída)
    }
    
    return NULL;
}

int main(void) {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    print_header();
    
    // Configura handler de sinais
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Cria diretório de saída se não existir
    mkdir(OUTPUT_DIR, 0755);
    
    // ============================================================
    // INICIALIZAÇÃO DOS RECURSOS IPC
    // ============================================================
    
    // Cria fila de mensagens
    LOG_SETUP("Criando fila de mensagens: %s", QUEUE_NAME);
    g_mq = create_message_queue(QUEUE_NAME);
    if (g_mq == (mqd_t)-1) {
        LOG_ERROR("Falha ao criar fila de mensagens");
        return 1;
    }
    
    // Cria memória compartilhada
    LOG_SETUP("Criando memória compartilhada: %s", SHM_NAME);
    g_stats = create_shared_memory(SHM_NAME, &g_shm_fd);
    if (!g_stats) {
        LOG_ERROR("Falha ao criar memória compartilhada");
        cleanup_ipc_coordinator(g_mq, NULL, -1);
        return 1;
    }
    
    // Inicializa mutex e cond na memória compartilhada
    if (init_shared_mutex(&g_stats->mutex, &g_stats->mutex_attr) != 0) {
        LOG_ERROR("Falha ao inicializar mutex");
        cleanup_ipc_coordinator(g_mq, g_stats, g_shm_fd);
        return 1;
    }
    
    if (init_shared_cond(&g_stats->cond_finished, &g_stats->cond_attr) != 0) {
        LOG_ERROR("Falha ao inicializar variável de condição");
        cleanup_ipc_coordinator(g_mq, g_stats, g_shm_fd);
        return 1;
    }
    
    // Cria semáforo para controle de I/O
    LOG_SETUP("Criando semáforo de I/O (limite: %d)", NUM_WORKERS);
    g_io_sem = create_semaphore(SEM_IO_NAME, NUM_WORKERS);
    if (!g_io_sem) {
        LOG_ERROR("Falha ao criar semáforo");
        cleanup_ipc_coordinator(g_mq, g_stats, g_shm_fd);
        return 1;
    }
    
    // Cria pipe para logs
    if (create_pipe(log_pipe) != 0) {
        LOG_ERROR("Falha ao criar pipe");
        cleanup_sync(g_io_sem);
        cleanup_ipc_coordinator(g_mq, g_stats, g_shm_fd);
        return 1;
    }
    
    // ============================================================
    // BUSCA DE IMAGENS
    // ============================================================
    
    int found = scan_images_directory();
    if (found <= 0) {
        LOG_ERROR("Nenhuma imagem encontrada em %s/", INPUT_DIR);
        LOG_ERROR("Coloque imagens JPG ou PNG na pasta %s/", INPUT_DIR);
        cleanup_sync(g_io_sem);
        cleanup_ipc_coordinator(g_mq, g_stats, g_shm_fd);
        return 1;
    }
    
    LOG_SETUP("Encontradas %d imagens em %s/", num_images, INPUT_DIR);
    
    // Inicializa estatísticas
    g_stats->total_images = num_images;
    g_stats->processed_images = 0;
    g_stats->failed_images = 0;
    g_stats->total_processing_time = 0;
    g_stats->workers_active = 0;
    g_stats->workers_done = 0;
    
    // ============================================================
    // CRIAÇÃO DOS WORKERS (FORK)
    // ============================================================
    
    LOG_COORD("Iniciando %d workers...", NUM_WORKERS);
    
    for (int i = 0; i < NUM_WORKERS; i++) {
        pid_t pid = fork();
        
        if (pid == -1) {
            LOG_ERROR("Falha no fork do worker %d", i);
            // Mata workers já criados
            for (int j = 0; j < i; j++) {
                kill(worker_pids[j], SIGTERM);
                waitpid(worker_pids[j], NULL, 0);
            }
            cleanup_sync(g_io_sem);
            cleanup_ipc_coordinator(g_mq, g_stats, g_shm_fd);
            return 1;
        }
        
        if (pid == 0) {
            // Processo filho (worker)
            close(log_pipe[0]);  // Fecha leitura
            worker_main(i, log_pipe[1]);
            // worker_main chama exit()
        }
        
        // Processo pai
        worker_pids[i] = pid;
    }
    
    // Fecha escrita do pipe no pai
    close(log_pipe[1]);
    
    // Cria thread para ler logs (não bloqueia)
    pthread_t log_thread;
    pthread_create(&log_thread, NULL, log_reader_thread, NULL);
    
    // ============================================================
    // PRODUTOR: ENVIA TAREFAS PARA FILA
    // ============================================================
    
    // Pequena pausa para workers iniciarem
    usleep(100000);
    
    for (int i = 0; i < num_images; i++) {
        if (send_task(g_mq, image_files[i], i) != 0) {
            LOG_ERROR("Falha ao enviar tarefa: %s", image_files[i]);
        }
    }
    
    // Envia sinais de término para cada worker
    for (int i = 0; i < NUM_WORKERS; i++) {
        send_terminate(g_mq);
    }
    
    // ============================================================
    // MONITORAMENTO DE PROGRESSO
    // ============================================================
    
    int last_processed = 0;
    while (1) {
        mutex_lock(&g_stats->mutex);
        
        int processed = g_stats->processed_images + g_stats->failed_images;
        int done = g_stats->workers_done;
        
        mutex_unlock(&g_stats->mutex);
        
        // Atualiza barra de progresso
        if (processed != last_processed) {
            print_progress(processed, num_images);
            last_processed = processed;
        }
        
        // Todos workers terminaram
        if (done >= NUM_WORKERS) {
            break;
        }
        
        usleep(100000);  // 100ms
    }
    
    // ============================================================
    // AGUARDA WORKERS TERMINAREM
    // ============================================================
    
    LOG_COORD("Aguardando workers finalizarem...");
    
    for (int i = 0; i < NUM_WORKERS; i++) {
        int status;
        waitpid(worker_pids[i], &status, 0);
        
        if (WIFEXITED(status)) {
            // Worker terminou normalmente
        } else if (WIFSIGNALED(status)) {
            LOG_ERROR("Worker %d terminado por sinal %d", i, WTERMSIG(status));
        }
    }
    
    LOG_COORD("Todos os workers finalizaram");
    
    // Fecha pipe de leitura
    close(log_pipe[0]);
    pthread_cancel(log_thread);
    pthread_join(log_thread, NULL);
    
    // ============================================================
    // ESTATÍSTICAS FINAIS
    // ============================================================
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double total_time = get_time_diff(start_time, end_time);
    g_stats->total_processing_time = total_time;
    
    print_statistics(g_stats);
    
    // ============================================================
    // LIMPEZA
    // ============================================================
    
    destroy_mutex(&g_stats->mutex, &g_stats->mutex_attr);
    destroy_cond(&g_stats->cond_finished, &g_stats->cond_attr);
    cleanup_sync(g_io_sem);
    cleanup_ipc_coordinator(g_mq, g_stats, g_shm_fd);
    
    return 0;
}
