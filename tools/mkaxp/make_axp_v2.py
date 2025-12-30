#!/usr/bin/env python3
# -*- coding:utf-8 -*-

import getopt
import sys, os
import zipfile, shutil
import xml.etree.ElementTree as ET
import hashlib
import copy

'''
- V2.0 2023-12-01: change image input arg to ID=img path, eg: FDL1=/mnt/fdl1.bin FDL2=/mnt2/fdl2/bin
- V1.3 2022-06-10: Support to pack different directory but same file name
- V1.2 2022-06-07: Check unqiue of <name> attribute and <ID> of <Img> node for xml file
- V1.1 2020-08-01: Count files by dwFlag & 0x01 (function: check_need_input_file)
- V1.0 2020-07-29: First release
'''
SCRIPT_VERSION = '2.0'

XML_NODE_Img = 'Img'
XML_ATTR_flag = 'flag'
XML_ATTR_name = 'name'
XML_ATTR_select = 'select'
XML_ATTR_version = 'version'
XML_NODE_File = 'File'
XML_NODE_ID = 'ID'
XML_NODE_Project = 'Project'
XML_NODE_Auth = 'Auth'
XML_ATTR_algo = 'algo'

# Config = [
#     # ID            file      select   flag   auth
#     ['EIP',         '$1',       1,      1,     1],
#     ['FDL1',        '$2',       1,      1,     1],
#     ['FDL2',        '$3',       1,      1,     1],
#     ['SYSTEM',      '$4',       1,      1,     1],
#     ['USERDATA',    '$5',       1,      1,     1]
# ]
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


def print_usage():
    print('usage:')
    print('python3 make_axp.py [-h] -p [-v] -o -x ID1=file1 ID2=file2 ...')
    print('     [Mandatory arguments]:')
    print('         -p, --project=project               set project name')
    print('         -o, --output=output axp file        set output .axp file')
    print('         -x, --xml=xml file                  XML configuration file')
    print('         ID=file                             input image ID and file path with =, eg: FDL1=/mnt/fdl1.bin')
    print('     [Optional arguments]:')
    print('         -h, --help                          usage help')
    print('         -v, --version=version               set version')


def get_fname(path):
    return os.path.basename(path)


def get_abspath(path):
    return os.path.normpath(os.path.abspath(path))


def check_need_input_file(flag):
    if flag and ((int(flag) & 0x01) == 0x01):  # 0x01: need a file
        return True
    else:
        return False


def copy_warpper(src, dst):
    print('\t\t{}  -->  {}'.format(src, dst))
    if os.path.isfile(src):
        shutil.copy(src, dst)
    if os.path.isdir(src):
        shutil.copytree(src, dst)


def calc_auth_value(file, is_md5=True):
    print('\tCalculating %s  -->  file: %s, size: %d' % (
    ('md5' if is_md5 else 'crc16'), get_fname(file), os.path.getsize(file)))

    def read_block_from_file(file, block_size):
        with open(file, 'rb') as f:
            while True:
                block = f.read(block_size)
                if block:
                    yield block
                else:
                    return

    BLOCK_SIZE = 10 * 1024 * 1024
    if is_md5:
        m = hashlib.md5()
    else:
        crc16 = 0
    for block in read_block_from_file(file, BLOCK_SIZE):
        if is_md5:
            m.update(block)
        else:
            for data in block:
                crc16 = (crc16 >> 8) ^ (crc16_table[(crc16 ^ data) & 0xFF])
    if is_md5:
        md5 = m.hexdigest()
        print('\t\tMD5 = %s' % md5)
        return md5
    else:
        crc16 = hex(crc16)
        print('\t\tcrc16 = %s' % crc16)
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


def make_zip(zip_dir, zip_path):
    cwd = os.getcwd()
    os.chdir(zip_dir)

    def get_zip_files(path):
        l = []
        for root, dirs, files in os.walk(path):
            for name in files:
                l.append(os.path.join(root, name)[len(path) + 1:])
            for name in dirs:
                l.append(os.path.join(root, name)[len(path) + 1:])
        return l

    fs = get_zip_files(zip_dir)
    zf = zipfile.ZipFile(zip_path, 'w', zipfile.zlib.DEFLATED, allowZip64=True)
    zipped = 0
    try:
        for f in fs:
            print('\t\tCompressing %s' % f)
            zf.write(f)
            zipped += 1
    except Exception as e:
        raise e
    finally:
        zf.close()
        os.chdir(cwd)
        print('\t\t%d files are packed' % zipped)


def find_child(parent, name):
    child = parent.findall(name)
    if child:
        return child[0]
    else:
        return ET.SubElement(parent, name)


def update_xml(path, args):
    tree = ET.parse(path)
    root = tree.getroot()

    node_project = root.find(XML_NODE_Project)
    node_project.set(XML_ATTR_name, args['project'])
    if args['version']:
        node_project.set(XML_ATTR_version, args['version'])

    for node_img in root.iter(XML_NODE_Img):
        flag = node_img.get(XML_ATTR_flag)
        if check_need_input_file(flag):
            node_file = find_child(node_img, XML_NODE_File)
            node_id = find_child(node_img, XML_NODE_ID)
            for k, v in args['imgs'].items():
                if k == node_id.text:
                    node_file.text = get_fname(v)
                    break

            node_auth = find_child(node_img, XML_NODE_Auth)
            algo = node_auth.get(XML_ATTR_algo)
            if algo and int(algo) > 0:
                node_auth.text = calc_auth_value(args['imgs'][node_id.text], True if int(algo) == 1 else False)
    tree.write(path)


def parse_args():
    args = {'project': None, 'version': None, 'xml': None, 'zip': None, 'imgs': {}}

    opts, imgs = getopt.getopt(sys.argv[1:], '-h-p:-v:-o:-x:',
                               ['help', 'project=', 'version=', 'output=', 'xml='])
    if len(opts) == 0:
        print_usage()
        exit(0)

    for opt_name, opt_value in opts:
        if opt_name in ('-h', '--help'):
            print_usage()
            exit(0)

        if opt_name in ('-p', '--project'):
            args['project'] = opt_value

        if opt_name in ('-v', '--version'):
            args['version'] = opt_value

        if opt_name in ('-x', '--xml'):
            path = get_abspath(opt_value)
            if not os.path.exists(path):
                print('%s not exist!' % opt_value)
                exit(1)
            args['xml'] = path

        if opt_name in ('-o', '--output'):
            args['zip'] = get_abspath(opt_value)

    # check args
    if not args['project']:
        print('[Error] Input project name by [-p] or [--project]')
        exit(1)
    if not args['zip']:
        print('[Error] Input the .axp file by [-o] or [--output]')
        exit(1)

    try:
        ids = []
        img = {}
        for d in imgs:
            img = d
            k, v = d.split('=')

            k = k.strip()
            ids.append(k)

            v = get_abspath(v.strip())
            if not os.path.exists(v):
                print(f'image {v} does not exist')
                exit(1)
            if not os.path.isfile(v):
                print(f'image {v} is not a file')
                exit(1)
            if os.path.getsize(v) == 0:
                print(f'image {v} size is 0')
                exit(1)

            # args['imgs']: {'FDL1' : /mnt/fdl1.bin', 'FDL2' : /mnt/fdl2.bin', ...}
            args['imgs'][k] = v

        if len(set(ids)) != len(imgs):
            print('image ID should be equal')
            exit(1)
    except:
        print(f'unknown image args {img}')
        print_usage()
        exit(1)

    file_nums = 0
    try:
        tree = ET.parse(args['xml'])
        root = tree.getroot()
        alias = []
        id = []
        for node in root.iter(XML_NODE_Img):
            alias.append(node.get(XML_ATTR_name))
            flag = node.get(XML_ATTR_flag)
            node_id = find_child(node, XML_NODE_ID)
            if check_need_input_file(flag):
                file_nums += 1
                if node_id.text not in args['imgs'].keys():
                    print(f'image of ID {node_id.text} not found')
                    exit(1)
            id.append(node_id.text)

        if file_nums == 0:
            print('[Error] Invalid xml file: 0 disk file')
            exit(1)

        s1 = set(alias)
        if len(s1) != len(alias):
            print("[Error] attribute <name> value of <Img> nodes must be unique")
            exit(1)

        s2 = set(id)
        if len(s2) != len(id):
            print("[Error] <ID> value of <Img> nodes must be unique")
            exit(1)

    except ET.ParseError as e:
        print('[Error] Parse <{}> error: {}'.format(args['xml'], e))
        exit(1)

    if file_nums != len(args['imgs']):
        print('[Error] The number of input disk files <{}> is not equal to expected <{}>'.format(len(args['imgs']),
                                                                                                 file_nums))
        exit(1)

    return args


if __name__ == "__main__":
    print('==== Make AXP Script Version: %s ====' % SCRIPT_VERSION)
    args = parse_args()

    try:
        zip_path = args['zip']
        zip_dir, zip_file = os.path.split(zip_path)
        zip_name, _ = os.path.splitext(zip_file)

        zip_dir = os.path.join(zip_dir, zip_name)
        rm(args['zip'])
        rm(zip_dir)
        os.mkdir(zip_dir)
        if not os.path.exists(zip_dir):
            print('\t[Error] mk <%s> error' % zip_dir)
            exit(1)

        # Copy files including .XML file
        src_files = []
        dst_files = []

        src_files.append(args['xml'])
        dst_files.append(os.path.join(zip_dir, get_fname(args['xml'])))

        for k, v in args['imgs'].items():
            src_files.append(v)
            dst_file = os.path.join(zip_dir, get_fname(v))
            if dst_file in dst_files:
                # same file name of different directory, append .1 suffix
                # simply rename and copy again even the file contents are equal
                # example:
                #  file1: a/b/uImage
                #  file2: a/uImage
                #  then destination files are:
                #      dst/uImage    -- copied from a/b/uImage
                #      dst/uImage.1  -- copied from a/uImage
                dst_file = dst_file + '.1'
                args['imgs'][k] = dst_file
            dst_files.append(dst_file)

        print('\tCopying files ...')
        for z in zip(src_files, dst_files):
            copy_warpper(z[0], z[1])
        print('\t\t%d files are copied.' % len(src_files))

        # Update .XML file
        xml_file = get_fname(args['xml'])
        xml_path = os.path.join(zip_dir, xml_file)
        print('\tUpdating %s ...' % xml_file)
        update_xml(xml_path, args)

        # Make .zip
        print('\tMaking %s ...' % zip_file)
        make_zip(zip_dir, zip_path)
        rm(zip_dir)

    except Exception as e:
        print('[Error] Make axp error: %s' % e)
    else:
        print('\n----- SUCCESS -----')
