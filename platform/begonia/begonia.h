/*
 * Board config for the Xiaomi Redmi Note 8 Pro ("begonia", MediaTek MT6785 /
 * Helio G90T): 2x Cortex-A76 + 6x Cortex-A55, ARMv8.2, no PAC.
 *
 * Generic ARM like BCM2837 (no APPLE_ARM64_ARCH_FAMILY: none of Apple's
 * implementation-defined system registers exist), 16K pages and GICv3 like
 * VMAPPLE. Unlike VMAPPLE the GIC has two security states
 * (GICD_TYPER SecurityExtn=1 per POX DTS arm,gic-v3): Group 0 belongs to
 * TrustZone, so every interrupt, including the timer, is a Group 1 IRQ taken
 * through sleh_irq().
 *
 * ALL hardware addresses below are taken from the live FDT at boot by the
 * begonia boot shim and passed to XNU via the ADT; this header only records
 * the POX DTS reference values for cross-checking (pox-begonia-kernel
 * @ 2fb79cd9cba6, arch/arm64/boot/dts/mediatek/mt6785.dts).
 *
 * Reference values from POX DTS (verify against live FDT; do not hardcode):
 *   UART0 base         0x11002000  mt6785.dts:2093  IRQ SPI 115
 *   GICv3 distributor  0x0c000000  mt6785.dts:411   size 0x40000
 *   GICv3 redistributor 0x0c040000 mt6785.dts:412   size 0x200000
 *   armv8-timer PPI    13/14/11/10 mt6785.dts:779   virtual timer = PPI 13
 *   PSCI method        smc         mt6785.dts:265   arm,psci-1.0
 *   Watchdog           0x10007000 mt6785.dts:578   IRQ SPI 72 (do not touch)
 *   DRAM               /memory node mt6785.dts:283   0x40000000/0x3e605000
 *                    (target ships 6 GiB; parse live FDT /memory)
 *
 * DISCREPANCY: POX DTS marks cpu6/cpu7 as arm,cortex-a75 (mt6785.dts:112).
 * Target spec says Cortex-A76. Probe MIDR_EL1 at boot; do not rely on the
 * DTS compatible string.
 */
#ifndef _PEXPERT_ARM64_BEGONIA_H
#define _PEXPERT_ARM64_BEGONIA_H

#define NO_MONITOR                1
#define NO_ECORE                  1

#define BEGONIA                   1

#define ARM_ARCH_TIMER
#define __ARM_ARCH__              8
#define __ARM_VMSA__              8
#define __ARM_VFP__               4
#define __ARM_COHERENT_CACHE__    1
#define __ARM_DEBUG__             7
#define __ARM_ENABLE_SWAP__       1
#define __ARM_V8_CRYPTO_EXTENSIONS__ 1
#define __ARM_PAN_AVAILABLE__     1
#define __ARM_16K_PG__            1

#define HAS_GIC_V3                1

#ifndef ASSEMBLER
#define PLATFORM_PANIC_LOG_DISABLED
#endif /* ! ASSEMBLER */

/*
 * MediaTek numbers the architected timer PPIs as described in the POX DTS
 * arm,armv8-timer node: virtual timer CNTV is PPI 13 (INTID 16 + 13 = 29?
 * No: PPIs are INTID 16-31, so PPI 13 = INTID 29). The XNU sleh_irq reads
 * ICC_IAR1_EL1 directly; the ADT publishes virtual-timer-intid = 13 and
 * pe_fiq.c reads it via SecureDTGetProperty.
 */
#define BEGONIA_VTIMER_INTID      13

#define GIC_SPURIOUS_IRQ          1023    // IRQ no. for GIC spurious interrupt

#define GICR_PE_SIZE              0x20000 // Size of each redistributor region

/* GICv3 register definitions; see GICv3 spec (Arm IHI 0069G) */
#define GICD_CTLR                 0x0

/* Non-secure view of GICD_CTLR (DS=0) */
#define GICD_CTLR_ENABLEGRP1      0x1
#define GICD_CTLR_ENABLEGRP1A     0x2
#define GICD_CTLR_ARE_NS          0x10
#define GICD_CTLR_RWP             0x80000000

#define GICR_TYPER                              0x08
#define GICR_WAKER                              0x14
#define GICR_IGROUPR0                           0x10080
#define GICR_ISENABLER0                         0x10100
#define GICR_IPRIORITYR0                        0x10400

#define GICR_TYPER_AFFINITY_VALUE_SHIFT         32
#define GICR_TYPER_LAST                         0x10

#define GICR_WAKER_PROCESSORSLEEP               0x2
#define GICR_WAKER_CHILDRENASLEEP               0x4

#define ICC_CTLR_EOIMODE                        0x2

#define ICC_SRE_SRE                             0x1
/* End of GICv3 register definitions */

/* SMP: six A55s available for XNU until the A76s are audited. */
#define BEGONIA_SMP_MAX_CPUS         6
#define BEGONIA_MAX_CPUS             8
#define BEGONIA_MAX_CPU_CLUSTERS     2

#endif /* ! _PEXPERT_ARM64_BEGONIA_H */
