/* example.c */
#include "../kro.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    const uint32_t W = 256, H = 256;
    size_t pixel_count = W * H;
    uint16_t* data = malloc(pixel_count * 4 * sizeof(uint16_t));

    // RGBA 渐变：R=x, G=y, B=0, A=65535
    for (uint32_t y = 0; y < H; y++) {
        for (uint32_t x = 0; x < W; x++) {
            size_t i = (y * W + x) * 4;
            data[i + 0] = (uint16_t)(x * 257);      // R
            data[i + 1] = (uint16_t)(y * 257);      // G
            data[i + 2] = 0;                        // B
            data[i + 3] = 65535;                    // A
        }
    }

    FILE* fp = fopen("output.kro", "wb");
    if (!fp) { perror("fopen"); return 1; }

    kro_infop kro = kro_create_write_struct(fp);
    kro_set_IHDR(kro, W, H, 16, 4);  // 16-bit RGBA
    kro_write_info(kro);
    kro_write_image(kro, data);
    kro_destroy_write_struct(&kro);
    fclose(fp);

    free(data);
    printf("output.kro written (16-bit RGBA)\n");
    return 0;
}
