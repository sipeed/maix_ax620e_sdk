MAIX AX620E 系统构建
=====

## 说明

本仓库存放的是AX620E平台的系统源码, 主要用于构建`MaixCam2`, `KVM Pro`等基础系统镜像。

本仓库还依赖了子仓库[maix_ax620e_sdk_msp](https://github.com/sipeed/maix_ax620e_sdk_msp)和[maix_ax620e_sdk_kernel](https://github.com/sipeed/maix_ax620e_sdk_kernel), 确保构造镜像前通过`git submodule`命令拉取了依赖的子仓库。

> 注意：目前只测试过在`Ubuntu22.04`编译通过

## 获取代码

```shell
git clone https://github.com/sipeed/maix_ax620e_sdk
git submodule update --init --recursive
```

## 编译

* 下载工具链

```shell
# 下载和解压
wget https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
sudo tar -xvf gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz -C /opt

# 配置环境变量
echo 'export PATH=/opt/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin:${PATH}' >> ~/.bashrc
source ~/.bashrc
```

> 工具链备用下载方法：在[这里](https://developer.arm.com/downloads/-/gnu-a)下载 gcc9.2-2019.12 工具链

* 测试工具链
```shell
aarch64-none-linux-gnu-gcc --version
which aarch64-none-linux-gnu-gcc
```

* 安装必要的软件
```shell
sudo dpkg-reconfigure dash # 然后选择No

sudo apt install make libc6:i386 lib32stdc++6 zlib1g-dev libncurses5-dev ncurses-term libncursesw5-dev g++ u-boot-tools texinfo texlive gawk libssl-dev openssl bc bison flex gcc libgcc1 gdb build-essential lib32z1 u-boot-tools device-tree-compiler qemu qemu-user-static fusefat

sudo python3 -m pip install --upgrade pip
sudo pip3 install lxml
```

* 构建 ubuntu rootfs

```shell
# 只要rootfs源码没有修改， 就只需要构建一次
cd rootfs/arm64/glibc/ubuntu_rootfs/

# For MaixCAM2
./mk_ubuntu_base_sipeed.sh .

# For KVM Pro
./mk_ubuntu_kvm_sipeed.sh .
```

* 编译系统
> 注意：如果第一次编译时出现缺少符号的错误， 请尝试一下重新编译， 这可能有些依赖是在编译过程中生成的。 或者编译时添加`-j1`选项来避免多线程编译。

```shell
# export use_ubuntu_rootfs=no  # 对于 MaixCAM2 和 KVM Pro 工程, 默认是 yes

cd build

# MaixCAM2
make p=AX630C_emmc_arm64_k419_sipeed_maixcam2 clean all install axp -j8

# KVM Pro
make p=AX630C_emmc_arm64_k419_sipeed_nanokvm clean all install axp -j8
```

* 在 `build/out`目录下就会有 `axp` 格式的包了。

## 定制rootfs

开发时如果需要增加/删除rootfs的包， 可以修改对应平台的打包脚本

打包脚本的路径：

| 平台     | 打包脚本路径                                              |
| -------- | --------------------------------------------------------- |
| MaixCAM2 | rootfs/arm64/glibc/ubuntu_rootfs/mk_ubuntu_base_sipeed.sh |
| KVM Pro  | rootfs/arm64/glibc/ubuntu_rootfs/mk_ubuntu_kvm_sipeed.sh  |

修改方法参考：

- 如果需要安装`i2c-tools`工具， 则在脚本的`apt install`代码附进添加`apt install -y i2c-tools`
- 如果需要安装`Python`包`pyaudio`，则在脚本中`pip install`代码附加添加`pip install pyaudio`, 或者在`rootfs/arm64/glibc/ubuntu_rootfs/requirements.txt`文件中添加`toml`, 对于`KVM Pro`的配置文件是`kvm_requirements.txt`

阅读打包脚本的代码， 可以通过修改打包脚本更自由裁剪`rootfs`

> 注意： 如果修改rootfs后增加了过多文件，则需要修改 `build/projects/xxxx/partition_ab.mak` 中的 `ROOTFS_PARTITION_SIZE` 保证足够大，否则出现打包打错

## 基于rootfs定制文件

这一步是基于已经打包后的`rootfs`进行二次修改， 用来减少每次修改`rootfs`都需要重新打包`rootfs`的时间。

| 平台     | 工程目录                                              |
| -------- | ----------------------------------------------------- |
| MaixCam2 | build/projects/AX630C_emmc_arm64_k419_sipeed_maixcam2 |
| KVM Pro  | build/projects/AX630C_emmc_arm64_k419_sipeed_nanokvm  |

在工程目录下通过修改`bootfs.filelist`来指定更新`/boot`的内容， 修改`rootfs.filelist`来指定更新到`/`目录下的内容。

例如默认已经写好了将`rootfs/opt`中的目录拷贝到`/opt`目录下，因此如果想在`/opt`目录添加新的文件， 则只需要将新文件添加到`rootfs/opt`目录即可。

## 烧录和启动

下载烧录工具[AXDL](https://dl.sipeed.com/fileList/MaixCAM/MaixCAM2/Software/Tools/AXDL_V1.24.22.1.7z)
下载[AXDL驱动](https://dl.sipeed.com/fileList/MaixCAM/MaixCAM2/Software/Tools/Driver_V1.20.46.1.7z), 解压后运行DriverSetup.exe安装驱动.

编译默认生成`axp`文件，一般输出路径为`build/out/xxx.axp`

* axp 可以直接用官方软件 `AXDL` 烧录 EMMC（永远可以烧录）。
* 通过 USB 虚拟 U盘烧录 EMMC（在 uboot  kernel 分区没有损坏的情况下可以烧录）：
  * 转换镜像，使用`pip install axp-tools`（源码在`MaixPy/tools/os/maixcam2`下）安装工具，然后`axp2img -i xxxx.axp`生成 `xxxx.img.xz`镜像。
  * 按住boot按键复位（也可以在boot分区创建 rec 文件重启进入recovery 模式）
  * 保持boot按下，直到电脑出现U盘设备（文件管理器看不到，设备管理器会有）（实际上是Uboot检测，所以按到uboot启动就好了）
  * 松开boot
  * 使用 etcher 或者 dd 命令将 `.img.xz` 写入 虚拟U盘。

## 制作SD启动卡

1. 插入SD卡
2. 格式化SD卡，并创建MBR格式分区表(一定要是MBR格式分区表)
3. 创建分区1为`fat32`格式, 分配128M空间
4. 创建分区2为`ext4`格式, 分配所有剩余空间, 注意分区顺序一定不能变
5. 拷贝`build/out/AX630C_emmc_arm64_k419_sipeed_maixcam2/images/sd_boot_pack`的所有内容到分区1
6. 拷贝`build/out/AX630C_emmc_arm64_k419_sipeed_maixcam2/images/spl_AX630C_emmc_arm64_k419_sipeed_maixcam2_sd_signed.bin`到分区1, 并重命名为`boot.bin`
7. 拷贝路径`build/out/AX630C_emmc_arm64_k419_sipeed_maixcam2/objs/ubuntu_rootfs`的所有内容到分区2
8. 插入 sdcard，并按住 boot，点按 rst。bootrom 会尝试加载 sdcard fat32 分区内 boot.bin 并启动，而 boot.bin 会启动 atf.img 和 uboot.bin。uboot.bin 会加载 kernel.img dtb.img 并启动进入 linux 系统。uboot 启动部分可见 `boot/uboot/u-boot-2020.04/cmd/axera/sd_boot/sd_boot.c`。

## 改动点和注意点

**代码改动都添加**
```makefile
### SIPEED EDIT ###
改动内容
### SIPEED EDIT END ###
```
注意空格，全局搜索即可找到。

**分配CMM和SYSTEM内存时修改点**
1. 默认值，修改partition.mak/partition_ab.mak中BOARD_xxx_OS_MEM_SIZE的大小
2. 烧录之后修改： 修改 /boot/configs 中的 `maix_memory_cmm` 大小即可，会在 uboot和开机加载驱动时自动读取。

