/* SPDX-License-Identifier: MIT
 * Host validation test for the Begonia boot shim FDT parser and ADT builder.
 * Builds with the host C compiler, links against bootshim/fdt.c and adt.c,
 * then parses a dtc-compiled begonia-like DTB.
 *
 * Build:
 *   gcc -I../bootshim -I. test_fdt.c ../bootshim/fdt.c ../bootshim/adt.c \
 *       ../bootshim/handoff.c -o test_fdt
 *
 * Requires a dtc-compiled DTB at tests/begonia-test.dtb.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "fdt.h"
#include "handoff.h"

static uint8_t *load_file(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc(n);
    if (!buf) { perror("malloc"); exit(1); }
    if (fread(buf, 1, n, f) != (size_t)n) { perror("fread"); exit(1); }
    fclose(f);
    *out_len = (size_t)n;
    return buf;
}

int main(int argc, char **argv)
{
    const char *dtb_path = (argc > 1) ? argv[1] : "begonia-test.dtb";
    size_t len;
    uint8_t *dtb = load_file(dtb_path, &len);

    struct fdt_info fdt;
    memset(&fdt, 0, sizeof(fdt));
    int rc = fdt_inspect(dtb, len, &fdt);
    if (rc != 0) { fprintf(stderr, "FAIL: fdt_inspect returned %d\n", rc); return 1; }

    int failures = 0;
    #define CHECK(cond, msg) do { \
        if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } \
        else { printf("PASS: %s\n", msg); } \
    } while (0)

    printf("FDT bytes=%llu\n", (unsigned long long)fdt.bytes);
    printf("memory_count=%u reserved_count=%u\n",
           fdt.memory_count, fdt.reserved_count);
    printf("initrd_start=0x%llx initrd_end=0x%llx\n",
           (unsigned long long)fdt.initrd_start, (unsigned long long)fdt.initrd_end);
    printf("uart_base=0x%llx uart_size=0x%llx\n",
           (unsigned long long)fdt.uart_base, (unsigned long long)fdt.uart_size);
    printf("gic_dist=0x%llx(0x%llx) gic_redist=0x%llx(0x%llx)\n",
           (unsigned long long)fdt.gic_dist_base, (unsigned long long)fdt.gic_dist_size,
           (unsigned long long)fdt.gic_redist_base, (unsigned long long)fdt.gic_redist_size);

    /* Expect one memory bank at 0x40000000 size 0x3e605000 */
    CHECK(fdt.memory_count == 1, "exactly one memory bank");
    if (fdt.memory_count >= 1) {
        CHECK(fdt.memory[0].base == 0x40000000ULL, "memory base 0x40000000");
        CHECK(fdt.memory[0].size == 0x3e605000ULL, "memory size 0x3e605000");
    }
    /* Expect initrd at 0x43000000 size 0x800000 */
    CHECK(fdt.initrd_start == 0x43000000ULL, "initrd start 0x43000000");
    CHECK(fdt.initrd_end == 0x43800000ULL, "initrd end 0x43800000");
    /* Expect one reservation at 0x50000000 size 0x1000 */
    CHECK(fdt.reserved_count == 1, "exactly one reservation");
    if (fdt.reserved_count >= 1) {
        CHECK(fdt.reserved[0].base == 0x50000000ULL, "reservation base 0x50000000");
        CHECK(fdt.reserved[0].size == 0x1000ULL, "reservation size 0x1000");
    }
    /* UART discovered at 0x11002000 */
    CHECK(fdt.uart_base == 0x11002000ULL, "uart base 0x11002000");
    CHECK(fdt.uart_size == 0x1000ULL, "uart size 0x1000");
    /* GIC discovered */
    CHECK(fdt.gic_dist_base == 0x0c000000ULL, "gic dist 0x0c000000");
    CHECK(fdt.gic_dist_size == 0x40000ULL, "gic dist size 0x40000");
    CHECK(fdt.gic_redist_base == 0x0c040000ULL, "gic redist 0x0c040000");
    CHECK(fdt.gic_redist_size == 0x200000ULL, "gic redist size 0x200000");

    /* fdt_chosen_u64 helper */
    uint64_t v = 0;
    CHECK(fdt_chosen_u64(dtb, "linux,initrd-start", &v) == 1, "chosen initrd-start readable");
    CHECK(v == 0x43000000ULL, "chosen initrd-start value");
    v = 0;
    CHECK(fdt_chosen_u64(dtb, "linux,initrd-end", &v) == 1, "chosen initrd-end readable");
    CHECK(v == 0x43800000ULL, "chosen initrd-end value");
    v = 0;
    CHECK(fdt_chosen_u64(dtb, "nonexistent-prop", &v) == 0, "absent prop returns 0");

    /* ADT build smoke test with a minimal payload */
    struct xnu_payload payload;
    memset(&payload, 0, sizeof(payload));
    memcpy(payload.magic, "BEGXNU01", 8);
    payload.version = 1;
    payload.segment_count = 1;
    payload.virt_base = 0xffffff8000200000ULL;
    payload.phys_base = 0x40000000ULL;
    payload.mem_size = 0x20000000ULL;
    payload.entry_va = 0xffffff8000200000ULL;
    payload.kernel_end_pa = 0x40020000ULL;
    payload.segments_offset = sizeof(payload);
    payload.data_offset = sizeof(payload) + sizeof(struct xnu_segment);
    payload.total_size = sizeof(payload);
    struct xnu_segment seg = {
        .physical = 0x40000000ULL,
        .memory_size = 0x20000ULL,
        .file_offset = payload.data_offset,
        .file_size = 0x1000ULL,
    };
    /* Recompute totals to be self-consistent */
    payload.total_size = payload.data_offset + seg.file_size;
    payload.segment_count = 1;

    unsigned char adt_buf[XNU_PAGE];
    memset(adt_buf, 0, sizeof(adt_buf));
    size_t adt_size = xnu_make_adt(adt_buf, sizeof(adt_buf), &payload, &fdt, 13000000);
    CHECK(adt_size > 0, "ADT builds non-empty");
    CHECK(adt_size <= XNU_PAGE, "ADT fits in one page");

    printf("\n%d checks, %d failures\n", 20, failures);
    free(dtb);
    return failures ? 1 : 0;
}
