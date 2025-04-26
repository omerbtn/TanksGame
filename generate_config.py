#!/usr/bin/env python3

import sys

def main(input_file, output_file):
    with open(input_file, 'r') as f:
        lines = f.readlines()

    entries = []
    for line in lines:
        line = line.strip()
        if not line or line.startswith('#'):
            continue  # skip empty lines and comments
        if '=' not in line:
            continue  # skip invalid lines
        key, value = line.split('=', 1)
        entries.append((key.strip(), value.strip()))

    with open(output_file, 'w') as f:
        f.write('#pragma once\n\n')
        f.write('#include <array>\n')
        f.write('#include <string_view>\n\n')
        f.write('struct ConfigEntry {\n')
        f.write('    std::string_view key;\n')
        f.write('    std::string_view value;\n')
        f.write('};\n\n')
        f.write(f'constexpr std::array<ConfigEntry, {len(entries)}> config_entries{{{{\n')
        for key, value in entries:
            f.write(f'    {{"{key}", "{value}"}},\n')
        f.write('}};\n')

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input_config_file> <output_header_file>")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])
