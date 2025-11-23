/**
 * @file kro.c  This file is part of libkro project
 * @brief: Full implementation of libkro
 * @version: 1.0.0
 * @copyright: GPL v2
 * @author: Shilong YANG
 * @date: 2025-11-23 13:23:59
 * 
 **/

#include "kro.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(_WIN32)
# include <winsock2.h>
# if defined(_MSC_VER)
#   pragma comment(lib, "ws2_32.lib")
# endif
#else
# include <arpa/inet.h>
#endif

// Internal state
struct kro_struct {
	FILE* fp;
	int mode; // KRO_MODE_READ or KRO_MODE_WRITE
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t components;
	int header_valid;
	size_t bytes_per_pixel;
	size_t bytes_per_row;
	long data_offset; // offset to pixel data start
};

// Helper: host float <-> big-endian uint32
uint32_t kro_htonf(float f) {
	uint32_t u;
	memcpy(&u, &f, sizeof(u));
	return htonl(u);
}

float kro_ntohf(uint32_t u) {
	u = ntohl(u);
	float f;
	memcpy(&f, &u, sizeof(f));
	return f;
}

// Validate header values
static int validate_header(uint32_t depth, uint32_t components) {
	if (depth != 8 && depth != 16 && depth != 32) return 0;
	if (components != 3 && components != 4) return 0;
	return 1;
}

// Compute sizes
static void compute_sizes(kro_infop kro) {
	kro->bytes_per_pixel = (kro->depth / 8) * kro->components;
	kro->bytes_per_row = kro->bytes_per_pixel * kro->width;
}

// ------------------ WRITE ------------------

kro_infop kro_create_write_struct(FILE* fp) {
	if (!fp) return NULL;
	kro_infop kro = (kro_infop)calloc(1, sizeof(struct kro_struct));
	if (!kro) return NULL;
	kro->fp = fp;
	kro->mode = KRO_MODE_WRITE;
	kro->data_offset = 20; // fixed header size
	return kro;
}

void kro_destroy_write_struct(kro_infop* kro_ptr) {
	if (kro_ptr && *kro_ptr) {
		free(*kro_ptr);
		*kro_ptr = NULL;
	}
}

int kro_set_IHDR(kro_infop kro,
	uint32_t width,
	uint32_t height,
	uint32_t depth,
	uint32_t components) {
		if (!kro || kro->mode != KRO_MODE_WRITE) return KRO_INVALID_ARG;
		if (!validate_header(depth, components)) return KRO_INVALID_ARG;
		if (width == 0 || height == 0) return KRO_INVALID_ARG;
		
		kro->width = width;
		kro->height = height;
		kro->depth = depth;
		kro->components = components;
		compute_sizes(kro);
		return KRO_OK;
	}

int kro_write_info(kro_infop kro) {
	if (!kro || kro->mode != KRO_MODE_WRITE) return KRO_INVALID_ARG;
	if (!kro->header_valid && !kro->width) return KRO_INVALID_ARG;
	
	FILE* fp = kro->fp;
	if (fwrite("KRO", 1, 3, fp) != 3) return KRO_IO_ERROR;
	if (fputc(1, fp) == EOF) return KRO_IO_ERROR;
	
	uint32_t w = htonl(kro->width);
	uint32_t h = htonl(kro->height);
	uint32_t d = htonl(kro->depth);
	uint32_t c = htonl(kro->components);
	
	if (fwrite(&w, 4, 1, fp) != 1) return KRO_IO_ERROR;
	if (fwrite(&h, 4, 1, fp) != 1) return KRO_IO_ERROR;
	if (fwrite(&d, 4, 1, fp) != 1) return KRO_IO_ERROR;
	if (fwrite(&c, 4, 1, fp) != 1) return KRO_IO_ERROR;
	
	kro->header_valid = 1;
	return KRO_OK;
}

static int write_row_internal(kro_infop kro, const void* row_data) {
	if (kro->depth == 8) {
		if (fwrite(row_data, 1, kro->bytes_per_row, kro->fp) != kro->bytes_per_row)
			return KRO_IO_ERROR;
	}
	else if (kro->depth == 16) {
		const uint16_t* src = (const uint16_t*)row_data;
		size_t n = kro->bytes_per_row / 2;
		for (size_t i = 0; i < n; i++) {
			uint16_t val = htons(src[i]);
			if (fwrite(&val, 2, 1, kro->fp) != 1) return KRO_IO_ERROR;
		}
	}
	else if (kro->depth == 32) {
		const float* src = (const float*)row_data;
		size_t n = kro->bytes_per_row / 4;
		for (size_t i = 0; i < n; i++) {
			uint32_t val = kro_htonf(src[i]);
			if (fwrite(&val, 4, 1, kro->fp) != 1) return KRO_IO_ERROR;
		}
	}
	return KRO_OK;
}

int kro_write_row(kro_infop kro, const void* row_data, uint32_t row_index) {
	if (!kro || kro->mode != KRO_MODE_WRITE || !kro->header_valid)
		return KRO_INVALID_ARG;
	if (row_index >= kro->height) return KRO_INVALID_ARG;
	if (!row_data) return KRO_INVALID_ARG;
	
	// Seek to correct row position
	long pos = kro->data_offset + (long)row_index * (long)kro->bytes_per_row;
	if (fseek(kro->fp, pos, SEEK_SET) != 0) return KRO_IO_ERROR;
	
	return write_row_internal(kro, row_data);
}

int kro_write_image(kro_infop kro, const void* image_data) {
	if (!kro || kro->mode != KRO_MODE_WRITE || !kro->header_valid)
		return KRO_INVALID_ARG;
	if (!image_data) return KRO_INVALID_ARG;
	
	if (fseek(kro->fp, kro->data_offset, SEEK_SET) != 0)
		return KRO_IO_ERROR;
	
	for (uint32_t y = 0; y < kro->height; y++) {
		const char* row = (const char*)image_data + y * kro->bytes_per_row;
		int ret = write_row_internal(kro, row);
		if (ret != KRO_OK) return ret;
	}
	return KRO_OK;
}

// ------------------ READ ------------------

kro_infop kro_create_read_struct(FILE* fp) {
	if (!fp) return NULL;
	kro_infop kro = (kro_infop)calloc(1, sizeof(struct kro_struct));
	if (!kro) return NULL;
	kro->fp = fp;
	kro->mode = KRO_MODE_READ;
	kro->data_offset = 20;
	return kro;
}

void kro_destroy_read_struct(kro_infop* kro_ptr) {
	if (kro_ptr && *kro_ptr) {
		free(*kro_ptr);
		*kro_ptr = NULL;
	}
}

int kro_read_info(kro_infop kro) {
	if (!kro || kro->mode != KRO_MODE_READ) return KRO_INVALID_ARG;
	
	char magic[4];
	if (fread(magic, 1, 3, kro->fp) != 3) return KRO_IO_ERROR;
	if (memcmp(magic, "KRO", 3) != 0) return KRO_ERROR;
	
	uint8_t version = fgetc(kro->fp);
	if (version != 1) return KRO_NOT_SUPPORTED;
	
	uint32_t buf[4];
	if (fread(buf, 4, 4, kro->fp) != 4) return KRO_IO_ERROR;
	
	kro->width = ntohl(buf[0]);
	kro->height = ntohl(buf[1]);
	kro->depth = ntohl(buf[2]);
	kro->components = ntohl(buf[3]);
	
	if (!validate_header(kro->depth, kro->components))
		return KRO_ERROR;
	
	compute_sizes(kro);
	kro->header_valid = 1;
	return KRO_OK;
}

int kro_get_IHDR(const kro_infop kro,
	uint32_t* width,
	uint32_t* height,
	uint32_t* depth,
	uint32_t* components) {
		if (!kro || !kro->header_valid) return KRO_INVALID_ARG;
		if (width) *width = kro->width;
		if (height) *height = kro->height;
		if (depth) *depth = kro->depth;
		if (components) *components = kro->components;
		return KRO_OK;
	}

static int read_row_internal(kro_infop kro, void* row_buffer) {
	if (kro->depth == 8) {
		if (fread(row_buffer, 1, kro->bytes_per_row, kro->fp) != kro->bytes_per_row)
			return KRO_IO_ERROR;
	}
	else if (kro->depth == 16) {
		uint16_t* dst = (uint16_t*)row_buffer;
		size_t n = kro->bytes_per_row / 2;
		for (size_t i = 0; i < n; i++) {
			uint16_t val;
			if (fread(&val, 2, 1, kro->fp) != 1) return KRO_IO_ERROR;
			dst[i] = ntohs(val);
		}
	}
	else if (kro->depth == 32) {
		float* dst = (float*)row_buffer;
		size_t n = kro->bytes_per_row / 4;
		for (size_t i = 0; i < n; i++) {
			uint32_t val;
			if (fread(&val, 4, 1, kro->fp) != 1) return KRO_IO_ERROR;
			dst[i] = kro_ntohf(val);
		}
	}
	return KRO_OK;
}

int kro_read_row(kro_infop kro, void* row_buffer, uint32_t row_index) {
	if (!kro || kro->mode != KRO_MODE_READ || !kro->header_valid)
		return KRO_INVALID_ARG;
	if (row_index >= kro->height) return KRO_INVALID_ARG;
	if (!row_buffer) return KRO_INVALID_ARG;
	
	long pos = kro->data_offset + (long)row_index * (long)kro->bytes_per_row;
	if (fseek(kro->fp, pos, SEEK_SET) != 0) return KRO_IO_ERROR;
	
	return read_row_internal(kro, row_buffer);
}

int kro_read_image(kro_infop kro, void* image_buffer) {
	if (!kro || kro->mode != KRO_MODE_READ || !kro->header_valid)
		return KRO_INVALID_ARG;
	if (!image_buffer) return KRO_INVALID_ARG;
	
	if (fseek(kro->fp, kro->data_offset, SEEK_SET) != 0)
		return KRO_IO_ERROR;
	
	for (uint32_t y = 0; y < kro->height; y++) {
		char* row = (char*)image_buffer + y * kro->bytes_per_row;
		int ret = read_row_internal(kro, row);
		if (ret != KRO_OK) return ret;
	}
	return KRO_OK;
}
