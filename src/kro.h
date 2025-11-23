/**
 * @file kro.h
 * @brief: KRO image I/O library
 * @version: 1.0.0
 * @copyright: GPL v2
 * @author: Shilong YANG
 * @date: 2025-11-23 13:23:59
 *
 **/

#ifndef KRO_H
#define KRO_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes */
#define KRO_OK          0
#define KRO_ERROR       (-1)
#define KRO_INVALID_ARG (-2)
#define KRO_IO_ERROR    (-3)
#define KRO_NOT_SUPPORTED (-4)

/* Opaque structures */
typedef struct kro_struct kro_struct;
typedef kro_struct* kro_infop;

/* I/O modes */
#define KRO_MODE_READ   0
#define KRO_MODE_WRITE  1

/* Create/Destroy */
kro_infop kro_create_read_struct(FILE* fp);
kro_infop kro_create_write_struct(FILE* fp);
void kro_destroy_read_struct(kro_infop* kro_ptr);
void kro_destroy_write_struct(kro_infop* kro_ptr);

/* Set/Get image header info */
int kro_set_IHDR(kro_infop kro,
                 uint32_t width,
                 uint32_t height,
                 uint32_t depth,        // 8, 16, 32
                 uint32_t components);  // 3=RGB, 4=RGBA

int kro_get_IHDR(const kro_infop kro,
                 uint32_t* width,
                 uint32_t* height,
                 uint32_t* depth,
                 uint32_t* components);

/* Write operations */
int kro_write_info(kro_infop kro);
int kro_write_row(kro_infop kro, const void* row_data, uint32_t row_index);
int kro_write_image(kro_infop kro, const void* image_data);

/* Read operations */
int kro_read_info(kro_infop kro);
int kro_read_row(kro_infop kro, void* row_buffer, uint32_t row_index);
int kro_read_image(kro_infop kro, void* image_buffer);

/* Utility */
uint32_t kro_htonf(float f);
float kro_ntohf(uint32_t u);

#ifdef __cplusplus
}
#endif

#endif /* KRO_H */
