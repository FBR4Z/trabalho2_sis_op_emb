#ifndef WORKER_H
#define WORKER_H

#include "common.h"

// Função principal do worker (chamada após fork)
void worker_main(int worker_id, int pipe_fd);

// Processa uma imagem (cria threads, aplica filtros)
int process_image(worker_context_t *ctx, const char *filename);

// Atualiza estatísticas na memória compartilhada
void update_stats(shared_stats_t *stats, int success, double elapsed_time);

// Envia log via pipe
void send_log(int pipe_fd, int worker_id, const char *message);

#endif // WORKER_H
