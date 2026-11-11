# Panel Home Assistant na CYD

Panel Arduino/PlatformIO dla **ESP32-2432S028R**, ekran ILI9341
320 × 240 w poziomie, dotyk rezystancyjny XPT2046. Trzy kafelki:
grzejnik, klimatyzacja i bojler. Inne warianty CYD (np. ekran ST7789
albo dotyk pojemnosciowy) wymagaja zmiany sterownika i konfiguracji.

Interfejs jest poziomy (320 x 240): trzy kafelki obok siebie, ikony grzejnika,
platka sniegu i bojlera, ciemne tlo i osobny kolor kazdego urzadzenia.
Aktywne urzadzenie ma podswietlony przycisk i obramowanie. Wskaznik Wi-Fi
opisuje polaczenie z siecia, a dostepnosc urzadzen wynika z odczytow HA.
Panel sklada obraz pasami RGB565 320x16 w buforze ok. 10 kB. Porownuje
podpisy fragmentow 16x16 i wysyla do LCD tylko zmienione obszary, laczac
sasiednie fragmenty. Nie czysci fizycznego ekranu przed aktualizacja.
Dotyczy to tez zmiany strony, komunikatow i odswiezania stanow z HA.
W razie braku pamieci na bufor pozostaje awaryjne rysowanie bezposrednie;
status alokacji jest widoczny w logu startowym. Zegar LCD pozostaje 10 MHz.

Oznaczenie TPM408-2.8 wystepuje na panelach CYD z ILI9341. Uzywana jest
alternatywna inicjalizacja `ILI9341_2_DRIVER`, stosowana w
[projekcie testowym dla tego oznaczenia](https://github.com/rockebest/ESP32-2432S028-LCD-Touch-Calibration/blob/main/TFT_eSPI_Setup/User_Setup_ESP32_2432S028.h).
Sam napis nie identyfikuje jednoznacznie kazdej wersji modulu.
Orientacje definiuje `PanelUI::ROTATION` w `include/panel_ui.h`: `1` oznacza
poziomo, `3` poziomo odwrocone o 180 stopni. Ekran i dotyk korzystaja
z tej samej wartosci. Po zmianie obrotu dostosuj zakresy kalibracji dotyku.

Po starcie monitor szeregowy 115200 pokazuje identyfikator `CYD-HA LCD2`,
czas kompilacji i logiczne wymiary LCD. Pozwala to potwierdzic, ze wgrano
nowa wersje. Wymiary 320x240 w logu opisuja konfiguracje programu;
nie dowodza, ze fizyczny panel poprawnie przyjal komende obrotu.

## Konfiguracja

Przykladowe encje (wlasne wpisz w lokalnym `include/config.h`):

| Kafelek | Encja sterujaca | Dzialanie |
| --- | --- | --- |
| Grzejnik | `climate.grzejnik` | `climate.turn_on` / `climate.turn_off` |
| Klimatyzacja | `climate.klimatyzacja` | `climate.turn_on` / `climate.turn_off` |
| Bojler | `switch.bojler_zasilanie` | `switch.turn_on` / `switch.turn_off` |

Bojler pokazuje `Grzeje` / `Nie grzeje` na podstawie
`binary_sensor.bojler_grzanie`. Wylaczony przelacznik
ma pierwszenstwo i daje opis `Wylaczone`. Temperatura `Zadana` pochodzi z
`sensor.bojler_temperatura_zadana` i jest tylko do odczytu.
Nie jest to aktualna temperatura wody. Czujniki sa odpytywane niezaleznie
od przelacznika; awaria czujnika nie blokuje sterowania zasilaniem bojlera.

Grzejnik i klimatyzacja pokazuja aktualny tryb (np. Grzanie, Chlodzenie) i temperatury
w kolejnosci **aktualna / zadana**, w jednostce ustawionej w Home Assistant.
Przycisk korzysta z wlaczania/wylaczania integracji; nie wymusza trybu cool
ani nie zmienia nastawy temperatury. Zachowanie wznowienia trybu zalezy od
integracji urzadzenia. Program nie wlacza urzadzen samodzielnie przy starcie.

### Ustawienia na ekranie

Dotknij gornej czesci dowolnego kafelka, aby otworzyc szczegoly.
Przycisk na dole kafelka nadal sluzy do wlaczania/wylaczania. W szczegolach
przycisk `Zasilanie` ma to samo dzialanie, a `< Wroc` wraca do panelu.

Temperature zadana zmieniaj przez `-` i `+`, nastepnie dotknij `Zapisz`.
Same przyciski `-`/`+` nie wysylaja polecenia. Powrot do ekranu glownego
porzuca niezapisana temperature. Polling HA nie nadpisuje edytowanej wartosci.
Zakres i krok sa odczytywane z HA (obecnie bojler 40..80 co 1, klimatyzacja
8..30 co 1, grzejnik 5..40 co 1). Gdy aktualne metadane sa niedostepne,
zapis jest zablokowany. Grzejnik ma wybor grzanie / wylaczony i regulacje
temperatury. Pola wentylatora, presetow i nawiewu sa ukryte, jesli urzadzenie
ich nie udostepnia. Akcje grzejnika i klimatyzacji trafiaja do osobnych encji.

Klimatyzacja udostepnia wybor trybu pracy, wentylatora, presetu oraz nawiewu
pionowego i poziomego. Dotknij pola i wybierz opcje z listy; wybor od razu
wysyla ustawienie do HA. Dluzsze listy maja przyciski stron. Dostepne opcje
pochodza z atrybutow encji i sa dodatkowo sprawdzane przez `supported_features`.

Temperatura bojlera jest ustawiana przez `water_heater.set_temperature` dla
`water_heater.bojler` (`BOILER_CONTROL_ENTITY` w konfiguracji).
Nie jest zmieniana encja maksymalnego limitu temperatury. Szczegoly bojlera
pokazuja aktualna temperature wody i pozwalaja tez wybrac obslugiwany tryb
pracy (obecnie reczny / program).

HTTP 200 oznacza przyjecie wywolania przez HA. Panel po poleceniu ponownie
odczytuje encje, a nie pokazuje wyslanej wartosci jako potwierdzonej.
Integracja lub urzadzenie moze zglosic nowy stan z opoznieniem. Brak aktualnych
danych lub utrata Wi-Fi blokuje ustawienia; polecenia nie sa ponawiane
automatycznie.

Test granic i kroku temperatury, bez zmieniania urzadzen:

```sh
g++ -std=c++11 -Iinclude test/control_rules_test.cpp -o /tmp/cyd-control-test
/tmp/cyd-control-test
g++ -std=c++11 -Iinclude test/dirty_tiles_test.cpp -o /tmp/cyd-dirty-tiles-test
/tmp/cyd-dirty-tiles-test
g++ -std=c++11 -Itest/fakes -Iinclude test/partial_display_test.cpp -o /tmp/cyd-partial-display-test
/tmp/cyd-partial-display-test
```

Edytuj `include/config.h`.
Jesli go brakuje, skopiuj `include/config.example.h` jako `include/config.h`.

1. Wpisz nazwe sieci Wi-Fi 2,4 GHz i haslo.
2. Wpisz adres swojego HA; `http://192.168.1.100:8123` jest przykladem.
3. W profilu uzytkownika Home Assistant, w sekcji bezpieczenstwa,
   utworz dlugoterminowy token dostepu i wpisz go w `HA_TOKEN`.
4. W tablicy `DEVICES` mozesz zmienic identyfikatory encji.
   Znajdziesz je w Narzedziach deweloperskich HA, zakladka Stany.
   Do tego czasu kafelki wyswietlaja `Brak encji` i nie wysylaja polecen.

Dla encji `switch` pozostaw `switch`, `turn_on`, `turn_off` i puste obiekty
JSON. Dla `climate` uzyj przykladu `set_hvac_mode` w konfiguracji.
Dla `water_heater` dobierz usluge i tryb do konkretnej integracji
(np. `set_operation_mode` z `operation_mode`). Sprawdz je najpierw
w Narzedziach deweloperskich HA, zakladka Akcje. Stan inny niz `off`
oznacza wlaczony tryb urzadzenia; nie jest pomiarem poboru mocy.


## Ekran i dotyk

Konfiguracja LCD w `platformio.ini`: SCLK 14, MOSI 13, MISO 12,
CS 15, DC 2, podswietlenie 21. Dotyk ma osobna magistrale:
SCLK 25, MOSI 32, MISO 39, CS 33, IRQ 36.

Jesli dotyk nie pokrywa sie z kafelkami, ustaw `TOUCH_DEBUG = true`,
otworz monitor 115200 i odczytaj x/y przy lewym gornym oraz prawym dolnym
rogu ekranu. Wpisz zakresy w `TOUCH_X_MIN/MAX` i `TOUCH_Y_MIN/MAX`.
Gdy os jest odwrocona, zamien jej MIN z MAX. Wgraj program ponownie.
Teksty ekranowe uzywaja ASCII, poniewaz wbudowane fonty nie maja
polskich znakow.

Zrodla: [REST API Home Assistant](https://developers.home-assistant.io/docs/api/rest/),
[konfiguracja sprzetowa CYD](https://github.com/BOlaerts/ESP32-2432s028),
[biblioteka dotyku autora](https://github.com/PaulStoffregen/XPT2046_Touchscreen).
