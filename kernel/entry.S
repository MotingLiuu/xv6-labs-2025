        # qemu -kernel loads the kernel at 0x80000000
        # and causes each hart (i.e. CPU) to jump there.
        # kernel.ld causes the following code to
        # be placed at 0x80000000.
.section .text
.global _entry
_entry:
        # set up a stack for C.
        # stack0 is declared in start.c,
        # with a 4096-byte stack per CPU.
        # sp = stack0 + ((hartid + 1) * 4096)
        la sp, stack0
        li a0, 1024*4
        csrr a1, mhartid
        // mhartid is from 0, 1, 2,...
        addi a1, a1, 1
        mul a0, a0, a1
        add sp, sp, a0
        // sp points to the next byte after the top of the stack for this CPU
        # jump to start() in start.c
        call start
spin:
        j spin
