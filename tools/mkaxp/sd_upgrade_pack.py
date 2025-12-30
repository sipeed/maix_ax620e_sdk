import os
import sys
import traceback
import struct
import shutil


def print_usage():
    print('\nsdk_update_pack.py -type <name> -path <path>')
    print('-type <name>: sd_update or sd_boot\n')
    print('-path <path>: eg: ./out/AX650_emmc/images\n')
    print('-xml <xml>:   path/AX650X_XXX.xml\n')
    print('-optee <optee>: eg: true or false\n')

def pack_sd_update(path, xmlpath, optee=False):
    f = os.listdir(path)
    print(f)
    os.chdir(path)
    if os.path.exists('sd_update_pack'):
        os.system("rm sd_update_pack/*")
    else:
        os.mkdir('sd_update_pack')
### SIPEED EDIT ###
    if "ubuntu_rootfs_sparse" in f and "rootfs_sparse" in f:
        f.remove("rootfs_sparse.ext4")
### SIPEED EDIT END ###
    for file in f:
        try:
            if(file == 'u-boot_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'uboot.bin')
            if(file == 'uboot_bk.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'uboot_bk.img')
            if(file == 'boot_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'kernel.img')
            if(file == 'atf_bl31_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'atf.img')
            if(file == 'AX650_emmc_signed.dtb'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'dtb.img')
            if(file == 'AX620E_emmc_signed.dtb'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'dtb.img')
### SIPEED EDIT ###
            if(file in ['ubuntu_rootfs_sparse.ext4', "rootfs_sparse.ext4"]):
### SIPEED EDIT END ###
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'rootfs.img')
            if(file == 'param_sparse.ext4'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'param.img')
            if(file == 'opt_sparse.ext4'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'opt.img')
            if(file == 'soc_sparse.ext4'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'soc.img')
            if(file == 'spl_AX620E_emmc_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'spl.bin')
            if(optee and (file == 'optee_signed.bin')):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_update_pack/' + 'optee.img')
            os.system("cp {} ./sd_update_pack".format(xmlpath))
        except:
            continue

def pack_sd_boot(path, optee=False):
    f = os.listdir(path)
    print(f)
    os.chdir(path)
    if os.path.exists('sd_boot_pack'):
        os.system("rm sd_boot_pack/*")
    else:
        os.mkdir('sd_boot_pack')
    for file in f:
        try:
            if(file == 'u-boot_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_boot_pack/' + 'boot.bin')
            if(file == 'boot_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_boot_pack/' + 'kernel.img')
            if(file == 'AX650_emmc_signed.dtb'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_boot_pack/' + 'dtb.img')
            if(file == 'AX620E_emmc_signed.dtb'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_boot_pack/' + 'dtb.img')
            if(file == 'atf_bl31_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_boot_pack/' + 'atf.img')
            if(file == 'spl_AX620E_emmc_signed.bin'):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_boot_pack/' + 'spl.bin')
            if(optee and (file == 'optee_signed.bin')):
                shutil.copyfile(os.path.realpath(file), os.path.dirname(os.path.realpath(file)) + '/sd_boot_pack/' + 'optee.img')
        except:
            continue


def main(type, path, xmlpath, optee=False):
    if(type == 'sd_update'):
        pack_sd_update(path, xmlpath, optee)
    elif(type == 'sd_boot'):
        pack_sd_boot(path, optee)
    else:
        print_usage()

if __name__ == '__main__':
    if len(sys.argv) < 6:
        print('[error] param is invalid\n')
        print_usage()
        exit(-1)

    for i in range(1, len(sys.argv)):
        if sys.argv[i] == '-type':
            name = sys.argv[i+1]
        if sys.argv[i] == '-path':
            path = sys.argv[i+1]
        if sys.argv[i] == '-xml':
            xmlpath = sys.argv[i+1]
        if sys.argv[i] == '-optee':
            optee = sys.argv[i+1].lower()=="true"

    main(name, path, xmlpath, optee)
