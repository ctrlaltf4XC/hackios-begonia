#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../bootshim/fdt.h"

static uint8_t *load_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc(n); fread(buf, 1, n, f); fclose(f);
    *out_len = (size_t)n; return buf;
}

static uint32_t be32(const unsigned char *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}

int main(int argc, char **argv){
    const char *dtb_path = (argc > 1) ? argv[1] : "begonia-test.dtb";
    size_t len; uint8_t *dtb = load_file(dtb_path, &len);
    printf("len=%zu\n", len);
    printf("magic=0x%08x total=%u off_struct=%u off_strings=%u off_rsvmap=%u\n",
        be32(dtb), be32(dtb+4), be32(dtb+8), be32(dtb+12), be32(dtb+16));
    printf("version=%u last_comp=%u size_strings=%u size_struct=%u\n",
        be32(dtb+20), be32(dtb+24), be32(dtb+32), be32(dtb+36));
    struct fdt_info fdt; memset(&fdt,0,sizeof(fdt));
    int rc = fdt_inspect(dtb, len, &fdt);
    printf("fdt_inspect rc=%d\n", rc);
    return 0;
}
