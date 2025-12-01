#ifndef FILTERS_H
#define FILTERS_H

#include "common.h"

// Funções de thread para cada filtro
void* thread_grayscale(void *args);
void* thread_blur(void *args);
void* thread_resize(void *args);

// Funções auxiliares dos filtros
void apply_grayscale(unsigned char *image, int width, int height, int channels);
void apply_blur(unsigned char *src, unsigned char *dst, int width, int height, int channels);
void apply_resize(unsigned char *src, int src_w, int src_h, int channels,
                  unsigned char **dst, int *dst_w, int *dst_h);

// Carregamento e salvamento de imagens
unsigned char* load_image(const char *filename, int *width, int *height, int *channels);
int save_image(const char *filename, unsigned char *data, int width, int height, int channels);
void free_image(unsigned char *data);

// Nome do filtro
const char* get_filter_name(int filter_type);

#endif // FILTERS_H
