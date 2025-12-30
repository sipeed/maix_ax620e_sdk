#!/usr/bin/env python3
# -*- coding:utf-8 -*-

import getopt
import sys
import os
import shutil
import xml.etree.ElementTree as ET
import hashlib
from ctypes import *

'''
- V1.1 2021-07-07 parse <ResetAddr> tag
- V1.0 2021-05-13 pac first release
'''
SCRIPT_VERSION = '1.1'

XML_NODE_ImgList = 'ImgList'
XML_NODE_Img = 'Img'
XML_ATTR_flag = 'flag'
XML_ATTR_name = 'name'
XML_ATTR_select = 'select'
XML_ATTR_version = 'version'
XML_NODE_File = 'File'
XML_NODE_ID = 'ID'
XML_NODE_Type = 'Type'
XML_NODE_Block = 'Block'
XML_NODE_Base = 'Base'
XML_NODE_Size = 'Size'
XML_NODE_id = 'id'
XML_NODE_Project = 'Project'
XML_NODE_Auth = 'Auth'
XML_ATTR_algo = 'algo'
XML_ATTR_id = 'id'
XML_NODE_ResetAddr = 'ResetAddr'

MAX_BLOCK_NUM = 1
PAC_MAGIC = 0x5C6D8E9F
PAC_VERSION = 1
BLOCK_SIZE = 10 * 1024 * 1024

crc16_table = [
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
        0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
        0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
        0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
        0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
        0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
        0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
        0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
        0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
        0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
        0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
        0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
        0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
        0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
        0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
        0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
        0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
        0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
        0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
        0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
        0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
        0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
        0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
        0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
        0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
        0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
        0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
        0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
        0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
        0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
        0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
        0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
]


class PAC_HEAD_T(Structure):
    _pack_ = 8
    _fields_ = [
        ('nMagic', c_uint32),
        ('nPacVer', c_uint32),
        ('u64PacSize', c_uint64),
        ('szProdName', c_char * 32),
        ('szProdVer', c_char * 32),
        ('nFileOffset', c_uint32),  # offset for PAC_FILE_T
        ('nFileCount', c_uint32),
        ('nAuth', c_uint32),  # 0: no md5  1: md5  2: crc16
        ('crc16', c_uint32),
        ('md5', c_char * 32),
        ('nResetAddr', c_uint32)
    ]


class BLOCK_T(Structure):
    _fields_ = [
        ('u64Base', c_uint64),
        ('u64Size', c_uint64),
        ('szPartID', c_char * 72)
    ]


class PAC_FILE_T(Structure):
    _pack_= 8
    _fields_ = [
        ('szID', c_char * 32),
        ('szType', c_char * 32),
        ('szFile', c_char * 256),
        ('u64CodeOffset', c_uint64),
        ('u64CodeSize', c_uint64),
        ('tBlock', BLOCK_T),
        ('nFlag', c_uint32),
        ('nSelect', c_uint32),
        ('reserved', c_uint32 * 8)
    ]


def print_usage():
    print('usage:')
    print('python3 make_pac.py [-h] -p -d file1 file2 file3 ...')
    print('     [Mandatory arguments]:')
    print('         -p, --pac=pac file updated          set pac file updated')
    print('         -d, --dest=destnation directory     set =destnation directory of RAMDISK, default: isp_modules')
    print('         file                                input the modules files separated by blank space')
    print('     [Optional arguments]:')
    print('         -h, --help                          usage help')
    print('         -V, --verbose                       display more log information')


def get_fname(path):
    return os.path.basename(path)


def get_abspath(path):
    return os.path.normpath(os.path.abspath(path))


def check_need_input_file(flag):
    if flag and ((int(flag) & 0x01) == 0x01):  # 0x01: need a file
        return True
    else:
        return False


def copy(src, dst):
    # print('\t\t{}  -->  {}'.format(src, dst))
    if os.path.isfile(src):
        shutil.copy(src, dst)
    if os.path.isdir(src):
        shutil.copytree(src, dst)


def str2int(str_val):
    if str_val.lower().find('0x') < 0:
        return int(str_val, 10)
    else:
        return int(str_val, 16)


def read_block_from_file(file, block_size, offset=0):
    with open(file, 'rb') as f:
        f.seek(offset, 0)
        while True:
            block = f.read(block_size)
            if block:
                yield block
            else:
                return


def calc_auth_value(file, offset=0, is_md5=True):
    print('\tcalculating %s  -->  file: %s, offset: %d' % (('md5' if is_md5 else 'crc16'), get_fname(file), offset))
    if is_md5:
        m = hashlib.md5()
    else:
        crc16 = 0

    size = 0
    for block in read_block_from_file(file, BLOCK_SIZE, offset):
        if is_md5:
            m.update(block)
        else:
            for data in block:
                crc16 = (crc16 >> 8) ^ (crc16_table[(crc16 ^ data) & 0xFF])

        size += len(block)

    if is_md5:
        md5 = m.hexdigest()
        print(f'\t\tMD5 = {md5}, size = {size}')
        return md5
    else:
        print(f'\t\tcrc16 = {crc16}, size = {size}')
        return crc16


def rm(paths):
    l = []
    if isinstance(paths, list):
        l = paths
    else:
        l.append(paths)

    for f in l:
        if os.path.exists(f):
            if os.path.isfile(f):
                os.remove(f)
            elif os.path.isdir(f):
                shutil.rmtree(f)

def parse_args():
    args = {'pac': None, 'verbose': '', 'dest': 'isp_modules', 'imgs': []}

    opts, args['imgs'] = getopt.getopt(sys.argv[1:], 'hVp:d:',
                                       ['help', 'verbose', 'pac=', 'dest='])
    if len(opts) == 0:
        print_usage()
        exit(0)

    for opt_name, opt_value in opts:
        if opt_name in ('-h', '--help'):
            print_usage()
            exit(0)

        if opt_name in ('-V', '--verbose'):
            args['verbose'] = '-V'

        if opt_name in ('-p', '--pac'):
            path = get_abspath(opt_value)
            if not os.path.exists(path):
                print('%s not exist!' % opt_value)
                exit(1)
            args['pac'] = path

        if opt_name in ('-d', '--dest'):
            args['dest'] = opt_value

    # check args
    if not args['pac']:
        print('Input the .pac file by [-p] or [--pac]')
        exit(1)

    args['imgs'] = list(map(get_abspath, args['imgs']))
    for img in args['imgs']:
        if not os.path.isfile(img):
            if not os.path.exists(img):
              print('<{}> not exist'.format(img))
              exit(1)
        else:
          if os.path.getsize(img) == 0:
            print('<{}> 0 size'.format(img))
            exit(1)

    return args

def update_pac(args, ramdisk_file):
    pac_file = args['pac']

    hd = PAC_HEAD_T()
    exsist_ramdisk = False

    with open(pac_file, "r+b") as f:
          size = sizeof(PAC_HEAD_T)
          bin = f.read(size)
          memmove(pointer(hd), cast(bin, c_char_p), size)
          f.seek(hd.nFileOffset, 0)
          for i in range(hd.nFileCount):
            pf = PAC_FILE_T()
            size = sizeof(PAC_FILE_T)
            bin = f.read(size)
            memmove(pointer(pf), cast(bin, c_char_p), size)
            if (pf.szID.decode('utf-8') == 'RAMDISK'):
              if pf.u64CodeSize != 0:
                exsist_ramdisk = True
                f.seek(pf.u64CodeOffset, 0)
                bin = f.read(pf.u64CodeSize)
                with open(ramdisk_file, "wb") as wrf:
                      #Write to ramdisk.img
                      wrf.write(bin)

                #Update RAMDISK
                for img in args['imgs']:
                  #file
                  shell_cmd='../../build/ax_mkimage.sh -I ' + ramdisk_file+' -K -s ' + img + ' -d ' + args['dest'] + ' ' +  args['verbose']
                  os.system(shell_cmd)

                with open(ramdisk_file, "rb") as rrf:
                      #read from ramdisk.img
                      rrf.seek(0, 0)
                      bin = rrf.read(pf.u64CodeSize)
                      #Write RAMDISK back to Pac
                      f.seek(pf.u64CodeOffset, 0)
                      f.write(bin)
              break

    if exsist_ramdisk != True:
      print("No Exist RAMDISK in", pac_file)
      sys.exit()

if __name__ == "__main__":
    print('==== Script Version: %s ====' % SCRIPT_VERSION)
    args = parse_args()

    try:
        pac_file = args['pac']
        out_dir, _ = os.path.split(pac_file)
        tmp_dir = os.path.join(out_dir, "tmp123")
        rm(tmp_dir)
        os.mkdir(tmp_dir)

        # update .pac
        ramdisk_file = os.path.join(tmp_dir, "ramdisk.img")
        print('updating pac %s ...' % pac_file)
        update_pac(args, ramdisk_file)
        rm(tmp_dir)

        # -------------- verify -------------------
        # hd = PAC_HEAD_T()
        # fl = []
        # with open(out_file, "rb") as f:
        #     size = sizeof(PAC_HEAD_T)
        #     bin = f.read(size)
        #     memmove(pointer(hd), cast(bin, c_char_p), size)
        #     f.seek(hd.nFileOffset, 0)
        #     for i in range(hd.nFileCount):
        #         size = sizeof(PAC_FILE_T)
        #         bin = f.read(size)
        #         pf = PAC_FILE_T()
        #         memmove(pointer(pf), cast(bin, c_char_p), size)
        #         fl.append(pf)
        #
        #     out_dir = os.path.join(out_dir, 'unpack')
        #     rm(out_dir)
        #     os.mkdir(out_dir)
        #     for _p in fl:
        #         f.seek(_p.u64CodeOffset, 0)
        #         data = f.read(_p.u64CodeSize)
        #         file = os.path.join(out_dir, _p.szID.decode('utf-8'))
        #         if _p.u64CodeSize > 0:
        #             with open(file, "wb") as wf:
        #                 wf.write(data)

    except Exception as e:
        print('!!!! Make pac error: %s, line: %d' % (e, e.__traceback__.tb_lineno))
    else:
        print('\n----- SUCCESS -----')
        print(f'pac updated: {pac_file}')
