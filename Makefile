# Makefile - Sistema de Processamento Paralelo de Imagens

CC = gcc
CFLAGS = -Wall -Wextra -pthread -D_GNU_SOURCE -I./include
LDFLAGS = -pthread -lrt -lm

TARGET = image_processor
SRC_DIR = src
INC_DIR = include
OBJ_DIR = src

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/worker.c \
       $(SRC_DIR)/filters.c \
       $(SRC_DIR)/ipc_manager.c \
       $(SRC_DIR)/sync_manager.c

OBJS = $(SRCS:.c=.o)

# Cores para output
GREEN = \033[0;32m
YELLOW = \033[0;33m
NC = \033[0m

.PHONY: all clean run setup download-libs help

all: $(TARGET)
	@echo "$(GREEN)✓ Compilação concluída!$(NC)"
	@echo "  Execute: ./$(TARGET)"

$(TARGET): $(OBJS)
	@echo "$(YELLOW)Linkando...$(NC)"
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "$(YELLOW)Compilando $<...$(NC)"
	$(CC) $(CFLAGS) -c $< -o $@

# Dependências de headers
$(SRC_DIR)/main.o: $(INC_DIR)/common.h $(INC_DIR)/ipc_manager.h $(INC_DIR)/sync_manager.h $(INC_DIR)/worker.h
$(SRC_DIR)/worker.o: $(INC_DIR)/common.h $(INC_DIR)/worker.h $(INC_DIR)/filters.h $(INC_DIR)/ipc_manager.h $(INC_DIR)/sync_manager.h
$(SRC_DIR)/filters.o: $(INC_DIR)/common.h $(INC_DIR)/filters.h $(INC_DIR)/stb_image.h $(INC_DIR)/stb_image_write.h
$(SRC_DIR)/ipc_manager.o: $(INC_DIR)/common.h $(INC_DIR)/ipc_manager.h
$(SRC_DIR)/sync_manager.o: $(INC_DIR)/common.h $(INC_DIR)/sync_manager.h

clean:
	@echo "$(YELLOW)Limpando arquivos compilados...$(NC)"
	rm -f $(TARGET) $(OBJS)
	@echo "$(GREEN)✓ Limpo!$(NC)"

run: all
	@echo ""
	@echo "$(GREEN)Executando $(TARGET)...$(NC)"
	@echo ""
	./$(TARGET)

setup: download-libs
	@mkdir -p images output
	@echo "$(GREEN)✓ Diretórios criados$(NC)"

download-libs:
	@echo "$(YELLOW)Baixando bibliotecas stb_image...$(NC)"
	@wget -q -O $(INC_DIR)/stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h 2>/dev/null || \
		curl -s -o $(INC_DIR)/stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
	@wget -q -O $(INC_DIR)/stb_image_write.h https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h 2>/dev/null || \
		curl -s -o $(INC_DIR)/stb_image_write.h https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
	@echo "$(GREEN)✓ Bibliotecas baixadas$(NC)"

clean-ipc:
	@echo "$(YELLOW)Limpando recursos IPC...$(NC)"
	@rm -f /dev/mqueue/img_queue 2>/dev/null || true
	@rm -f /dev/shm/img_stats 2>/dev/null || true
	@rm -f /dev/shm/sem.img_io_sem 2>/dev/null || true
	@echo "$(GREEN)✓ Recursos IPC limpos$(NC)"

clean-output:
	@echo "$(YELLOW)Limpando imagens de saída...$(NC)"
	@rm -f output/*
	@echo "$(GREEN)✓ Pasta output limpa$(NC)"

clean-all: clean clean-ipc clean-output
	@echo "$(GREEN)✓ Tudo limpo!$(NC)"

help:
	@echo ""
	@echo "$(GREEN)Comandos disponíveis:$(NC)"
	@echo "  make          - Compila o projeto"
	@echo "  make run      - Compila e executa"
	@echo "  make clean    - Remove arquivos compilados"
	@echo "  make setup    - Cria diretórios e baixa bibliotecas"
	@echo "  make clean-ipc    - Remove recursos IPC órfãos"
	@echo "  make clean-output - Limpa pasta de saída"
	@echo "  make clean-all    - Limpa tudo"
	@echo "  make help     - Mostra esta ajuda"
	@echo ""
