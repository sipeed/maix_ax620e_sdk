#encoding=utf-8
import re
import os
import sys

try:
    import lxml.etree as ET
except ModuleNotFoundError as e:
    os.system("python3 -m pip install --upgrade pip")
    os.system("pip3 install lxml")

def get_partition_sizes(filename):
    partition_sizes = {}

    with open(filename, 'r') as file:
        lines = file.readlines()

    auto_fit_partition_name = None
    for line in lines:
        line = line.strip()
        if line.startswith("#"):
            continue
        match = re.search(r'(\w+)_PARTITION_SIZE\s+:=\s+(\w+)', line)
        if match:
            partition_name = match.group(1).lower()
            partition_size = match.group(2)
            partition_sizes[partition_name] = partition_size
        else:
            match = re.search(r'AUTO_FIT_PARTITION\s+:=\s+(\w+)', line)
            if match:
                auto_fit_partition_name = match.group(1).lower()

    if auto_fit_partition_name in partition_sizes.keys():
        partition_sizes[auto_fit_partition_name] = "0xffffffff"

    return partition_sizes

def convert_to_MB(size):
    if size.endswith('K'):
        return str(int(size[:-1])//1024)
    elif size.endswith('M'):
        return str(int(size[:-1]))
    else:
        if (str(size).lower() == "0xffffffff"):
            return str(size).lower()
        raise ValueError(f"Invalid size: {size}")

def convert_to_512KB(size):
    if size.endswith('K'):
        return str(int(size[:-1]) // 512)
    elif size.endswith('M'):
        return str( (int(size[:-1]) * 1024) // 512)
    else:
        if (str(size).lower() == "0xffffffff"):
            return str(size).lower()
        raise ValueError(f"Invalid size: {size}")

def convert_to_KB(size):
    if size.endswith('K'):
        return str(int(size[:-1]))
    elif size.endswith('M'):
        return str(int(size[:-1]) * 1024)
    else:
        if (str(size).lower() == "0xffffffff"):
            return str(size).lower()
        raise ValueError(f"Invalid size: {size}")

def convert_to_Byte(size):
    if size.endswith('K'):
        return str(int(size[:-1])*1024)
    elif size.endswith('M'):
        return str(int(size[:-1]) * 1048576)
    else:
        if (str(size).lower() == "0xffffffff"):
            return str(size).lower()
        raise ValueError(f"Invalid size: {size}")

def convert_to_sector(size):
    raise ValueError(f"Unsupported sector unit!")

def gen_new_xml(src_xml, dst_xml, partitions_info):
    tree = ET.parse(src_xml)
    root = tree.getroot()
    #Unit: 0, 1M Byte; 1, 512K Byte; 2, 1K Byte; 3, 1 Byte; 4, 1Sector
    convert_size = {
        0 : convert_to_MB,
        1 : convert_to_512KB,
        2 : convert_to_KB,
        3 : convert_to_Byte,
        4 : convert_to_sector
    }
    for partitions in root.iter('Partitions'):
        for partName, partSize in partitions_info.items():
            matched = False
            unit = int(partitions.attrib.get('unit'))
            for partition in partitions:
                if partition.attrib.get('id') == partName:
                    partition.set('size', convert_size[unit](partSize))
                    matched = True
                    print(f"matched partition: {partName}, set size to: {partSize}")
            if not matched:
                raise AttributeError(f"{src_xml} 文件没有 '{partName}'分区!")
    tree.write(dst_xml, encoding="utf-8", xml_declaration=True, method="xml", pretty_print=True)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("para error!")
        print("usage: gen_axp_xml.py src_xml project.mak dst_xml")
        sys.exit(-1)

    src_xml     = sys.argv[1]
    project_mak = sys.argv[2]
    dst_xml     = sys.argv[3]

    partition_sizes = get_partition_sizes(project_mak)
    gen_new_xml(src_xml, dst_xml, partition_sizes)
    print("gen axp xml done!!!")

