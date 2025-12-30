make clean
make
rm -f spl.bin spl.dis spl.hex
arm-linux-gnueabi-objcopy -O binary spl.axf spl.bin
arm-linux-gnueabi-objdump -D -S -m arm spl.axf > spl.dis
