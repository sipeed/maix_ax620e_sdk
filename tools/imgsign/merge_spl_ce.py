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
    print('\nmerge_spl_ce.py -spl <spl_file> -ce <ce_file)> -o <out_file>')
    print('  -spl :  input signed spl file path, the file size must be 112K bytes')
    print('  -ce  :  input signed ce file path, the file size must be 112K bytes')
    print('  -o   :  output file\n')

'''
SPL 112K
pad to 128K
SPL 112K
pad to 128K
CE 112K
pad to 128K
CE 112K
pad to 128K
'''

FILE_SIZE = 512 * 1024  # 512K


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

def do_merge(spl_file, ce_file, out_file):
    # load pub and private key file
    try:
        with open(spl_file, 'rb') as f:
            spl_data = f.read()

        with open(ce_file, 'rb') as f:
            ce_data = f.read()
    except:
        print('[error] load spl or ce file failed\n')
        print(sys.exc_info())
        print(traceback.format_exc())
        return False

    if os.path.getsize(spl_file) != 112*1024 or os.path.getsize(ce_file) != 112*1024:
        print('[error] spl or ce file size is not equal 112K\n')
        return False

    # create 512K buffer
    out_data = (ctypes.c_uint8 * (FILE_SIZE))(0)

    # put spl to offset 0
    copy_data_by_bytes(spl_data, out_data, 112*1024, 0)
    # put spl to offset 128K
    copy_data_by_bytes(spl_data, out_data, 112*1024, 128 * 1024)

    # put ce to offset 256K
    copy_data_by_bytes(ce_data, out_data, 112*1024, 256 * 1024)
    # put ce to offset 128K
    copy_data_by_bytes(ce_data, out_data, 112*1024, 384 * 1024)


    # save data to out file
    try:
        with open(out_file, 'wb') as f:
            f.write(out_data)
    except:
        print('[error] create merged outpu file failed\n')
        print(sys.exc_info())
        print(traceback.format_exc())
        return False
    return True


if __name__ == '__main__':
    #gen_rsa_2048_keys()

    spl_file = None
    ce_file = None
    out_file = None

    if len(sys.argv) < 7:
        print('[error] param is invalid\n')
        print_usage()
        exit(-1)

    for i in range(1, len(sys.argv)):
        if sys.argv[i] == '-spl':
            spl_file = sys.argv[i+1]
        elif sys.argv[i] == '-ce':
            ce_file = sys.argv[i+1]
        elif sys.argv[i] == '-o':
            out_file = sys.argv[i + 1]

    if spl_file is None or ce_file is None or out_file is None:
        print('[error] param is invalid\n')
        print_usage()
        exit(-1)

    if (not os.path.exists(spl_file)) or (not os.path.exists(ce_file)):
        print('[error] merge input files does not exist\n')
        exit(-1)

    ret = do_merge(spl_file, ce_file, out_file)
    if ret:
        print('merge complete')

