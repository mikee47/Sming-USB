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


HOST_CLASSES = {
    'hub': {
    },
    'cdc': {
    },
    'hid': {
    },
    'midi': {
    },
    'msc': {
    },
    'vendor': {
    },
}


HID_PROTOCOLS = ['none', 'keyboard', 'mouse']
HID_REPORTS = ['keyboard', 'mouse', 'consumer', 'system-control', 'gamepad', 'fido-u2f', 'generic-inout']
HID_DEFAULT_EP_BUFSIZE = 64


def write_file(dirname, filename, content):
    path = os.path.join(dirname, filename)
    status(f'Creating "{path}"')
    os.makedirs(dirname, exist_ok=True)
    with open(path, 'w') as f:
        f.write(content)


def readTemplate(name, vars):
    pathname = os.path.join(os.path.dirname(os.path.abspath(__file__)), f'templates/{name}')
    with open(pathname) as f:
        tmpl = string.Template(f.read())
    return tmpl.substitute(vars)


def make_identifier(s):
    return s.replace('-', '_').upper()


def indent(defs):
    return f"  {defs}\n" if isinstance(defs, str) else "\n".join(f"  {d}" for d in defs)


def parse_devices(config, cfg_vars, output_dir):
    if 'devices' not in config:
        cfg_vars['device_enabled'] = 0
        cfg_vars['device_classes'] = ''
        cfg_vars['hid_ep_bufsize'] = 0
        return

    cfg_vars['device_enabled'] = 1

    # Build sorted string list
    strings = set()

    def add_string(s):
        if s:
            strings.add(s)

    for dev in config['devices'].values():
        for f in STRING_FIELDS['device']:
            strings.add(dev[f])
        for cfg in dev['configs'].values():
            add_string(cfg.get('description'))
            for itf in cfg['interfaces'].values():
                add_string(itf.get('description'))
    strings = sorted(strings)

    def get_string_idx(s):
        return strings.index(s) + 1 if s else 0

    desc_c = ""
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
        desc_c += readTemplate('desc.c', vars)

    # Configuration descriptors

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
            hid_callback = []
            hid_report_ids = set()
            hid_ep_bufsize = 0
            for itf_tag, itf in cfg['interfaces'].items():
                if itf['class'] != 'hid':
                    continue
                hid_ep_bufsize = max(hid_ep_bufsize, itf.get('bufsize', HID_DEFAULT_EP_BUFSIZE))
                hid_report += f'static const uint8_t desc_{itf_tag}_report[] = {{\n'
                for r in itf['reports']:
                    if r in HID_REPORTS:
                        id = make_identifier(r)
                        hid_report_ids.add(f"REPORT_ID_{id},")
                        hid_report += indent(f"TUD_HID_REPORT_DESC_{id}\t(HID_REPORT_ID(REPORT_ID_{id}) ),")
                    else:
                        raise InputError(f'Unknown report "{r}"')
                hid_report += '};\n\n'
                hid_callback += [f'case {hid_inst}: return desc_{itf_tag}_report;']
                hid_inst += 1

            if hid_inst:
                vars = {
                    'report': hid_report,
                    'callback': indent(hid_callback),
                }
                desc_c += readTemplate('hid_report.c', vars)

            # Emit Configuration descriptors
            desc_idx = get_string_idx(cfg.get('description'))
            if 'attributes' in cfg:
                attr = " | ".join([CONFIG_ATTRIBUTES[a] for a in cfg['attributes']])
            else:
                attr = 0
            power = cfg['power']
            config_desc = ['// Config number, interface count, string index, total length, attribute, power in mA']
            config_desc += [
                f"TUD_CONFIG_DESCRIPTOR({cfg_num}, ITF_NUM_TOTAL, {desc_idx}, CONFIG_TOTAL_LEN, {attr}, {power}),"]
            itf_num = 0
            itfnum_defs = []
            ep_num = 1
            epnum_defs = []
            for itf_tag, itf in cfg['interfaces'].items():
                itf_class = itf['class']
                info = INTERFACE_CLASSES[itf_class]
                itf_id = make_identifier(itf_tag)
                itfnum_defs += [f"ITF_NUM_{itf_id} = {itf_num},"]
                itf_desc_idx = get_string_idx(itf.get('description'))
                if itf_class == 'hid':
                    epnum_defs += [f"EPNUM_{itf_id} = 0x{ep_num | 0x80:02x},"]
                    ep_bufsize = itf.get('bufsize', HID_DEFAULT_EP_BUFSIZE)
                    poll_interval = itf['poll-interval']
                    protocol = f"HID_ITF_PROTOCOL_{itf['protocol'].upper()}"
                    config_desc += ['// Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval']
                    config_desc += [
                        f'TUD_HID_DESCRIPTOR(ITF_NUM_{itf_id}, {itf_desc_idx}, {protocol}, sizeof(desc_{itf_tag}_report), EPNUM_{itf_id}, {ep_bufsize}, {poll_interval}),']
                    ep_num += 1  # IAD

                elif itf_class == 'cdc':
                    epnum_defs += [f"EPNUM_{itf_id}_NOTIF = 0x{ep_num | 0x80:02x},"]
                    ep_num += 1
                    epnum_defs += [f"EPNUM_{itf_id}_OUT = 0x{ep_num:02x},"]
                    epnum_defs += [f"EPNUM_{itf_id}_IN = 0x{ep_num | 0x80:02x},"]
                    ep_num += 1
                    config_desc += [
                        '// Interface number, string index, EP notification address and size, EP data address (out, in) and size.']
                    config_desc += [
                        f'TUD_CDC_DESCRIPTOR(ITF_NUM_{itf_id}, {itf_desc_idx}, EPNUM_{itf_id}_NOTIF, 8, EPNUM_{itf_id}_OUT, EPNUM_{itf_id}_IN, 64),']
                    itf_num += 1  # IAD specifies 2 interfaces

                elif itf_class == 'msc':
                    epnum_defs += [f"EPNUM_{itf_id}_OUT = 0x{ep_num:02x},"]
                    epnum_defs += [f"EPNUM_{itf_id}_IN = 0x{ep_num | 0x80:02x},"]
                    ep_num += 1
                    config_desc += ['// Interface number, string index, EP Out & EP In address, EP size']
                    config_desc += [
                        f'TUD_MSC_DESCRIPTOR(ITF_NUM_{itf_id}, {itf_desc_idx}, EPNUM_{itf_id}_OUT, EPNUM_{itf_id}_IN, 64),']

                else:
                    raise InputError(f'Unsupported class "{itf_class}"')

                itf_num += 1

            itfnum_defs += [f"ITF_NUM_TOTAL = {itf_num},"]
            vars = {
                'itfnum_defs': indent(itfnum_defs),
                'config_total_len': 'TUD_CONFIG_DESC_LEN' + "".join(f" + TUD_{itf['class'].upper()}_DESC_LEN" for itf in cfg['interfaces'].values()),
                'epnum_defs': indent(epnum_defs),
                'config_desc': indent(config_desc),
            }
            desc_c += readTemplate('interface.c', vars)

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
        desc_c += readTemplate('string.c', vars)

        classes = ""
        for c, n in itf_counts.items():
            classes += f"#define CFG_TUD_{make_identifier(c)}\t{n}\n"
        cfg_vars['device_classes'] = classes
        cfg_vars['hid_ep_bufsize'] = hid_ep_bufsize

        vars = {
            'hid_report_ids': indent(hid_report_ids),
        }
        desc_h = readTemplate('desc.h', vars)

        if ep_num > 16:
            raise InputError(f'Too many endpoints ({ep_num})')

        write_file(output_dir, 'usb_descriptors.c', desc_c)
        write_file(output_dir, 'usb_descriptors.h', desc_h)


def parse_host(config, cfg_vars):
    host_enabled = 0
    classes = ""

    if 'host' in config:
        class_counts = dict((c, 0) for c in HOST_CLASSES)
        for dev in config['host'].values():
            class_counts[dev['class']] += 1
            host_enabled = 1
        for c, n in class_counts.items():
            classes += f"#define CFG_TUH_{make_identifier(c)}\t{n}\n"

    cfg_vars['host_enabled'] = host_enabled
    cfg_vars['host_classes'] = classes


def main():
    parser = argparse.ArgumentParser(description='Sming USB configuration utility')

    parser.add_argument('--quiet', '-q', help="Don't print non-critical status messages to stderr", action='store_true')
    parser.add_argument('input', help='Path to configuration file')
    parser.add_argument('output', help='Output directory')

    args = parser.parse_args()

    common.quiet = args.quiet

    config = json_load(args.input)

    cfg_vars = {}
    parse_devices(config, cfg_vars, args.output)
    parse_host(config, cfg_vars)

    config_h = readTemplate('config.h', cfg_vars)
    write_file(args.output, 'tusb_config.h', config_h)


if __name__ == '__main__':
    try:
        main()
    except InputError as e:
        print("** ERROR! %s" % e, file=sys.stderr)
        sys.exit(2)
