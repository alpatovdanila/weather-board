import os
from PIL import Image

# === CONFIGURATION ===
SRC_DIR = "assets/weather_icons"     # folder containing .png files
OUT_DIR = "include/weather_icons"       # output folder for .h files

# OpenWeatherMap → your icon name mapping
ApiIconsToIcons = {
    '01d': 'Sun',
    '01n': 'Moon',
    '02d': 'Cloud',
    '02n': 'Cloud',
    '03d': 'Cloud',
    '03n': 'Cloud',
    '04d': 'Cloud',
    '04n': 'Cloud',
    '09d': 'CloudRain',
    '09n': 'CloudRain',
    '10d': 'CloudDrizzle',
    '10n': 'CloudDrizzle',
    '11d': 'CloudLightning',
    '11n': 'CloudLightning',
    '13d': 'CloudSnow',
    '13n': 'CloudSnow',
    '50d': 'Droplet',
    '50n': 'Droplet'
}

os.makedirs(OUT_DIR, exist_ok=True)


def image_to_1bit_array(image_path):
    """Convert an image (with possible alpha) to 1-bit monochrome and return byte array."""
    img = Image.open(image_path).convert("RGBA")
    width, height = img.size
    pixels = img.load()

    data = bytearray()
    for y in range(height):
        byte = 0
        bit_count = 0
        for x in range(width):
            r, g, b, a = pixels[x, y]
            if a < 128:
                # transparent pixel → white (off)
                bit = 0
            else:
                # solid pixel → black (on) if dark enough
                luminance = 0.299 * r + 0.587 * g + 0.114 * b
                bit = 1 if luminance < 128 else 0

            byte = (byte << 1) | bit
            bit_count += 1
            if bit_count == 8:
                data.append(byte)
                byte = 0
                bit_count = 0

        if bit_count > 0:
            data.append(byte << (8 - bit_count))

    return data, width, height

def save_as_header(name, data, width, height, out_path):
    """Save 1-bit image data as a C header file."""
    array_name = name.lower()
    with open(out_path, "w") as f:
        f.write(f"// Auto-generated from {name}.png\n")
        f.write("#pragma once\n\n")
        f.write(f"#define {array_name}_width {width}\n")
        f.write(f"#define {array_name}_height {height}\n\n")
        f.write(f"const unsigned char {array_name}_bits[] = {{\n")

        for i, byte in enumerate(data):
            if i % 12 == 0:
                f.write("    ")
            f.write(f"0x{byte:02X}, ")
            if (i + 1) % 12 == 0:
                f.write("\n")
        if len(data) % 12 != 0:
            f.write("\n")
        f.write("};\n")
    print(f"✅ Created {out_path}")


# === Convert PNGs → .h files ===
for code, name in ApiIconsToIcons.items():
    src = os.path.join(SRC_DIR, f"{name}.png")
    dst = os.path.join(OUT_DIR, f"{name}.h")
    if not os.path.exists(src):
        print(f"⚠️ Missing: {src}")
        continue
    data, w, h = image_to_1bit_array(src)
    save_as_header(name, data, w, h, dst)


# === Generate root icons.h ===
master_path = os.path.join(OUT_DIR, "icons.h")
with open(master_path, "w") as f:
    f.write("// Auto-generated icon map\n#pragma once\n#include <Arduino.h>\n\n")

    # Include all icon headers
    for name in sorted(set(ApiIconsToIcons.values())):
        f.write(f'#include "{name}.h"\n')

    f.write("\n")
    f.write("typedef struct {\n")
    f.write("    const unsigned char *data;\n")
    f.write("    int width;\n")
    f.write("    int height;\n")
    f.write("} IconEntry;\n\n")

    f.write("static const struct {\n")
    f.write("    const char *code;\n")
    f.write("    IconEntry icon;\n")
    f.write("} ApiIconsToIcons[] = {\n")

    for code, name in ApiIconsToIcons.items():
        f.write(f'    {{"{code}", {{{name.lower()}_bits, {name.lower()}_width, {name.lower()}_height}}}},\n')

    f.write("};\n\n")

    # Helper: getIconByCode (const char*)
    f.write("inline const IconEntry* getIconByCode(const char *code) {\n")
    f.write("    for (size_t i = 0; i < sizeof(ApiIconsToIcons) / sizeof(ApiIconsToIcons[0]); i++) {\n")
    f.write("        if (strcmp(ApiIconsToIcons[i].code, code) == 0)\n")
    f.write("            return &ApiIconsToIcons[i].icon;\n")
    f.write("    }\n")
    f.write("    return nullptr;\n")
    f.write("}\n\n")

    # Helper: getIconByCode (String)
    f.write("inline const IconEntry* getIconByCode(const String &code) {\n")
    f.write("    return getIconByCode(code.c_str());\n")
    f.write("}\n")

print(f"\n🗂️ Master header generated: {master_path}")
