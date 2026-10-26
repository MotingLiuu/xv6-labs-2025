
set confirm off
set pagination off
set disassemble-next-line on
set architecture riscv:rv64

symbol-file kernel/kernel
add-symbol-file user/_test 0x0
add-symbol-file kernel/trampoline.o 0 -s trampsec 0x3ffffff000

target remote 127.0.0.1:25501

display/3i $pc

