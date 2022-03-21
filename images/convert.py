#!/usr/bin/env python3

import sys
import json
import re

def error():
    print('provide file name and image title')
    exit(1)


if len(sys.argv) < 3:
    error()

try:
    title = sys.argv[2]
    with open(sys.argv[1], "r") as fp:
        imgraw = fp.read()
except:
    error()

img = {}
img["width"] = int(re.search(r"static unsigned int width = (\d+);", imgraw).group(1))
img["height"] = int(re.search(r"static unsigned int height = (\d+);", imgraw).group(1))
img["cmap"] = []
for v in re.findall(r"\{\s*(\d+),\s*(\d+),\s*(\d+)},", imgraw):
    img["cmap"].append(v)

tmp = re.findall(r"static char header_data\[\] = \{(.*)\};", imgraw, re.DOTALL)[0]
tmp = "[{}]".format(tmp.replace('\\n\\t', ''))

img["data"] = json.loads(tmp)

# with open("{}.raw.json".format(title), "w+") as fp:
#     json.dump(img, fp, indent=2)

imgout = "\n#include <lvgl.h>\n\nstatic uint8_t "+ title + "_data[] = {\n    "

for i in range(16):
    imgout += "0x{:02X}, ".format(int(img["cmap"][i][2]))
    imgout += "0x{:02X}, ".format(int(img["cmap"][i][1]))
    imgout += "0x{:02X}, ".format(int(img["cmap"][i][0]))
    imgout += "0xFF,\n    "

tmp = 0
for i,v in enumerate(img["data"]):
    if i % 2 == 0:
        tmp += v << 4
    else:
        tmp += v << 0
        imgout += "0x{:02X}, ".format(tmp)
        if i % 32 == 31:
            imgout += "\n    "
        tmp = 0

imgout += "};\n\n"

imgout += \
"static lv_img_dsc_t {2} = {{\n\
    .header = {{\n\
        .cf = LV_IMG_CF_INDEXED_{3}BIT,\n\
        .always_zero = 0,\n\
        .reserved = 0,\n\
        .w = {0},\n\
        .h = {1},\n\
    }},\n\
    .data_size = {3}*{0}*{1}/8 + {3}*{3}*4,\n\
    .data = {2}_data\n\
}};\n".format(img["width"], img["height"], title, 4)

with open("../include/{}.h".format(title), "w+") as fp:
    fp.write(imgout)
