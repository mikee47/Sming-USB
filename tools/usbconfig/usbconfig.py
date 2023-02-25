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
from dataclasses import dataclass

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

HID_PROTOCOLS = ['none', 'keyboard', 'mouse']
HID_REPORTS = ['keyboard', 'mouse', 'consumer', 'system-control', 'gamepad', 'fido-u2f', 'generic-inout']
HID_DEFAULT_EP_BUFSIZE = 64


@dataclass
class ClassItem:
    """Manages device or host interface definition"""
    dev_class: str
    code_class: str
    tag: str
    is_host: bool

    def namespace(self):
        return make_identifier(self.dev_class)


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
    return s.replace('-', '_').replace(' ', '_').upper()


def indent(defs):
    return f"  {defs}\n" if isinstance(defs, str) else "\n".join(f"  {d}" for d in defs)


def parse_devices(config, cfg_vars, classdefs, output_dir):
    if 'devices' not in config:
        cfg_vars['device_enabled'] = 0
        cfg_vars['device_classes'] = ''
        cfg_vars['hid_ep_bufsize'] = 0
        return

    templates = json_load(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'interfaces.json'))

    cfg_vars['device_enabled'] = 1

    # Build list of descriptor strings
    strings = []

    def add_string(itf_tag: str, name: str, value):
        if isinstance(value, dict):
            value = value.get(name, "")
        """Add a string to the descriptor list and return its index identifier"""
        id = f'STRING_INDEX_{itf_tag.upper()}_{make_identifier(name)}'
        strings.append((id, value))
        return id

    desc_c = ""
    # Device descriptors

    for tag, dev in config['devices'].items():
        vars = dict([(key, value) for key, value in dev.items() if not isinstance(value, dict)])
        vars['device_tag'] = tag
        vars['class_id'] = DEVICE_CLASSES[dev['class']]
        vars['subclass_id'] = DEVICE_SUBCLASSES[dev['subclass']]
        vars['protocol_id'] = DEVICE_PROTOCOLS[dev['protocol']]
        vars['vendor_id'] = dev['vendor_id']
        ver = round(float(dev['version']) * 100)
        vars['version_bcd'] = f"0x{ver:04}"
        vars['manufacturer_idx'] = add_string(tag, 'manufacturer', dev)
        vars['product_idx'] = add_string(tag, 'product', dev)
        vars['serial_idx'] = add_string(tag, 'serial', dev)
        vars['config_count'] = len(dev['configs'])
        desc_c += readTemplate('device/desc.c', vars)

    # Configuration descriptors

    for dev in config['devices'].values():
        cfg_num = 1
        for cfg_tag, cfg in dev['configs'].items():
            # Count instances of each class type
            itf_counts = {}
            for itf_tag, itf in cfg['interfaces'].items():
                template_tag = itf['template']
                itf_class = templates[template_tag]['class']
                itf_counts[itf_class] = itf_counts.get(itf_class, 0) + 1
                classdefs.append(ClassItem(itf_class, 'Device', itf_tag, False))

            # Emit HID report descriptors
            hid_inst = 0
            hid_report = ""
            hid_report_list = []
            hid_report_ids = set()
            hid_ep_bufsize = 0
            for itf_tag, itf in cfg['interfaces'].items():
                template_tag = itf['template']
                itf_class = templates[template_tag]['class']
                if itf_class != 'hid':
                    continue
                hid_ep_bufsize = max(hid_ep_bufsize, itf.get('bufsize', HID_DEFAULT_EP_BUFSIZE))
                report_name = f"desc_{itf_tag}_report"
                hid_report += f'static const uint8_t {report_name}[] = {{\n'
                for r in itf['reports']:
                    if r in HID_REPORTS:
                        id = make_identifier(r)
                        hid_report_ids.add(f"REPORT_ID_{id},")
                        hid_report += indent(f"TUD_HID_REPORT_DESC_{id} (HID_REPORT_ID(REPORT_ID_{id}) ),")
                    else:
                        raise InputError(f'Unknown report "{r}"')
                hid_report += '};\n\n'
                hid_report_list.append(report_name)
                hid_inst += 1

            if hid_inst:
                vars = {
                    'report': hid_report,
                    'report_list': ", ".join(hid_report_list)
                }
                desc_c += readTemplate('device/hid_report.c', vars)

            # Emit Configuration descriptors
            if 'attributes' in cfg:
                attr = " | ".join([CONFIG_ATTRIBUTES[a] for a in cfg['attributes']])
            else:
                attr = 0
            power = cfg['power']
            desc_fields = [
                ('Config number', cfg_num),
                ('interface count', 'ITF_NUM_TOTAL'),
                ('string index', add_string(cfg_tag, 'description', cfg)),
                ('total length', 'CONFIG_TOTAL_LEN'),
                ('attribute', attr),
                ('Power in mA', power)
            ]
            descriptors = [('TUD_CONFIG_DESCRIPTOR', desc_fields)]
            itf_num = 0
            itfnum_defs = []
            config_total_len = 'TUD_CONFIG_DESC_LEN'
            ep_num = 1
            epnum_defs = []
            ep_num_in = 0
            ep_num_out = 0

            for itf_tag, itf in cfg['interfaces'].items():
                template_tag = itf['template']
                template = templates[template_tag]
                itf_class = template['class']
                itf_id = make_identifier(itf_tag)
                itfnum_defs.append((itf_id, itf_num))
                desc_fields = [
                    ('Interface number', f"ITF_NUM_{itf_id}"),
                ]
                config_total_len += " + " + eval('f"' + template.get('desc-len', 'TUD_{make_identifier(template_tag)}_DESC_LEN') + '"')
                for name, value in template['desc-fields'].items():
                    if value == '@': # Endpoint number
                        uname = name.upper()
                        if not name.startswith('EP '):
                            raise InterfaceError(f"Bad endpoint name '{name}', must start with 'EP'")
                        if uname.endswith(' IN'):
                            if ep_num == ep_num_in:
                                ep_num += 1
                            ep_num_in = ep_num
                            ep = ep_num | 0x80
                        elif uname.endswith(' OUT'):
                            if ep_num == ep_num_out:
                                ep_num += 1
                            ep = ep_num_out = ep_num
                        else:
                            raise InterfaceError(f"Bad endpoint name '{name}', must end with 'IN' or 'OUT'")
                        value = f"EPNUM_{itf_id}_{uname[3:].replace(' ', '_')}"
                        epnum_defs.append((value, ep))
                    elif isinstance(value, str):
                        if value.startswith('$'): # String index
                            value = eval('f"' + value[1:] + '"')
                            if value.startswith('['):
                                for i, s in enumerate(eval(value)):
                                    id = add_string(itf_tag, f"{name}{i}", s)
                                    if i == 0:
                                        value = id
                            else:
                                value = add_string(itf_tag, name, value)
                        else:
                            value = eval('f"' + value + '"')
                    desc_fields.append((name, value))

                descriptors.append((template['desc-name'], desc_fields))
                itf_num += template.get('itf_count', 1)

            itfnum_defs.append(("TOTAL", itf_num))

            config_desc = ""
            for name, df in descriptors:
                config_desc += '  // ' + ", ".join(name for (name, value) in df) + '\n'
                config_desc += f'  {name}(' + ", ".join(str(value) for (name, value) in df) + '),\n'
            vars = {
                'itfnum_defs': "\n".join(f"  ITF_NUM_{name} = {value}," for (name, value) in itfnum_defs),
                'config_total_len': config_total_len,
                'epnum_defs': "\n".join(f"  {name} = 0x{value:02x}," for (name, value) in epnum_defs),
                'config_desc': config_desc,
            }
            desc_c += readTemplate('device/interface.c', vars)

        max_string_len = 0
        string_ids = []
        string_data = []
        for tag, value in strings:
            string_ids.append(f'{tag},')
            if value:
                encoded = value.encode('utf-16le')
                max_string_len = max(max_string_len, len(encoded))
                desc_array = bytes([2 + len(encoded), TUSB_DESC_STRING]) + encoded
                string_data.append('"' + "".join(f"\\x{c:02x}" for c in desc_array) + '"')
            else:
                string_data.append('NULL')

        vars = {
            'max_string_len': max_string_len,
            'string_data': ",\n".join(f'  // {tag}: "{value}"\n'
                                      f'  {data}' for (tag, value), data in zip(strings, string_data)),
        }
        desc_c += readTemplate('device/string.c', vars)

        classes = ""
        for c, n in itf_counts.items():
            classes += f"#define CFG_TUD_{make_identifier(c)} {n}\n"
        cfg_vars['device_classes'] = classes
        cfg_vars['hid_ep_bufsize'] = hid_ep_bufsize

        vars = {
            'string_ids': indent(string_ids),
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
        class_counts = {}
        for tag, dev in config['host'].items():
            dev_class = dev['class']
            class_counts[dev_class] = class_counts.get(dev_class, 0) + 1
            classdefs.append(ClassItem(dev['class'], 'HostDevice', tag, True))
        for c, n in class_counts.items():
            classes += f"#define CFG_TUH_{make_identifier(c)} {n}\n"

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
    classdefs = []
    parse_devices(config, cfg_vars, classdefs, args.output)
    parse_host(config, cfg_vars, classdefs, args.output)

    code_classes = set(f'{item.namespace()}/{item.code_class}' for item in classdefs)
    vars = {
        'includes': "\n".join(f'#include <USB/{c}.h>' for c in code_classes),
        'classdefs': "".join(f'extern {item.namespace()}::{item.code_class} {item.tag};\n' for item in classdefs),
    }
    classdefs_h = readTemplate('classdefs.h', vars)
    write_file(args.output, 'usb_classdefs.h', classdefs_h)

    class_types = {}
    for item in classdefs:
        class_types.setdefault(item.namespace(), []).append(item)
    txt = ""
    for ns, items in class_types.items():
        for inst, item in enumerate(items):
            txt += f'{ns}::{item.code_class} {item.tag}({inst}, "{item.tag}");\n'
        txt += f'namespace {ns} {{\n'
        devices = ', '.join(f'&{item.tag}' for item in items if not item.is_host)
        if devices:
            txt += f'  void* devices[] {{{devices}}};\n'
        host_devices = ', '.join(f'&{item.tag}' for item in items if item.is_host)
        if host_devices:
            txt += f'  void* host_devices[] {{{host_devices}}};\n'
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
