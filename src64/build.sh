TRIPLET=x86_64-elf-

rm -f obj64/* bts64.bin tftpboot/bts64.bin 2>/dev/null
mkdir -p obj64

${TRIPLET}as src64/boot.s -o obj64/boot.o
${TRIPLET}as src64/interrupts.s -o obj64/interrupts.o
${TRIPLET}gcc -c kernel.c -o obj64/kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
${TRIPLET}gcc -c tests.c -o obj64/tests.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
${TRIPLET}gcc -c bts.c -o obj64/bts.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
${TRIPLET}gcc -c heap.c -o obj64/heap.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
${TRIPLET}gcc -c terminal.c -o obj64/terminal.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
${TRIPLET}gcc -c misc.c -o obj64/misc.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
${TRIPLET}gcc -T src64/linker.ld -o bts64.bin -ffreestanding -O2 -nostdlib obj64/boot.o obj64/terminal.o obj64/kernel.o obj64/misc.o obj64/tests.o obj64/bts.o obj64/heap.o obj64/interrupts.o -lgcc
${TRIPLET}objcopy -I elf64-x86-64 -O elf32-i386 bts64.bin
#cp bts64.bin isodir/boot/
#cp bts64.bin tftpboot/
#grub-mkrescue -o bts64.iso isodir/
