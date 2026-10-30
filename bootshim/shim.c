/* SPDX-License-Identifier: MIT
 * Begonia (MT6785) XNU boot shim.
 * Loads an XNU Mach-O payload from the bootloader initrd, builds the Apple
 * boot_args + ADT, and jumps to the XNU entry point with MMU/caches off.
 * All hardware addresses are read from the live FDT; nothing is invented.
 */
#include <stdint.h>
#include "fdt.h"
#include "handoff.h"

__attribute__((noreturn)) void enter_el1(uint64_t dtb);
__attribute__((noreturn)) void exception_report(uint64_t slot);

static struct fdt_info observed_fdt;

#define READ_SYSREG(reg) ({ uint64_t v; __asm__ volatile("mrs %0, " #reg : "=r"(v)); v; })

/* UART0 polled console. Base comes from the live FDT; fall back to the POX
 * DTS value only if the FDT walk did not discover it (must not happen). */
static uintptr_t uart_base;

static void putc_uart(char c)
{
    uintptr_t base = uart_base ? uart_base : 0x11002000UL;
    for (unsigned int n = 0; n < 2000000; n++) {
        if (*(volatile uint32_t *)(base + 0x14) & (1u << 5)) {
            *(volatile uint32_t *)base = (uint8_t)c;
            return;
        }
    }
}
static void puts_uart(const char *s)
{
    while (*s) {
        if (*s == '\n') putc_uart('\r');
        putc_uart(*s++);
    }
}
static void value(const char *label, uint64_t v)
{
    puts_uart(label);
    puts_uart("=0x");
    for (int shift = 60; shift >= 0; shift -= 4) {
        unsigned int digit = (unsigned int)(v >> shift) & 15;
        putc_uart((char)(digit < 10 ? '0' + digit : 'a' + digit - 10));
    }
    puts_uart("\n");
}
static void message(const char *s) { puts_uart(s); }

void handoff_trace(const char *label, uint64_t v) { value(label, v); }
void handoff_message(const char *message) { puts_uart(message); }

__attribute__((noreturn)) static void stop(void)
{
    for (;;) __asm__ volatile("wfe");
}

__attribute__((noreturn)) void exception_report(uint64_t slot)
{
    uint64_t el = READ_SYSREG(CurrentEL);
    message("XNU_BEGONIA_EXCEPTION\n");
    value("vector", slot);
    value("CurrentEL", el);
    if (el == 8) {
        value("ESR_EL2", READ_SYSREG(ESR_EL2));
        value("ELR_EL2", READ_SYSREG(ELR_EL2));
        value("FAR_EL2", READ_SYSREG(FAR_EL2));
    } else {
        value("ESR_EL1", READ_SYSREG(ESR_EL1));
        value("ELR_EL1", READ_SYSREG(ELR_EL1));
        value("FAR_EL1", READ_SYSREG(FAR_EL1));
    }
    stop();
}

__attribute__((noreturn)) void shim_el1(uint64_t dtb)
{
    message("XNU_BEGONIA_EL1_V2\n");
    value("CurrentEL", READ_SYSREG(CurrentEL));
    value("SCTLR_EL1", READ_SYSREG(SCTLR_EL1));
    value("CNTFRQ_EL0", READ_SYSREG(CNTFRQ_EL0));
    value("CNTVCT_EL0", READ_SYSREG(CNTVCT_EL0));
    value("DTB", dtb);
    value("MIDR_EL1", READ_SYSREG(MIDR_EL1));
    value("MPIDR_EL1", READ_SYSREG(MPIDR_EL1));

    if (fdt_inspect((const void *)(uintptr_t)dtb, 0x1000000, &observed_fdt)) {
        message("FDT_INVALID\n");
        stop();
    }
    value("FDT_bytes", observed_fdt.bytes);
    value("FDT_memory_count", observed_fdt.memory_count);
    for (unsigned i = 0; i < observed_fdt.memory_count; i++) {
        value("FDT_memory_base", observed_fdt.memory[i].base);
        value("FDT_memory_size", observed_fdt.memory[i].size);
    }
    value("FDT_reserved_count", observed_fdt.reserved_count);
    for (unsigned i = 0; i < observed_fdt.reserved_count; i++) {
        value("FDT_reserved_base", observed_fdt.reserved[i].base);
        value("FDT_reserved_size", observed_fdt.reserved[i].size);
    }
    if (observed_fdt.initrd_end > observed_fdt.initrd_start) {
        value("FDT_initrd_start", observed_fdt.initrd_start);
        value("FDT_initrd_end", observed_fdt.initrd_end);
    }
    value("FDT_uart_base", observed_fdt.uart_base);
    value("FDT_gic_dist", observed_fdt.gic_dist_base);
    value("FDT_gic_redist", observed_fdt.gic_redist_base);

    uart_base = observed_fdt.uart_base;

    const char *result = handoff_boot(&observed_fdt, dtb);
    message(result);
    message("\n");
    stop();
}

__attribute__((noreturn)) void shim_main(uint64_t x0, uint64_t x1,
                                          uint64_t x2, uint64_t x3, uint64_t old_sp)
{
    uint64_t el = READ_SYSREG(CurrentEL);
    message("\nXNU_BEGONIA_SHIM_V1\n");
    value("CurrentEL", el);
    value("entry_x0", x0);
    value("entry_x1", x1);
    value("entry_x2", x2);
    value("entry_x3", x3);
    value("entry_sp", old_sp);
    value("MIDR_EL1", READ_SYSREG(MIDR_EL1));
    value("MPIDR_EL1", READ_SYSREG(MPIDR_EL1));
    value("ID_AA64MMFR0_EL1", READ_SYSREG(ID_AA64MMFR0_EL1));
    value("CNTFRQ_EL0", READ_SYSREG(CNTFRQ_EL0));
    value("CNTVCT_EL0", READ_SYSREG(CNTVCT_EL0));
    if (el == 8) value("SCTLR_EL2", READ_SYSREG(SCTLR_EL2));
    else         value("SCTLR_EL1", READ_SYSREG(SCTLR_EL1));
    message("XNU_BEGONIA_SHIM_DONE\n");

    if (el == 8) {
        if (READ_SYSREG(SCTLR_EL2) & ((1u << 0) | (1u << 2) | (1u << 12))) {
            message("EL1_TRANSITION_REFUSED_MMU_OR_CACHE_ON\n");
            stop();
        }
        enter_el1(x0);
    }
    if (el == 4) {
        shim_el1(x0);
    }
    stop();
}
