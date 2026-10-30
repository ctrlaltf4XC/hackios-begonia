/* SPDX-License-Identifier: MIT
 * Begonia boot shim linker script.
 * Produces a position-independent ARM64 Linux Image payload.
 */
OUTPUT_ARCH(aarch64)
SECTIONS
{
    /* Linux Image header (64 bytes) + code, linked at 0 for position independence. */
    . = 0;
    .text.head : {
        *(.text.head)
    }
    .text : {
        *(.text *)
    }
    .rodata : {
        *(.rodata *)
    }
    .data : {
        *(.data *)
    }
    .bss : {
        _bss_start = .;
        *(.bss *)
        *(COMMON)
        _bss_end = .;
    }
    _end = .;
    /* Stack starts immediately after the image and grows down. */
    _stack_top = _end;
    /DISCARD/ : {
        *(.note.GNU-stack)
        *(.eh_frame *)
        *(.comment)
    }
}
