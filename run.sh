#!/bin/bash

# run.sh - Compila e executa o projeto

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo ""

# Verifica se setup foi executado
if [[ ! -f include/stb_image.h ]]; then
    echo -e "${YELLOW}[INFO] Bibliotecas não encontradas. Executando setup...${NC}"
    ./setup.sh
    if [[ $? -ne 0 ]]; then
        echo -e "${RED}[ERRO] Falha no setup${NC}"
        exit 1
    fi
fi

# Verifica se há imagens
IMG_COUNT=$(ls -1 images/*.jpg images/*.jpeg images/*.png images/*.bmp 2>/dev/null | wc -l)
if [[ $IMG_COUNT -eq 0 ]]; then
    echo -e "${RED}[ERRO] Nenhuma imagem encontrada em images/${NC}"
    echo -e "${YELLOW}[INFO] Coloque imagens JPG ou PNG na pasta images/${NC}"
    echo -e "${YELLOW}[INFO] Ou execute ./setup.sh para baixar imagens de exemplo${NC}"
    exit 1
fi

echo -e "${GREEN}[INFO] Encontradas ${IMG_COUNT} imagens${NC}"

# Limpa recursos IPC antigos (pode ter ficado de execução anterior)
rm -f /dev/mqueue/img_queue 2>/dev/null
rm -f /dev/shm/img_stats 2>/dev/null
rm -f /dev/shm/sem.img_io_sem 2>/dev/null

# Compila
echo -e "${YELLOW}[INFO] Compilando...${NC}"
make clean > /dev/null 2>&1
make
if [[ $? -ne 0 ]]; then
    echo -e "${RED}[ERRO] Falha na compilação${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}[INFO] Executando image_processor...${NC}"
echo ""

# Executa
./image_processor

# Mostra resultados
echo ""
echo -e "${GREEN}[INFO] Imagens processadas salvas em: output/${NC}"
echo ""

OUTPUT_COUNT=$(ls -1 output/* 2>/dev/null | wc -l)
if [[ $OUTPUT_COUNT -gt 0 ]]; then
    echo -e "Arquivos gerados:"
    ls -lh output/ | tail -n +2
fi

echo ""
