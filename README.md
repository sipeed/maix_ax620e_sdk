Sipeed MaixCAM2 系统构建
=====

## 代码版本管理和 SDK 更新

* 本仓库提交到内部git。
* 尽量少地修改爱芯的代码，方便之后更新。能制作副本的就制作副本，比如 新建一个 project/kernel配置等，修改需要在本文档记录。
* 如果要更新爱芯的 SDK：
  * 先按照爱芯的文档把 patch 打完构建 AX630C_emmc_arm64_k419 工程通过。
  * 然后复制文件覆盖本 git。
  * 仔细！！ 查看每一个文件更改，提交改动，这里要十分注意不要把我们的改动（看下面的改动点）覆盖丢失了。
  * 更新前后自己阅读下面的改动点，看是不是破坏了我们的使用方式。


## 编译

* 安装工具链

到 https://developer.arm.com/downloads/-/gnu-a 下载 gcc9.2-2019.12 工具链并接到到某个位置。
然后设置环境变量`PATH`追加`工具链路径/bin`目录，可以在`.bashrc/.zshrc`中设置。
然后测试
```shell
aarch64-none-linux-gnu-gcc --version
which aarch64-none-linux-gnu-gcc
```

* 安装必要的软件
```shell
# 按照爱芯的文档安装(make, ncurses-term, u-boot-tools, g++等)
# ...

# 需要再安装qemu
sudo apt install qemu qemu-user-static

# 需要再安装fusefat
sudo apt install fusefat
```

另外还有一些必要的包
```shell
sudo apt install build-essential make libc6:i386 lib32stdc++6 zlib1g-dev libncurses5-dev ncurses-term libncursesw5-dev g++ texinfo texlive gawk libssl-dev openssl bc bison flex gcc libgcc1 gdb lib32z1 u-boot-tools device-tree-compiler
```
如果遇到了问题也可以看爱芯官方SDK文档 `00 - AX SDK 使用说明.pdf`。


* 构建 ubuntu rootfs
```shell
cd rootfs/arm64/glibc/ubuntu_rootfs/
./mk_ubuntu_base_sipeed.sh .

# nanokvm rootfs
./mk_ubuntu_kvm_sipeed.sh .
```

* 编译系统
```shell
cd build
# export use_ubuntu_rootfs=no  # 对于 AX630C_emmc_arm64_k419_sipeed_maixcam2 工程默认是 yes
# 需要手动指定 export use_ubuntu_rootfs=yes，因为子层 build/projects/AX630C_emmc_arm64_k419_sipeed_maixcam2/ 下的 Makefile 定义的 use_ubuntu_rootfs := yes 并不能向上传递给 build/ 下的 Makefile 的 axp 目标用来传递给 axp_make.sh 的 -u 参数，因此一定需要手动指定。
make p=AX630C_emmc_arm64_k419_sipeed_maixcam2 clean all install axp

# 多线程编译
make p=AX630C_emmc_arm64_k419_sipeed_maixcam2 clean all install axp -j`nproc`

# nanokvm 编译
make p=AX630C_emmc_arm64_k419_sipeed_nanokvm clean all install axp -j`nproc`
```

* 在 `build/out`目录下就会有 `axp` 格式的包了。

## 关于内置文件打包

默认系统**必要的文件**和脚本内置到系统了，在 `build/projects/xxxx/bootfs` 和 `rootfs` 目录下。
增删文件夹需要修改 `bootfs.filelist`和 `rootfs.filelist` 文件。
需要修改 `build/projects/xxxx/partition_ab.mak` 中的 `ROOTFS_PARTITION_SIZE` 保证足够大，不知道多大没关系，不够大编译会报错的，多试两次（有时间也可以优化脚本自动计算）。

这是**基础系统**，稳定后一般不需要经常修改和编译，对于 MaixCAM2，需要增加 MaixPy，内置应用，模型等文件，会经常升级，可以在无需重新编译的情况下增删内置文件：
用 `MaixPy/tools/os/maixcam2/gen_os.sh` 脚本传参增加和删除一些文件重新打包。
对于 MaixCAM2 内置文件，在[https://git.corp.sipeed.com/sipeed_software/maix/maixcam2_builtin_files](https://git.corp.sipeed.com/sipeed_software/maix/maixcam2_builtin_files)，`gen_os.sh`脚本传参时用这个工程里面提供的文件夹就好。


## 制作SD启动卡

1. 插入SD卡
2. 格式化SD卡，并创建MBR格式分区表(一定要是MBR格式分区表)
3. 创建分区1为`fat32`格式, 分配128M空间
4. 创建分区2为`ext4`格式, 分配所有剩余空间, 注意分区顺序一定不能变
5. 拷贝`build/out/AX630C_emmc_arm64_k419_sipeed_maixcam2/images/sd_boot_pack`的所有内容到分区1
6. 拷贝`build/out/AX630C_emmc_arm64_k419_sipeed_maixcam2/images/spl_AX630C_emmc_arm64_k419_sipeed_maixcam2_sd_signed.bin`到分区1, 并重命名为`boot.bin`
7. 拷贝路径`build/out/AX630C_emmc_arm64_k419_sipeed_maixcam2/objs/ubuntu_rootfs`的所有内容到分区2
8. 插入SD卡即可启动

## 烧录和启动

* axp 可以直接用官方软件 AX-DL 烧录 EMMC（永远可以烧录）。
* 通过 USB 虚拟 U盘烧录 EMMC（在 uboot  kernel 分区没有损坏的情况下可以烧录）：
  * 转换镜像，使用`pip install axp-tools`（源码在`MaixPy/tools/os/maixcam2`下）安装工具，然后`axp2img -i xxxx.axp`生成 `xxxx.img.xz`镜像。
  * 按住boot按键复位（也可以在boot分区创建 rec 文件重启进入recovery 模式）
  * 保持boot按下，直到电脑出现U盘设备（文件管理器看不到，设备管理器会有）（实际上是Uboot检测，所以按到uboot启动就好了）
  * 松开boot
  * 使用 etcher 或者 dd 命令将 `.img.xz` 写入 虚拟U盘。
* 从 SD 卡启动：
  * 格式化 SD 卡为 FAT32 分区和 EXT4 分区。
  * 在 `build/out/*/images` 下有 `sd_boot_pack` 文件夹内 `uboot.bin kernel.img dtb.img atf.img` 拷贝到 sdcard fat32 分区下。
  * 拷贝 images 下 `spl_AX630C_emmc_arm64_k419_sipeed_maixcam2_sd_signed.bin` 并重命名为 `boot.bin` 到 sdcard fat32 分区下。
  * 挂载 rootfs.ext4（`mkdir tmp2/rootfs && mount -t ext4 rootfs.ext4 tmp2/rootfs`） 到电脑，然后拷贝所有内容到 sdcard 第二分区 ext4 下。
  * 插入 sdcard，并按住 boot，点按 rst。bootrom 会尝试加载 sdcard fat32 分区内 boot.bin 并启动，而 boot.bin 会启动 atf.img 和 uboot.bin。uboot.bin 会加载 kernel.img dtb.img 并启动进入 linux 系统。uboot 启动部分可见 `boot/uboot/u-boot-2020.04/cmd/axera/sd_boot/sd_boot.c`。
* SD 卡烧录 EMMC：TODO：


## 改动点和注意点

**代码改动都添加**
```makefile
### SIPEED EDIT ###
改动内容
### SIPEED EDIT END ###
```
注意空格，全局搜索即可找到。

* 新增 project p=AX630C_emmc_arm64_k419_sipeed_maixcam2， 同时新建 kernel kconfig 默认defconfig 和 dts文件，均和工程名保持一致不破坏爱芯构建系统。
* rootfs/arm64/glibc/ubuntu_rootfs/mk_ubuntu_base.sh 复制定制到 mk_ubuntu_base_sipeed.sh。
* 新增 build/tools/pinmux/AX630C_sipeed_maixcam2_pinmux.xlsm, 用来生成 pinmux.h。

**分配CMM和SYSTEM内存时修改点**
1. 默认值，修改partition.mak/partition_ab.mak中BOARD_xxx_OS_MEM_SIZE的大小
2. 烧录之后修改： 修改 /boot/configs 中的 `maix_memory_cmm` 大小即可，会在 uboot和开机加载驱动时自动读取。

