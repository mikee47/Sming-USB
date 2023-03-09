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
    'self-powered': 'TUSB_DESC_CONFIG_ATT_SELF_POWERED',
}

# See tusb_option.h
INTERFACE_CLASSES = {
    'audio': {
    },
    'bth': {
    },
    'cdc': {
    },
    'dfu': {
    },
    'dfu-runtime': {
    },
    'ecm-rndis': {
    },
    'hid': {
    },
    'midi': {
    },
    'msc': {
    },
    'ncm': {
    },
    'net': {
    },
    'usbtmc': {
    },
    'vendor': {
    },
    'video': {
    },
}


HID_PROTOCOLS = ['none', 'keyboard', 'mouse']
HID_REPORTS = ['keyboard', 'mouse', 'consumer', 'system-control', 'gamepad', 'fido-u2f', 'generic-inout']
HID_DEFAULT_EP_BUFSIZE = 64

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


def readTemplate(name, vars):
    pathname = os.path.join(os.path.dirname(os.path.abspath(__file__)), f'templates/{name}')
    with open(pathname) as f:
        tmpl = string.Template(f.read())
    return tmpl.substitute(vars)


def main():
    parser = argparse.ArgumentParser(description='Sming USB configuration utility')

    parser.add_argument('--quiet', '-q', help="Don't print non-critical status messages to stderr", action='store_true')
    parser.add_argument('input', help='Path to configuration file')
    parser.add_argument('output', help='Output directory')

    args = parser.parse_args()

    common.quiet = args.quiet

    output = None
    config = json_load(args.input)

    # Build sorted string list
    strings = set()

    def add_string(s):
        if s:
            strings.add(s)

    def get_string_idx(s):
        return strings.index(s) + 1 if s else 0

    for dev in config['devices'].values():
        for f in STRING_FIELDS['device']:
            strings.add(dev[f])
        for cfg in dev['configs'].values():
            add_string(cfg.get('description'))
            for itf in cfg['interfaces'].values():
                add_string(itf.get('description'))
    strings = sorted(strings)

    config_c = ""
    # Device descriptors

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
            vars[f'{f}_idx'] = get_string_idx(dev[f])
        vars['config_count'] = len(dev['configs'])
        config_c += readTemplate('desc.c', vars)

    # Configuration descriptors

    def make_identifier(s):
        return s.replace('-', '_').upper()

    for dev in config['devices'].values():
        cfg_num = 1
        for cfg_tag, cfg in dev['configs'].items():
            # Count instances of each class type
            itf_counts = dict((c, 0) for c in INTERFACE_CLASSES)
            for itf in cfg['interfaces'].values():
                itf_counts[itf['class']] += 1

            # Emit HID report descriptors
            hid_inst = 0
            hid_report = ""
            hid_callback = ""
            hid_ep_bufsize = 0
            for itf_tag, itf in cfg['interfaces'].items():
                if itf['class'] != 'hid':
                    continue
                hid_ep_bufsize = max(hid_ep_bufsize, itf.get('bufsize', HID_DEFAULT_EP_BUFSIZE))
                hid_report += f'static const uint8_t desc_{itf_tag}_report[] = {{\n'
                for r in itf['reports']:
                    if r in HID_REPORTS:
                        id = make_identifier(r)
                        hid_report += f"  TUD_HID_REPORT_DESC_{id}\t(HID_REPORT_ID(REPORT_ID_{id})\t),\n"
                    else:
                        raise InputError(f'Unknown report "{r}"')
                hid_report += '};\n\n'
                hid_callback += f'    case {hid_inst}:\n'
                hid_callback += f'      return desc_{itf_tag}_report;\n'
                hid_inst += 1

            if hid_inst:
                vars = {
                    'report': hid_report,
                    'callback': hid_callback,
                }
                config_c += readTemplate('hid_report.c', vars)

            # Emit Configuration descriptors
            itf_num_total = len(cfg['interfaces'])
            desc_idx = get_string_idx(cfg.get('description'))
            config_total_len = 'TUD_CONFIG_DESC_LEN' + \
                "".join(f" + TUD_{itf['class'].upper()}_DESC_LEN" for itf in cfg['interfaces'].values())
            if 'attributes' in cfg:
                attr = " | ".join([CONFIG_ATTRIBUTES[a] for a in cfg['attributes']])
            else:
                attr = 0
            power = cfg['power']
            config_desc = '  // Config number, interface count, string index, total length, attribute, power in mA\n'
            config_desc += f"  TUD_CONFIG_DESCRIPTOR({cfg_num}, {itf_num_total}, {desc_idx}, {config_total_len}, {attr}, {power}),\n"
            for itf_tag, itf in cfg['interfaces'].items():
                itf_class = itf['class']
                info = INTERFACE_CLASSES[itf_class]
                # config_data.append(build_itf_desc(itf_num, itf_tag, itf))

            vars = {
                'config_desc': config_desc,
            }
            config_c += readTemplate('interface.c', vars)

        # tusb_config_h
        class_counts = ""
        for itf_class, count in itf_counts.items():
            class_counts += f"#define CFD_TUD_{make_identifier(itf_class)}\t{count}\n"
        vars = {
            'class_counts': class_counts,
            'hid_ep_bufsize': hid_ep_bufsize,
        }
        config_h = readTemplate('config.h', vars)
        print(config_h)

        # usb_descriptors.h
        # TODO

        # usb_descriptors.c
        print(config_c)


#   // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
#   TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)

#   // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
#   TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

#   // Interface number, string index, EP Out & EP In address, EP size
#   TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),

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
