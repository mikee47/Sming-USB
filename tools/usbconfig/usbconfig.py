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


def parse_devices(config, cfg_vars, classdefs, output_dir):
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
        desc_c += readTemplate('device/desc.c', vars)

    # Configuration descriptors

    for dev in config['devices'].values():
        cfg_num = 1
        for cfg_tag, cfg in dev['configs'].items():
            # Count instances of each class type
            itf_counts = dict((c, 0) for c in INTERFACE_CLASSES)
            for itf_tag, itf in cfg['interfaces'].items():
                itf_counts[itf['class']] += 1
                classdefs[itf['class']].append(('Device', itf_tag))

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
                desc_c += readTemplate('device/hid_report.c', vars)

            # Emit Configuration descriptors
            desc_idx = get_string_idx(cfg.get('description'))
            if 'attributes' in cfg:
                attr = " | ".join([CONFIG_ATTRIBUTES[a] for a in cfg['attributes']])
            else:
                attr = 0
            power = cfg['power']
            desc_fields = [
                ('Config number', cfg_num),
                ('interface count', 'ITF_NUM_TOTAL'),
                ('string index', desc_idx),
                ('total length', 'CONFIG_TOTAL_LEN'),
                ('attribute', attr),
                ('Power in mA', power)
            ]
            descriptors = [('TUD_CONFIG_DESCRIPTOR', desc_fields)]
            itf_num = 0
            itfnum_defs = []
            ep_num = 1
            epnum_defs = []

            def EP_OUT(name, value):
                return (f"EPNUM_{name}_OUT", value)

            def EP_IN(name, value):
                return (f"EPNUM_{name}_IN", value | 0x80)

            for itf_tag, itf in cfg['interfaces'].items():
                itf_class = itf['class']
                info = INTERFACE_CLASSES[itf_class]
                itf_id = make_identifier(itf_tag)
                itfnum_defs += [(itf_id, itf_num)]
                itf_desc_idx = get_string_idx(itf.get('description'))
                desc_fields = [
                    ('Interface number', f"ITF_NUM_{itf_id}"),
                    ('String index', itf_desc_idx),
                ]
                if itf_class == 'hid':
                    ep1 = EP_IN(itf_id, ep_num)
                    ep_num += 1
                    epnum_defs += [ep1]
                    ep_bufsize = itf.get('bufsize', HID_DEFAULT_EP_BUFSIZE)
                    poll_interval = itf['poll-interval']
                    protocol = f"HID_ITF_PROTOCOL_{itf['protocol'].upper()}"
                    desc_fields += [
                        ('Protocol', protocol),
                        ('Report descriptor len', f"sizeof(desc_{itf_tag}_report)"),
                        ('EP IN Address', ep1[0]),
                        ('Size', ep_bufsize),
                        ('Polling interval', poll_interval),
                    ]
                    descriptors += [('TUD_HID_DESCRIPTOR', desc_fields)]

                elif itf_class == 'cdc':
                    ep1 = EP_IN(f'{itf_id}_NOTIF', ep_num)
                    ep_num += 1
                    ep2 = EP_OUT(itf_id, ep_num)
                    ep3 = EP_IN(itf_id, ep_num)
                    ep_num += 1
                    epnum_defs += [ep1, ep2, ep3]
                    desc_fields += [
                        ('EP notify address', ep1[0]),
                        ('EP notify size', 8),
                        ('EP data OUT', ep2[0]),
                        ('EP data IN', ep3[0]),
                        ('EP data size', 64),
                    ]
                    itf_num += 1  # IAD specifies 2 interfaces
                    descriptors += [('TUD_CDC_DESCRIPTOR', desc_fields)]

                elif itf_class == 'msc':
                    ep1 = EP_OUT(itf_id, ep_num)
                    ep2 = EP_IN(itf_id, ep_num)
                    ep_num += 1
                    epnum_defs += [ep1, ep2]
                    desc_fields += [
                        ('EP OUT', ep1[0]),
                        ('EP IN', ep2[0]),
                        ('EP Size', 64),
                    ]
                    descriptors += [('TUD_MSC_DESCRIPTOR', desc_fields)]

                else:
                    raise InputError(f'Unsupported class "{itf_class}"')

                itf_num += 1

            itfnum_defs += [("TOTAL", itf_num)]

            config_desc = ""
            for name, df in descriptors:
                config_desc += '  // ' + ", ".join(name for (name, value) in df) + '\n'
                config_desc += f'  {name}(' + ", ".join(str(value) for (name, value) in df) + '),\n'
            vars = {
                'itfnum_defs': "\n".join(f"  ITF_NUM_{name} = {value}," for (name, value) in itfnum_defs),
                'config_total_len': 'TUD_CONFIG_DESC_LEN' + "".join(f" + TUD_{itf['class'].upper()}_DESC_LEN" for itf in cfg['interfaces'].values()),
                'epnum_defs': "\n".join(f"  {name} = 0x{value:02x}," for (name, value) in epnum_defs),
                'config_desc': config_desc,
            }
            desc_c += readTemplate('device/interface.c', vars)

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
        desc_c += readTemplate('device/string.c', vars)

        classes = ""
        for c, n in itf_counts.items():
            classes += f"#define CFG_TUD_{make_identifier(c)}\t{n}\n"
        cfg_vars['device_classes'] = classes
        cfg_vars['hid_ep_bufsize'] = hid_ep_bufsize

        vars = {
            'hid_report_ids': indent(hid_report_ids),
        }
        desc_h = readTemplate('device/desc.h', vars)

        if ep_num > 16:
            raise InputError(f'Too many endpoints ({ep_num})')

        write_file(output_dir, 'usb_descriptors.c', desc_c)
        write_file(output_dir, 'usb_descriptors.h', desc_h)


def parse_host(config, cfg_vars, classdefs, output_dir):
    host_enabled = 0
    classes = ""

    if 'host' in config:
        host_enabled = 1
        class_counts = dict((c, 0) for c in HOST_CLASSES)
        for tag, dev in config['host'].items():
            class_counts[dev['class']] += 1
            classdefs[dev['class']].append(('HostDevice', tag))
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
    classdefs = dict((c, []) for c in INTERFACE_CLASSES | HOST_CLASSES)
    parse_devices(config, cfg_vars, classdefs, args.output)
    parse_host(config, cfg_vars, classdefs, args.output)

    include_txt = ""

    class_types = set()
    for dev_class, devs in classdefs.items():
        for clsname, _ in devs:
            class_types.add(f'{dev_class.upper()}/{clsname}')

    txt = ""
    for dev_class, devs in classdefs.items():
        if not devs:
            continue
        include_txt += f'#include <USB/{dev_class.upper()}/{clsname}'
        for clsname, tag in devs:
            txt += f'extern {dev_class.upper()}::{clsname} {tag};\n'
    vars = {
        'includes': "\n".join(f'#include <USB/{c}.h>' for c in class_types),
        'classdefs': txt
    }
    classdefs_h = readTemplate('classdefs.h', vars)
    write_file(args.output, 'usb_classdefs.h', classdefs_h)

    txt = ""
    for dev_class, devs in classdefs.items():
        if not devs:
            continue
        for inst, (clsname, tag) in enumerate(devs):
            txt += f'{dev_class.upper()}::{clsname} {tag}({inst}, "{tag}");\n'
        txt += f'namespace {dev_class.upper()} {{\n'
        txt += '  void* devices[] {' + ', '.join(f'&{tag}' for _, tag in devs) + '};\n'
        txt += '}\n'
    vars = {
        'classdefs': txt
    }
    classdefs_cpp = readTemplate('classdefs.cpp', vars)
    write_file(args.output, 'usb_classdefs.cpp', classdefs_cpp)

    config_h = readTemplate('config.h', cfg_vars)
    write_file(args.output, 'tusb_config.h', config_h)


if __name__ == '__main__':
    try:
        main()
    except InputError as e:
        print("** ERROR! %s" % e, file=sys.stderr)
        sys.exit(2)
