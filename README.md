About
=====

This is a small 'bare metal' utility to test the Branch Trace Store (or BTS) functionality of Intel CPUs. It was intended to be used to build BTS support into a hypervisor, but unfortunately I didn't find time to do that. 

It produces a bootable ISO containing 32- and 64-bit binaries, and also .bin files that you can load via PXE or directly into an emulator.

Debugging
=========

I found Bochs particularly useful for debugging, especially the transition to protected mode. There's a bochsrc file included which you can use.

Building
========

The easiest way to build is via Docker. I use a version of brett's Dockerfile to handle building the toolchain, so you can just:
```
docker build . -f Dockerfile.64bit -t bts64
docker run --name bts64 bts64
docker cp bts64:/root/bts64.bin isodir/

docker build . -f Dockerfile.32bit -t bts32
docker run --name bts32 bts32
docker cp bts32:/root/bts32.bin isodir/
```
Once you've got bts32.bin and bts64.bin, you can make the iso:
```
grub-mkrescue -o bts.iso isodir/
```
Running
=======

You can just boot the ISO in your favourite hypervisor (or emulator), burn it to CD to run on physical hardware, or just PXE boot it:
```
cp bts.iso tftpboot/bts.iso
```
Setting up PXE is out of the scope of this readme (sorry!), but you should be able to just use the pxelinux 'memdisk' driver to boot the CD:
```
echo "default memdisk initrd=bts.iso" > tftpboot/pxelinux.cfg/default
```

