# STI Editor

Narzędzie developerskie (Python + Pillow + Tk) do odczytu, podglądu, analizy
i edycji plików `.sti` (STCI -- Sir-Tech's Crazy Image) używanych przez JA2
Stracciatella. Jest niezależne od silnika: niczego nie kompiluje, nie
modyfikuje kodu JA2 i nie jest częścią buildu.

| Plik | Rola |
|---|---|
| `sti.py` | parser/zapis formatu STI, ETRLE, eksport do Pillow/PNG |
| `drawing.py` | maski kształtów (ołówek, linia, prostokąt, elipsa, wypełnienie, tekst) |
| `sti_editor.py` | desktopowy edytor (GUI, Tk) |
| `sti_tool.py` | wersja wiersza poleceń: info, eksport, analiza, piksel, round-trip |
| `tests/test_sti.py` | testy (`unittest`) |

## Wymagania

- Python 3.9+ z Tk (standardowy instalator Pythona dla Windows zawiera Tk),
- Pillow (testowane na Pillow 12.3, Python 3.14, Tk 9.0).

Nie są potrzebne żadne dodatkowe biblioteki.

## Uruchomienie

Z katalogu głównego repozytorium (na Windows: launcher `py`):

```
py -3 tools/sti_editor/sti_editor.py
py -3 tools/sti_editor/sti_editor.py ścieżka/do/pliku.sti
```

Wiersz poleceń:

```
py -3 tools/sti_editor/sti_tool.py info      plik.sti [--frames]
py -3 tools/sti_editor/sti_tool.py analyze   plik.sti [--frame N]
py -3 tools/sti_editor/sti_tool.py pixel     plik.sti X Y [--frame N]
py -3 tools/sti_editor/sti_tool.py export    plik.sti KATALOG [--frame N] [--mask]
py -3 tools/sti_editor/sti_tool.py roundtrip plik1.sti [plik2.sti ...]
```

`roundtrip` niczego nie zapisuje na dysk: serializuje plik w pamięci i
sprawdza, czy bajty są identyczne, a następnie wymusza ponowną kompresję
wszystkich klatek i porównuje każdy piksel, maskę, pozycję, paletę i dane aux.

Testy:

```
py -3 -m unittest discover -s tools/sti_editor/tests -v
```

Testy czytają wszystkie pliki `.sti` z `assets/` (tylko odczyt -- na końcu
sprawdzają sumy SHA-256, że żaden nie został zmieniony). Zapisy testowe idą do
katalogów tymczasowych.

## Funkcje edytora

- **Plik:** Otwórz, Zapisz, Zapisz jako, Nowy z PNG, eksport bieżącej klatki
  do PNG, eksport wszystkich klatek do PNG, eksport maski klatki, import PNG do
  bieżącej klatki. Przed zapisem plik jest weryfikowany ponownym odczytem.
- **Klatki (multi-page):** ⏮ / ◀ Poprz. / Nast. ▶ / ⏭, pole *Nr obr.* (Enter),
  PageUp/PageDown, strzałki ←/→; duplikowanie, nowa pusta klatka, usuwanie,
  czyszczenie klatki.
- **Informacje:** rozmiar bieżącej klatki, pozycja X/Y (edytowalna,
  `sOffsetX/sOffsetY`), flagi, rozmiar z nagłówka, liczba klatek, dane aux.
- **Odtwarzanie:** ▶ Odtwórz / ⏸ Pauza (spacja) z regulowanym czasem klatki --
  wyłącznie podgląd w edytorze, zob. ograniczenia.
- **Paleta:** 256 kolorów; klik = wybór koloru, dwuklik = edycja koloru
  palety, przycisk *Przezroczysty*. Najechanie pokazuje indeks i RGB.
- **Narzędzia:** ołówek (P), linia (L), prostokąt (R), elipsa (E) -- obrys lub
  wypełnione, wypełnienie (F), tekst (T), pipeta (I, także prawy przycisk
  myszy przy każdym narzędziu), gumka/przezroczystość (X); rozmiar pędzla
  1--16, rozmiar czcionki.
- **Widok:** zoom 1×--32× (Ctrl+/Ctrl-, Ctrl+kółko), tło: szachownica / czarne
  / magenta / białe, podgląd maski przezroczystości, siatka pikseli (zoom ≥ 6).
- **Undo/Redo:** Ctrl+Z / Ctrl+Y, do 100 kroków (piksele, pozycje, aux,
  paleta, dodawanie/usuwanie klatek). Cofnięcie wszystkich zmian przywraca
  oryginalne bajty pliku.
- **Analiza:** statystyki klatki (piksele przezroczyste/nieprzezroczyste,
  bbox, liczba użytych indeksów, najczęstsze kolory, rozmiar ETRLE),
  informacje o pliku, pasek stanu z x/y, indeksem i RGB piksela pod kursorem.

## Format STI w JA2 (źródło: kod silnika)

Na podstawie `src/sgp/ImgFmt.h`, `src/sgp/STCI.cc`, `src/sgp/HImage.h`,
`src/game/Utils/STIConvert.cc` i `src/sgp/VObject_Blitters.cc`.
Little-endian, wyrównanie struktur jak w C.

```
STCIHeader            64 B
paleta                uiNumberOfColours × 3 B (RGB)        -- tylko STCI_INDEXED
STCISubImage[]        usNumberOfSubImages × 16 B             -- tylko STCI_ETRLE_COMPRESSED
dane obrazu           uiStoredSize B
dane aplikacji        uiAppDataSize B (AuxObjectData[], 16 B na klatkę)
```

- `STCIHeader`: `cID "STCI"`, `uiOriginalSize`, `uiStoredSize`,
  `uiTransparentValue`, `fFlags`, `usHeight`, `usWidth`, unia 20 B (indeksowany:
  `uiNumberOfColours`, `usNumberOfSubImages`, głębie R/G/B; RGB: maski i głębie
  kanałów), `ubDepth` (offset 44), 3 B wyrównania, `uiAppDataSize` (offset 48),
  12 B nieużywane. Bajty wyrównania i nieużywane są zachowywane dosłownie.
- Flagi: `TRANSPARENT 0x01`, `ALPHA 0x02`, `RGB 0x04`, `INDEXED 0x08`,
  `ZLIB 0x10`, `ETRLE 0x20`.
- `STCISubImage` (klatka): `uiDataOffset`, `uiDataLength`, `sOffsetX`,
  `sOffsetY` (INT16, dodawane do pozycji blitu), `usHeight`, `usWidth`.
- **ETRLE:** każda linia to ciąg bajtów sterujących zakończony bajtem `0x00`;
  bit 7 = 1 → tyle pikseli przezroczystych (1--127), bit 7 = 0 → tyle
  następujących po nim indeksów palety.
- `AuxObjectData`: `ubWallOrientation`, `ubNumberOfTiles`, `usTileLocIndex`,
  3 B, `ubCurrentFrame`, `ubNumberOfFrames`, `fFlags` (`FULL_TILE`,
  `ANIMATED_TILE`, `DYNAMIC_TILE`, `INTERACTIVE_TILE`, `IGNORES_HEIGHT`,
  `USES_LAND_Z`), 6 B.

### Zapis bez utraty danych

- Klatki, których nie zmieniono, są zapisywane z oryginalnymi bajtami ETRLE;
  jeśli nie zmieniono nic, plik jest identyczny bajt w bajt (sprawdzone na
  wszystkich 31 plikach `.sti` w `assets/`, 163 524 klatki).
- Zmienione klatki są kompresowane algorytmem z `STIConvert.cc`
  (`ETRLECompress`). Na plikach z repozytorium ponowna kompresja każdej
  klatki daje dokładnie oryginalne bajty.
- Przezroczystość jest przechowywana jako osobna maska, a nie jako „indeks 0”.
  Silnik rysuje każdy piksel z serii dosłownej, także o indeksie 0 -- a takie
  nieprzezroczyste piksele o indeksie 0 występują w prawdziwych plikach
  (np. `inventory-graphic-not-found-*.sti`). Edytor je zachowuje.
- Paleta, pozycje X/Y, `AuxObjectData`, nierozpoznane dane aplikacji i
  ewentualne bajty za końcem pliku są zachowywane.

## Ograniczenia (świadomie niesymulowane)

- **Key Frame:** format STI nie ma takiej informacji. Jedyne dane animacji to
  `AuxObjectData` (`ubNumberOfFrames`, `ubCurrentFrame`, flaga
  `ANIMATED_TILE`) -- edytor je pokazuje i pozwala je zmienić, gdy plik je ma.
- **Czas klatki / Pause:** STI nie przechowuje czasu klatki (tempo animacji
  ustala kod gry). Odtwarzanie i jego prędkość działają tylko jako podgląd
  w edytorze i nie są zapisywane.
- **Przezroczystość:** tylko binarna (piksel jest albo nie jest przezroczysty);
  STI indeksowane nie ma kanału alfa ani półprzezroczystości.
- **Pliki RGB (16 bpp, `STCI_RGB`):** tylko podgląd, analiza i eksport PNG;
  zapis zachowuje dane bez zmian. W repozytorium nie ma takich plików.
- **Indeksowane bez ETRLE:** odczyt, podgląd i eksport; edycja wyłączona
  (silnik tworzy obiekty wideo tylko z plików ETRLE).
- **ZLIB:** nieobsługiwany -- tak jak w silniku (`STCI.cc` odrzuca takie pliki).
- **Edycja palety** zmienia kolor we wszystkich klatkach (paleta jest wspólna
  dla pliku).
- **Rozmiar z nagłówka** (`usWidth/usHeight`): w plikach wieloklatkowych
  opisuje arkusz źródłowy, a nie klatkę, więc nie jest zmieniany. W plikach
  jednoklatkowych, w których odpowiadał klatce, jest aktualizowany razem z nią
  (tak jak zapisuje go `WriteSTIFile`).
- **Import PNG / Nowy z PNG:** kolory są mapowane na najbliższy kolor palety
  (dokładne dopasowania zostają dokładne); piksele z alfą < 128 stają się
  przezroczyste. Przy imporcie nieprzezroczyste piksele nie dostają indeksu 0,
  bo narzędzia zapisujące JA2 (`STIConvert.cc`) traktują go jako przezroczysty.
- **Tekst** jest rysowany domyślną czcionką Pillow (bez antyaliasingu), nie
  czcionkami gry.
- **Zoom** jest ograniczony tak, by powiększony obraz nie przekroczył ok.
  24 mln pikseli (pamięć).
