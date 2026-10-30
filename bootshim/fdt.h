/* SPDX-License-Identifier: MIT
 * Flattened device tree inspector for the Begonia boot shim.
 * Parses the live FDT provided by the MediaTek bootloader: /memory regions,
 * /reserved-memory, /chosen linux,initrd{start,end}, plus the UART and GIC
 * register addresses needed to build the Apple ADT.
 * Adapted from xnu_bootshim_benesse_a04br3/fdt.c.
 */
#ifndef BEGONIA_FDT_H
#define BEGONIA_FDT_H
#include <stddef.h>
#include <stdint.h>

#define FDT_MAX_MEMORY   16
#define FDT_MAX_RESERVED 32

struct fdt_range { uint64_t base, size; };
struct fdt_info {
    uint64_t bytes;
    unsigned memory_count, reserved_count;
    struct fdt_range memory[FDT_MAX_MEMORY];
    struct fdt_range reserved[FDT_MAX_RESERVED];
    uint64_t initrd_start, initrd_end;
    /* Device register addresses discovered during the walk. */
    uint64_t uart_base, uart_size;
    uint64_t gic_dist_base, gic_dist_size;
    uint64_t gic_redist_base, gic_redist_size;
};

/* Returns 0 on success, -1 on invalid FDT. Fills info. */
int fdt_inspect(const void *data, size_t available, struct fdt_info *info);

/* Read a u64 property from /chosen by name. Returns 1 on success, 0 absent. */
int fdt_chosen_u64(const void *fdt, const char *name, uint64_t *out);

#endif
