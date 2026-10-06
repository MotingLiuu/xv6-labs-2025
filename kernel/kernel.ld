/*
 * kernel.ld is read by the linker during make's link phase.
 * in xv6, -T kernel/kernel.ld tells the linker to use this file as the link script.
 * riscv64-unknown-elf-ld \
 *  -T kernel/kernel.ld \
 *  -o kernel/kernel \
 *  kernel/entry.o \
 *  kernel/start.o \
 *  kernel/main.o \
 *  ...
 *
 * Linker does:
 * 1. merge sections: Map input sections from .o file into output sections according to .ld
 * 2. Decide the VMA/LMA of output sections.
 * 3. Resolve the symbols and perform relocation.
 * 4. Determine the entry point
 * 5. Generate the ELF header / program header
 * Output sections may later be grouped in to loadable SEGMENTS according to their attributes/permissions.
 *
 *
 * ELF Header describes the basic information of the executable file.
 * 1. Magic number
 * 2. Target architecture and machine type
 * 3. Entry point's address
 * 4. Program header table's offset and section head offset
 *
 * Program Header Table (tell the os how to map the segments of file to va)
 * 1. type: whether the segment needs to be loaded into memory
 * 2. flags: read/write/execute (ELF_PROG_FLAG_READ, WRITE, EXEC)
 * 3. off: this segment's offset in the file
 * 4. vaddr: virtual address mapped to
 * 5. filesz/memsz: size of the segment in the file / size of the segment in memory, if memsz > filesz, pad with zeros in va
 */

/*
 * OUTPUT_ARCH( "riscv" ) sets the target architecture to riscv
 * ENTRY( _entry ) sets the e_entry in ELF Header to _entry's final address. Question: is this va or pa? Is there difference
 * between user prog and os prog?
 *
 * ENTRY(_entry) sets ELF e_entry to the final address of _entry.
 *
 * e_entry itself is just an address value. Whether it is interpreted
 * as a VA or a PA depends on the execution environment.
 *
 * User ELF:
 *   e_entry and p_vaddr are user virtual addresses.
 *   exec() allocates physical pages, maps them at ph.vaddr,
 *   loads the segment, and finally sets trapframe->epc = elf.entry.
 *
 * Kernel ELF:
 *   kernel.ld primarily defines section VMAs.
 *
 *   xv6 deliberately links the kernel at 0x80000000, which is also
 *   where QEMU loads it physically.
 *
 *   Before paging is enabled:
 *       PC 0x80000000 is effectively a physical address.
 *
 *   After paging is enabled:
 *       kernel VA 0x80000000
 *            -> PA 0x80000000
 *
 *   Thus in xv6 the kernel's link-time VA and physical load address
 *   have the same numeric value, which makes them easy to confuse.
 *
 */
OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

/*
 * SECTIONS describes how input sections are mapped into output sections and determine the VMAs of those output sections.
 */
SECTIONS
{
  /*
   * ensure that entry.S / _entry is at 0x80000000,
   * where qemu's -kernel jumps.
   *
   * Location counter, this line means put the following content from 0x80000000.
   */
  . = 0x80000000;

  /*
  * PROVIDE(symbol = .), If etext is referenced but not defined elsewhere, define linker symbol etext whose value is the current location counter.
  *
  * 1. kernel/entry.o(_entry) is [filename(section-name)], this means select the _entry section from kernel/entry.o, but there is no section named _entry
  * 2. *(.text .text.*), * means the input files, .text means .text section in .o file, .text.* means .text.foo, .text.main ...
  * 3. _trampoline = ., define a symbol for the trampoline section
  * 4. Question: what is *(trampsec)?
  *    Answer: *(trampsec) means select the trampsec section from the input files.
  * 5. Question: Will not trampoline.S be compiler into .o file with .text section? If specific the section and its name,
  *    the output file compiled only will only contain the specified section? In what case, the section name is .text? what doest .text represent?
  */
  .text : {
    kernel/entry.o(_entry)
    *(.text .text.*)
    . = ALIGN(0x1000);
    /*
     * set the current address to PAGEUPGROUND(current address)
     */
    _trampoline = .;
    *(trampsec)
    . = ALIGN(0x1000);
    ASSERT(. - _trampoline == 0x1000, "error: trampoline larger than one page");
    PROVIDE(etext = .);
  }

  /*
   * .srodata / .sdata / .sbss are "small data" input sections that
   * a RISC-V toolchain may generate for small global/static objects.
   *
   * They can participate in optimizations such as gp-relative addressing
   * in environments configured for it.
   *
   * xv6 does not need to distinguish them in its final memory layout:
   *   .srodata -> output .rodata
   *   .sdata   -> output .data
   *   .sbss    -> output .bss
   */
  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(16);
    *(.rodata .rodata.*)
  }

 /*
  * .data:
  * initialized writable objects with static storage duration,
  * typically initialized global variables and static variables.
  */

 /*
  * .bss:
  * zero-initialized / uninitialized writable objects with static
  * storage duration, typically global variables and static variables.
  */
  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*) /* do not need to distinguish this from .data */
    . = ALIGN(16);
    *(.data .data.*)
  }

 .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
  }

  PROVIDE(end = .);
}
