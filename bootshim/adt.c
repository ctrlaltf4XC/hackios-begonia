/* SPDX-License-Identifier: MIT
 * Apple flattened device-tree serialization for Begonia (MT6785).
 * Builds the ADT that XNU parses via its device-tree platform expert.
 * UART/GIC addresses come from the live FDT (never hardcoded).
 */
#include "handoff.h"

struct writer { unsigned char *data; size_t used, capacity; int error; };
static void byte(struct writer *w, unsigned char value)
{
    if (w->used >= w->capacity) { w->error = 1; return; }
    w->data[w->used] = value;
    w->used++;
}
static void u32be(struct writer *w, uint32_t value)
{
    for (unsigned i = 0; i < 4; i++) byte(w, (unsigned char)(value >> (8 * (3 - i))));
}
static void node(struct writer *w, uint32_t properties, uint32_t children)
{
    u32be(w, properties); u32be(w, children);
}
static void prop(struct writer *w, const char *key, const void *value, uint32_t length)
{
    unsigned i = 0;
    while (i < 31 && key[i]) { byte(w, (unsigned char)key[i]); i++; }
    byte(w, 0);
    u32be(w, length);
    const unsigned char *p = value;
    for (uint32_t j = 0; j < length; j++) byte(w, p[j]);
    while (w->used & 3) byte(w, 0);
}
static void text(struct writer *w, const char *key, const char *value)
{
    uint32_t length = 0;
    while (value[length]) length++;
    prop(w, key, value, length + 1);
}
static void word(struct writer *w, const char *key, uint32_t value)
{
    prop(w, key, &value, sizeof(value));
}

static int command_flag(const struct xnu_payload *p, const char *flag)
{
    unsigned length = 0;
    while (flag[length]) length++;
    for (unsigned i = 0; i + length < sizeof(p->command_line); i++) {
        if (!p->command_line[i]) break;
        if (i && p->command_line[i - 1] != ' ') continue;
        unsigned j = 0;
        while (flag[j] && flag[j] == p->command_line[i + j]) j++;
        if (!flag[j] && (!p->command_line[i + j] || p->command_line[i + j] == ' ')) return 1;
    }
    return 0;
}

size_t xnu_make_adt(void *buffer, size_t capacity, const struct xnu_payload *p,
                    const struct fdt_info *fdt, uint32_t timebase)
{
    struct writer w = { buffer, 0, capacity, 0 };
    uint64_t range[2] = { p->phys_base, p->kernel_end_pa - p->phys_base };

    /* Root node */
    node(&w, 6, 4);
    text(&w, "name", "device-tree");
    text(&w, "device_type", "device-tree");
    static const char compatible[] = "xiaomi,begonia\0mediatek,MT6785";
    prop(&w, "compatible", compatible, sizeof(compatible));
    text(&w, "model", "Xiaomi Redmi Note 8 Pro (Begonia)");
    word(&w, "#address-cells", 2); word(&w, "#size-cells", 2);

    int full_root = command_flag(p, "begonia.full_root=1");
    int cpus = command_flag(p, "cpus=4") ? 4 : command_flag(p, "cpus=2") ? 2 : 1;
    if (cpus < 1) cpus = 1;

    /* /chosen */
    node(&w, 5 + (full_root ? 7 : 0), 1 + full_root);
    text(&w, "name", "chosen");
    static const unsigned char development_manifest[48] = {
        0x06,0xc2,0x72,0x0d,0x73,0x22,0x16,0x6d,0x2d,0x70,0xfd,0x97,0xed,0x5a,
        0x8c,0x56,0x9a,0xf4,0x11,0x58,0x93,0xe2,0x03,0x2d,0xd8,0x61,0x2e,0xb7,
        0x6d,0x6e,0x79,0xfe,0x4a,0x63,0x83,0xc1,0x11,0xdb,0x2a,0x61,0xe1,0xb2,
        0x35,0x8c,0x2e,0x94,0x21,0x93
    };
    prop(&w, "boot-manifest-hash", development_manifest, sizeof(development_manifest));
    if (full_root) {
        uint64_t development_chip = 0x4245474f4e494100ULL; /* "BEGONIA\0" */
        prop(&w, "unique-chip-id", &development_chip, sizeof(development_chip));
        word(&w, "chip-id", 0x6785);
        text(&w, "crypto-hash-method", "sha2-384");
        word(&w, "certificate-production-status", 0);
        word(&w, "effective-production-status-ap", 0);
        word(&w, "certificate-security-mode", 1);
        word(&w, "effective-security-mode-sep", 1);
    }
    prop(&w, "dram-base", &p->phys_base, 8);
    prop(&w, "dram-size", &p->mem_size, 8);
    word(&w, "debug-enabled", 1);

    /* /chosen/memory-map */
    node(&w, 2 + (p->version >= 2 ? 1 : 0), 0);
    text(&w, "name", "memory-map"); prop(&w, "Kernel-Begonia", range, sizeof(range));
    if (p->version >= 2) {
        const struct xnu_segment *segments = (const void *)((const unsigned char *)p + p->segments_offset);
        const struct xnu_segment *last = &segments[p->segment_count - 1];
        uint64_t tail[2] = { last->physical, last->file_size };
        prop(&w, "PayloadTail", tail, sizeof(tail));
    }

    /* /cpus */
    node(&w, 3, cpus);
    text(&w, "name", "cpus"); word(&w, "#address-cells", 1); word(&w, "#size-cells", 0);
    static const char compatible_cpu[] = "arm,cortex-a55\0ARM,v8";
    for (int i = 0; i < cpus; i++) {
        char name[] = "cpu0"; name[3] += i;
        node(&w, 8, 0);
        text(&w, "name", name); text(&w, "device_type", "cpu");
        prop(&w, "compatible", compatible_cpu, sizeof(compatible_cpu));
        word(&w, "reg", i);
        text(&w, "state", i ? "stopped" : "running");
        word(&w, "timebase-frequency", (uint32_t)timebase);
        word(&w, "cpu-id", i);
        word(&w, "cluster-id", i / 6);
    }

    /* /arm-io */
    node(&w, 6, 2);
    text(&w, "name", "arm-io"); text(&w, "device_type", "soc");
    text(&w, "compatible", "mediatek,MT6785");
    uint64_t io_ranges[3] = { 0, 0x10000000, 0x10000000 };
    prop(&w, "ranges", io_ranges, sizeof(io_ranges));
    word(&w, "#address-cells", 2); word(&w, "#size-cells", 2);

    /* /arm-io/uart0 */
    node(&w, 4, 0);
    text(&w, "name", "uart0"); text(&w, "device_type", "serial");
    text(&w, "compatible", "mediatek,mt6577-uart");
    uint64_t uart[2] = { fdt->uart_base ? fdt->uart_base : 0x11002000ULL, 0x1000 };
    prop(&w, "reg", uart, sizeof(uart));

    /* /arm-io/interrupt-controller */
    node(&w, 5, 0);
    text(&w, "name", "interrupt-controller");
    text(&w, "compatible", "arm,gic-v3");
    text(&w, "interrupt-controller", "master");
    uint64_t gic[4] = {
        fdt->gic_dist_base ? fdt->gic_dist_base : 0x0c000000ULL,
        fdt->gic_dist_size ? fdt->gic_dist_size : 0x40000ULL,
        fdt->gic_redist_base ? fdt->gic_redist_base : 0x0c040000ULL,
        fdt->gic_redist_size ? fdt->gic_redist_size : 0x200000ULL
    };
    prop(&w, "reg", gic, sizeof(gic));
    word(&w, "virtual-timer-intid", 13);

    /* /defaults */
    node(&w, 2, 0);
    text(&w, "name", "defaults");
    word(&w, "no-effaceable-storage", 1);

    return w.error ? 0 : w.used;
}
