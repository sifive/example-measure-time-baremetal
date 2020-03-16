/* Copyright 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

/*
 * This example uses memory mapped mtime and CSR mcycle
 * to measure time.
 */

#include <stdio.h>
#include <stdlib.h>

/* These includes get created at build time, and are based on the contents
 * in the bsp folder.  They are useful since they allow us
 * to use auto generated symbols and base addresses which may change
 * based on the design, and every design has it's own unique bsp.
 */
#include <metal/machine.h>
#include <metal/machine/platform.h>
#include <metal/machine/inline.h>

#define RTC_FREQ                32768
#define CPU_FREQ                32500000

/* Compile time options to determine which interrupt modules we have */
#define CLINT_PRESENT                           (METAL_MAX_CLINT_INTERRUPTS > 0)
#define CLIC_PRESENT                            (METAL_MAX_CLIC_INTERRUPTS > 0)

/* Offsets for multi-core systems */
#define MSIP_PER_HART_OFFSET                             0x4
#define MTIMECMP_PER_HART_OFFSET                         0x8

#if CLINT_PRESENT
#define CLINT_BASE_ADDR                                 METAL_RISCV_CLINT0_0_BASE_ADDRESS
#define MSIP_BASE_ADDR(hartid)                          (CLINT_BASE_ADDR + METAL_RISCV_CLINT0_MSIP_BASE + (hartid * MSIP_PER_HART_OFFSET))
#define MTIMECMP_BASE_ADDR(hartid)                      (CLINT_BASE_ADDR + METAL_RISCV_CLINT0_MTIMECMP_BASE + (hartid * MTIMECMP_PER_HART_OFFSET))
#define MTIME_BASE_ADDR                                 (CLINT_BASE_ADDR + METAL_RISCV_CLINT0_MTIME)
#endif

#if CLIC_PRESENT
#define CLIC_BASE_ADDR                                  METAL_SIFIVE_CLIC0_0_BASE_ADDRESS
#define MSIP_BASE_ADDR(hartid)                          (CLIC_BASE_ADDR + METAL_SIFIVE_CLIC0_MSIP_BASE + (hartid * MSIP_PER_HART_OFFSET))
#define MTIMECMP_BASE_ADDR(hartid)                      (CLIC_BASE_ADDR + METAL_SIFIVE_CLIC0_MTIMECMP_BASE + (hartid * MTIMECMP_PER_HART_OFFSET))
#define MTIME_BASE_ADDR                                 (CLIC_BASE_ADDR + METAL_SIFIVE_CLIC0_MTIME)
#endif

#define NUM_TICKS_ONE_S                                 RTC_FREQ            // it takes this many ticks of mtime for 1s to elapse

/* Defines to access CSR registers within C code */
#define read_csr(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define write_csr(reg, val) ({ \
  asm volatile ("csrw " #reg ", %0" :: "rK"(val)); })

#define write_dword(addr, data)                 ((*(uint64_t *)(addr)) = data)
#define read_dword(addr)                        (*(uint64_t *)(addr))
#define write_word(addr, data)                  ((*(uint32_t *)(addr)) = data)
#define read_word(addr)                         (*(uint32_t *)(addr))
#define write_byte(addr, data)                  ((*(uint8_t *)(addr)) = data)
#define read_byte(addr)                         (*(uint8_t *)(addr))

/* Main - Show demonstration of mtime and mcycle registers */
int main() {

    uint64_t t0, t1;
    uint64_t mcycle0, mcycleh0, mcycle1, mcycleh1;
    long cycle_ticks, mtime_ticks;

    /* Welcome message for MTIME */
    printf ("Measuring 10s with MTIME, a 64-bit memory mapped register...\n");
    fflush(stdout);

    // 10s test using mtime
    t0 = read_dword(MTIME_BASE_ADDR);
    while (read_dword(MTIME_BASE_ADDR) < (t0 + (10 * RTC_FREQ)));
    t1 = read_dword(MTIME_BASE_ADDR);
    mtime_ticks = (long)(t1 - t0);
    printf ("Ten second mtime ticks: %d\n", mtime_ticks);
    fflush(stdout);

    // measure ticks of mtime
    t0 = read_dword(MTIME_BASE_ADDR);
    printf ("This is a print statement!\n");
    t1 = read_dword(MTIME_BASE_ADDR);
    mtime_ticks = (long)(t1 - t0);
    printf ("MTIME Ticks for printf: %d\n", mtime_ticks);
    fflush(stdout);

    /* Message for MCYCLE demo */
    printf ("\nNow measuring 10s with MCYCLE, a 64-bit Control Status Register (CSR)...\n");
    fflush(stdout);

    // 10s test using mcycle.  CPU Frequency is 32.5 MHz on Arty
#if __riscv_xlen == 32

    // The do/while method guarantees no rollover on 32-bit architectures
    do {
        mcycleh0 = read_csr(mcycleh);
        mcycle0 = read_csr(mcycle);
        while (read_csr(mcycle) < (mcycle0 + (10 * CPU_FREQ)));
        mcycle1 = read_csr(mcycle);
        mcycleh1 = read_csr(mcycleh);
        cycle_ticks = (long)(mcycle1 - mcycle0);
    } while (mcycleh0 != mcycleh1);

    printf ("Ten second mcycle ticks: %d\n", cycle_ticks);
    fflush(stdout);

    // measure ticks of mcycle and guarantee no rollover
    do {
        // measure printf statment using mcycle
        mcycleh0 = read_csr(mcycleh);
        mcycle0 = read_csr(mcycle);
        printf ("This is a print statement!\n");
        mcycle1 = read_csr(mcycle);
        mcycleh1 = read_csr(mcycleh);
        cycle_ticks = (long)(mcycle1 - mcycle0);
    } while (mcycleh0 != mcycleh1);

#elif __riscv_xlen == 64

    // on 64-bit architectures, mcycle is a 64-bit register
    mcycle0 = read_csr(mcycle);
    while (read_csr(mcycle) < (mcycle0 + (10 * CPU_FREQ)));
    mcycle1 = read_csr(mcycle);
    cycle_ticks = (long)(mcycle1 - mcycle0);

    printf ("ten seconds mcycle ticks: %d\n", cycle_ticks);
    fflush(stdout);

    // measure printf statment using mcycle
    mcycle0 = read_csr(mcycle);
    printf ("This is a print statement!\n");
    mcycle1 = read_csr(mcycle);
    cycle_ticks = (long)(mcycle1 - mcycle0);

#endif

    printf("MCYCLE Ticks for printf: %d\n", cycle_ticks);
    fflush(stdout);

    printf ("Thanks!  Now exiting...\n");

    exit (0);
}

