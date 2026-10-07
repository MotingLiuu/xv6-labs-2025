// Summary:
// 1. set up mstatus.MPP and mepc. use mret to jump into main and s-mode
// 2. delegate all interrupts(mideleg) and exceptions(medeleg) to s mode, enable SIE_SEIE and SIE_STIE
// 3. set PMP
// 4. enable timer interrupt
// 5. save mhartid into tp with w_tp() in advance, for mhartid is a M mode register, can not be accessed in S mode

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

void main();
void timerinit();

// entry.S needs one stack per CPU.
// __attribute__ ((aligned (16))) tells the compiler to put the start address of stack0 should be 16-byte aligned.
// stack0 is a var that not be initialized. It will be put into the .bss section.
__attribute__ ((aligned (16))) char stack0[4096 * NCPU];

// entry.S jumps here in machine mode on stack0.
void
start()
{
  // set M Previous Privilege mode to Supervisor, for mret.
  unsigned long x = r_mstatus();
  // mstatus register's MPP(Machine Previous Privilege), records the previous privilege mode before trap or exception.
  //
  // mstatus register's MIE(Machine Interrupt Enable), MIE == 1 means machine-level interrupt is allowed. mstatus.MIE is main switch, mie.MTIE, mie.MEIE is the specific switch.
  // There is another CSR named mie, (contains MTIE(machine timer interrupt enable), MEIE(machine external interrupt enable), MSIE(machine software interrupt enable))
  //
  // mastatus.MPIE(Machine Previous Interrupt Enable), records the MIE's value before entering M-mode trap.
  //
  //
  // when mret is executed, the privilege mode will be restored to the value in MPP.
  x &= ~MSTATUS_MPP_MASK; // clean MPP field
  x |= MSTATUS_MPP_S; // set MPP to Supervisor mode
  w_mstatus(x); // write x into mstatus

  // Note: when CPU get into Machine mode trap, cpu save current PC into mepc.
  // mepc is also software-writable. Here no trap has occured;
  // xv6 manually sets mepc = main so that mret jumps to main in S-mode.
  // e.g.
  // PC = 0x80001234
  // trap happens, get into M-mode
  // hardware:
  // mepc = 0x80001234
  // pc = mtvec(trap handler)
  //
  //
  // Note: mret
  // 1. sets the program counter to mepc.
  // 2. change the privilege mode to the value in mstatus.MPP
  // 3. restore the machine interrupt-enable state: MIE gets the old value of MPIE, and MPIE is set to 1.
  // set M Exception Program Counter to main, for mret.
  // 4. restets MPP to the least-privilege
  // requires gcc -mcmodel=medany
  w_mepc((uint64)main);

  // disable paging for now.
  w_satp(0);

  // delegate all interrupts and exceptions to supervisor mode.
  //
  // medeleg decides which exceptions are delegated to supervisor mode.
  //
  // mideleg decides which interrupts are delegated to supervisor mode.
  //
  // sie(Supervisor Interrupt Enable Register), SIE_SEIE(Supervisor External Interrupt Enable), SIE_STIE(Supervisor Timer Interrupt Enable)
  // interrupt arrives
  // -> mideleg, should this interrupt be delegated to supervisor mode?
  // -> sie, is s-mode allowed to handle this interrupt?
  // -> take s-mode interrupt

  w_medeleg(0xffff);
  w_mideleg(0xffff);
  w_sie(r_sie() | SIE_SEIE | SIE_STIE);

  // configure Physical Memory Protection to give supervisor mode
  // access to all of physical memory.
  //
  // pmpaddr0 saves the boundary of PMP0 region, physical boundary = pmpaddr0 << 2
  // pmpcfg0 is a 8bit configuration, the first byte control pmpaddr0. 0x0f means TOR(TOP of Range), RWX
  // 0x0f = 0000 1111
  //        ││││ │││└ R
  //        ││││ ││└─ W
  //        ││││ │└── X
  //        ││└└───── A
  //        └──────── L
  // Question: So access to a pa(write, read, excute) is checked by page's permission and pmp? two mechanisms?
  // Answer: yes, PageTable controls va->pa and va's permission, PMP controls whether the final pa is accessible(RWX) in S mode.
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf);

  // ask for clock interrupts.
  timerinit();

  // keep each CPU's hartid in its tp register, for cpuid().
  int id = r_mhartid();
  w_tp(id);

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
}

// ask each hart to generate timer interrupts.
void
timerinit()
{
  // enable supervisor-mode timer interrupts.
  w_mie(r_mie() | MIE_STIE);

  // enable the sstc extension (i.e. stimecmp).
  w_menvcfg(r_menvcfg() | (1L << 63));

  // allow supervisor to use stimecmp and time.
  w_mcounteren(r_mcounteren() | 2);

  // ask for the very first timer interrupt.
  w_stimecmp(r_time() + 1000000);
}
