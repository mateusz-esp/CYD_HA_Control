"""Render the actual firmware views on a desktop (g++, Pillow required)."""
from pathlib import Path
import subprocess
import tempfile
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='cyd-ui-preview-') as temporary:
    binary = str(Path(temporary) / 'preview')
    subprocess.run(['g++', '-std=c++11', '-Wall', '-Wextra', '-Itest/fakes', '-Iinclude',
                    'tools/render_ui_preview.cpp', 'src/ui_views.cpp', 'src/ui_font.cpp',
                    '-o', binary], cwd=root, check=True)
    subprocess.run([binary], cwd=root, check=True)

board = Image.new('RGB', (1020, 830), '#e6eaf0')
font_path = Path('/usr/share/fonts/TTF/FiraSans-Medium.ttf')
font = ImageFont.truetype(str(font_path), 18) if font_path.exists() else ImageFont.load_default(size=18)
names = ['home', 'climate', 'boiler', 'options']
titles = ['Panel główny', 'Klimatyzacja', 'Bojler', 'Wybór ustawienia']
for i, name in enumerate(names):
    ppm = root / 'docs/previews' / (name + '.ppm')
    with Image.open(ppm) as source:
        source.save(ppm.with_suffix('.png'))
        image = source.resize((480, 360), Image.Resampling.NEAREST)
    x, y = 20 + (i % 2) * 500, 38 + (i // 2) * 410
    board.paste(image, (x, y))
    ImageDraw.Draw(board).text((x, y - 27), titles[i], font=font, fill='#27384d')
    ppm.unlink()
board.save(root / 'docs/previews/overview.png')
