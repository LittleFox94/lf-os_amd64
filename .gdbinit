set architecture i386:x86-64
add-symbol-file src/kernel/arch/amd64-uefi/main.efi.debug 0xFFFF800000004000
target remote :1234
