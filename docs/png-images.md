# Grafiki PNG

Gra wczytuje grafiki także z plików PNG, obok STI i PCX. Ten dokument opisuje,
jak przygotować takie pliki i gdzie je umieścić. Kod: `src/sgp/PNG.cc`,
`src/sgp/HImage.cc` (`CreateImage()`), `src/sgp/VObject*.cc`.

## Jak podmienić grafikę

**Plik obok oryginału.** Plik `nazwa.png` w tym samym katalogu co
`nazwa.sti` lub `nazwa.pcx` jest wczytywany zamiast niego. Nie trzeba zmieniać
JSON ani kodu. PNG może leżeć w katalogu moda (`mods/<mod>/data/...`), luźno w
katalogu `Data` gry albo w SLF.

PNG wygrywa tylko wtedy, gdy leży w tej samej albo wyższej warstwie VFS niż
oryginał. Kolejność warstw (od najwyższej): katalog domowy, mody, `externalized`,
`Data`, pliki SLF. Przykłady:

- PNG w modzie zastępuje STI z `Interface.slf`;
- luźny PNG w `Data/Interface` zastępuje STI z `Interface.slf`;
- PNG w `Data` **nie** zastępuje STI dostarczonego przez mod.

**Ścieżka w JSON.** Tam, gdzie grafika jest wskazana w danych
(`items.json`, `weapons.json`, `loading-screens.json` itd.), można wpisać ścieżkę
do pliku `.png`.

Podmiany nie dotyczą kafli, animacji postaci ani kursorów (używają danych
dodatkowych, których PNG nie ma).

### Gdy PNG nie da się użyć

Jeśli plik PNG obok oryginału jest uszkodzony albo nie pasuje do miejsca
użycia, w logu gry pojawia się wpis
`Cannot use <plik>.png instead of <plik>.sti, loading the original: <powód>`
i gra wczytuje oryginał. PNG wskazany wprost w JSON nie ma oryginału, więc
błąd zatrzymuje wczytanie tej grafiki.

### Wyłączenie podmian

W `ja2.json` (np. `%APPDATA%\JA2\ja2.json`):

```json
"image_png_override": false
```

wyłącza wczytywanie PNG obok oryginałów. Gra wygląda wtedy dokładnie jak
bez plików PNG. Pliki `.png` wskazane wprost w JSON są nadal wczytywane.

## Rodzaje PNG

### PNG z paletą (zalecany dla grafik gry)

Odpowiada STI z paletą 1:1. Indeksy palety są zachowane, więc działają wszystkie
efekty gry: cieniowanie, oświetlenie, szare twarze poległych, kolory czcionek.

- **Indeks 0 jest zawsze przezroczysty**, tak jak w STI.
- Indeks z alfą poniżej 128 w chunku `tRNS` też jest przezroczysty. Alfa od 128
  do 254 jest rysowana jako pełna, z ostrzeżeniem w logu.
- **Indeks 254** ma takie znaczenie jak w STI: w grafikach przedmiotów to piksele
  obrysu (np. amunicji kompatybilnej), w kaflach i postaciach cień.
- Głębia 1, 2, 4 lub 8 bitów, z przeplotem lub bez. Paleta może mieć mniej niż 256
  wpisów (reszta jest czarna).
- Program graficzny **nie może przestawiać ani usuwać wpisów palety**
  (wiele programów to robi przy zapisie, np. przy „optymalizacji palety”).
  Najbezpieczniej eksportować z STI narzędziem
  `tools/sti_editor/sti_tool.py png-sheet`.

### PNG w pełnym kolorze (RGBA, RGB, szarości)

- Jako **obiekt** (ikony, elementy interfejsu): rysowany z mieszaniem alfy.
  Alfa 0 jest pomijana, 255 rysowana wprost, wartości pośrednie mieszane z tłem.
  Czysta czerń pozostaje widoczna.
- **Obrys** (np. przedmiotu kompatybilnego) jest rysowany automatycznie wokół
  nieprzezroczystych pikseli (alfa co najmniej 128): na przezroczystych pikselach
  stykających się z nimi z lewej, prawej, góry lub dołu. Obrys musi zmieścić się w
  klatce, więc zostaw 1 piksel przezroczystego marginesu. Wyłączenie obrysu:
  `"outline": false` w metadanych (niżej).
- Nie ma palety, więc nie działają efekty oparte na palecie. W tych miejscach
  gra używa oryginału zamiast PNG RGBA (z wpisem w logu):
  - portrety najemników i postaci IMP (cieniowanie, szare twarze poległych),
  - czcionki,
  - radar i znaczniki osób na mapie taktycznej,
  - grafika przycisku ogólnego `DEFAULT_GENERIC_BUTTON_OFF` (kolor tekstu
    przycisków).

  W tych miejscach można używać PNG z paletą.
- 16 bitów na kanał jest obcinane do 8 bitów. Ekran gry ma 16 bitów (RGB565), więc
  gładkie przejścia kolorów mogą mieć widoczne pasma.

### Tła (ekrany ładowania, pulpit laptopa, mapa strategiczna, tła okienek)

- PNG z paletą daje tło 8-bitowe z tymi samymi indeksami (`tRNS` jest pomijany,
  jak w PCX).
- PNG w pełnym kolorze daje tło 16-bitowe. Piksele z alfą poniżej 128 są
  przezroczyste tam, gdzie gra używa przezroczystości tła; czarne piksele
  pozostają widoczne.
- **Mapa strategiczna** (`interface/b_map_*.pcx`) musi być PNG z paletą, bo gra
  rysuje ją pomniejszoną i cieniowaną przez paletę. PNG w pełnym kolorze kończy
  się czytelnym błędem.

### Tła menu głównego i opcji początkowych w rozdzielczości gry

Oryginalne tła tych ekranów mają 640×480 i są rysowane na środku ekranu.
Tło na cały ekran dla bieżącej rozdzielczości gry to plik w tym samym
katalogu, z rozdzielczością w nazwie:

| Ekran | Oryginał | Tło dla 1366×768 |
|---|---|---|
| menu główne | `loadscreens/mainmenubackground.sti` (`Loadscreens.slf`) | `loadscreens/mainmenubackground_1366x768.png` |
| opcje początkowe (nowa gra) | `interface/optionsscreenbackground.sti` (`Interface.slf`) | `interface/optionsscreenbackground_1366x768.png` |

- Nazwa powstaje z bieżącej rozdzielczości, więc działa dla każdej rozdzielczości
  z presetów Standard i High Res, a także dla własnych. Wielkość liter nie ma
  znaczenia. Plik może leżeć w modzie albo luźno w `Data`.
- Szukane są kolejno `.png`, `.sti` i `.pcx` (STI musi być 16-bitowe RGB albo
  8-bitowe bez ETRLE).
- Obraz o innym rozmiarze niż ekran jest rozciągany do pełnego ekranu (najbliższy
  piksel, proporcje mogą się zmienić), z ostrzeżeniem w logu.
- Bez pliku dla bieżącej rozdzielczości (albo gdy nie da się go wczytać, z błędem w
  logu) ekran rysuje oryginał 640×480 jak dotąd.
- Przyciski, logo, napisy i przyciemniony prostokąt na ekranie opcji zostają w
  obszarze 640×480 na środku ekranu: przy rozdzielczości W×H zaczyna się on w
  punkcie ((W-640)/2, (H-480)/2). Tło trzeba projektować z myślą o tym.
- Tło jest 16-bitowe jak cały ekran gry, więc pełny kolor PNG jest wyświetlany w
  65 536 kolorach. Przezroczystość nie ma tu znaczenia (piksele z alfą poniżej 128
  są czarne).
- Pamięć: 2 bajty na piksel ekranu (np. 1366×768: 2 MB, 3840×2160: 16 MB,
  7680×4320: 66 MB), na czas pobytu na danym ekranie.

## Metadane: `nazwa.png.json`

Opcjonalny plik obok PNG. Bez niego cały obraz jest jedną klatką bez
przesunięcia i z obrysem.

Lista klatek (jak podobrazy STI, z przesunięciem rysowania):

```json
{
  "frames": [
    { "x": 0,  "y": 0, "w": 56, "h": 17, "offsetX": 1, "offsetY": 1 },
    { "x": 57, "y": 0, "w": 26, "h": 20, "offsetX": 1, "offsetY": 2 }
  ]
}
```

Siatka jednakowych komórek, od lewej do prawej i z góry na dół:

```json
{
  "grid": { "w": 32, "h": 24, "count": 10 },
  "offsets": [ [0, 0], [-1, 2], ... ]
}
```

- `offsetX`/`offsetY` są opcjonalne (domyślnie 0).
- `count` jest opcjonalne (domyślnie wszystkie komórki).
- `offsets` jest opcjonalne, a jeśli jest, musi mieć jeden wpis na klatkę.
- `"outline": false` wyłącza obrys grafiki w pełnym kolorze (dla PNG z paletą
  nie ma znaczenia). Może stać sam: `{ "outline": false }`.

Kolejność klatek musi odpowiadać kolejności podobrazów w STI, które PNG zastępuje
(gra odwołuje się do nich po numerze).

### Czas klatek animacji

Czas wyświetlania klatki w milisekundach (liczba całkowita 0–65535):

```json
{
  "frameDuration": 100,
  "frames": [
    { "x": 0,   "y": 0, "w": 248, "h": 110, "duration": 400 },
    { "x": 249, "y": 0, "w": 248, "h": 110 }
  ]
}
```

```json
{ "grid": { "w": 64, "h": 64, "count": 4 }, "durations": [80, 80, 120, 200] }
```

- `duration` w klatce z listy `frames`, `durations` (jeden wpis na klatkę) obok
  `grid`;
- `frameDuration` dotyczy klatek bez własnego czasu (także pojedynczej klatki
  bez `frames`/`grid`);
- `0` albo brak oznacza „bez zmiany”: animacja używa swojego dotychczasowego
  czasu z gry.

Czasy działają tylko w animacjach, które je obsługują. Gdzie indziej są pomijane i
animacja zachowuje tempo z gry (tak jak przy STI, które nie mają czasów klatek).
Animacje obsługujące czasy z PNG:

| Animacja | Plik | Uwagi |
|---|---|---|
| reklama kwiaciarni (AIM) | `laptop/flowerad_16.png` | 16 klatek; czas klatki określa, jak długo jest widoczna przed następną (domyślnie 150 ms); końcowe wyświetlenie klatki 0 z tekstem trwa jak dotąd |
| reklama „Twoja reklama” (AIM) | `laptop/yourad_13.png` | 13 klatek, domyślnie 150 ms |
| reklama ubezpieczeń (AIM) | `laptop/insurancead_10.png` | 10 klatek, domyślnie 150 ms |
| reklama zakładu pogrzebowego (AIM) | `laptop/funeralad_9.png` | 9 klatek, domyślnie 250 ms |
| reklama Bobby Ray's (AIM) | `laptop/bobbyrayad_21.png` | 21 klatek, odtwarzane jako 0–6, 0–6, 7–20; domyślnie 300 ms |

W reklamach AIM z czasami klatek każda klatka jest rysowana na tle strony
zapamiętanym przy jej rysowaniu, a nie na poprzedniej klatce, więc klatki mogą
mieć przezroczystość i półprzezroczystość. Klatka bez własnego czasu trwa
domyślny czas reklamy z tabeli. Bez czasów w PNG (i dla STI) reklamy działają
dokładnie jak dotąd, także z dłużej wyświetlaną pierwszą i ostatnią klatką;
z czasami o długości każdej klatki decyduje tylko `png.json`. Pliki reklam w
innych wersjach językowych mają inne nazwy (np. `german/yourad_13_german`).

## Narzędzia

Eksport STI do PNG z paletą z zachowaniem indeksów, klatek i przesunięć:

```
py -3 tools/sti_editor/sti_tool.py png-sheet plik.sti plik.png [--max-width N] [--duration MS]
```

Tworzy `plik.png` i, przy wielu klatkach, przesunięciach lub `--duration`,
`plik.png.json` (`--duration` zapisuje `frameDuration`).

Złożenie osobnych plików klatek (np. `explosion/000.png`, `001.png`, … w
kolejności nazw) w jeden arkusz z metadanymi:

```
py -3 tools/sti_editor/sti_tool.py png-assemble explosion --out explosion.png [--duration MS] [--durations 80,80,120]
```

Jeśli wszystkie klatki są PNG z tą samą paletą, arkusz też ma paletę (indeksy bez
zmian); w przeciwnym razie jest RGBA. Przesunięcia klatek są równe 0 i można je
potem zmienić w `explosion.png.json`. Gra zawsze wczytuje arkusz, nie osobne pliki
klatek. Szczegóły w `tools/sti_editor/README.md`.

## Ograniczenia

- Wymiary obrazu do 65535 pikseli w każdym kierunku i do 2^26 pikseli łącznie.
- Animacje postaci, kafle i kursory nadal wymagają STI.
- Grafika PNG z paletą zajmuje w pamięci tyle samo co STI, grafika w pełnym
  kolorze około 5 bajtów na piksel (RGBA i maska obrysu).

## Wydajność

Pomiar (build RelWithDebInfo, jedno wczytanie z pamięci podręcznej plików,
2026-10-02) na grafikach gry, test `PNGLoadTest.DISABLED_benchmark`:

| Grafika | Użycie | Oryginał | PNG | Pamięć oryginał | Pamięć PNG |
|---|---|---:|---:|---:|---:|
| `b_map_1024` (1460×1190, z paletą) | tło 8 bpp | 9,0 ms | 20,2 ms | 1697 KiB | 1697 KiB |
| `ls_dayomerta` (640×480, RGB) | tło 16 bpp | 0,2 ms | 10,7 ms | 600 KiB | 600 KiB |
| `gun01` (1 klatka, z paletą) | obiekt | 0,13 ms | 0,45 ms | 1,3 KiB | 1,3 KiB |
| `mdguns` (50 klatek, z paletą) | obiekt | 0,14 ms | 1,35 ms | 25 KiB | 25 KiB |
| `mdguns` (50 klatek, RGBA) | obiekt RGBA | 0,14 ms | 1,87 ms | 25 KiB | 207 KiB |

Sprawdzenie, czy obok grafiki leży PNG: około 7 µs przy pierwszym wczytaniu danej
ścieżki (przegląd warstw VFS), potem 0,5 µs (wynik jest zapamiętywany).

Wnioski:

- PNG wczytuje się wolniej niż STI/PCX (rozpakowanie zlib, kodowanie ETRLE
  albo konwersja do 16 bitów), ale to milisekundy na grafikę, płacone przy
  wczytaniu ekranu, nie przy każdej klatce rysowania.
- Rysowanie grafik z paletą jest identyczne jak STI. Grafiki RGBA są mieszane
  piksel po pikselu, co ma znaczenie tylko przy dużych grafikach rysowanych
  co klatkę.
- PNG z paletą zajmuje tyle samo pamięci co STI; RGBA około 8 razy więcej
  niż ETRLE.
- Gdy PNG nie ma, koszt to sprawdzenie raz na ścieżkę: przy kilkuset grafikach
  na ekranie to ułamki milisekundy przy pierwszym wczytaniu.

## Testy

- Testy jednostkowe: `ja2 -unittests` (grupy `PNG`, `ETRLE`, `PNGLoadTest`).
  Pliki testowe tworzy `tools/generate_png_unittest_data.py`.
- Pomiar czasu i pamięci (domyślnie wyłączony): pary `nazwa.png` i
  `nazwa.sti`/`nazwa.pcx` w `<katalog buildu>/unittests/data/pngbench/`, potem

  ```
  GTEST_ALSO_RUN_DISABLED_TESTS=1 GTEST_FILTER=PNGLoadTest.DISABLED_benchmark ja2 -unittests
  ```
