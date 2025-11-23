/* example/read_example.c - Example: Read a KRO image using libkro */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../kro.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input.kro>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    kro_infop kro = kro_create_read_struct(fp);
    if (!kro) {
        fprintf(stderr, "Failed to create KRO read struct\n");
        fclose(fp);
        return EXIT_FAILURE;
    }

    int ret = kro_read_info(kro);
    if (ret != KRO_OK) {
        fprintf(stderr, "Error reading KRO header: %d\n", ret);
        kro_destroy_read_struct(&kro);
        fclose(fp);
        return EXIT_FAILURE;
    }

    uint32_t width, height, depth, components;
    kro_get_IHDR(kro, &width, &height, &depth, &components);

    printf("KRO Image Info:\n");
    printf("  File: %s\n", filename);
    printf("  Width: %u\n", width);
    printf("  Height: %u\n", height);
    printf("  Bit depth per channel: %u\n", depth);
    printf("  Color components: %u (%s)\n", components,
           components == 3 ? "RGB" : (components == 4 ? "RGBA" : "Unknown"));

    // Allocate buffer for full image
    size_t bytes_per_pixel = (depth / 8) * components;
    size_t image_size = (size_t)width * height * bytes_per_pixel;
    void* image_data = malloc(image_size);
    if (!image_data) {
        fprintf(stderr, "Out of memory\n");
        kro_destroy_read_struct(&kro);
        fclose(fp);
        return EXIT_FAILURE;
    }

    // Read entire image
    ret = kro_read_image(kro, image_data);
    if (ret != KRO_OK) {
        fprintf(stderr, "Failed to read image data: %d\n", ret);
        free(image_data);
        kro_destroy_read_struct(&kro);
        fclose(fp);
        return EXIT_FAILURE;
    }

    // Print top-left and bottom-right pixel (for verification)
    printf("\nSample pixels (row-major, top-left origin):\n");

    if (depth == 8) {
        uint8_t* data = (uint8_t*)image_data;
        printf("  Top-left (0,0): ");
        for (uint32_t c = 0; c < components; c++) {
            printf("%u ", data[c]);
        }
        printf("\n");

        size_t last_pixel = ((size_t)width * height - 1) * components;
        printf("  Bottom-right (%u,%u): ", width - 1, height - 1);
        for (uint32_t c = 0; c < components; c++) {
            printf("%u ", data[last_pixel + c]);
        }
        printf("\n");
    }
    else if (depth == 16) {
        uint16_t* data = (uint16_t*)image_data;
        printf("  Top-left (0,0): ");
        for (uint32_t c = 0; c < components; c++) {
            printf("%u ", data[c]);
        }
        printf("\n");

        size_t last_pixel = ((size_t)width * height - 1) * components;
        printf("  Bottom-right (%u,%u): ", width - 1, height - 1);
        for (uint32_t c = 0; c < components; c++) {
            printf("%u ", data[last_pixel + c]);
        }
        printf("\n");
    }
    else if (depth == 32) {
        float* data = (float*)image_data;
        printf("  Top-left (0,0): ");
        for (uint32_t c = 0; c < components; c++) {
            printf("%.3f ", data[c]);
        }
        printf("\n");

        size_t last_pixel = ((size_t)width * height - 1) * components;
        printf("  Bottom-right (%u,%u): ", width - 1, height - 1);
        for (uint32_t c = 0; c < components; c++) {
            printf("%.3f ", data[last_pixel + c]);
        }
        printf("\n");
    }

    // Optional: read a single row (e.g., middle row)
    void* row_buffer = malloc(width * bytes_per_pixel);
    uint32_t mid_row = height / 2;
    ret = kro_read_row(kro, row_buffer, mid_row);
    if (ret == KRO_OK) {
        printf("\nSuccessfully read middle row (%u)\n", mid_row);
    }
    free(row_buffer);

    // Cleanup
    free(image_data);
    kro_destroy_read_struct(&kro);
    fclose(fp);

    printf("\n Successfully read KRO file.\n");
    return EXIT_SUCCESS;
}
