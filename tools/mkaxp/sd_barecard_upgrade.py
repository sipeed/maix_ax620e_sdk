import os
import sys
import traceback
import struct
import shutil


def print_usage():
    print('\nsd_barecard_upgrade.py -type emmc -path <path>')
    print('-type <name>: emmc or nand\n')
    print('-path <path>: eg: ./out/AX620_demo\n')
    print('-xml <xml>:   path/AX620X_XXX.xml\n')

def pack_sd_update_emmc(path, xmlpath):
    f = os.listdir(path)
    print(f)
    os.chdir(path)
    if os.path.exists('sd_update_pack'):
        os.system("rm sd_update_pack/*")
    else:
        os.mkdir('sd_update_pack')
    for file in f:
        try:
            if(file == 'spl_AX620_demo_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'spl.bin')
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'boot.bin')
            if(file == 'u-boot_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'uboot.bin')
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'u-boot.bin')
            if(file == 'boot.img'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'kernel.img')
            if(file == 'AX620_demo.dtb'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'dtb.img')
            if(file == 'rootfs_sparse.ext4'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'rootfs.img')
            if(file == 'param_sparse.ext4'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'param.img')
            if(file == 'opt_sparse.ext4'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'opt.img')
            if(file == 'soc_sparse.ext4'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'soc.img')
            os.system("cp {} ./sd_update_pack/AX620X.xml".format(xmlpath))

        except:
            continue

def pack_sd_update_nand(path, xmlpath):
    f = os.listdir(path)
    print(f)
    os.chdir(path)
    if os.path.exists('sd_update_pack'):
        os.system("rm sd_update_pack/*")
    else:
        os.mkdir('sd_update_pack')
    for file in f:
        try:
            if(file == 'spl_AX620_nand_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'spl.bin')
            if(file == 'u-boot_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'uboot.bin')
            if(file == 'uImage'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'kernel.img')
            if(file == 'AX620_nand.dtb'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'kernel-dtb.img')
            if(file == 'rootfs_soc_opt.ubi'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'rootfs.img')
            if(file == 'u-boot_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_boot_pack/' + 'u-boot.bin')
            if(file == 'spl_AX620_nand_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'boot.bin')
            os.system("cp {} ./sd_update_pack/AX620X.xml".format(xmlpath))

        except:
            continue

def main(type, path, xmlpath):
    if(type == 'emmc'):
        pack_sd_update_emmc(path, xmlpath)
    elif(type == 'nand'):
        pack_sd_update_nand(path, xmlpath)
    else:
        print_usage()

if __name__ == '__main__':
    if len(sys.argv) < 6:
        print('[error] param is invalid\n')
        print_usage()
        exit(-1)

    for i in range(1, len(sys.argv)):
        if sys.argv[i] == '-type':
            type = sys.argv[i+1]
        if sys.argv[i] == '-path':
            path = sys.argv[i+1]
        if sys.argv[i] == '-xml':
            xmlpath = sys.argv[i+1]

    main(type, path, xmlpath)
