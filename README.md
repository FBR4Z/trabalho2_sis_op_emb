# 🖼️ Sistema de Processamento Paralelo de Imagens

Sistema multiprocessado e multithreaded para aplicar filtros em imagens utilizando conceitos avançados de Sistemas Operacionais.

---

## 📌 Descrição

Este projeto demonstra na prática os principais conceitos de **programação concorrente** e **comunicação entre processos (IPC)** estudados na disciplina de Sistemas Operacionais. O sistema processa múltiplas imagens em paralelo, aplicando três filtros: grayscale, blur e resize.

---

## 🎯 Objetivos Acadêmicos

Demonstrar domínio de:
- Criação e gerenciamento de **processos** (fork, wait, exit)
- Programação com **threads POSIX** (pthread)
- **Sincronização** com semáforos, mutexes e variáveis de condição
- **Comunicação entre processos** via filas de mensagens, memória compartilhada e pipes
- Padrão **Produtor-Consumidor**

---

## 🏗️ Arquitetura do Sistema

```
┌─────────────────────────────────────────────────────────────────┐
│                      PROCESSO COORDENADOR (PAI)                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │  Lê imagens │─►│ Envia tasks │─►│ Monitora progresso via  │  │
│  │  do disco   │  │ (mq_send)   │  │ memória compartilhada   │  │
│  └─────────────┘  └──────┬──────┘  └─────────────────────────┘  │
│                          │                                      │
│         FILA DE MENSAGENS POSIX (/img_queue)                    │
│                          │                                      │
│          ┌───────────────┴───────────────┐                      │
│          ▼                               ▼                      │
│  ┌───────────────┐               ┌───────────────┐              │
│  │   WORKER 0    │               │   WORKER 1    │              │
│  │   (fork)      │               │   (fork)      │              │
│  │  ┌─────────┐  │               │  ┌─────────┐  │              │
│  │  │Thread 0 │  │   SEMÁFORO    │  │Thread 0 │  │              │
│  │  │grayscale│  │◄─────────────►│  │grayscale│  │              │
│  │  ├─────────┤  │  (max 2 I/O)  │  ├─────────┤  │              │
│  │  │Thread 1 │  │               │  │Thread 1 │  │              │
│  │  │  blur   │  │    MUTEX      │  │  blur   │  │              │
│  │  ├─────────┤  │◄─────────────►│  ├─────────┤  │              │
│  │  │Thread 2 │  │ (stats lock)  │  │Thread 2 │  │              │
│  │  │ resize  │  │               │  │ resize  │  │              │
│  │  └─────────┘  │               │  └─────────┘  │              │
│  └───────┬───────┘               └───────┬───────┘              │
│          │                               │                      │
│          │      PIPE (logs/status)       │                      │
│          └───────────────┬───────────────┘                      │
│                          ▼                                      │
│              ┌─────────────────────┐                            │
│              │ MEMÓRIA COMPARTILHADA│                            │
│              │   (estatísticas)     │                            │
│              │  - total_images      │                            │
│              │  - processed_images  │                            │
│              │  - failed_images     │                            │
│              │  - processing_time   │                            │
│              └─────────────────────┘                            │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🔧 Mecanismos Utilizados

### Processos
| Função | Uso no Projeto |
|--------|----------------|
| `fork()` | Cria 2 processos workers |
| `wait()` / `waitpid()` | Coordenador aguarda término dos workers |
| `exit()` | Workers finalizam após processar todas as tarefas |

### Threads POSIX
| Função | Uso no Projeto |
|--------|----------------|
| `pthread_create()` | Cria 3 threads por worker (uma para cada filtro) |
| `pthread_join()` | Aguarda conclusão das threads de filtro |
| `pthread_mutex_*` | Protege atualização de estatísticas |
| `pthread_cond_*` | Notifica coordenador sobre conclusão |

### Sincronização
| Mecanismo | Uso no Projeto |
|-----------|----------------|
| **Semáforo POSIX** | Limita acesso ao disco (máx. 2 workers lendo/escrevendo) |
| **Mutex** | Exclusão mútua ao atualizar estatísticas na memória compartilhada |
| **Variável de Condição** | Workers sinalizam quando terminam uma tarefa |

### Comunicação Entre Processos (IPC)
| Mecanismo | Uso no Projeto |
|-----------|----------------|
| **Fila de Mensagens** | Coordenador envia tarefas, workers consomem (produtor-consumidor) |
| **Memória Compartilhada** | Estatísticas globais acessíveis por todos os processos |
| **Pipe** | Workers enviam logs de status para o coordenador |

---

## 📁 Estrutura de Arquivos

```
image_processor/
├── src/
│   ├── main.c              # Processo coordenador
│   ├── worker.c            # Lógica dos workers
│   ├── filters.c           # Implementação dos filtros
│   ├── ipc_manager.c       # Gerenciamento de IPC
│   └── sync_manager.c      # Gerenciamento de sincronização
├── include/
│   ├── common.h            # Definições compartilhadas
│   ├── worker.h            # Header do worker
│   ├── filters.h           # Header dos filtros
│   ├── ipc_manager.h       # Header do IPC
│   ├── sync_manager.h      # Header de sincronização
│   ├── stb_image.h         # Biblioteca de leitura de imagens
│   └── stb_image_write.h   # Biblioteca de escrita de imagens
├── images/                 # Imagens de entrada
├── output/                 # Imagens processadas
├── Makefile
├── README.md
├── INSTALL.md              # Guia detalhado de instalação
├── setup.sh                # Script de configuração
└── run.sh                  # Script de execução
```

---

## 🚀 Como Usar

### Instalação Rápida

```bash
# 1. Dar permissão aos scripts
chmod +x setup.sh run.sh

# 2. Configurar (baixa bibliotecas e imagens de teste)
./setup.sh

# 3. Compilar e executar
./run.sh
```

### Instalação Detalhada

Consulte o arquivo **[INSTALL.md](INSTALL.md)** para um guia completo passo a passo.

---

## 💻 Exemplo de Saída

```
════════════════════════════════════════════════════════════
   SISTEMA DE PROCESSAMENTO PARALELO DE IMAGENS
════════════════════════════════════════════════════════════
[SETUP] Criando fila de mensagens: /img_queue
[SETUP] Criando memória compartilhada: /img_stats
[SETUP] Criando semáforo de I/O (limite: 2)
[SETUP] Encontradas 5 imagens em images/
[COORDENADOR] Iniciando 2 workers...
[WORKER 0] PID 12345 iniciado
[WORKER 1] PID 12346 iniciado
[WORKER 0] Processando: sample_1.jpg
[WORKER 0]   Thread 0: grayscale ✓
[WORKER 0]   Thread 1: blur ✓
[WORKER 0]   Thread 2: resize ✓
[WORKER 0] Concluído: sample_1.jpg (1.2s)
...
[PROGRESSO] [████████████████████] 100% (5/5)
[COORDENADOR] Todos os workers finalizaram

════════════════════════════════════════════════════════════
                    ESTATÍSTICAS FINAIS
════════════════════════════════════════════════════════════
  Total de imagens:      5
  Processadas:           5
  Falhas:                0
  Tempo total:           6.5s
  Tempo médio/imagem:    1.3s
════════════════════════════════════════════════════════════
  Resultados salvos em: output/
════════════════════════════════════════════════════════════
```

---

## 🖼️ Filtros Implementados

| Filtro | Descrição | Saída |
|--------|-----------|-------|
| **Grayscale** | Converte para tons de cinza usando luminância | `*_grayscale.jpg` |
| **Blur** | Aplica blur de caixa 3x3 | `*_blur.jpg` |
| **Resize** | Reduz para 50% do tamanho original | `*_resize.jpg` |

---

## ⚙️ Compilação Manual

```bash
# Limpar arquivos anteriores
make clean

# Compilar
make

# Executar
./image_processor

# Ou compilar e executar
make run
```

---

## 📚 Disciplina

**Sistemas Operacionais**

Projeto desenvolvido para demonstrar conceitos de:
- Programação concorrente
- Sincronização de processos e threads
- Comunicação entre processos (IPC)
- Padrão produtor-consumidor

---

## 👥 Autores

| Nome | RA |
|------|-----|
|  |  |
|  |  |
|  |  |

---

## 📄 Licença

Projeto acadêmico - Uso educacional
