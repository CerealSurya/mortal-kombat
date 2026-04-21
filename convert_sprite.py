from PIL import Image
import sys
import os

def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def convert(input_path, var_name, transparent_color=(255, 0, 255)):
    img = Image.open(input_path).convert("RGB")
    width, height = img.size

    print(f"// {os.path.basename(input_path)} — {width}x{height}")
    print(f"SPRITE_ARRAY {var_name}[] = {{")

    # BMP order: bottom row first, so we iterate rows in reverse
    pixels = []
    for row in range(height - 1, -1, -1):
        for col in range(width):
            r, g, b = img.getpixel((col, row))

            # Replace your transparent color (default magenta 255,0,255)
            # with the SPRITE_TRANSPARENT_COLOR key
            if (r, g, b) == transparent_color:
                pixels.append(0xF81F)
            else:
                pixels.append(rgb888_to_rgb565(r, g, b))

    # Print 8 values per line for readability
    for i, p in enumerate(pixels):
        if i % 8 == 0:
            print("    ", end="")
        print(f"0x{p:04X}", end="")
        if i < len(pixels) - 1:
            print(", ", end="")
        if i % 8 == 7:
            print()

    print("\n};")
    print()

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 convert_sprite.py sprite.bmp VAR_NAME")
        print("Example: python3 convert_sprite.py char1_idle.bmp CHAR1_IDLE")
        sys.exit(1)

    convert(sys.argv[1], sys.argv[2])