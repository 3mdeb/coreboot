#!/usr/bin/env python

from __future__ import print_function
import sys

# If pyelftools is not installed, the example can also run from the root or
# examples/ dir of the source distribution.
sys.path[0:0] = ['.', '..']

from elftools.elf.elffile import ELFFile
import elftools.elf.sections as sections


def process_file(filename):
    sect_dict = {}
    with open(filename, 'rb') as f:
        for sect in ELFFile(f).iter_sections():
            if isinstance(sect, sections.NullSection):
                continue
            if sect.name == ".text":
                sect_dict['text'] = hex(sect.header['sh_addr'])
            if sect.name == ".bss":
                sect_dict['bss'] = hex(sect.header['sh_addr'])
            if sect.name == ".data":
                sect_dict['data'] = hex(sect.header['sh_addr'])
            if sect.name == ".heap":
                sect_dict['heap'] = hex(sect.header['sh_addr'])
    cmd = "add-symbol-file {0} {1}".format(filename, sect_dict['text'])
    for k, v in sect_dict.iteritems():
        cmd += " -s .{0} {1}".format(k, v)
    print(cmd)

if __name__ == '__main__':
    process_file("build/cbfs/fallback/romstage.elf")
    process_file("build/cbfs/fallback/ramstage.elf")
