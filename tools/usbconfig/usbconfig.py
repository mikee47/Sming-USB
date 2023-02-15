#!/usr/bin/env python3
#
# Sming hardware configuration tool
#

import common
import argparse
import os
import json
import string
import math
from common import *

TUSB_DESC_STRING = 3

STRING_FIELDS = {
    "device": ['manufacturer', 'product', 'serial']
}

DEVICE_CLASSES = {
    'misc': 'TUSB_CLASS_MISC',
}

DEVICE_SUBCLASSES = {
    'none': 0,
    'common': 'MISC_SUBCLASS_COMMON',
}

DEVICE_PROTOCOLS = {
    'none': 0,
    'iad': 'MISC_PROTOCOL_IAD',
}

CONFIG_ATTRIBUTES = {
    'remote-wakeup': 'TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP',
}

def openOutput(path):
    if path == '-':
        try:
            stdout_binary = sys.stdout.buffer  # Python 3
        except AttributeError:
            stdout_binary = sys.stdout
        return stdout_binary
    status("Writing to '%s'" % path)
    output_dir = os.path.abspath(os.path.dirname(path))
    os.makedirs(output_dir, exist_ok=True)
    return open(path, 'wb')


def main():
    parser = argparse.ArgumentParser(description='Sming USB configuration utility')

    parser.add_argument('--quiet', '-q', help="Don't print non-critical status messages to stderr", action='store_true')
    parser.add_argument('input', help='Path to configuration file')
    parser.add_argument('output', help='Output directory')

    args = parser.parse_args()

    common.quiet = args.quiet

    output = None
    config = json_load(args.input)

    strings = set()

    # DEVICE

    for tag, dev in config['devices'].items():
        for f in STRING_FIELDS['device']:
            strings.add(dev[f])

    # Sort all strings into a list
    strings = sorted(strings)

    for tag, dev in config['devices'].items():
        vars = dict([(key, value) for key, value in dev.items() if not isinstance(value, dict)])
        vars['device_tag'] = tag
        vars['class_id'] = DEVICE_CLASSES[dev['class']]
        vars['subclass_id'] = DEVICE_SUBCLASSES[dev['subclass']]
        vars['protocol_id'] = DEVICE_PROTOCOLS[dev['protocol']]
        vars['vendor_id'] = dev['vendor']
        ver = round(float(dev['version']) * 100)
        vars['version_bcd'] = f"0x{ver:04}"
        for f in STRING_FIELDS['device']:
            vars[f'{f}_idx'] = strings.index(dev[f]) + 1

    def readTemplate(name, vars):
        pathname = os.path.join(os.path.dirname(os.path.abspath(__file__)), f'templates/{name}')
        with open(pathname) as f:
            tmpl = string.Template(f.read())
        return tmpl.substitute(vars)

    s = readTemplate('usb.c', vars)
    print(s)

    max_string_len = 0
    string_data = []
    for i, s in enumerate(strings):
        s_encoded = s.encode('utf-16le')
        max_string_len = max(max_string_len, len(s_encoded))
        desc_array = bytes([2 + len(s_encoded), TUSB_DESC_STRING]) + s_encoded
        string_data.append("".join(f"\\x{c:02x}" for c in desc_array))

    vars = {
        'max_string_len': max_string_len,
        'string_data': ",\n".join(f'  // {i+1}: "{s}"\n'
                                  f'  "{d}"' for ((i, s), d) in zip(enumerate(strings), string_data)),
    }

    s = readTemplate('string.c', vars)
    print(s)

    # output = globals()['handle_' + args.command](args, config, part)

    # if output is not None:
    #     openOutput(args.output).write(output)


if __name__ == '__main__':
    try:
        main()
    except InputError as e:
        print("** ERROR! %s" % e, file=sys.stderr)
        sys.exit(2)
