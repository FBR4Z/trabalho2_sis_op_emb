# 🛠️ Guia de Instalação - Sistema de Processamento Paralelo de Imagens

Este guia foi feito para **Windows com WSL (Ubuntu)**. Siga cada passo na ordem.

---

## 📋 Índice

1. [Requisitos do Sistema](#1-requisitos-do-sistema)
2. [Instalação do WSL (se ainda não tiver)](#2-instalação-do-wsl)
3. [Instalação de Dependências](#3-instalação-de-dependências)
4. [Download do Projeto](#4-download-do-projeto)
5. [Configuração Automática](#5-configuração-automática)
6. [Compilação](#6-compilação)
7. [Preparação de Imagens de Teste](#7-preparação-de-imagens-de-teste)
8. [Execução](#8-execução)
9. [Verificação de Resultados](#9-verificação-de-resultados)
10. [Troubleshooting](#10-troubleshooting)
11. [Limpeza](#11-limpeza)

---

## 1. Requisitos do Sistema

### Mínimos:
- Windows 10 versão 2004+ ou Windows 11
- WSL 2 com Ubuntu 20.04 ou superior
- 4 GB de RAM livre
- 500 MB de espaço em disco

### Software necessário (será instalado):
- GCC 9.0+
- GNU Make
- wget/curl

---

## 2. Instalação do WSL

> ⚠️ **Pule esta seção se já tem WSL instalado**

### Passo 2.1: Abrir PowerShell como Administrador
- Pressione `Win + X`
- Clique em "Terminal (Admin)" ou "PowerShell (Admin)"

### Passo 2.2: Instalar WSL
```powershell
wsl --install
```

**Output esperado:**
```
Instalando: Plataforma de Máquina Virtual
Instalando: Subsistema Windows para Linux
Instalando: Ubuntu
```

### Passo 2.3: Reiniciar o computador
```powershell
shutdown /r /t 0
```

### Passo 2.4: Configurar Ubuntu
Após reiniciar, o Ubuntu abrirá automaticamente. Configure:
- **Username:** escolha um nome (ex: `aluno`)
- **Password:** escolha uma senha (vai precisar dela)

### Passo 2.5: Verificar instalação
Abra o Ubuntu (pesquise "Ubuntu" no menu Iniciar) e digite:
```bash
cat /etc/os-release
```

**Output esperado:**
```
NAME="Ubuntu"
VERSION="22.04.x LTS"
...
```

✅ **WSL instalado com sucesso!**

---

## 3. Instalação de Dependências

Todos os comandos abaixo devem ser executados **dentro do terminal Ubuntu/WSL**.

### Passo 3.1: Atualizar lista de pacotes
```bash
sudo apt update
```
Digite sua senha quando pedido (não aparece enquanto digita, é normal).

**Output esperado (final):**
```
Reading package lists... Done
```

### Passo 3.2: Atualizar pacotes existentes
```bash
sudo apt upgrade -y
```

Aguarde... pode demorar alguns minutos.

### Passo 3.3: Instalar ferramentas de compilação
```bash
sudo apt install -y build-essential gcc make
```

**Output esperado (final):**
```
build-essential is already the newest version...
```
ou
```
Setting up build-essential...
```

### Passo 3.4: Instalar bibliotecas necessárias
```bash
sudo apt install -y libpthread-stubs0-dev wget curl
```

### Passo 3.5: Verificar instalações

**Verificar GCC:**
```bash
gcc --version
```
**Output esperado:**
```
gcc (Ubuntu 11.x.x) 11.x.x
...
```

**Verificar Make:**
```bash
make --version
```
**Output esperado:**
```
GNU Make 4.x
...
```

**Verificar wget:**
```bash
wget --version | head -1
```
**Output esperado:**
```
GNU Wget 1.x.x ...
```

✅ **Dependências instaladas!**

---

## 4. Download do Projeto

### Opção A: Clonar do GitHub (recomendado)

```bash
cd ~
git clone https://github.com/FBR4Z/trabalho2_sis_op_emb.git
cd image_processor
```

### Opção B: Copiar arquivos manualmente

Se você tem os arquivos no Windows, eles ficam acessíveis em `/mnt/c/`:

```bash
# Exemplo: se o projeto está em C:\Users\SeuNome\Downloads\image_processor
cp -r /mnt/c/Users/SeuNome/Downloads/image_processor ~/
cd ~/image_processor
```

### Opção C: Criar do zero (se tiver só os fontes)

```bash
cd ~
mkdir -p image_processor/{src,include,images,output}
cd image_processor
# Depois copie os arquivos .c e .h para as pastas corretas
```

### Verificar estrutura
```bash
ls -la
```

**Output esperado:**
```
drwxr-xr-x src/
drwxr-xr-x include/
drwxr-xr-x images/
drwxr-xr-x output/
-rw-r--r-- Makefile
-rw-r--r-- README.md
-rw-r--r-- INSTALL.md
-rwxr-xr-x setup.sh
-rwxr-xr-x run.sh
```

✅ **Projeto baixado!**

---

## 5. Configuração Automática

### Passo 5.1: Dar permissão de execução aos scripts
```bash
chmod +x setup.sh run.sh
```

### Passo 5.2: Executar script de configuração
```bash
./setup.sh
```

**O que o setup.sh faz:**
1. ✅ Verifica se está no Linux/WSL
2. ✅ Verifica se GCC e Make estão instalados
3. ✅ Cria diretórios `images/` e `output/`
4. ✅ Baixa bibliotecas `stb_image.h` e `stb_image_write.h`
5. ✅ Baixa 5 imagens de exemplo para testar

**Output esperado:**
```
[SETUP] Verificando ambiente...
[SETUP] ✓ Sistema Linux/WSL detectado
[SETUP] ✓ GCC encontrado
[SETUP] ✓ Make encontrado
[SETUP] Criando diretórios...
[SETUP] Baixando bibliotecas stb_image...
[SETUP] Baixando imagens de exemplo...
[SETUP] ✓ Configuração concluída!
```

### Passo 5.3: Verificar se as bibliotecas foram baixadas
```bash
ls include/stb_image*.h
```

**Output esperado:**
```
include/stb_image.h  include/stb_image_write.h
```

### Passo 5.4: Verificar se há imagens de teste
```bash
ls images/
```

**Output esperado:**
```
sample_1.jpg  sample_2.jpg  sample_3.jpg  sample_4.jpg  sample_5.jpg
```

✅ **Configuração concluída!**

---

## 6. Compilação

### Passo 6.1: Compilar o projeto
```bash
make clean
make
```

**Output esperado:**
```
rm -f image_processor src/*.o
gcc -Wall -Wextra -pthread ... -c src/main.c -o src/main.o
gcc -Wall -Wextra -pthread ... -c src/worker.c -o src/worker.o
gcc -Wall -Wextra -pthread ... -c src/filters.c -o src/filters.o
gcc -Wall -Wextra -pthread ... -c src/ipc_manager.c -o src/ipc_manager.o
gcc -Wall -Wextra -pthread ... -c src/sync_manager.c -o src/sync_manager.o
gcc ... -o image_processor src/*.o -pthread -lrt -lm
```

### Passo 6.2: Verificar se compilou
```bash
ls -l image_processor
```

**Output esperado:**
```
-rwxr-xr-x 1 user user 45678 ... image_processor
```

### Erros comuns de compilação

**Erro: "stb_image.h: No such file or directory"**
```bash
# Solução: baixar manualmente
wget -O include/stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
wget -O include/stb_image_write.h https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
```

**Erro: "undefined reference to pthread_create"**
```bash
# Solução: verificar flags no Makefile
# Deve ter: LDFLAGS = -pthread -lrt -lm
```

**Erro: "undefined reference to shm_open"**
```bash
# Solução: garantir que -lrt está no LDFLAGS
```

✅ **Projeto compilado!**

---

## 7. Preparação de Imagens de Teste

O `setup.sh` já baixa imagens de exemplo. Mas você pode adicionar suas próprias:

### Formatos suportados
- ✅ JPG / JPEG
- ✅ PNG
- ✅ BMP
- ✅ TGA

### Adicionar imagens do Windows

```bash
# Copiar uma imagem específica
cp /mnt/c/Users/SeuNome/Pictures/foto.jpg ~/image_processor/images/

# Copiar várias imagens
cp /mnt/c/Users/SeuNome/Pictures/*.jpg ~/image_processor/images/
```

### Baixar imagens de teste da internet

```bash
cd ~/image_processor/images

# Baixar imagens do Picsum (serviço de imagens aleatórias)
wget -O test1.jpg "https://picsum.photos/800/600"
wget -O test2.jpg "https://picsum.photos/1024/768"
wget -O test3.jpg "https://picsum.photos/640/480"
```

### Verificar imagens
```bash
ls -lh images/
```

**Output esperado:**
```
-rw-r--r-- 1 user user 150K ... sample_1.jpg
-rw-r--r-- 1 user user 200K ... sample_2.jpg
...
```

✅ **Imagens prontas!**

---

## 8. Execução

### Opção A: Usar script run.sh (recomendado)
```bash
./run.sh
```

### Opção B: Executar diretamente
```bash
./image_processor
```

### Opção C: Executar com make
```bash
make run
```

### Output esperado durante execução

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
[PROGRESSO] [████████████████░░░░] 80% (4/5)
...
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

✅ **Execução concluída!**

---

## 9. Verificação de Resultados

### Passo 9.1: Listar imagens processadas
```bash
ls -lh output/
```

**Output esperado:**
```
-rw-r--r-- 1 user user  80K ... sample_1_grayscale.jpg
-rw-r--r-- 1 user user 145K ... sample_1_blur.jpg
-rw-r--r-- 1 user user  40K ... sample_1_resize.jpg
-rw-r--r-- 1 user user  90K ... sample_2_grayscale.jpg
...
```

### Passo 9.2: Ver imagens no Windows

As imagens processadas ficam acessíveis pelo Windows Explorer:

1. Abra o Explorador de Arquivos
2. Na barra de endereço, digite: `\\wsl$\Ubuntu\home\SEU_USUARIO\image_processor\output`
3. Ou navegue: `Linux > Ubuntu > home > seu_usuario > image_processor > output`

### Passo 9.3: Abrir imagem pelo terminal (abre no Windows)
```bash
# Abrir uma imagem específica
explorer.exe output/sample_1_grayscale.jpg
```

### Passo 9.4: Contar arquivos processados
```bash
echo "Grayscale: $(ls output/*_grayscale.* 2>/dev/null | wc -l)"
echo "Blur: $(ls output/*_blur.* 2>/dev/null | wc -l)"
echo "Resize: $(ls output/*_resize.* 2>/dev/null | wc -l)"
```

✅ **Resultados verificados!**

---

## 10. Troubleshooting

### ❌ Erro: "Permission denied"

**Problema:** Arquivos sem permissão de execução.

**Solução:**
```bash
chmod +x setup.sh run.sh
chmod +x image_processor
```

---

### ❌ Erro: "No such file or directory" ao executar

**Problema:** Executável não foi compilado.

**Solução:**
```bash
make clean
make
```

---

### ❌ Erro: "mq_open: Permission denied" ou "mq_open: Too many open files"

**Problema:** Recursos IPC de execuções anteriores não foram limpos.

**Solução:**
```bash
# Remover fila de mensagens
rm -f /dev/mqueue/img_queue

# Remover memória compartilhada  
rm -f /dev/shm/img_stats

# Remover semáforo
rm -f /dev/shm/sem.img_io_sem
```

---

### ❌ Erro: "Cannot allocate memory"

**Problema:** WSL com pouca memória.

**Solução:**
```bash
# Verificar memória disponível
free -h

# Fechar programas no Windows para liberar RAM
```

---

### ❌ Erro: "No images found in images/"

**Problema:** Pasta images/ está vazia.

**Solução:**
```bash
# Executar setup novamente para baixar imagens de exemplo
./setup.sh

# Ou baixar manualmente
wget -O images/test.jpg "https://picsum.photos/800/600"
```

---

### ❌ Erro: "Segmentation fault"

**Problema:** Geralmente imagem corrompida ou muito grande.

**Solução:**
```bash
# Testar com uma imagem menor
wget -O images/small.jpg "https://picsum.photos/400/300"
./image_processor
```

---

### ❌ Programa trava / não responde

**Problema:** Deadlock ou recurso IPC travado.

**Solução:**
```bash
# Matar processos órfãos
pkill -9 image_processor

# Limpar recursos IPC
rm -f /dev/mqueue/img_queue
rm -f /dev/shm/img_stats
rm -f /dev/shm/sem.img_io_sem

# Tentar novamente
./run.sh
```

---

### ❌ WSL muito lento

**Solução:** Criar arquivo `.wslconfig` no Windows:

1. Abra o Bloco de Notas
2. Cole:
```
[wsl2]
memory=4GB
processors=4
```
3. Salve como: `C:\Users\SeuNome\.wslconfig`
4. Reinicie o WSL:
```powershell
wsl --shutdown
```

---

## 11. Limpeza

### Limpar arquivos compilados
```bash
make clean
```

### Limpar imagens de saída
```bash
rm -f output/*
```

### Limpar recursos IPC manualmente
```bash
# Fila de mensagens
rm -f /dev/mqueue/img_queue

# Memória compartilhada
rm -f /dev/shm/img_stats

# Semáforo
rm -f /dev/shm/sem.img_io_sem
```

### Limpar tudo (reset completo)
```bash
make clean
rm -f output/*
rm -f /dev/mqueue/img_queue
rm -f /dev/shm/img_stats
rm -f /dev/shm/sem.img_io_sem
```

### Ver recursos IPC ativos no sistema
```bash
# Listar filas de mensagens
ls -la /dev/mqueue/

# Listar memória compartilhada
ls -la /dev/shm/
```

---

## 📞 Suporte

Se encontrar problemas não listados aqui:

1. Verifique se seguiu todos os passos na ordem
2. Tente `make clean && make` novamente
3. Limpe os recursos IPC e tente novamente
4. Verifique se há espaço em disco: `df -h`
5. Verifique memória disponível: `free -h`

---

**Guia criado para a disciplina de Sistemas Operacionais**
