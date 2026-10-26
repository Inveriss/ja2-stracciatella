# Plan: inwentarz pojazdów (transport przedmiotów)

Polska wersja dokumentu [vehicle-inventory-plan.md](vehicle-inventory-plan.md).

Status: **tylko plan**, niewdrożony (2026-10-26, użytkownik w trybie Remote
Control; V9 dla helikoptera dopisane później tego samego dnia). Branch `Vehicles`. Tło: [vehicles-current-implementation-pl.md](vehicles-current-implementation-pl.md).

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
| V5 | Gdzie można otworzyć inwentarz pojazdu? | **wariant B**: najpierw ekran mapy, **a taktyka jako zupełnie osobny, niezależny etap** (etap 4), rozpoczęty dopiero po zakończeniu etapów ekranu mapy |
| V6 | Które sloty? | **tylko kieszenie**; dłonie i sloty pancerza/głowy zablokowane |
| V7 | Uszkodzony pojazd (stan < `OKLIFE` 15) | **inwentarz nadal dostępny**, wg rekomendacji |
| V8 | Ładunek przy zniszczeniu pojazdu | **przedmioty w środku ulegają uszkodzeniu w wybuchu**, zależnie od jego siły (rozdział 4.5) |
| V8a | Siła uszkodzeń | **100** = 2 × siła wybuchu pojazdu (`GREAT_BIG_EXPLOSION` 50), ten sam mnożnik, którego gra używa dla przedmiotów leżących w miejscu wybuchu |
| V8b | Materiały wybuchowe w ładunku | **mogą wybuchnąć reakcją łańcuchową** (istniejąca reguła dla przedmiotów najemników) |
| V8c | Zniszczone przedmioty | **zostają w porzuconym stosie jako zepsute** (stan poniżej `USABLE`); w stosach sztuki ze stanem 1 są usuwane (istniejąca reguła) |
| V9 | Helikopter (od kiedy ma własną postać, plan docs/helicopter-team-list-repair-plan-pl.md) | **też dostaje otwierany inwentarz**; przy katastrofie **ładunek przepada całkowicie** (rozdział 4.6) |

| V9a | Kiedy można przenosić przedmioty do / z helikoptera? | gdy **wylądował albo wisi nad sektorem**, z każdym w tym sektorze i z inwentarzem sektora; w locie **między sektorami** tylko z **jego pasażerami** (zmienione 2026-10-26 z „tylko po wylądowaniu”: ląduje tylko na lotniskach do tankowania, gdzie indziej wisi) |
| V9b | Jego `slotLimitMultiplier` | **4**, jak pojazdy naziemne (wg propozycji) |

Wszystkie decyzje V1–V9b są podjęte; nic nie zostało otwarte.

## 4. Projekt

### 4.1 Dane

`vehicles.json`, nowe opcjonalne pole dla każdego pojazdu:

```
"slotLimitMultiplier": 4
```

`VehicleModel` dostaje `uint8_t slot_limit_multiplier` (domyślnie 1, gdy pola
brak = limity najemnika). Ustawić 4 dla Hummera i furgonetki z lodami (i
pozostałych pojazdów naziemnych); helikopter wg V9b.

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

### 4.5 Uszkodzenie ładunku przy wybuchu pojazdu (V8)

Jak to działa dziś:

- Najemnik trafiony wybuchem dostaje uszkodzenia przedmiotów z
  `CheckEquipmentForDamage()` (`Items.cc`), wołanej w `SoldierTakeDamage()`;
  na każdą sztukę `DamageItem()` odejmuje losowo 0…(obrażenia − 1) stanu
  (o połowę mniej dla przedmiotów metalowych, mniej o ochronę pancerza),
  amunicja ma szansę = % obrażeń na zniszczenie, słoiki się tłuką, materiały
  wybuchowe mogą wybuchnąć łańcuchowo.
- **Pojazdy to omijają**: `SoldierTakeDamage()` wraca wcześniej dla pojazdu
  (ścieżka `VehicleTakeDamage`), przed uszkodzeniem przedmiotów, a sama
  `CheckEquipmentForDamage()` od razu kończy się dla czołgów.
- Przy zniszczeniu `HandleCriticalHitForVehicleInLocation()` odpala
  `GREAT_BIG_EXPLOSION` (siła 50, promień 5) w miejscu pojazdu; przedmioty
  trafiają na ziemię później, przy tworzeniu wraku (`Rotting_Corpses.cc`).
  Przedmioty leżące już na polu trafionym wybuchem są uszkadzane przez
  `DamageItemOnGround()` siłą 2 × obrażenia.
- Czołgi wroga są tworzone z działem czołgowym i FN MINIMI w stanie 80–100 %
  (`Inventory_Choosing.cc`). W kodzie nie ma osobnej reguły uszkadzającej
  przedmioty czołgu przy jego śmierci; czy przedmioty z wraku dostają
  dodatkowe uszkodzenia od jego własnego wybuchu, zależy od kolejności wybuchu
  i upuszczenia — **niepotwierdzone bez testu w grze**.

Plan: gdy pojazd gracza zostaje zniszczony, zaraz po ustawieniu `fDestroyed`
wywołać `CheckEquipmentForDamage(postać pojazdu, siła V8a)` (nie dla czołgów)
na jego inwentarzu, a potem zwykłe upuszczenie kładzie na ziemi to, co zostało.

### 4.6 Inwentarz helikoptera (V9)

- Od etapu 1 planu helikoptera helikopter ma postać w drużynie gracza
  (`IsHelicopterSoldier()`, tylko strategiczną), więc ta sama zmiana warunku
  (4.4) otwiera jego inwentarz; jego sektor podąża za helikopterem.
- Przenoszenie (V9a): `MapscreenCanPassItemToChar` sprawdza tylko sektor i
  stan „między sektorami”, a sektor postaci helikoptera to ten, nad którym
  leci. Dla helikoptera trzeba więc dodać: przedmioty przenosi się tylko po
  wylądowaniu (`!fHelicopterIsAirBorne`), z wyjątkiem wymiany między
  helikopterem a jego pasażerami (`InHelicopter(s)`), którzy mogą to robić
  także w locie. Inwentarz sektora też wymaga wylądowania.
- Katastrofa (V9): `SkyriderDestroyed()` usuwa postać helikoptera przez
  `TacticalRemoveSoldier()` — jej inwentarz znika razem z nią, nic nie spada
  na ziemię. To jest zdecydowany skutek, więc nic więcej nie trzeba; po prostu
  nie dodawać tam upuszczania. Uszkodzenie ładunku z 4.5 nie dotyczy
  helikoptera (brak wybuchu w taktyce).
- Naprawa dalej dotyczy tylko stanu; inwentarz nie wpływa na lot (helikopter
  nie ma modelu wagi).

### 4.7 Przenoszenie na ekranie mapy: walka i podróż

- **W trakcie walki** `MapscreenCanPassItemToChar` odmawia każdego przekazania
  między postaciami, gdy we wczytanym sektorze są wrogowie
  (`gTacticalStatus.fEnemyInSector`) — po cichu: kliknięcie innej postaci z
  przedmiotem w ręce nic nie robi. Propozycja: pokazać komunikat (np.
  istniejący „Podczas walki nie można korzystać z tego panelu.” albo nowy
  tekst). Uwaga: flaga dotyczy sektora wczytanego w taktyce, więc walka tam
  blokuje przenoszenie na mapie wszędzie (zachowanie oryginalne).
- **W trakcie podróży** wymiana jest dozwolona tylko w tym samym oddziale lub
  pojeździe (równe `bAssignment`). Postać pojazdu ma jako `bAssignment` numer
  oddziału, a jej pasażerowie `VEHICLE`, więc pasażer ↔ pojazd w drodze
  zostałby odrzucony — dopuścić to, gdy `iVehicleId` pasażera to ten pojazd.

## 5. Etapy

1. **Dostęp do inwentarza** (ekran mapy): zmiana warunku + ograniczenie slotów
   oraz reguła przenoszenia dla helikoptera (V9a). Test: otworzyć inwentarz
   Hummera, przenieść przedmioty od najemnika i z inwentarza sektora, pojechać
   do innego sektora, wyjąć je; to samo z helikopterem (po wylądowaniu / w
   locie z jego pasażerami); katastrofa usuwa ładunek; zapis/odczyt.
2. **Limity pojazdów**: `slotLimitMultiplier`, `SoldierSlotLimit`, trzy
   miejsca wywołań. Test: 32 magazynki w dużej i 16 w małej kieszeni Hummera;
   najemnik nadal 8/4; bronie i kastet nadal przy limicie najemnika; limity
   pieniędzy bez zmian.
3. **Uszkodzenie ładunku** (V8): zniszczyć załadowany pojazd w taktyce;
   przedmioty spadają uszkodzone, amunicja częściowo zniszczona, materiały
   wybuchowe mogą wybuchnąć (V8b).

Rozmiar etapów 1–3: mały–średni; bez zmiany formatu zapisu.

### Etap 4 — inwentarz pojazdu w taktyce (osobny, niezależny)

Rozpoczynany **dopiero po zakończeniu i przetestowaniu etapów 1–3** (V5). Cel:
korzystanie z ładunku pojazdu w trakcie walki. Przed wdrożeniem wymaga własnej
szczegółowej analizy i decyzji; znane punkty wyjścia:

- Panel SM (inwentarz) jest zamknięty dla ciał innych niż ludzkie
  (`Interface.cc`, `!IS_MERC_BODY_TYPE(s)` → panel drużyny); pojazd nie ma
  sylwetki, statystyk ani przycisków dłoni, które miałyby sens na tym panelu.
- Przekazywanie przedmiotów innej postaci w taktyce wyklucza pojazdy
  (`Interface_Items.cc`, sprawdzenie celu przekazania).
- Pytania do rozstrzygnięcia wtedy: kto ma dostęp do ładunku (pasażer,
  najemnik stojący obok pojazdu), koszt AP za przeniesienie, co panel pokazuje
  dla pojazdu, zachowanie w walce turowej i przy zniszczeniu pojazdu.

Rozmiar etapu 4: średni–duży, do oszacowania we własnej analizie.

#### Analiza etapu 4 (2026-10-27)

Jak dziś działa przekazywanie przedmiotów w taktyce (najemnicy):

- Panel SM (inwentarz) najemnika otwiera się z jego portretu w panelu drużyny
  (`Interface_Panels.cc`, nie dla EPC, kierowców ani pasażerów).
- **Przekazywanie między najemnikami:** z przedmiotem na kursorze wziętym z
  panelu SM najemnika A panel SM może przełączyć się na najemnika B tylko wtedy,
  gdy B jest w zasięgu `PASSING_ITEM_DISTANCE_OKLIFE` (3 pola) i w polu widzenia
  A (`gfSMDisableForItems`, `Interface_Panels.cc`); przedmiot trafia wtedy do
  slotu B. Albo A upuszcza przedmiot na B na mapie: obok (≤ 3 pola) to
  podanie za `AP_PICKUP_ITEM`, dalej rzut (`AP_TOSS_ITEM`, `Interface_Items.cc`,
  `SoldierGiveItem()`).
- Pojazdy są wykluczone na każdym kroku: panel SM odrzuca ciała inne niż
  ludzkie (`SetCurrentTacticalPanelCurrentMerc`, `!IS_MERC_BODY_TYPE`), panel
  drużyny pokazuje pasażerów pojazdu, ale nie sam pojazd, kliknięcie pojazdu na
  mapie znaczy „wsiądź”, a pojazd nigdy nie jest odbiorcą podania
  (`!(tgt->uiStatusFlags & SOLDIER_VEHICLE)`).
- Pasażerowie też nie mają panelu SM (są ukryci w środku; ich sloty dłoni
  pokazują ikony pojazdu, druga to przycisk wysiadania).
- Już zrobione po drodze: panel inwentarza nie rysuje sylwetki dla pojazdu
  (etap 1), sloty dłoni/pancerza/głowy odrzucają przedmioty (V6).

Proponowany projekt, oparty na mechanice podawania:

1. Przedmiot najemnika **do** pojazdu: upuścić go na pojazd na mapie, stojąc w
   odległości do 3 pól — podanie do kieszeni ładunku (automatyczne układanie),
   koszt `AP_PICKUP_ITEM` dla najemnika w walce turowej. Bez rzucania do pojazdu
   z dalszej odległości.
2. **Z** pojazdu: otworzyć własny panel SM pojazdu — z panelu SM najemnika
   nowy przełącznik „ładunek pojazdu”, widoczny, gdy ten najemnik jest do 3 pól
   od pojazdu swojej drużyny (albo klawisz: Ctrl+klik na pojeździe) — potem
   wziąć przedmiot i upuścić go na najemnika / do jego panelu, czyli podanie z
   pojazdu, koszt `AP_PICKUP_ITEM` dla **najemnika**.
3. Panel SM pojazdu: tylko jego kieszenie, bez sylwetki, bez statystyk;
   przyciski tylko dla najemnika (postawa, seria, patrz, …) wyłączone.
4. Pasażerowie: bez dostępu w środku (najpierw wysiadają), jak dziś.
5. Walka turowa: dozwolone, za powyższy koszt AP; czas rzeczywisty: bez kosztu.
6. Zniszczony pojazd: brak panelu; ładunek spada uszkodzony (etap 3).

Decyzje (podjęte 2026-10-27, wszystkie wg propozycji):

| # | Pytanie | Decyzja |
|---|---|---|
| T1 | Jak gracz otwiera ładunek pojazdu w taktyce? | przełącznik „ładunek pojazdu” na panelu SM najemnika stojącego do 3 pól od pojazdu (alternatywa: Ctrl+klik na pojeździe) |
| T2 | Kto płaci AP i ile? | najemnik, `AP_PICKUP_ITEM` za każde przeniesienie, tylko w walce turowej |
| T3 | Zasięg | 3 pola od pojazdu (`PASSING_ITEM_DISTANCE_OKLIFE`) i pole widzenia, jak przy podawaniu między najemnikami |
| T4 | Rzucanie przedmiotów do pojazdu z większej odległości? | nie |
| T5 | Dostęp pasażerów w środku | brak (najpierw wysiadają) |

Rozmiar: średni–duży — panel SM dla ciała innego niż ludzkie (przyciski, układ
na panelach 1366x768), nowy przełącznik/grafika, reguła odbiorcy podania,
obsługa AP.
