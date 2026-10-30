/* SPDX-License-Identifier: MIT
 * Begonia boot shim entry.
 * ARM64 Linux Image header so the MediaTek bootloader / ABL treats us like a
 * kernel and enters at EL1 with MMU and caches off, x0 = DTB pointer.
 *
 * Enters at EL1 (or EL2 if that is what the MediaTek LK provides). The
 * observed exception level is reported over UART before any state change.
 */
.section .text.head, "ax"
.global _head
_head:
    b _start
    .long 0
    .quad 0          /* text_offset */
    .quad _end       /* image_size */
    .quad 0xa        /* flags: LE, 4K pages, relocatable */
    .quad 0, 0, 0
    .ascii "ARM\x64"
    .long 0

.text
.global _start
_start:
    msr daifset, #0xf
    mov x19, x0          /* DTB pointer */
    mov x20, x1
    mov x21, x2
    mov x22, x3
    mov x23, sp          /* incoming sp */
    msr spsel, #1
    adr x1, _stack_top
    mov sp, x1
    adr x1, vectors
    mrs x2, CurrentEL
    cmp x2, #8
    b.eq 1f
    cmp x2, #4
    b.ne halt
    msr vbar_el1, x1
    b 2f
1:  msr vbar_el2, x1
2:  isb
    adr x1, _bss_start
    adr x2, _bss_end
3:  cmp x1, x2
    b.hs 4f
    str xzr, [x1], #8
    b 3b
4:  mov x0, x19
    mov x1, x20
    mov x2, x21
    mov x3, x22
    mov x4, x23
    bl shim_main
halt:
    wfe
    b halt

.global enter_el1
enter_el1:
    /* Drop EL2 -> EL1h. Called only with EL2 MMU/caches off (verified). */
    mov x1, #(1 << 31)
    msr hcr_el2, x1
    mov x1, #3
    msr cnthctl_el2, x1
    msr cntvoff_el2, xzr
    mov x1, #0x33ff
    msr cptr_el2, x1
    msr hstr_el2, xzr
    msr vttbr_el2, xzr
    mov x1, #0x0800
    movk x1, #0x30d0, lsl #16
    msr sctlr_el1, x1
    adr x1, vectors
    msr vbar_el1, x1
    adr x1, _stack_top
    msr sp_el1, x1
    adr x1, el1_start
    msr elr_el2, x1
    mov x1, #0x3c5
    msr spsr_el2, x1
    dsb sy
    isb
    eret
el1_start:
    mov x29, xzr
    mov x30, xzr
    bl shim_el1
    b halt

.macro vector slot
    .balign 128
    mov x0, #\slot
    b exception_report
.endm
.balign 2048
vectors:
    vector 0
    vector 1
    vector 2
    vector 3
    vector 4
    vector 5
    vector 6
    vector 7
    vector 8
    vector 9
    vector 10
    vector 11
    vector 12
    vector 13
    vector 14
    vector 15
