// IMPORTANTE: defines devem vir ANTES de qualquer include
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "filters.h"
#include "stb_image.h"
#include "stb_image_write.h"

// ============================================================
// CARREGAMENTO E SALVAMENTO DE IMAGENS
// ============================================================

unsigned char* load_image(const char *filename, int *width, int *height, int *channels) {
    unsigned char *data = stbi_load(filename, width, height, channels, 0);
    if (!data) {
        LOG_ERROR("Falha ao carregar: %s - %s", filename, stbi_failure_reason());
    }
    return data;
}

int save_image(const char *filename, unsigned char *data, int width, int height, int channels) {
    // Determina formato pelo nome do arquivo
    const char *ext = strrchr(filename, '.');
    int result = 0;
    
    if (ext && (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)) {
        result = stbi_write_jpg(filename, width, height, channels, data, 90);
    } else if (ext && strcmp(ext, ".png") == 0) {
        result = stbi_write_png(filename, width, height, channels, data, width * channels);
    } else {
        // Default: JPG
        result = stbi_write_jpg(filename, width, height, channels, data, 90);
    }
    
    if (!result) {
        LOG_ERROR("Falha ao salvar: %s", filename);
        return -1;
    }
    return 0;
}

void free_image(unsigned char *data) {
    if (data) {
        stbi_image_free(data);
    }
}

const char* get_filter_name(int filter_type) {
    switch (filter_type) {
        case FILTER_GRAYSCALE: return "grayscale";
        case FILTER_BLUR:      return "blur";
        case FILTER_RESIZE:    return "resize";
        default:               return "unknown";
    }
}

// ============================================================
// IMPLEMENTAÇÃO DOS FILTROS
// ============================================================

void apply_grayscale(unsigned char *image, int width, int height, int channels) {
    // Só faz sentido se tiver RGB ou RGBA
    if (channels < 3) return;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * channels;
            
            // Luminância: 0.299R + 0.587G + 0.114B
            unsigned char r = image[idx];
            unsigned char g = image[idx + 1];
            unsigned char b = image[idx + 2];
            unsigned char gray = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
            
            image[idx] = gray;
            image[idx + 1] = gray;
            image[idx + 2] = gray;
            // Alpha (se existir) permanece inalterado
        }
    }
}

void apply_blur(unsigned char *src, unsigned char *dst, int width, int height, int channels) {
    // Box blur 3x3
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < channels; c++) {
                int sum = 0;
                int count = 0;
                
                // Kernel 3x3
                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        int ny = y + ky;
                        int nx = x + kx;
                        
                        // Verifica limites
                        if (ny >= 0 && ny < height && nx >= 0 && nx < width) {
                            int idx = (ny * width + nx) * channels + c;
                            sum += src[idx];
                            count++;
                        }
                    }
                }
                
                int dst_idx = (y * width + x) * channels + c;
                dst[dst_idx] = (unsigned char)(sum / count);
            }
        }
    }
}

void apply_resize(unsigned char *src, int src_w, int src_h, int channels,
                  unsigned char **dst, int *dst_w, int *dst_h) {
    // Reduz para 50%
    *dst_w = src_w / 2;
    *dst_h = src_h / 2;
    
    if (*dst_w < 1) *dst_w = 1;
    if (*dst_h < 1) *dst_h = 1;
    
    *dst = (unsigned char*)malloc((*dst_w) * (*dst_h) * channels);
    if (!*dst) {
        LOG_ERROR("Falha ao alocar memória para resize");
        return;
    }
    
    // Interpolação simples (nearest neighbor)
    for (int y = 0; y < *dst_h; y++) {
        for (int x = 0; x < *dst_w; x++) {
            int src_x = x * 2;
            int src_y = y * 2;
            
            if (src_x >= src_w) src_x = src_w - 1;
            if (src_y >= src_h) src_y = src_h - 1;
            
            for (int c = 0; c < channels; c++) {
                int src_idx = (src_y * src_w + src_x) * channels + c;
                int dst_idx = (y * (*dst_w) + x) * channels + c;
                (*dst)[dst_idx] = src[src_idx];
            }
        }
    }
}

// ============================================================
// FUNÇÕES DE THREAD PARA FILTROS
// ============================================================

void* thread_grayscale(void *args) {
    thread_args_t *targs = (thread_args_t*)args;
    
    // Copia dados da imagem para não interferir com outras threads
    size_t size = targs->width * targs->height * targs->channels;
    unsigned char *img_copy = (unsigned char*)malloc(size);
    if (!img_copy) {
        LOG_ERROR("Worker %d: Falha ao alocar memória (grayscale)", targs->worker_id);
        targs->success = 0;
        return NULL;
    }
    
    memcpy(img_copy, targs->image_data, size);
    
    // Aplica filtro
    apply_grayscale(img_copy, targs->width, targs->height, targs->channels);
    
    // Salva resultado
    if (save_image(targs->output_file, img_copy, targs->width, targs->height, targs->channels) == 0) {
        targs->success = 1;
    } else {
        targs->success = 0;
    }
    
    free(img_copy);
    return NULL;
}

void* thread_blur(void *args) {
    thread_args_t *targs = (thread_args_t*)args;
    
    size_t size = targs->width * targs->height * targs->channels;
    unsigned char *img_blur = (unsigned char*)malloc(size);
    if (!img_blur) {
        LOG_ERROR("Worker %d: Falha ao alocar memória (blur)", targs->worker_id);
        targs->success = 0;
        return NULL;
    }
    
    // Aplica blur
    apply_blur(targs->image_data, img_blur, targs->width, targs->height, targs->channels);
    
    // Salva resultado
    if (save_image(targs->output_file, img_blur, targs->width, targs->height, targs->channels) == 0) {
        targs->success = 1;
    } else {
        targs->success = 0;
    }
    
    free(img_blur);
    return NULL;
}

void* thread_resize(void *args) {
    thread_args_t *targs = (thread_args_t*)args;
    
    unsigned char *resized = NULL;
    int new_w, new_h;
    
    // Aplica resize
    apply_resize(targs->image_data, targs->width, targs->height, targs->channels,
                 &resized, &new_w, &new_h);
    
    if (!resized) {
        targs->success = 0;
        return NULL;
    }
    
    // Salva resultado
    if (save_image(targs->output_file, resized, new_w, new_h, targs->channels) == 0) {
        targs->success = 1;
    } else {
        targs->success = 0;
    }
    
    free(resized);
    return NULL;
}
