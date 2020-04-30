set architecture i386:x86-64
set disassemble-next-line on
set pagination off

add-symbol-file src/loader/loader.efi.debug  -o 0x1E369000
add-symbol-file src/kernel/arch/amd64/kernel
add-symbol-file src/init/init
