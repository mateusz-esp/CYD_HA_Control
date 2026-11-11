"""Generate compact 4-bit glyph coverage from installed OFL Fira Sans fonts.
Usage: python3 scripts/generate_fonts.py /usr/share/fonts/TTF
Requires Pillow. Generated C++ is committed; no generation is needed to build.
"""
from pathlib import Path
import sys
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parents[1]
source = Path(sys.argv[1] if len(sys.argv) > 1 else '/usr/share/fonts/TTF')
characters = ''.join(sorted(set(''.join(chr(i) for i in range(32, 127)) + 'ąćęłńóśźżĄĆĘŁŃÓŚŹŻ°·…')))
output = ['// Generated from Fira Sans (SIL OFL 1.1); see assets/fonts/OFL.txt.\n#pragma once\n']
for name, size, weight in [('Small',10,'Regular'),('Body',12,'Medium'),('Label',14,'Medium'),('Title',20,'Medium'),('Number',32,'Medium')]:
    font = ImageFont.truetype(str(source / f'FiraSans-{weight}.ttf'), size)
    chars = ' -.0123456789°' if name == 'Number' else characters
    data, glyphs = [], []
    for char in chars:
        box = font.getbbox(char, anchor='ls')
        x0,y0,x1,y1=box; w,h=x1-x0,y1-y0
        im=Image.new('L',(max(1,w),max(1,h)))
        ImageDraw.Draw(im).text((-x0,-y0),char,font=font,fill=255,anchor='ls')
        values=[round(v*15/255) for v in im.tobytes()] if w and h else []
        offset=len(data)
        if len(values)%2:values.append(0)
        data.extend((values[i]<<4)|values[i+1] for i in range(0,len(values),2))
        glyphs.append((ord(char),offset,w,h,round(font.getlength(char)),x0,y0))
    output.append(f'const uint8_t {name}Pixels[] PROGMEM = {{\n')
    output.extend('  '+','.join(str(v) for v in data[i:i+24])+',\n' for i in range(0,len(data),24))
    output.append('};\n'+f'const Glyph {name}Glyphs[] PROGMEM = {{\n')
    output.extend('  {'+','.join(str(v) for v in glyph)+'},\n' for glyph in glyphs)
    output.append('};\n')
    ascent,descent=font.getmetrics()
    output.append(f'const Font {name} = {{{name}Pixels, {name}Glyphs, {len(glyphs)}, {ascent}, {ascent+descent}}};\n')
(root/'assets/fonts/fira_generated.h').write_text(''.join(output))
print('Generated Fira Sans glyphs:', (root/'assets/fonts/fira_generated.h').stat().st_size, 'bytes of source')
