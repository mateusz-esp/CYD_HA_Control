# Fira Sans

Interfejs używa fontu Fira Sans (Regular i Medium), udostępnionego na
licencji SIL Open Font License 1.1. Pełna treść i informacje o autorach:
[OFL.txt](OFL.txt).

`fira_generated.h` zawiera podzbiór znaków z 4-bitową informacją o pokryciu
piksela. Obejmuje polskie znaki i symbol stopnia. Dane są wbudowane
w firmware; nie wymagają dodatkowego systemu plików na ESP32.

Ponowne wygenerowanie wymaga Pillow i źródłowych plików fontu:

```sh
python3 scripts/generate_fonts.py /ścieżka/do/katalogu/fontów
```

Standardowa kompilacja PlatformIO korzysta z gotowego pliku nagłówkowego.
