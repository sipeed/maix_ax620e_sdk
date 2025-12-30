#*************************************************************************
#     File Name:    boot_AX650_sign.py
#     Author:       wangjiwei
#     Mail:         wangjiwei@axera-tech.com
#     Function:     make boot.img file. file include uImage, etc ....
#*************************************************************************

import os
import sys
import rsa
import ctypes
import traceback
import struct
import hashlib

def print_usage():
    print('\nmake_boot_AX650_sign.py -i <input_file> -pub <public_key_file(*.pem)> -prv <private_key_file(*.pem)> -o <out_file>')
    print('  -dtb :  input original dtb file path')
    print('  -kernel :  input original kernel file path')
    print('  -pub :  input rsa public key file (*.pem) path')
    print('  -prv :  input rsa private key file (*.pem) path')
    print('  -o   :  output boot image file with header + inputfile\n')
    print('  -cap :  input capability of header field\n')

'''
#define BOOT_MAGIC_SIZE 6

struct rsa_key {
    uint32_t key_n_header;
    uint32_t rsa_key_n[96];
    uint32_t key_e_header;
    uint32_t rsa_key_e;
};

struct image_signature {
    uint32_t sig_header;
    uint32_t signature[96];
};

struct multi_image_header {
    uint8_t     magic[BOOT_MAGIC_SIZE]; // AXERA image file magic
    uint32_t    image_size;
    uint8_t     partition_num;
    uint32_t    partition_size[32];
    uint32_t    partition_offset[32];
    uint8_t     hash_digest[32];
};

image header struct:
struct image_header {
	uint32_t                    check_sum;         /* header check sum */
	uint32_t                    capbility;
	struct multi_image_header   mul_img_hdr;
	struct rsa_key	            key;
	struct image_signature	    img_signature;
};

header_size = 1096
'''

header_size = 1096

class image_header(ctypes.Structure):
    _pack_ = 4
    _fields_ = [
        ("check_sum", ctypes.c_uint),
        ("capability", ctypes.c_uint),
        ("magic", ctypes.c_uint8 * 6),
        ("image_size", ctypes.c_uint),
        ("partition_num", ctypes.c_uint8),
        ("partition_size", ctypes.c_uint * 32),
        ("partition_offset", ctypes.c_uint * 32),
        ("hash_digest", ctypes.c_uint8 * 32),
        ("key_n_header", ctypes.c_uint),  # 0x02000800 //0x00, 0x08, 0x00, 0x02
        ("rsa_key_n", ctypes.c_uint8 * 384), # 4 x 96
        ("key_e_header", ctypes.c_uint),  # 0x02010020 //0x20, 0x00, 0x01, 0x02
        ("rsa_key_e", ctypes.c_uint8 * 4),
        ("sig_header", ctypes.c_uint),    # 0x01000800 //0x00, 0x08, 0x00, 0x01
        ("signature", ctypes.c_uint8 * 384) # 4 x 96
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

def to_bytes(n, length, byteorder='big'):
    if sys.version > '3':
        return n.to_bytes(length, byteorder=byteorder, signed=False)
    else:
        if byteorder == 'big':
            data = b''
            for i in reversed(range(length)):
                d = (n >> i * 8) & 0xff
                data += struct.pack("B", d)
            return data
        else:
            data = b''
            for i in range(length):
                d = (n >> i * 8) & 0xff
                data += struct.pack("B", d)
            return data

def make_image(dtb_file, kernel_file, pub_file, prv_file, output_file, capability):
    # load pub and private key file
    try:
        with open(pub_file, 'rb') as f:
            pub_key = rsa.PublicKey.load_pkcs1(f.read())

        with open(prv_file, 'rb') as f:
            prv_key = rsa.PrivateKey.load_pkcs1(f.read())
    except:
        print('[error] load public or private key file failed\n')
        print(sys.exc_info())
        print(traceback.format_exc())
        return False

    # load dtb file data
    dtb_size = os.path.getsize(dtb_file)
    try:
        with open(dtb_file, 'rb') as f:
            dtb_data = f.read()
    except:
        print('[error] load input dtb file failed\n')
        print(sys.exc_info())
        print(traceback.format_exc())
        return False

    # load kernel file data
    kernel_size = os.path.getsize(kernel_file)
    try:
        with open(kernel_file, 'rb') as f:
            kernel_data = f.read()
    except:
        print('[error] load input kernel file failed\n')
        print(sys.exc_info())
        print(traceback.format_exc())
        return False
    image_data = dtb_data + kernel_data

    header = image_header()
    p = ctypes.cast(ctypes.pointer(header), ctypes.POINTER(ctypes.c_uint8 * header_size))
    ctypes.memset(p, 0, header_size)
    # capability
    header.capability = capability
    # magic, AXERA!
    header.magic[0] = 0x41
    header.magic[1] = 0x58
    header.magic[2] = 0x45
    header.magic[3] = 0x52
    header.magic[4] = 0x41
    header.magic[5] = 0x21
    # image size
    header.image_size = header_size + dtb_size + kernel_size
    # 打包dtb+kernel
    header.partition_num = 2
    header.partition_size[0] = dtb_size
    header.partition_size[1] = kernel_size
    header.partition_offset[0] = header_size
    header.partition_offset[1] = header_size + dtb_size
    # compute image data SHA256
    id = hashlib.sha256(image_data).digest()
    copy_data_by_bytes(id, header.hash_digest, 32, 0)
    # rsa key
    # key_bit = 3072
    header.key_n_header = 0x02000C00
    header.key_e_header = 0x02010020
    # rsa_key_n
    pub_n = to_bytes(pub_key.n, 384, byteorder='little')
    copy_data_by_bytes(pub_n, header.rsa_key_n, 384)
    # rsa_key_e
    pub_e = to_bytes(pub_key.e, 4, byteorder='little')
    copy_data_by_bytes(pub_e, header.rsa_key_e, 4)
    # sign data
    # key_bit = 3072
    header.sig_header = 0x01000C00
    signature = rsa.sign(image_data, prv_key, 'SHA-256')
    copy_data_by_bytes(signature, header.signature, 384, 0, True)
    
    # verify test
    try:
        ret = rsa.verify(image_data, signature, pub_key)
    except e:
        print('[error] verify failed\n')
        print(sys.exc_info())
        print(traceback.format_exc())
        exit(-1)
    # calc header check sum
    header_data = (ctypes.c_uint8 * header_size)(0)
    p = ctypes.cast(ctypes.pointer(header), ctypes.POINTER(ctypes.c_uint8 * header_size))
    check_sum = ctypes.c_uint(0)
    for i in range(1, (header_size) // 4):
        data = p.contents[4 * i] + (p.contents[4 * i + 1] << 8) + (p.contents[4 * i + 2] << 16) + (p.contents[4 * i + 3] << 24)
        check_sum.value += ctypes.c_int(data).value
    header.check_sum = check_sum
    print("header check sum:")
    print(check_sum)

    output_data = (ctypes.c_uint8 * (header.image_size))(0)
    p = ctypes.cast(ctypes.pointer(header), ctypes.POINTER(ctypes.c_uint8 * header_size))
    for i in range(header_size):
        output_data[i] = p.contents[i]
    copy_data_by_bytes(image_data, output_data, dtb_size + kernel_size, header_size)

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
    #gen_rsa_2048_keys()

    dtb_file = None
    kernel_file = None
    pub_file = None
    prv_file = None
    out_file = None
    capability = None

    if len(sys.argv) < 13:
        print('[error] param is invalid\n')
        print_usage()
        exit(-1)

    for i in range(1, len(sys.argv)):
        if sys.argv[i] == '-dtb':
            dtb_file = sys.argv[i+1]
        if sys.argv[i] == '-kernel':
            kernel_file = sys.argv[i+1]
        elif sys.argv[i] == '-pub':
            pub_file = sys.argv[i+1]
        elif sys.argv[i] == '-prv':
            prv_file = sys.argv[i+1]
        elif sys.argv[i] == '-o':
            out_file = sys.argv[i + 1]
        elif sys.argv[i] == '-cap':
            capability = int(sys.argv[i + 1],16)

    if dtb_file is None or kernel_file is None or pub_file is None or prv_file is None or out_file is None or capability is None:
        print('[error] param is invalid\n')
        print_usage()
        exit(-1)

    if (not os.path.exists(dtb_file)) or (not os.path.exists(kernel_file)) or (not os.path.exists(pub_file)) or (not os.path.exists(prv_file)):
        print('[error] sign input file does not exist\n')
        exit(-1)

    ret = make_image(dtb_file, kernel_file, pub_file, prv_file, out_file, capability)
    if ret:
        print('make image complete')
