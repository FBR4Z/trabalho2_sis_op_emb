#!/bin/bash

# setup.sh - Configura o ambiente para o projeto

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo ""
echo -e "${YELLOW}[SETUP] Verificando ambiente...${NC}"

# Verifica se está no Linux/WSL
if [[ "$(uname)" != "Linux" ]]; then
    echo -e "${RED}[ERRO] Este projeto requer Linux ou WSL${NC}"
    exit 1
fi
echo -e "${GREEN}[SETUP] ✓ Sistema Linux/WSL detectado${NC}"

# Verifica GCC
if ! command -v gcc &> /dev/null; then
    echo -e "${RED}[ERRO] GCC não encontrado. Instale com: sudo apt install build-essential${NC}"
    exit 1
fi
echo -e "${GREEN}[SETUP] ✓ GCC encontrado$(NC)"

# Verifica Make
if ! command -v make &> /dev/null; then
    echo -e "${RED}[ERRO] Make não encontrado. Instale com: sudo apt install make${NC}"
    exit 1
fi
echo -e "${GREEN}[SETUP] ✓ Make encontrado${NC}"

# Cria diretórios
echo -e "${YELLOW}[SETUP] Criando diretórios...${NC}"
mkdir -p images output include

# Baixa bibliotecas stb_image
echo -e "${YELLOW}[SETUP] Baixando bibliotecas stb_image...${NC}"

STB_URL="https://raw.githubusercontent.com/nothings/stb/master"

if command -v wget &> /dev/null; then
    wget -q -O include/stb_image.h "${STB_URL}/stb_image.h"
    wget -q -O include/stb_image_write.h "${STB_URL}/stb_image_write.h"
elif command -v curl &> /dev/null; then
    curl -s -o include/stb_image.h "${STB_URL}/stb_image.h"
    curl -s -o include/stb_image_write.h "${STB_URL}/stb_image_write.h"
else
    echo -e "${RED}[ERRO] wget ou curl não encontrado${NC}"
    exit 1
fi

# Verifica se baixou
if [[ ! -f include/stb_image.h ]] || [[ ! -f include/stb_image_write.h ]]; then
    echo -e "${RED}[ERRO] Falha ao baixar bibliotecas stb_image${NC}"
    exit 1
fi
echo -e "${GREEN}[SETUP] ✓ Bibliotecas baixadas${NC}"

# Baixa imagens de exemplo
echo -e "${YELLOW}[SETUP] Baixando imagens de exemplo...${NC}"

PICSUM_URL="https://picsum.photos"

for i in {1..5}; do
    if command -v wget &> /dev/null; then
        wget -q -O "images/sample_${i}.jpg" "${PICSUM_URL}/800/600?random=${i}" 2>/dev/null
    else
        curl -s -L -o "images/sample_${i}.jpg" "${PICSUM_URL}/800/600?random=${i}" 2>/dev/null
    fi
done

# Conta imagens baixadas
IMG_COUNT=$(ls -1 images/*.jpg 2>/dev/null | wc -l)
echo -e "${GREEN}[SETUP] ✓ ${IMG_COUNT} imagens de exemplo baixadas${NC}"

# Dá permissão de execução aos scripts
chmod +x setup.sh run.sh 2>/dev/null

echo ""
echo -e "${GREEN}════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}                 CONFIGURAÇÃO CONCLUÍDA!${NC}"
echo -e "${GREEN}════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "Próximos passos:"
echo -e "  1. Compilar:  ${YELLOW}make${NC}"
echo -e "  2. Executar:  ${YELLOW}./image_processor${NC}"
echo -e ""
echo -e "Ou simplesmente: ${YELLOW}./run.sh${NC}"
echo ""
