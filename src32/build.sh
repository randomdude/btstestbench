TRIPLET=i686-elf-

rm -f obj32 bts32.bin tftpboot/bts32.bin 2>/dev/null
mkdir -p obj32

${TRIPLET}as src32/boot.s -o obj32/boot.o
${TRIPLET}as src32/interrupts.s -o obj32/interrupts.o
${TRIPLET}gcc -c kernel.c -o obj32/kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
${TRIPLET}gcc -c tests.c -o obj32/tests.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
${TRIPLET}gcc -c bts.c -o obj32/bts.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
${TRIPLET}gcc -c heap.c -o obj32/heap.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
${TRIPLET}gcc -c terminal.c -o obj32/terminal.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
${TRIPLET}gcc -c misc.c -o obj32/misc.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
${TRIPLET}gcc -T src32/linker.ld -o bts32.bin -ffreestanding -O2 -nostdlib obj32/boot.o obj32/terminal.o obj32/kernel.o obj32/misc.o obj32/tests.o obj32/bts.o obj32/heap.o obj32/interrupts.o -lgcc

#cp bts32.bin isodir/boot/
#cp bts32.bin tftpboot/
#grub-mkrescue -o bts.iso isodir/
