#*************************************************************************
#     File Name:    boot_AX620_sign.py
#     Author:       wangxiaoxu
#     Mail:         wangxiaoxu@axera-tech.com
#     Function:     make boot.bin file. file include uImage, etc....
#*************************************************************************

import os
import sys
import rsa
import ctypes
import traceback
import struct
import hashlib

def print_usage():
    print('\nboot_AX620_sign.py -i <input_file> -o <output file>')
    print('  -i   :  input file path')
    print('  -o   :  output boot image file with header + inputfile\n')

'''
#define AXERA!
#define BOOT_MAGIC_SIZE 6
struct boot_header{
    unsigned char magic[BOOT_MAGIC_SIZE];        //AXERA image file magic
    unsigned int img_size;                       //output file actual size
    unsigned int input_size;                     //input file size
    unsigned int input_addr;                     //input start addr in image file
    unsigned char id[32];                        //image file HASH
};
'''

HEADER_SIZE = 64
class output_header(ctypes.Structure):
    _pack_ = 4
    _fields_ = [
        ("magic", ctypes.c_uint8 * 6),
        ("img_size", ctypes.c_uint),
        ("data_size", ctypes.c_uint),
        ("data_addr", ctypes.c_uint),
        ("id", ctypes.c_uint8*32)
    ]

def copy_data_by_bytes(src, dst, length, dst_offset= 0, revers=False):
    if sys.version > '3':
        for i in range(length):
            if revers:
                dst[dst_offset + i] = ctypes.c_uint8(src[length - i - 1])
            else:
                dst[dst_offset + i] = ctypes.c_uint8(src[i])
    else:
        for i in range(length):
            if revers:
                dst[dst_offset + i] = ctypes.c_uint8(ord(src[length - i - 1]))
            else:
                dst[dst_offset + i] = ctypes.c_uint8(ord(src[i]))

def dump_header(hdr):
    print('file size    :' + format(hdr.img_size))
    print('input file size  :' + format(hdr.data_size))
    print('input file addr  :' + format(hdr.data_addr))
    ## print('id           :' + format(hdr.id))

def make_image(input_file, output_file):

    # load input file data
    input_size = os.path.getsize(input_file)

    try:
        with open(input_file, 'rb') as f:
            input_data = f.read()
    except:
        print('[error] load input file failed\n')
        print(sys.exc_info())
        print(traceback.format_exc())
        return False

    # input file data size need to 4096 bytes align
    data_addr = HEADER_SIZE
    data_size = input_size + 4096 - input_size % 4096

    # Compute data SHA256
    compute_data = (ctypes.c_uint8 * (data_size))(0)
    copy_data_by_bytes(input_data, compute_data, input_size, 0)

    # debug sha256 value
    # data_sha256 = hashlib.sha256(compute_data).hexdigest()
    # print(data_sha256)

    data_sha256 = hashlib.sha256(compute_data).digest()
    output_size = data_size + HEADER_SIZE
    hdr = output_header()

    # AXERA!
    hdr.magic[0] = 0x41
    hdr.magic[1] = 0x58
    hdr.magic[2] = 0x45
    hdr.magic[3] = 0x52
    hdr.magic[4] = 0x41
    hdr.magic[5] = 0x21
    hdr.img_size = output_size
    hdr.data_size = data_size
    hdr.data_addr = data_addr
    copy_data_by_bytes(data_sha256, hdr.id, 32, 0)

    # debug boot.bin hdr value
    # dump_header(hdr)

    output_data = (ctypes.c_uint8 * (output_size))(0)
    p = ctypes.cast(ctypes.pointer(hdr), ctypes.POINTER(ctypes.c_uint8 * HEADER_SIZE))
    for i in range(HEADER_SIZE):
        output_data[i] = p.contents[i]

    copy_data_by_bytes(input_data, output_data, input_size, data_addr)

    # save data to out file
    try:
        with open(output_file, 'wb') as f:
            f.write(output_data)
    except:
        print('[error] create boot image file failed\n')
        print(sys.exc_info())
        print(traceback.format_exc())
        return False
    return True

if __name__ == '__main__':
    input_file = None
    output_file = None

    if len(sys.argv) != 5:
        print('[error] param is invalid\n')
        print_usage()
        exit(-1)

    for i in range(1, len(sys.argv)):
        if sys.argv[i] == '-i':
            input_file = sys.argv[i+1]
        elif sys.argv[i] == '-o':
            output_file = sys.argv[i + 1]

    if input_file is None or output_file is None:
        print('[error] param is invalid\n')
        print_usage()
        exit(-1)

    if (not os.path.exists(input_file)):
        print('[error] input file does not exist\n')
        exit(-1)

    ret = make_image(input_file, output_file)
    if ret:
        print('\033[1;32mmake image complete')
        print('\033[0m')
