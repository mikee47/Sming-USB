#!/usr/bin/env python3
#
# Sming hardware configuration tool
#

import common
import argparse
import os
import json
import string
import ast
from common import *
from dataclasses import dataclass

TUSB_DESC_STRING = 3

@dataclass
class ClassItem:
    """Manages device or host interface definition"""
    dev_class: str
    code_class: str
    tag: str
    is_host: bool

    def namespace(self):
        return make_id(self.dev_class)


@dataclass
class StringItem:
    """Contains an entry in the string table"""

    def __init__(self, id: str, value: str):
        self.id = id  # Enumerated string ID
        self.value = value  # UTF-8 string value
        self.data = value.encode('utf-16le') if value else ""

    def data_str(self):
        if not self.value:
            return 'NULL'
        desc_array = bytes([2 + len(self.data), TUSB_DESC_STRING]) + self.data
        return '"' + "".join(f"\\x{c:02x}" for c in desc_array) + '"'


class Template(string.Template):
    idpattern = r'(?a:[_a-z][_\.-a-z0-9]*)'


def write_file(dirname, filename, content):
    path = os.path.join(dirname, filename)
    status(f'Creating "{path}"')
    os.makedirs(dirname, exist_ok=True)
    with open(path, 'w') as f:
        f.write(content)


def resolve_path(name):
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), name)


def readTemplate(name, vars):
    pathname = resolve_path(f'templates/{name}')
    with open(pathname) as f:
        tmpl = string.Template(f.read())
    return tmpl.substitute(vars)


def make_id(s):
    return s.replace('-', '_').replace(' ', '_').upper()


def indent(defs):
    return f"  {defs}\n" if isinstance(defs, str) else "\n".join(f"  {d}" for d in defs)


def parse_devices(config, cfg_vars, classdefs, output_dir):
    if 'devices' not in config:
        cfg_vars['device_enabled'] = 0
        cfg_vars['device_classes'] = '// none'
        cfg_vars['device_globals'] = '// none'
        return

    defs = json_load(resolve_path('schema/base.json'))['$defs']
    templates = json_load(resolve_path('schema/interfaces.json'))

    cfg_vars['device_enabled'] = 1

    # Build list of descriptor strings
    strings = OrderedDict()

    def add_string(itf_tag: str, name: str, value):
        if isinstance(value, dict):
            value = value.get(name, "")
        """Add a string to the descriptor list and return its index identifier"""
        id = f'STRING_INDEX_{itf_tag.upper()}_{make_id(name)}'
        strings[id] = StringItem(id, value)
        return id

    globals = {}
    desc_c = ""
    # Device descriptors

    for tag, dev in config['devices'].items():
        # Fill out default property values
        for prop_tag, prop in defs['Device']['properties'].items():
            dev.setdefault(prop_tag, prop.get('default'))

        vars = dict([(key, value) for key, value in dev.items() if not isinstance(value, dict)])
        vars['device_tag'] = tag
        clsid = make_id(dev['class'])
        vars['class_id'] = f"TUSB_CLASS_{clsid}"
        subclass = dev['subclass']
        vars['subclass_id'] = f"{clsid}_SUBCLASS_{make_id(subclass)}" if subclass else 0
        protocol = dev['protocol']
        vars['protocol_id'] = f"{clsid}_PROTOCOL_{make_id(protocol)}" if protocol else 0
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
            # Fill out default property values
            for prop_tag, prop in defs['Config']['properties'].items():
                cfg.setdefault(prop_tag, prop.get('default'))
            # Count instances of each class type
            itf_counts = {}
            for itf_tag, itf in cfg['interfaces'].items():
                template_tag = itf['template']
                itf_class = templates[template_tag]['class']
                itf_counts[itf_class] = itf_counts.get(itf_class, 0) + 1
                classdefs.append(ClassItem(itf_class, 'Device', itf_tag, False))
                # Fill out default property values
                template = templates[itf['template']]
                for prop_tag, prop in template['properties'].items():
                    itf.setdefault(prop_tag, prop.get('default'))

            # Emit HID report descriptors and build list of DFU alternate IDs
            hid_inst = 0
            hid_report = ""
            hid_report_list = []
            hid_report_ids = set()
            hid_ep_bufsize = 0
            dfu_alternate_ids = []
            for itf_tag, itf in cfg['interfaces'].items():
                template_tag = itf['template']
                itf_class = templates[template_tag]['class']

                if itf_class == 'dfu':
                    dfu_alternate_ids = [f"DFU_ALTERNATE_{make_id(a)}," for a in itf['alternates']]
                elif itf_class == 'hid':
                    bufsize = itf['ep-size']
                    hid_ep_bufsize = max(hid_ep_bufsize, bufsize)
                    report_name = f"desc_{itf_tag}_report"
                    hid_report += f'static const uint8_t {report_name}[] = {{\n'
                    for r in itf['reports']:
                        id = make_id(r)
                        hid_report_ids.add(f"REPORT_ID_{id},")
                        args = [f"HID_REPORT_ID(REPORT_ID_{id})"]
                        if id in ['GENERIC_INOUT', 'FIDO_U2F']:
                            args.insert(0, bufsize)
                        hid_report += indent(f"TUD_HID_REPORT_DESC_{id} ({', '.join(str(a) for a in args)}),")
                    hid_report += '};\n\n'
                    hid_report_list.append(report_name)
                    hid_inst += 1

            if hid_inst:
                globals['HID_EP_BUFSIZE'] = hid_ep_bufsize
                vars = {
                    'report': hid_report,
                    'report_list': ", ".join(hid_report_list)
                }
                desc_c += readTemplate('device/hid_report.c', vars)

            # Emit Configuration descriptors
            attr = cfg.get('attributes')
            attr = " | ".join([f"TUSB_DESC_CONFIG_ATT_{make_id(a)}" for a in attr]) if attr else 0
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
                properties = {
                    'tag': itf_tag,
                    'description': itf.get('description', ''),
                    'template.id': make_id(template_tag),
                }
                for prop_tag, prop in template['properties'].items():
                    prop_type = prop.get('type', 'string')
                    value = itf[prop_tag]
                    properties[prop_tag] = value
                    if prop_type == 'string':
                        properties[f"{prop_tag}.id"] = make_id(value)
                    if prop_type == 'array':
                        properties[f"{prop_tag}.length"] = len(value)
                        mask = prop.get('mask')
                        if mask:
                            properties[f"{prop_tag}.mask"] = ' | '.join(f'{mask}{make_id(a)}' for a in value)
                    if prop.get('global', False):
                        global_tag = make_id(f"{template['class']}_{prop_tag}")
                        if value != globals.get(global_tag, value):
                            raise InputError(f"Duplicate global '{prop_tag}' defined for device '{itf_tag}'")
                        globals[global_tag] = value

                def evaluate(value):
                    tmpl = Template(value)
                    return tmpl.substitute(properties)

                itf_class = template['class']
                itf_id = make_id(itf_tag)
                itfnum_defs.append((itf_id, itf_num))
                desc_fields = [
                    ('Interface number', f"ITF_NUM_{itf_id}"),
                ]
                config_total_len += " + " + evaluate(template.get('desc-len', 'TUD_${template.id}_DESC_LEN'))
                for name, value in template['fields'].items():
                    if value == '@':  # Endpoint number
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
                        if value.startswith('!'):  # String index
                            value = evaluate(value[1:])
                            if value.startswith('['):
                                value = ast.literal_eval(value)
                                for i, s in enumerate(value):
                                    id = add_string(itf_tag, f"{name}{i}", s)
                                    if i == 0:
                                        value = id
                            else:
                                value = add_string(itf_tag, name, value)
                        else:
                            value = evaluate(value)
                    desc_fields.append((name, value))

                descriptors.append((f'TUD_{make_id(template_tag)}_DESCRIPTOR', desc_fields))
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

        vars = {
            'max_string_len': max(len(item.data) for item in strings.values()),
            'string_data': ",\n".join(f'  // {item.id}: "{item.value}"\n'
                                      f'  {item.data_str()}' for item in strings.values()),
        }
        desc_c += readTemplate('device/string.c', vars)

        cfg_vars['device_classes'] = "\n".join(f"#define CFG_TUD_{make_id(name)} ({value})" for name, value in itf_counts.items())
        cfg_vars['device_globals'] = "\n".join(f"#define CFG_TUD_{make_id(name)} ({value})" for name, value in globals.items())
        # cfg_vars['hid_ep_bufsize'] = hid_ep_bufsize

        vars = {
            'string_ids': indent([f'{id},' for id in strings]),
            'hid_report_ids': indent(hid_report_ids),
            'dfu_alternate_ids': indent(dfu_alternate_ids),
        }
        desc_h = readTemplate('device/desc.h', vars)

        if ep_num > 16:
            raise InputError(f'Too many endpoints ({ep_num})')

        write_file(output_dir, 'usb_descriptors.c', desc_c)
        write_file(output_dir, 'usb_descriptors.h', desc_h)


def parse_host(config, cfg_vars, classdefs, output_dir):
    if not 'host' in config:
        cfg_vars['host_enabled'] = 0
        cfg_vars['host_classes'] = '// none'
        cfg_vars['host_globals'] = '// none'
        return

    config = config['host']
    properties = json_load(resolve_path('schema/base.json'))['$defs']['HostInterface']['properties']
    globals = {}
    for dev_class, cfg in config.items():
        classdefs.append(ClassItem(dev_class, 'HostDevice', dev_class, True))
        for prop_tag, prop in properties[dev_class]['properties'].items():
            value = cfg.setdefault(prop_tag, prop.get('default'))
            if not prop.get('global', False):
                continue
            global_tag = make_id(f"{dev_class}_{prop_tag}")
            globals[global_tag] = value

    cfg_vars['host_enabled'] = 1
    cfg_vars['host_classes'] = "\n".join(f"#define CFG_TUH_{make_id(dev_class)} {cfg['count']}" for dev_class, cfg in config.items())
    cfg_vars['host_globals'] = "\n".join(f"#define CFG_TUH_{make_id(name)} {value}" for name, value in globals.items())


def load_schema():
    schema = json_load(resolve_path('schema/base.json'))
    itf_defs = schema['$defs']['Interfaces']['oneOf']
    interfaces = json_load(resolve_path('schema/interfaces.json'))
    for tmpl_tag, tmpl in interfaces.items():
        tmpl.update({
            "type": "object",
            "additionalProperties": False,
            "required": ["template"] + [tag for tag, prop in tmpl['properties'].items() if 'default' not in prop],
        })
        tmpl["properties"].update({
            "template": {"const": tmpl_tag},
            "description": {"type": "string"}
        })
        itf_defs.append(tmpl)
    return schema


def validate_config(config):
    # Validate configuration against schema
    try:
        from jsonschema import Draft7Validator
        schema = load_schema()
        json_save(schema, resolve_path('../../schema.json'))
        v = Draft7Validator(schema)
        errors = sorted(v.iter_errors(config), key=lambda e: e.path)
        if errors != []:
            for e in errors:
                critical("%s @ %s" % (e.message, e.path))
            sys.exit(3)
    except ImportError as err:
        critical("\n** WARNING! %s: Cannot validate '%s', please run `make python-requirements **\n\n" %
                 (str(err), args.input))


def main():
    parser = argparse.ArgumentParser(description='Sming USB configuration utility')

    parser.add_argument('--quiet', '-q', help="Don't print non-critical status messages to stderr", action='store_true')
    parser.add_argument('input', help='Path to configuration file')
    parser.add_argument('output', help='Output directory')

    args = parser.parse_args()

    common.quiet = args.quiet

    config = json_load(args.input)
    validate_config(config)

    cfg_vars = {}
    classdefs = []
    parse_devices(config, cfg_vars, classdefs, args.output)
    parse_host(config, cfg_vars, classdefs, args.output)

    code_classes = set(f'{item.namespace()}/{item.code_class}' for item in classdefs)
    vars = {
        'includes': "\n".join(f'#include <USB/{c}.h>' for c in code_classes),
        'classdefs': "\n".join(f'extern {item.namespace()}::{item.code_class} {item.tag};' for item in classdefs if not item.is_host),
    }
    classdefs_h = readTemplate('classdefs.h', vars)
    write_file(args.output, 'usb_classdefs.h', classdefs_h)

    class_types = {}
    for item in classdefs:
        if not item.is_host:
            class_types.setdefault(item.namespace(), []).append(item)
    if class_types:
        txt = ""
        for ns, items in class_types.items():
            for inst, item in enumerate(items):
                txt += f'{ns}::{item.code_class} {item.tag}({inst}, "{item.tag}");\n'
            txt += f'namespace {ns} {{\n'
            devices = ", ".join(f'&{item.tag}' for item in items)
            if devices:
                txt += f'  void* devices[] {{{devices}}};\n'
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
