#!/usr/bin/env python3
#
# Sming hardware configuration tool
#

import common
import argparse
import os
import json
import string
from common import *


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

    # print(to_json(config))

    strings = set([''])

    string_fields = ['manufacturer', 'product', 'serial']

    for tag, dev in config['devices'].items():
        for f in string_fields:
            strings.add(dev[f])
    strings = sorted(strings)
    for i, s in enumerate(strings):
        print(f"{i}: {s}")

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

    for tag, dev in config['devices'].items():
        print(tag)
        dev['class_id'] = DEVICE_CLASSES[dev['class']]
        dev['subclass_id'] = DEVICE_SUBCLASSES[dev['subclass']]
        dev['protocol_id'] = DEVICE_PROTOCOLS[dev['protocol']]
        device_vars = dict([(f"device_{key}", value) for key, value in dev.items() if not isinstance(value, dict)])
        for f in string_fields:
            device_vars[f'device_{f}_id'] = strings.index(dev[f])
        print(device_vars)


    pathname = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'templates/usb_descriptors.c')
    print(pathname)
    with open(pathname) as f:
        tmpl = string.Template(f.read())
    print(tmpl.substitute(device_vars))

    # output = globals()['handle_' + args.command](args, config, part)

    # if output is not None:
    #     openOutput(args.output).write(output)


if __name__ == '__main__':
    try:
        main()
    except InputError as e:
        print("** ERROR! %s" % e, file=sys.stderr)
        sys.exit(2)
