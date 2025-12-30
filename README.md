MAIX AX620E System Build
=====

## Introduction

This repository contains the source code for building `MaixCam2`, `KVM Pro` etc. basic system images.

This repository requires the submodules [maix_ax620e_sdk_msp](https://github.com/sipeed/maix_ax620e_sdk_msp) and [maix_ax620e_sdk_kernel](https://github.com/sipeed/maix_ax620e_sdk_kernel) to be cloned before compiling.

> Note: Currently only tested to compile successfully on `Ubuntu22.04`

## Get Source Code

```shell
git clone https://github.com/sipeed/maix_ax620e_sdk
git submodule update --init --recursive
```

## Build

* Download toolchain

```shell
# Download and extract
wget https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
sudo tar -xvf gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz -C /opt

# Configure environment variables
echo 'export PATH=/opt/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin:${PATH}' >> ~/.bashrc
source ~/.bashrc
```

> Alternative toolchain download method: Download the gcc9.2-2019.12 toolchain from [here](https://developer.arm.com/downloads/-/gnu-a)


* Test toolchain
```shell
aarch64-none-linux-gnu-gcc --version
which aarch64-none-linux-gnu-gcc
```

* Install necessary software
```shell
sudo dpkg-reconfigure dash # Then choose No

sudo apt install make libc6:i386 lib32stdc++6 zlib1g-dev libncurses5-dev ncurses-term libncursesw5-dev g++ u-boot-tools texinfo texlive gawk libssl-dev openssl bc bison flex gcc libgcc1 gdb build-essential lib32z1 u-boot-tools device-tree-compiler qemu qemu-user-static fusefat

sudo python3 -m pip install --upgrade pip
sudo pip3 install lxml axp-tools
```

* Build ubuntu rootfs

```shell
# If the rootfs source code has not been modified, you only need to build once
cd rootfs/arm64/glibc/ubuntu_rootfs/

# For MaixCAM2
./mk_ubuntu_base_sipeed.sh .

# For KVM Pro
./mk_ubuntu_kvm_sipeed.sh .
```

* Build System Image
> Note: If you encounter a missing symbol error during the first compilation, try to recompile. This may be because some dependencies are generated during the compilation process. Or you can add the `-j1` option to avoid multi-threaded compilation when compiling.

```shell
# export use_ubuntu_rootfs=no  # For MaixCAM2 and KVM project, the default is yes

cd build

# For MaixCAM2
make p=AX630C_emmc_arm64_k419_sipeed_maixcam2 clean all install axp -j8

# For KVM Pro
make p=AX630C_emmc_arm64_k419_sipeed_nanokvm clean all install axp -j8
```

* In `build/out` directory, you will find the `axp` format package.

## Customize rootfs

If you need to add or remove rootfs packages during development, you can modify the packaging script for the corresponding platform.

The path of the packaging script:

| Platform     | Packaging script path    |
| -------- | --------------------------------------------------------- |
| MaixCAM2 | rootfs/arm64/glibc/ubuntu_rootfs/mk_ubuntu_base_sipeed.sh |
| KVM Pro  | rootfs/arm64/glibc/ubuntu_rootfs/mk_ubuntu_kvm_sipeed.sh  |

Modification method reference:

- If you need to install the `i2c-tools` utility, add `apt install -y i2c-tools` near the `apt install` section in the script.
- If you need to install the Python package `pyaudio`, add `pip install pyaudio` near the `pip install` section in the script, or add `toml` to the `rootfs/arm64/glibc/ubuntu_rootfs/requirements.txt` file. For KVM Pro, the configuration file is `kvm_requirements.txt`.

Read the packaging script code, you can customize the `rootfs` more freely by modifying the packaging script.

> Note: If you modify the rootfs and add too many files, you need to modify `build/projects/xxxx/partition_ab.mak` to ensure `ROOTFS_PARTITION_SIZE` is large enough, otherwise the packaging will be wrong.

## Based on rootfs Customize Files

This step is based on the already packaged `rootfs` to customize files, which can reduce the time required to modify `rootfs` every time.

| Platform | Project Directory                                    |
| -------- | ----------------------------------------------------- |
| MaixCAM2 | build/projects/AX630C_emmc_arm64_k419_sipeed_maixcam2 |
| KVM Pro  | build/projects/AX630C_emmc_arm64_k419_sipeed_nanokvm  |

In the project directory, modify `bootfs.filelist` to specify the content of `/boot` to be updated, modify `rootfs.filelist` to specify the content of `/` to be updated.

Examples: If you want to add new files to `/opt`, just add the new files to `rootfs/opt`.

## Download and Start

Download [AXDL](https://dl.sipeed.com/fileList/MaixCAM/MaixCAM2/Software/Tools/AXDL_V1.24.22.1.7z)
Download [AXDL Driver](https://dl.sipeed.com/fileList/MaixCAM/MaixCAM2/Software/Tools/Driver_V1.20.46.1.7z), extract and run `DriverSetup.exe` to install the driver.

By default, the build process generates an `axp` file, which is usually output to `build/out/xxx.axp`.

* The axp file can be directly flashed to EMMC using the official `AXDL` tool (this method always works).

## Creating an SD Boot Card

1. Insert the SD card.
2. Format the SD card and create an MBR partition table (it must be MBR).
3. Create partition 1 as `fat32` with 128 MB allocated.
4. Create partition 2 as `ext4`, allocating all remaining space. Note: the partition order must not be changed.
5. Copy all contents of `build/out/AX630C_emmc_arm64_k419_sipeed_maixcam2/images/sd_boot_pack` to partition 1.
6. Copy `build/out/AX630C_emmc_arm64_k419_sipeed_maixcam2/images/spl_AX630C_emmc_arm64_k419_sipeed_maixcam2_sd_signed.bin` to partition 1 and rename it to `boot.bin`.
7. Copy all contents of `build/out/AX630C_emmc_arm64_k419_sipeed_maixcam2/objs/ubuntu_rootfs` to partition 2.
8. Insert the SD card, hold down boot, then press rst. The bootrom will attempt to load boot.bin from the FAT32 partition of the SD card and start it. boot.bin will then launch atf.img and uboot.bin.uboot.bin will load kernel.img and dtb.img, and boot into the Linux system. The U-Boot startup logic can be found in`boot/uboot/u-boot-2020.04/cmd/axera/sd_boot/sd_boot.c`.

## Modifications and Notes

**All code changes are marked with:**
```makefile
### SIPEED EDIT ###
modified content
### SIPEED EDIT END ###
```
Pay attention to spaces. You can locate all changes by performing a global search.

**Modifications related to CMM and SYSTEM memory allocation:**
1. Default configuration: modify the `BOARD_xxx_OS_MEM_SIZE` value in `partition.mak` / `partition_ab.mak`.
2. After flashing: modify the `maix_memory_cmm size` in `/boot/configs`.
This value will be automatically read by U-Boot and during driver loading at boot time.

