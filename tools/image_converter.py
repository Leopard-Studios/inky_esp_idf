#! venv/bin/python

# Currently only 7 colour displays / uc8159 is supported

from argparse import ArgumentParser
from PIL import Image
import pyarrow as pa

DESATURATED_PALETTE = [
    [0, 0, 0],
    [255, 255, 255],
    [0, 255, 0],
    [0, 0, 255],
    [255, 0, 0],
    [255, 255, 0],
    [255, 140, 0],
    [255, 255, 255]
]

SATURATED_PALETTE = [
    [57, 48, 57],
    [255, 255, 255],
    [58, 91, 70],
    [61, 59, 94],
    [156, 72, 75],
    [208, 190, 71],
    [177, 106, 73],
    [255, 255, 255]
]

def _palette_blend(saturation, dtype="uint8"):
    saturation = float(saturation)
    palette = []
    for i in range(7):
        rs, gs, bs = [c * saturation for c in SATURATED_PALETTE[i]]
        rd, gd, bd = [c * (1.0 - saturation) for c in DESATURATED_PALETTE[i]]
        if dtype == "uint8":
            palette += [int(rs + rd), int(gs + gd), int(bs + bd)]
        if dtype == "uint24":
            palette += [(int(rs + rd) << 16) | (int(gs + gd) << 8) | int(bs + bd)]
    if dtype == "uint8":
        palette += [255, 255, 255]
    if dtype == "uint24":
        palette += [0xFFFFFF]
    return palette

def parseargs():
    parser = ArgumentParser("Inky Image Converter")
    parser.add_argument( "IMAGE", help="Image to be converted" )
    parser.add_argument( "OUTPUT", help="File to output result to." )
    parser.add_argument( "-name", "-n", default="image", help="Var name for the image array" )
    parser.add_argument( "-colours", "-c", type=int, help="Number of colours the display has" )
    parser.add_argument( "-dimensions", "-d", nargs=2, type=int, help="Dimensions of output image" )
    # parser.add_argument( "Colours", "-c", nargs=2, type=int, help="Number of colours the display has" )
    return parser.parse_args()

def main():
    print("Image converter")
    args = parseargs()
    print(args)
    im = Image.open(args.IMAGE)
    print(im.format, im.size, im.mode)
    w,h=im.size
    palette = _palette_blend(0.5)
    # Image size doesn't matter since it's just the palette we're using
    palette_image = Image.new("P", (1, 1))
    # Set our 7 colour palette (+ clear) and zero out the other 247 colours
    palette_image.putpalette(palette + [0, 0, 0] * 248)
    # Force source image data to be loaded for `.im` to work
    im.load()
    
    im = im.im.convert("P", True, palette_image.im)

    WIDTH = 10
    row = 0
    with open(args.OUTPUT, 'w') as f:

        
        f.write(f"#define WIDTH_{args.name.upper()} {w}\n")
        f.write(f"#define HEIGHT_{args.name.upper()} {h}\n")
        # f.write(f"#define WIDTH{args.name}[] \n")

        f.write(f"const uint8_t {args.name}[] = {{\n\t")
        for b in im:
            f.write(f"0x0{b}, ")
            row+=1
            if row == WIDTH:
                row=0
                f.write("\n\t")
        f.write("};")

if __name__ == '__main__':
    main()
