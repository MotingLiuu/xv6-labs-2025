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
 * 1. merge sections: linker would merge sections with the same permissions to a single section
 * 2. Decide the address of each section
 * 3. Do relocation
 * 4. Determine the entry point
 * 5. Generate the ELF header / program header
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
 * ENTRY( _entry ) sets the entry point to _entry
 */
OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

/*
 * SECTIONS block describes the final va layout
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
  * PROVIDE(symbol = .), define a global symbol which can be accessed in c code through (extern char symbol[], result is
  * the current va).
  *
  * 1. kernel/entry.o(_entry) puts the entry point at 0x80000000
  * 2. *(.text .text.*) puts the text section in compiled o file after the entry point
  * 3. _trampoline = ., define a symbol for the trampoline section
  * 4. Question: what is *(trampsec)? what is *(.text .text.*), why there is 2 member in *()?
  * 5. Question: .srodata (Small Read-Only Data), RISV-V compiler puts small read-only data .srodata, use gp to access
  * explain in detail
  * 6. Question: what is static var?
  */
  .text : {
    kernel/entry.o(_entry)
    *(.text .text.*)
    . = ALIGN(0x1000);
    _trampoline = .;
    *(trampsec)
    . = ALIGN(0x1000);
    ASSERT(. - _trampoline == 0x1000, "error: trampoline larger than one page");
    PROVIDE(etext = .);
  }

  /*
  * .srodata (Small Read-Only Data), RISV-V compiler puts small read-only data .srodata, use gp to access
  */
  .rodata :
    . = ALIGN(16);
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(16);
    *(.rodata .rodata.*)
  }

  /*
  * Store global var and static var with initial value
  */
  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*) /* do not need to distinguish this from .data */
    . = ALIGN(16);
    *(.data .data.*)
  }

  /*
  * Store global var and static var without initial value
  */
  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
  }

  PROVIDE(end = .);
}
