make clean
make
rm - f fdl.bin fdl.dis fdl.hex
arm - linux - gnueabi - objcopy - O binary fdl.axf fdl.bin
arm - linux - gnueabi - objdump - D - S - m arm fdl.axf > fdl.dis
