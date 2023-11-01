set architecture i386:x86-64
set disassemble-next-line on
set pagination off
set history save
set history remove-duplicates unlimited

target remote :1234

add-symbol-file shared/EFI/LFOS/BOOTX64.efi 0x1E1D1000 -s .rdata 0x1E1D5000 -s .data 0x1E1D6000
add-symbol-file shared/LFOS/kernel
add-symbol-file shared/LFOS/fbdemo
