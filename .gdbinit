set architecture i386:x86-64
add-symbol-file src/loader/loader.efi.debug           0x7E36C000         -s .data 0x7E375000
add-symbol-file src/kernel/arch/amd64-uefi/kernel.elf 0xffff800001000000 -s .data 0xffff800001018000
