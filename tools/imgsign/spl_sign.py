import os
import sys
import rsa
import ctypes
import traceback
import struct


def gen_rsa_2048_keys():
    try:
        (pubkey, prvkey) = rsa.newkeys(2048)
        print("pub: ", pubkey)
        print("prv: ", prvkey)

        data = pubkey.save_pkcs1(format='PEM')
        with open('public.pem', 'wb') as f:
            f.write(data)

        data = prvkey.save_pkcs1(format='PEM')
        with open('private.pem', 'wb') as f:
            f.write(data)
    except:
        print(sys.exc_info())
        print(traceback.format_exc())



def print_usage():
    print('\nbuild_spl.py -i <spl_file> -pub <public_key_file(*.pem)> -prv <private_key_file(*.pem)> -o <out_file>')
    print('  -i   :  input original spl file path, the file size must be less than 111K bytes')
    print('  -pub :  input rsa public key file (*.pem) path')
    print('  -prv :  input rsa private key file (*.pem) path')
    print('  -o   :  output new spl file with 112K bytes\n')

'''
struct spl_header{
    int check_sum;//0x00
    int magic_data;//0x04, should be 0x55543322
    int hash_size; //72*4
    int uid[2];
    int key_n_header;
    int rsa_key_n[64];
    int key_e_header;
    int rsa_key_e;
    int reserved2[119];
    int sig_header;
    int signature[64];
};
'''

FILE_SIZE = 112*1024  # 112K
MSG_SIZE = 111*1024   # 111K

class spl_header(ctypes.Structure):
    _pack_ = 4
    _fields_ = [
        ("check_sum", ctypes.c_int),
        ("magic_data", ctypes.c_int),
        ("hash_size", ctypes.c_int),
        ("uid", ctypes.c_int*2),
        ("key_n_header", ctypes.c_int),  # 0x02000800 //0x00, 0x08, 0x00, 0x02
        ("rsa_key_n", ctypes.c_uint8*256),
        ("key_e_header", ctypes.c_int),  # 0x02010020 //0x20, 0x00, 0x01, 0x02
        ("rsa_key_e", ctypes.c_uint8*4),
        ("reserved2", ctypes.c_int*119),
        ("sig_header", ctypes.c_int),    # 0x01000800 //0x00, 0x08, 0x00, 0x01
        ("signature", ctypes.c_uint8*256)
    ]


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

def do_spl(in_file, pub_file, prv_file, out_file):
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

    # load in file data
    in_size = os.path.getsize(in_file)
    if in_size > 111*1024:
        print('[error] file size({}) too big, must be not larger than 111K'.format(in_size))
        return False

    try:
        with open(in_file, 'rb') as f:
            file_data = f.read()
    except:
        print('[error] load image file failed\n')
        print(sys.exc_info())
        print(traceback.format_exc())
        return False

    # construct spl_header
    hdr = spl_header()
    hdr.magic_data = 0x55543322
    hdr.key_n_header = 0x02000800
    hdr.key_e_header = 0x02010020
    hdr.sig_header = 0x01000800
    hdr.hash_size = 288

    pub_n = to_bytes(pub_key.n, 256, byteorder='little')
    copy_data_by_bytes(pub_n, hdr.rsa_key_n, 256)

    pub_e = to_bytes(pub_key.e, 4, byteorder='little')
    copy_data_by_bytes(pub_e, hdr.rsa_key_e, 4)

    # sign data
    _message = file_data + b'\x00'*(MSG_SIZE - in_size)
    signature = rsa.sign(_message, prv_key, 'SHA-256')
    copy_data_by_bytes(signature, hdr.signature, 256, 0, True)
   
    '''
    # verify test
    try:
        ret = rsa.verify(_message, signature, pub_key)
    except e:
        print('[error] verify failed\n')
        print(sys.exc_info())
        print(traceback.format_exc())
        exit(-1)
    '''

    # create 112k buffer and put the file data offset 1K
    out_data = (ctypes.c_uint8 * FILE_SIZE)(0)
    copy_data_by_bytes(file_data, out_data, in_size, 1024)

    # put hdr in first 1k
    p = ctypes.cast(ctypes.pointer(hdr), ctypes.POINTER(ctypes.c_uint8 * 1024))
    for i in range(1024):
        out_data[i] = p.contents[i]

    # calc check sum
    check_sum = ctypes.c_int(0)
    for i in range(1, FILE_SIZE//4):
        data = out_data[4*i] + (out_data[4*i + 1] << 8) + (out_data[4*i + 2] << 16) + (out_data[4*i + 3] << 24)
        check_sum.value += ctypes.c_int(data).value

    # update check sum in buffer
    p = ctypes.cast(ctypes.pointer(check_sum), ctypes.POINTER(ctypes.c_uint8 * 4))
    for i in range(4):
        out_data[i] = p.contents[i]

    # save data to out file
    try:
        with open(out_file, 'wb') as f:
            f.write(out_data)
    except:
        print('[error] create spl file failed\n')
        print(sys.exc_info())
        print(traceback.format_exc())
        return False
    return True


if __name__ == '__main__':
    #gen_rsa_2048_keys()

    in_file = None
    pub_file = None
    prv_file = None
    out_file = None

    if len(sys.argv) < 7:
        print('[error] param is invalid\n')
        print_usage()
        exit(-1)

    for i in range(1, len(sys.argv)):
        if sys.argv[i] == '-i':
            in_file = sys.argv[i+1]  
        elif sys.argv[i] == '-pub':
            pub_file = sys.argv[i+1]
        elif sys.argv[i] == '-prv':
            prv_file = sys.argv[i+1]
        elif sys.argv[i] == '-o':
            out_file = sys.argv[i + 1]

    if in_file is None or pub_file is None or prv_file is None or out_file is None:
        print('[error] param is invalid\n')
        print_usage()
        exit(-1)
    
    if (not os.path.exists(in_file)) or (not os.path.exists(pub_file)) or (not os.path.exists(prv_file)):
        print('[error] sign input file does not exist\n')
        exit(-1)
    '''
    # test
    in_file = 'D:/fdl1.bin'
    pub_file = 'D:/public.pem'
    prv_file = 'D:/private.pem'
    out_file = 'D:/fdl1_sign.bin'
    '''
    ret = do_spl(in_file, pub_file, prv_file, out_file)
    if ret:
        print('sign complete')