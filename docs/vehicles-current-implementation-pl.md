# Pojazdy — obecna (oryginalna) implementacja

Polska wersja dokumentu [vehicles-current-implementation.md](vehicles-current-implementation.md).

Analiza referencyjna działania pojazdów w kodzie na branchu `Vehicles`
(stan `ja2master` d696d6a5a, 2026-10-26). Niczego tu nie zmieniono; to punkt
wyjścia do prac nad pojazdami.

## 1. Przegląd

Pojazd istnieje w **dwóch powiązanych postaciach**:

| Postać | Gdzie | Co przechowuje |
|---|---|---|
| `VEHICLETYPE` | `pVehicleList` (`src/game/Tactical/Vehicles.cc`) | dane strategiczne: typ, sektor, grupa ruchu, trasa na mapie, pasażerowie, flaga zniszczenia |
| `SOLDIERTYPE` z flagą `SOLDIER_VEHICLE` | zwykła tablica żołnierzy | ciało taktyczne: pozycja, animacja, **stan techniczny = `bLife`**, **paliwo = `sBreathRed`/`bBreath`**, drużyna, AP |

Powiązanie: `SOLDIERTYPE::bVehicleID` → indeks w `pVehicleList`;
`GetSoldierStructureForVehicle(v)` przeszukuje tablicę żołnierzy w drugą stronę.

**Wyjątek — helikopter (Skyrider):** ma tylko `VEHICLETYPE`
(`iHelicopterVehicleId`), **bez** `SOLDIERTYPE`. Wiele funkcji sprawdza
`IsHelicopter(v)` / `InHelicopter(s)` i rozgałęzia się.

Pasażerowie to zwykli najemnicy z `bAssignment == VEHICLE` i
`iVehicleId` = indeks pojazdu; pierwszy, który wsiądzie, dostaje
`SOLDIER_DRIVER`, pozostali `SOLDIER_PASSENGER`.

## 2. Pliki

| Plik | Rola |
|---|---|
| `src/game/Tactical/Vehicles.h/.cc` | rdzeń: lista, dodawanie/usuwanie, wsiadanie/wysiadanie, obrażenia, naprawa, inicjalizacja paliwa, zasady kierowcy, zapis/odczyt |
| `src/game/Tactical/LoadSaveVehicleType.cc` | 128-bajtowy rekord zapisu `VEHICLETYPE` |
| `src/externalized/VehicleModel.h/.cc` | model `vehicles.json` (dźwięki, profil, rodzaj ruchu, pancerz, miejsca) |
| `assets/externalized/vehicles.json` | 6 typów pojazdów (dane) |
| `src/game/Strategic/Strategic_Movement.cc` | grupy ruchu pojazdów, czas przejazdu wg terenu, zużycie paliwa, brak paliwa, tankowanie z kanistra |
| `src/game/Strategic/Strategic_Pathing.cc` | trasa pojazdu jest w `VEHICLETYPE::pMercPath` |
| `src/game/Strategic/Assignments.cc` | menu przydziału „Vehicle”, menu naprawy (pojazdy), zasady snu/kierowcy |
| `src/game/Strategic/Map_Screen_Interface.cc` | błędy ruchu (pusty / uszkodzony / bez paliwa), linia statusu (stan/paliwo) |
| `src/game/Strategic/MapScreen.cc` | pojazdy na liście drużyny (własne wiersze od `FIRST_VEHICLE`) |
| `src/game/Strategic/Map_Screen_Helicopter.cc` | helikopter: Skyrider, miejsca tankowania, zagrożenie SAM, koszty |
| `src/game/Strategic/PreBattle_Interface.cc` | wejście do bitwy: pasażerowie wysiadają; odwrót — pojazd odjeżdża |
| `src/game/Tactical/Soldier_Create.cc` | typ ciała → typ pojazdu, `AddVehicleToList` przy tworzeniu żołnierza-pojazdu |
| `src/game/Tactical/Soldier_Control.cc` | skalowanie obrażeń, zdarzenie wsiadania, tankowanie, AP, zasady postawy/obrotu |
| `src/game/Tactical/Turn_Based_Input.cc`, `Handle_UI.cc`, `UI_Cursors.cc` | klik na pojazd → podejście i wsiadanie; kursory; „pojazd nie może się ruszać” |
| `src/game/Tactical/Interface_Panels.cc`, `Interface.cc`, `Interface_Utils.cc` | panel drużyny (przycisk wysiadania w slotach dłoni), portret pojazdu, popup z przyciskiem Drive |
| `src/game/Tactical/Weapons.cc`, `LOS.cc`, `OppList.cc` | pancerz vs pociski, miejsce trafienia, widzenie |
| `src/game/Tactical/Squads.cc` | pojazd jako członek oddziału, zasady oddziałów |

## 3. Model danych

### 3.1 Typy pojazdów (`Vehicles.h`)

```
ELDORADO_CAR = 0, HUMMER, ICE_CREAM_TRUCK, JEEP_CAR, TANK_CAR, HELICOPTER
```

Kolejność jest zaszyta w kodzie: `vehicles.json` musi mieć co najmniej te 6
wpisów w tej kolejności (`VehicleModel::validateData`); HELICOPTER musi mieć
indeks 5.

### 3.2 Pola `vehicles.json` → `VehicleModel`

| Pole | Użycie |
|---|---|
| `enterSound` | odtwarzany przy wsiadaniu/wysiadaniu (`AddSoldierToVehicle`, `ExitVehicle`) |
| `moveSound` | zapętlony podczas ruchu w taktyce (`HandleVehicleMovementSound`) |
| `profile` | profil pojazdu (musi być typu `VEHICLE` lub `NOT_USED`); służy do nazwy/twarzy, `OKUseVehicle` |
| `movementType` | `FOOT`/`CAR`/`TRUCK`/`TRACKED`/`AIR` → `GROUP::ubTransportationMask` |
| `armourType` | przedmiot-pancerz używany jako ochrona pojazdu przed pociskami |
| `seats` | maks. liczba pasażerów, ≤ `MAX_PASSENGERS_IN_VEHICLE` (10) |

Obecne dane: wszystkie 5 pojazdów naziemnych ma typ `CAR` i 6 miejsc; Hummer i
Tank mają pancerz `SPECTRA_VEST`, pozostałe `KEVLAR_VEST`; Jeep i Tank wskazują
na zastępczy profil `NPC164`. W kampanii vanilla używane są tylko Hummer,
furgonetka z lodami i helikopter.

Profile pojazdów (`mercs-profile-info.json`, typ `VEHICLE`): 160
`PROF_HUMMER`, 161 `PROF_ELDERODO`, 162 `PROF_ICECREAM`, 163 `PROF_HELICOPTER`.

### 3.3 `VEHICLETYPE`

```
PathSt*   pMercPath        // trasa strategiczna (pojazd jest właścicielem trasy grupy)
UINT8     ubMovementGroup  // strategiczna grupa ruchu
UINT8     ubVehicleType    // indeks w vehicles.json
SGPSector sSector
BOOLEAN   fBetweenSectors
INT16     sGridNo          // pozycja taktyczna (ustawiana przy tworzeniu)
SOLDIERTYPE* pPassengers[10]
BOOLEAN   fDestroyed
UINT32    uiMovementSoundID
BOOLEAN   fValid           // slot zajęty
```

Limity: `MAX_VEHICLES` 10 (wcześniej utworzone grupy ruchu),
`MAX_PASSENGERS_IN_VEHICLE` 10.

### 3.4 Statystyki pojazdu przechowywane w `SOLDIERTYPE`

| Znaczenie | Pole | Uwagi |
|---|---|---|
| stan techniczny | `bLife` / `bLifeMax` | < `OKLIFE` → nie może jechać; 0 → zniszczony |
| paliwo | `sBreathRed` 0..10000, `bBreath` 0..100 | 10000 = pełny bak |
| rodzaj | `ubWhatKindOfMercAmI = MERC_TYPE__VEHICLE`, flaga `SOLDIER_VEHICLE` | |
| ciało | `ubBodyType` HUMVEE, ELDORADO, ICECREAMTRUCK, JEEP, TANK_NW, TANK_NE | |

## 4. Cykl życia

### 4.1 Tworzenie

- **Nowa gra:** `InitVehicles()` tworzy 10 trwałych grup ruchu pojazdów
  (`gubVehicleMovementGroups`), wszystkie z `ubTransportationMask = CAR`.
- **Pojazdy naziemne pochodzą z map:** pojazd jest umieszczony na mapie sektora
  jako żołnierz z typem ciała pojazdu (drużyna cywilów). Gdy ten żołnierz jest
  tworzony (`Soldier_Create.cc`), typ ciała wybiera typ pojazdu,
  `AddVehicleToList()` przydziela slot (używa gotowej grupy slotu i ustawia jej
  maskę transportu z `movementType`), a `SetVehicleValuesIntoSoldierType()`
  ustawia nazwę, profil, pełny bak i `MERC_TYPE__VEHICLE`. Hummer, Eldorado i
  furgonetka z lodami są tworzone jako `bNeutral`.
- **Helikopter:** `SetUpHelicopterForPlayer()` → `AddVehicleToList(..., HELICOPTER)`
  przy konfiguracji Skyridera.
- **Debug:** klawisz-cheat w `Turn_Based_Input.cc` zamienia wybranego żołnierza
  w ciało czołgu z MINIMI, zarejestrowane jako pojazd HUMMER.

### 4.2 Własność / pozwolenie na użycie

`OKUseVehicle(profile)`:

- Hummer → fakt `FACT_OK_USE_HUMMER` (ustawiany przez akcję NPC
  `NPC_ACTION_TIMER_FOR_VEHICLE`, zaplanowaną na następny dzień z dialogu NPC);
- furgonetka z lodami → `FACT_OK_USE_ICECREAM` (ustawiany przez skrypty NPC;
  Hamous i furgonetka przemieszczają się razem między sektorami, dopóki gracz
  nie ma furgonetki albo Hamousa);
- helikopter → `CanHelicopterFly()`;
- wszystko inne → dozwolone.

Żołnierz-pojazd zostaje w drużynie cywilów, dopóki nie wsiądzie pierwszy
najemnik: wtedy `AddSoldierToVehicle` wywołuje `ChangeSoldierTeam(vs, OUR_TEAM)`.
Od tej chwili pojazd zajmuje **miejsce w drużynie gracza** (`PlayerTeamFull`
liczy pojazdy; `NumberOfMercsOnPlayerTeam` nie).

### 4.3 Wsiadanie

Drogi do pojazdu, wszystkie kończą się w `PutSoldierInVehicle` → `AddSoldierToVehicle`:

- **Ekran mapy:** Przydział → Vehicle → wybór pojazdu (`DisplayVehicleMenu`).
  Tylko w trybie mapy, pojazd w tym samym sektorze i nie między sektorami,
  dostępny (`OKUseVehicle`), brak bitwy we wczytanym sektorze, najemnik bez
  ograniczeń EPC / nie mechaniczny (`CanCharacterVehicle`). Pełne pojazdy są
  wyszarzone.
- **Taktyka:** kliknięcie pojazdu (`CheckForAndHandleHandleVehicleInteractiveClick`)
  prowadzi najemnika obok pojazdu (akcja oczekująca `MERC_ENTER_VEHICLE`), potem
  `EVENT_SoldierEnterVehicle`. Tylko `OK_ENTERABLE_VEHICLE` = nie czołg i stan
  ≥ `OKLIFE`; kosztuje tylko AP za dojście.

Co robi `AddSoldierToVehicle`:

1. (nie helikopter) przenosi pojazd do drużyny gracza; jeśli pojazd był pusty,
   wstawia żołnierza-pojazd do własnego nowego oddziału
   (`AddCharacterToUniqueSquad`) i do grupy ruchu pojazdu;
2. sadza najemnika na pierwszym wolnym miejscu; wyjmuje go z innego pojazdu,
   oddziału lub grupy;
3. przydział → `VEHICLE`, ustawia `iVehicleId`, dodaje najemnika do grupy pojazdu;
4. pierwszy pasażer → `SOLDIER_DRIVER`, kolejni → `SOLDIER_PASSENGER`;
5. najemnik znika z siatki mapy i przenosi się na pozycję pojazdu; dźwięk
   wsiadania; w taktyce oddział pojazdu staje się bieżący.

Zatem **pojazd jest członkiem oddziału**, a jego pasażerowie należą do tego
oddziału pośrednio (`IsSoldierInThisVehicleSquad`). Oddział z pojazdem przyjmuje
tylko kierowców/pasażerów/sam pojazd (`Squads.cc`).

### 4.4 Wysiadanie

- **Taktyka:** kliknięcie drugiego slotu dłoni pasażera w panelu drużyny
  (`TMClickSecondHandInvCallbackPrimary` → `ExitVehicle`). Najemnik staje na
  wolnym polu obok pojazdu (szukanie w promieniu 5, potem 20), trafia do nowego
  osobnego oddziału i zostaje wybrany.
- **Gdzie indziej:** `TakeSoldierOutOfVehicle` → `RemoveSoldierFromVehicle` (bez
  rozmieszczania w taktyce). Używane przy przydziale do oddziału, innych
  przydziałach, końcu kontraktu, pojmaniu itd.
- Gdy z pojazdu naziemnego wysiada **ostatni** pasażer: trasa jest anulowana;
  jeśli pojazd był między sektorami, jest **teleportowany do bliższego z dwóch
  sektorów** (`TeleportVehicleToItsClosestSector`); zostaje usunięty z oddziału,
  wraca do swojej grupy pojazdu, a jego przydział jest czyszczony.
- Wyjście z helikoptera oznacza sektor jako odwiedzony i ustawia dane wejścia
  (lotnisko w Drassen ma stały gridno).

## 5. Ruch strategiczny (ekran mapy)

- Pojazd i pasażerowie tworzą **jedną grupę ruchu** (`fVehicle = TRUE`); trasa
  należy do pojazdu (`MoveCharactersPathToVehicle`, `SetUpMvtGroupForVehicle`).
- **Czy może jechać?** (`CanCharacterMoveInStrategic`):
  - pusty → `ME_VEHICLE_EMPTY` (potrzebny kierowca),
  - stan < `OKLIFE` → `ME_VEHICLE_DAMAGED`,
  - paliwo 0 → `ME_VEHICLE_NO_GAS`,
  - plus wszystkie zwykłe warunki (walka, wrogowie w sektorze, robot bez
    kontrolera w pojeździe, EPC bez nie-EPC w pojeździe, zbyt zmęczony…).
- **Kto prowadzi?** `SoldierMustDriveVehicle`: gdy pojazd jedzie i dokładnie
  jeden pasażer może prowadzić (nie helikopter, nie mechaniczny, nie EPC, życie
  ≥ `OKLIFE`, maks. oddech powyżej minimum), ten najemnik nie może spać. Przy
  kilku możliwych kierowcach nikt nie jest zmuszony.
- **Czas przejazdu** (`GetSectorMvtTimeForGroup`, minuty na sektor):

  | Maska | Baza | Dozwolony teren (prędkość %) |
  |---|---|---|
  | FOOT | 89 | droga 100, równina 85, rzadki las 70, gęsty las 60, piasek 50, wzgórza 50, bagno 35, woda/rzeki 25 |
  | **CAR** | **30** | **tylko droga** |
  | TRUCK | 32 | droga 100, równina 75, rzadki las 60, wzgórza 50 |
  | TRACKED | 46 | droga/równina 100, piasek 70, rzadki las 60, wzgórza 60, rzeki 20, woda 10 |
  | AIR | 10 | wszędzie, ten sam czas (także nad miastami) |

  Sektory miejskie: 5 minut dla wszystkiego oprócz AIR. Grupa używa najwolniejszej
  ze swoich masek. Wszystkie obecne pojazdy naziemne to `CAR` → tylko drogi.
- **Zużycie paliwa:** po przybyciu do każdego sektora (nie helikopter)
  `paliwo -= czas_przejazdu × 6` (`SpendVehicleFuel`). Przykład: sektor drogi
  (30 min) kosztuje 180 z 10000 → około 55 sektorów drogi na pełnym baku. Przy 0
  pokazuje się komunikat „brak paliwa” i trasa jest czyszczona.
- **Brak doświadczenia za podróż** dla najemników jadących pojazdem (daje je
  tylko marsz).
- Po przybyciu żołnierz-pojazd i pasażerowie dostają sektor, kierunek i kod
  wejścia; jeśli sektor jest wczytany, trafiają do taktyki.

## 6. Paliwo

- Pełny bak = `sBreathRed` 10000 (`bBreath` 100). Na ekranie mapy widoczne jako
  „paliwo X/100”.
- **Kanister (`GAS_CAN`) w taktyce:** najemnik z kanistrem w dłoni, kursor
  `REFUELCURS` nad pojazdem, koszt = AP na przykucnięcie + `AP_REFUEL_VEHICLE`
  (10), animacja `REFUEL_VEHICLE`, potem `AddFuelToVehicle`: każdy punkt stanu
  kanistra = 50 paliwa (kanister 100 % = pół baku); kanister traci stan, pusty
  jest usuwany.
- **Tankowanie przez NPC:** `NPC_ACTION_FILL_UP_CAR` napełnia Hummera (albo
  furgonetkę z lodami, jeśli Hummer jest pełny / go nie ma) do 100 %.
- Zapisy starsze niż wersja 74 dostają pełny bak przy wczytaniu
  (`AddVehicleFuelToSave`).

## 7. Stan techniczny, obrażenia i naprawa

- **Obrażenia** (`SoldierTakeDamage` dla żołnierza-pojazdu):
  - obrażenia od pocisków ÷ 3, od wybuchu powyżej 50 × 2 (czołgi: bez zmian);
  - walka wręcz i gaz nic nie robią;
  - pancerz: pociski są osłabiane przez przedmiot-pancerz z `armourType`
    (`Weapons.cc`, `GetVehicleArmour`);
  - `VehicleTakeDamage` → `HandleCriticalHitForVehicleInLocation`: życie spada,
    pokazuje się liczba obrażeń; życie < `OKLIFE` jest ustawiane na 0.
- **Zniszczenie** (życie 0): `fDestroyed`, `GREAT_BIG_EXPLOSION` w miejscu
  pojazdu, obsługa śmierci (animacja `VEHICLE_DIE`, wrak w jednym kierunku) i
  **śmierć każdego pasażera** (`KillAllInVehicle`, po 100 obrażeń).
- **Odgłosy bitewne pojazdu** (np. „OK” na rozkaz) wypowiada losowy pasażer
  (`PickRandomPassengerFromVehicle`); pasażerowie nie odnoszą obrażeń od trafień
  w pojazd, dopóki nie zostanie zniszczony.
- **Naprawa:** przydział Repair → pojazd (ekran mapy) albo klik zestawem
  narzędzi na pojazd w taktyce (ustawia ten sam przydział). Każde 10 punktów
  naprawy (`VEHICLE_REPAIR_POINTS_DIVISOR`) przywraca 1 punkt stanu. Możliwa,
  gdy pojazd nie jest zniszczony, jest uszkodzony i dostępny dla najemnika.
  Helikoptera nie da się w ten sposób uszkodzić ani naprawić.

## 8. Zachowanie w taktyce

- **Pojazdami nie da się jeździć w taktyce.** Próba ruchu wybranym pojazdem daje
  komunikat `VEHICLE_CANT_MOVE_IN_TACTICAL` (`HandleMoveModeInteractiveClick`).
  Zostały resztki obsługi jazdy (tekst przycisku Drive, kursory ruchu pojazdu,
  ciągły obrót w `PathAI.cc`, obrót w 32 kierunkach, dźwięk jazdy), ale punkt
  wejścia jest zablokowany.
- Żołnierze-pojazdy: tylko postawa „stoi”, brak obrotu na rozkaz, brak ataku
  (przycisk akcji w popupie to „N/A — dopóki nie będzie broni pokładowej”), brak
  widzenia dla pojazdów gracza (`OppList.cc`), +35 AP i limit 50 AP
  (`AP_VEHICLE_MAXIMUM`), brak regeneracji oddechu, brak świateł dla pasażerów.
- Pasażerowie są ukryci na siatce mapy i podążają za pozycją pojazdu
  (`UpdateAllVehiclePassengersGridNo`). W panelu drużyny ich sloty dłoni
  pokazują ikony pojazdu (`inventor.sti`): drugi to przycisk **wysiadania**;
  AP kierowcy to AP pojazdu.
- Portret pojazdu jest rysowany z `giCarPortraits[typ]`.
- AI ignoruje puste pojazdy; wrogowie mogą strzelać do pojazdów; pojazdy wroga
  (jeep, czołg) istnieją tylko jako typy ciała (edytor / debug), nie w kampanii.

## 9. Bitwy

- **Wejście do sektora bitwy:** interfejs przed bitwą wyjmuje wszystkich z
  pojazdów (`PutNonSquadMercsInBattleSectorOnSquads(TRUE)`); pasażerowie każdego
  pojazdu trafiają do jednego nowego pustego oddziału (potrzebne, bo
  jednoczesne grupy mogą wejść z różnych krawędzi). Sam pojazd nie bierze
  udziału (`PlayerMercInvolvedInThisCombat` wyklucza `SOLDIER_VEHICLE`); przy
  rozmieszczaniu taktycznym pojazdy są pomijane.
- **Odwrót:** pasażerowie zostają w środku i pojazd odjeżdża
  (`RetreatAllInvolvedPlayerGroups`, `fExitVehicles = FALSE`); puste grupy
  pojazdów się nie wycofują.
- **Auto-resolve:** pojazdy nie są walczącymi.
- **Pojmanie:** żołnierz-pojazd nigdy nie jest pojmany; pojmani najemnicy są
  najpierw wyjmowani z pojazdu.
- Siły wroga pomijają grupy pojazdów przy liczeniu grup gracza w sektorze.

## 10. Interfejs ekranu mapy

- Lista drużyny: pojazdy mają własne wiersze od `FIRST_VEHICLE`
  (= `PLAYER_TEAM_SIZE`), najwyżej 10.
- Linia statusu pojazdu: stan X/Y, paliwo X/Y.
- Menu przydziału: linia „Vehicle” (wyszarzona, gdy niemożliwe) → popup z
  dostępnymi pojazdami; menu Repair pokazuje pojazdy w sektorze (wyszarzone,
  jeśli nie do naprawy).
- Jadące pojazdy są na liście „w ruchu” jako `*nazwa*`, gdy jadą.

## 11. Zapis / odczyt

- `pVehicleList`: liczba (UINT8) + dla każdego slotu `fValid`, potem 128-bajtowy
  rekord (`LoadSaveVehicleType.cc`: miejsce na wskaźnik trasy, grupa, typ,
  sektor, flaga między sektorami, gridno, 10 **ID profili** pasażerów,
  zniszczony, id dźwięku, valid) + trasa strategiczna.
- `gubVehicleMovementGroups` (10 × INT8).
- Stan i paliwo pojazdu zapisują się razem z jego `SOLDIERTYPE`.
- Przy wczytaniu pasażer, którego przydział nie jest `VEHICLE`, jest poprawiany
  albo usuwany (obejście błędu 1885).

## 12. Miejsca zaszyte w kodzie — do pilnowania przy zmianach

1. Kolejność typów pojazdów i `HELICOPTER == 5` (kolejność w `vehicles.json`).
2. Mapowanie typ ciała → typ pojazdu tylko w `Soldier_Create.cc`.
3. Profile pojazdów 160–163 i fakty `OKUseVehicle` dla każdego profilu.
4. Paliwo i stan są w polach oddechu/życia `SOLDIERTYPE`; każda nowa logika
   „oddechu”/„życia” musi pomijać pojazdy (wiele miejsc już sprawdza
   `SOLDIER_VEHICLE`).
5. Wszystkie pojazdy naziemne to `CAR` → jazda tylko po drogach; `TRUCK`/`TRACKED`
   istnieją, ale są nieużywane.
6. `MAX_VEHICLES` 10 (gotowe grupy, wiersze listy drużyny) i
   `MAX_PASSENGERS_IN_VEHICLE` 10 (tablica w strukturze, rekord zapisu).
7. Pojazdy zajmują miejsca w drużynie gracza (`PLAYER_TEAM_SIZE`).
8. Jazda w taktyce jest zablokowana w jednym miejscu
   (`HandleMoveModeInteractiveClick`); stary kod jazdy za nią jest nieprzetestowany.
9. Helikopter nie ma `SOLDIERTYPE`; każda ogólna funkcja pojazdu potrzebuje
   gałęzi `IsHelicopter`.
