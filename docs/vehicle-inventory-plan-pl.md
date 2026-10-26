# Plan: inwentarz pojazdów (transport przedmiotów)

Polska wersja dokumentu [vehicle-inventory-plan.md](vehicle-inventory-plan.md).

Status: **tylko plan**, niewdrożony (2026-10-26, użytkownik w trybie Remote
Control). Branch `Vehicles`. Tło: [vehicles-current-implementation-pl.md](vehicles-current-implementation-pl.md).

## 1. Cel

1. Pojazdy mogą służyć do transportu przedmiotów.
2. Inwentarz pojazdu otwiera się jak u najemnika; przedmioty przenosi się do
   jego slotów tak samo jak u najemnika.
3. Pojazdy korzystają z tej samej konfiguracji i panelu inwentarza co najemnicy.
4. Pojazdy mają wyższy limit przedmiotów w małym / dużym slocie.

## 2. Ustalenia (analiza kodu)

- Pojazd ma już pełny inwentarz: jego `SOLDIERTYPE` ma te same sloty `inv[]`
  co najemnik i zapisuje się z drużyną — **bez zmiany formatu zapisu**.
- Inwentarz na ekranie mapy jest zamknięty dla pojazdów w jednym miejscu:
  `MapCharacterHasAccessibleInventory()` (`MapScreen.cc`) odrzuca
  `IsMechanical(s)`, co jest prawdą dla pojazdów **i** robota.
- Wszystko inne działa ogólnie dla każdej postaci: otwieranie panelu,
  przenoszenie przedmiotów między postaciami (`MapscreenCanPassItemToChar` — ten
  sam sektor, nie w drodze, brak wrogów), branie z inwentarza sektora,
  układanie w stosy.
- Pojazd zniszczony w taktyce zostawia przedmioty na ziemi jak poległy
  najemnik (`Rotting_Corpses.cc`) — ładunek nie przepada.
- Panel już ukrywa u pojazdów wagę / pancerz / kamuflaż; waga nie ma znaczenia
  (pojazd nie chodzi).
- Panel SM w taktyce jest zamknięty dla ciał innych niż ludzkie
  (`Interface.cc`, `!IS_MERC_BODY_TYPE`).
- Limity slotów w danych: prawie wszystkie przedmioty mają `ubPerPocket` 100
  (limit inwentarza sektora), a limity najemnika jako nadpisania
  `ubBigPerPocket` 8 / `ubSmallPerPocket` 4; przedmioty z `ubPerPocket` 0 to
  jedna sztuka na slot i nie mieszczą się w małej kieszeni; bronie 0–1 na slot
  oprócz noża bojowego 2, kamieni 2–3, noży do rzucania 4, kastetu 4.
- Limit slotu w inwentarzu postaci liczą tylko trzy miejsca: `PlaceObject`
  (`Items.cc`), automatyczne układanie (`AutoPlaceObject`, `Items.cc`),
  okienko dzielenia stosu (`Interface_Items.cc`, `InitItemStackPopup`). Inne
  miejsca sprawdzają tylko „czy w ogóle się mieści” (limit 0 lub nie).

## 3. Decyzje (2026-10-26)

| # | Pytanie | Decyzja |
|---|---|---|
| V1 | Mnożnik limitu najemnika dla pojazdów | **×4** na chwilę obecną |
| V2 | Jeden wspólny mnożnik czy osobny dla pojazdu? | **osobny dla każdego pojazdu**, pole w `vehicles.json` |
| V3 | Limity pieniędzy | **bez zmian** także dla pojazdów (100000 duża / 50000 mała kieszeń) |
| V4 | Wykluczone przedmioty | **wszystkie bronie (`IC_WEAPON`) i kastet (`IC_PUNCH`)** zostają przy limicie najemnika; na razie bez innych wyjątków |

Otwarte (propozycje z analizy, jeszcze bez decyzji):

| # | Pytanie | Propozycja |
|---|---|---|
| V5 | Gdzie można otworzyć inwentarz pojazdu? | tylko na ekranie mapy (panel SM w taktyce to osobna, większa zmiana) |
| V6 | Które sloty? | tylko kieszenie; dłonie i sloty pancerza/głowy zablokowane (bez efektu dla pojazdu, przedmioty w dłoni mogłyby wpływać na taktykę) |
| V7 | Uszkodzony pojazd (stan < `OKLIFE` 15) | inwentarz nadal dostępny (dzisiejsza reguła by go zamknęła) |

## 4. Projekt

### 4.1 Dane

`vehicles.json`, nowe opcjonalne pole dla każdego pojazdu:

```
"slotLimitMultiplier": 4
```

`VehicleModel` dostaje `uint8_t slot_limit_multiplier` (domyślnie 1, gdy pola
brak = limity najemnika). Ustawić 4 dla Hummera i furgonetki z lodami (i
pozostałych pojazdów naziemnych); helikopter nie ma inwentarza (brak
`SOLDIERTYPE`).

### 4.2 Jedna funkcja limitu dla slotu postaci

Nowa funkcja w `Items.cc`:

```
UINT8 SoldierSlotLimit(SOLDIERTYPE const& s, UINT16 usItem, INT8 bSlot)
{
	UINT8 limit = min(ApplyBigPerPocketOverride(usItem, bSlot, ItemSlotLimit(usItem, bSlot)), MAX_OBJECTS_PER_SLOT);
	if (s jest pojazdem && limit > 1 && !(klasa przedmiotu & (IC_WEAPON | IC_PUNCH)))
		limit = min(limit * mnożnik_tego_pojazdu, MAX_OBJECTS_PER_SLOT);
	return limit;
}
```

- `limit > 1`: przedmioty po jednej sztuce na slot (albo 0 = nie do tej
  kieszeni) zostają bez zmian, więc „nie mieści się w małej kieszeni” zostaje.
- Bronie i kastet: limit najemnika (V4).
- Górny limit 100 (`MAX_OBJECTS_PER_SLOT`).
- `MoneySlotLimit` bez zmian (V3).

Efekt dla typowego przedmiotu 8/4: najemnik 8 / 4, pojazd ×4 **32 / 16**.

### 4.3 Miejsca przełączone na `SoldierSlotLimit`

1. `PlaceObject` (`Items.cc`, `ubSlotLimit` liczony na początku).
2. Sprawdzenie układania w stos w `AutoPlaceObject` (`Items.cc`, porównanie z
   `ItemSlotLimit(...)` dla przedmiotów innych niż pieniądze).
3. `InitItemStackPopup` (`Interface_Items.cc`, `ubLimit`).

Bez zmian: `CanItemFitInPosition` (sprawdza tylko limit 0), kod inwentarza
sektora, handlarze, `Inventory_Choosing` (wyposażenie wrogów/milicji),
sprawdzenia dłoni na mapie/w taktyce (tylko > 0).

### 4.4 Otwieranie inwentarza

- `MapCharacterHasAccessibleInventory`: dopuścić pojazdy (robot nadal
  zablokowany), wg V5–V7.
- Ograniczenie slotów (V6): `CanItemFitInPosition` odrzuca dłonie i sloty
  pancerza/głowy dla pojazdu; panel może je wyszarzyć.
- Sprawdzić asercje wołające `MapCharacterHasAccessibleInventory`
  (`MapScreen.cc`) i logikę włączania przycisku inwentarza.

## 5. Etapy

1. **Dostęp do inwentarza** (ekran mapy): zmiana warunku + ograniczenie slotów.
   Test: otworzyć inwentarz Hummera, przenieść przedmioty od najemnika i z
   inwentarza sektora, pojechać do innego sektora, wyjąć je; zapis/odczyt.
2. **Limity pojazdów**: `slotLimitMultiplier`, `SoldierSlotLimit`, trzy
   miejsca wywołań. Test: 32 magazynki w dużej i 16 w małej kieszeni Hummera;
   najemnik nadal 8/4; bronie i kastet nadal przy limicie najemnika; limity
   pieniędzy bez zmian; zniszczyć pojazd w taktyce i sprawdzić, że przedmioty
   spadają na ziemię.

Rozmiar: mały–średni; bez zmiany formatu zapisu.
