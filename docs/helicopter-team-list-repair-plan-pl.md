# Plan: helikopter na liście drużyny, z naprawą (Repair)

Polska wersja dokumentu [helicopter-team-list-repair-plan.md](helicopter-team-list-repair-plan.md).

Status: **wdrożony** na branchu `Vehicles` (2026-10-26), każdy etap
sprawdzony w grze przez użytkownika — zob. rozdział 7. Najpierw powstał jako
sam plan (zlecony, gdy użytkownik pracował przez Remote Control). Decyzje
D1–D6 podjęte przez użytkownika 2026-10-26 (rozdział 5). Tło:
[vehicles-current-implementation-pl.md](vehicles-current-implementation-pl.md).
**Wymaga nowej gry**: zapisy sprzed etapu 1 nie mają postaci helikoptera.

## 1. Cel

Helikopter ma działać na ekranie mapy tak jak Hummer:

1. gdy helikopter Skyridera jest dostępny, pojawia się na liście drużyny w
   wierszach pojazdów (`newgoldpiece3_*`), ze swoim stanem technicznym;
2. najemnik może wziąć przydział **Repair** i wybrać helikopter, co przywraca
   jego stan.

## 2. Jak to działa dziś (luka)

| | Hummer | Helikopter |
|---|---|---|
| `VEHICLETYPE` | tak | tak (`iHelicopterVehicleId`) |
| `SOLDIERTYPE` | tak (`SOLDIER_VEHICLE`, w `OUR_TEAM` po wejściu pierwszego najemnika) | **brak** |
| wiersz na liście drużyny | tak (`AddCharacter` → od `FIRST_VEHICLE`) | **nie** — lista trzyma tylko `SOLDIERTYPE*` |
| stan techniczny | `bLife` jego żołnierza | **`gubHelicopterHitsTaken`** (0–3), nigdy nie maleje |
| obrażenia | pociski / wybuchy w taktyce | trafienia SAM w locie: 1. = kwestia, 2. = przymusowy powrót do bazy, 3. = zniszczenie z wszystkimi na pokładzie |
| naprawa | `DoesVehicleNeedAnyRepairs` / `RepairVehicle` na `bLife` | jawnie **wyłączona** („Skyrider isn't damagable/repairable”) |

W oryginalnej grze uszkodzenia helikoptera są więc **trwałe**: po dwóch
trafieniach SAM kolejne jest śmiertelne, na zawsze. Naprawa nadaje temu sens.

## 3. Zalecany projekt (wariant A): „pusty” żołnierz dla helikoptera

Helikopter dostaje `SOLDIERTYPE`, który istnieje tylko po stronie
strategicznej, jak u Hummera, ale **nigdy nie trafia do taktyki**.

- Tworzony w `SetUpHelicopterForPlayer()` zaraz po `AddVehicleToList(...,
  HELICOPTER)`: drużyna gracza, profil `PROF_HELICOPTER` (163, już profil typu
  `VEHICLE`), flaga `SOLDIER_VEHICLE`, `bVehicleID = iHelicopterVehicleId`
  (`fUseGivenVehicle`, więc nie powstaje drugi slot pojazdu), `bInSector =
  FALSE`, `sGridNo = NOWHERE`, stan `bLife = bLifeMax = 100`.
- Resztę daje w większości istniejący kod: wiersz na liście drużyny
  (`AddCharacter` → wiersze pojazdów, przewijanie już jest), linia statusu
  „stan X/Y”, pozycja w menu Repair (`CFOR_EACH_VEHICLE`), punkty naprawy
  (`RepairVehicle`), zapis/odczyt (zapisywany z drużyną).
- `GetSoldierStructureForVehicle(helikopter)` zaczyna działać; istniejące
  gałęzie `IsHelicopter` zostają, bo helikopter nadal nie ma ciała taktycznego.

Wariant B (odrzucony): specjalny wiersz nie-żołnierza na liście drużyny. Lista
drużyny, panel informacji, menu przydziałów i menu naprawy działają na
`SOLDIERTYPE*`; drugi rodzaj wpisu dotknąłby znacznie więcej kodu.

## 4. Zmiany w etapach

### Etap 1 — pusty żołnierz + wiersz na liście drużyny

- `Map_Screen_Helicopter.cc`: tworzenie w `SetUpHelicopterForPlayer`; usuwanie
  w `SkyriderDestroyed` (i wszędzie, gdzie helikopter jest usuwany).
- Do utworzenia żołnierza potrzebny jest typ ciała; użyć istniejącego ciała
  pojazdu (np. `HUMVEE`) z `fUseGivenVehicle`; nigdy nie jest rysowany.
- Upewnić się, że nic nie wstawia go do taktyki: kod przybycia już pomija
  helikopter (gałąź `IsHelicopter` w `GroupArrivedAtSector`); sprawdzić
  `UpdateMercInSector`, wczytywanie sektora (`AddSoldierToSector` dla drużyny
  gracza), rozmieszczanie taktyczne, auto-resolve, oddziały (helikopter bez
  oddziału, `AddCharacterToUniqueSquad` jest wołane tylko dla innych pojazdów).
- Lista drużyny: kolumny położenia/celu/przydziału dla helikoptera (sektor z
  `GetHelicopter()`, cel z jego trasy).
- Wybór wiersza helikoptera i wyznaczanie trasy: przekazać do istniejącego
  wyznaczania trasy Skyridera (decyzja D5).
- Nazwa i portret: `zVehicleName[HELICOPTER]`; `giCarPortraits` ma tylko 4
  pozycje (bez czołgu i helikoptera), więc odczyt musi sprawdzać zakres, a
  helikopter dostaje własny portret (etap 4).

### Etap 2 — stan techniczny zamiast licznika trafień

- Zastąpić `gubHelicopterHitsTaken` przez `bLife` pustego żołnierza (D2):
  **każde trafienie SAM −25 stanu** (`HELI_SAM_HIT_DAMAGE` 25). Od pełnego stanu:

  | Trafienie | Stan po | Skutek |
  |---|---|---|
  | 1 | 75 | kwestia `HELI_TOOK_MINOR_DAMAGE` |
  | 2 | 50 | kwestia `HELI_TOOK_MINOR_DAMAGE` |
  | 3 | 25 | kwestia `HELI_TOOK_MAJOR_DAMAGE`, `MakeHeliReturnToBase()` |
  | 4 | 0 | oryginalna katastrofa: kwestia `HELI_GOING_DOWN`, dźwięk upadku, `SkyriderDestroyed()` — giną wszyscy na pokładzie |

- Skutek wybiera się ze **stanu po trafieniu** (aktualizacja D2, 2026-10-26),
  więc naprawiony albo częściowo naprawiony helikopter podlega tym samym
  regułom:

  | Stan po trafieniu | Skutek |
  |---|---|
  | 100–50 | monolog o lekkim uszkodzeniu |
  | 49–25 | monolog o ciężkim uszkodzeniu + powrót do bazy |
  | 24 i mniej | oryginalna katastrofa, giną wszyscy na pokładzie |

  Przykłady: 82 → 57 monolog o lekkim uszkodzeniu; 70 → 45 monolog o ciężkim
  uszkodzeniu + powrót do bazy; 35 → 10 katastrofa. Stan nigdy nie spada
  poniżej 0. To zastępuje oryginalną regułę liczby trafień (kwestia, powrót,
  katastrofa przy trafieniach 1, 2, 3).
- Ścieżka obrażeń pojazdu (`VehicleTakeDamage`, która ustawia życie < `OKLIFE`
  na 0) nie jest używana dla trafień SAM; logika helikoptera sama zmienia `bLife`.
- Zapis: `gubHelicopterHitsTaken` zostaje w rekordzie zapisu i nadal liczy
  trafienia; stan zapisuje się razem z postacią helikoptera. Starsze zapisy nie
  mają postaci helikoptera, więc (brak zgodności zapisów, zasada projektu)
  potrzebna jest nowa gra.

### Etap 3 — naprawa

- `DoesVehicleNeedAnyRepairs`: usunąć wczesny powrót „helikopter nigdy nie jest
  uszkodzony” (użyć `bLife < bLifeMax`).
- `CanCharacterRepairVehicle` / `IsThisVehicleAccessibleToSoldier`: dla
  helikoptera `OKUseVehicle` znaczy „może lecieć” (`CanHelicopterFly`), co jest
  fałszem np. gdy Skyriderowi jesteśmy winni pieniądze albo helikopter wraca do
  bazy. Dla naprawy osobna reguła (decyzja D3): najemnik w sektorze
  helikoptera, helikopter **wylądował** (`!fHelicopterIsAirBorne`), nie jest
  między sektorami, nie jest zniszczony.
- Szybkość naprawy (D4): helikopter naprawia się wolniej — **20 punktów
  naprawy = 1 punkt stanu** (nowa stała `HELICOPTER_REPAIR_POINTS_DIVISOR` 20;
  Hummer zostaje przy `VEHICLE_REPAIR_POINTS_DIVISOR` 10). `RepairVehicle`
  wybiera dzielnik wg `IsHelicopter(v)`.
- `Faces.cc` zaokrągla pokazywane punkty naprawy do wielokrotności
  `VEHICLE_REPAIR_POINTS_DIVISOR`; musi używać tego samego dzielnika co pojazd.
- Blokada lotu (D6, aktualizacja 2026-10-26): **stan 100–50 — helikopter może
  latać; 49 i mniej — Skyrider odmawia lotu**, dopóki helikopter nie zostanie
  naprawiony do co najmniej 50. Nowy warunek w `CanHelicopterFly()` /
  `CanHelicopterTakeOff()` z komunikatem Skyridera, dlaczego
  (`HELI_FLIGHT_MIN_CONDITION` 50).
- Razem z D2: trafienie, które zostawia 49–25, odsyła helikopter do bazy (ten
  lot zostaje dokończony — przymusowy powrót nie jest startem); po wylądowaniu
  helikopter stoi na ziemi do naprawy do 50. Od pełnego stanu 3. trafienie (25)
  więc odsyła go do bazy i uziemia; naprawa wymaga 25 punktów stanu = 500
  punktów naprawy (D4).

### Etap 4 — limity, teksty, testy

- Miejsce w drużynie: pusty żołnierz zajmuje jedno miejsce `PLAYER_TEAM_SIZE`
  jak Hummer (`PlayerTeamFull` liczy pojazdy) — decyzja D1.
- Linia statusu: tylko stan, bez paliwa (helikopter nie ma modelu paliwa) —
  jak u robota.
- Tłumaczenia: komunikat blokady lotu (`pSkyriderText[3]`, wszystkie 9 języków).
- Portret: `giCarPortraits` nie ma go dla helikoptera; pokazywany jest
  opcjonalny `Interface/helicopter.png` 48x43 z danych gry, a bez niego ikona
  helikoptera z ekranu mapy (`helicop.sti`), wyśrodkowana.
- Testy w grze: zdobyć Skyridera, sprawdzić wiersz; przelecieć nad sektorem SAM,
  żeby dostać trafienia (75 / 50 / 25 i kwestie, powrót do bazy po 3.);
  wylądować; naprawić mechanikiem (20 punktów na punkt stanu); odmowa lotu przy
  49 i mniej, lot znów możliwy przy 50; zniszczyć helikopter (4. trafienie od pełnego stanu) i sprawdzić,
  że wiersz znika; zapis/odczyt.

## 5. Decyzje (podjęte 2026-10-26)

| # | Pytanie | Decyzja |
|---|---|---|
| D1 | Czy helikopter zajmuje miejsce w drużynie (jak Hummer)? | **tak**, jak Hummer (wg rekomendacji) |
| D2 | Obrażenia na trafienie SAM i skutki | **−25 na trafienie**; skutek wg stanu po trafieniu: **100–50** monolog o lekkim uszkodzeniu, **49–25** monolog o ciężkim uszkodzeniu + powrót do bazy, **24 i mniej** oryginalna katastrofa, wszyscy giną. Od pełnego stanu: trafienia 1–2 lekkie, 3. ciężkie + powrót, 4. katastrofa (szczegóły w etapie 2) |
| D3 | Gdzie można go naprawiać? | **tylko po wylądowaniu, najemnik w tym samym sektorze** (dowolny sektor, nie tylko miejsce tankowania), wg rekomendacji |
| D4 | Szybkość naprawy | **20 punktów naprawy = 1 punkt stanu** (wolniej niż 10 u Hummera, bo to helikopter). **Zmienione 2026-10-26 na 12**: nawet najlepsi mechanicy zdobywają poniżej 20 punktów naprawy na godzinę (Trevor 17, Static 18), a godzinowa naprawa zaokrągla w dół, więc przy 20 nic się nie naprawiało. Każdy pojazd ma teraz własny `repairPointsDivisor` w `vehicles.json`: helikopter 12, Hummer 10, furgonetka z lodami 8, pozostałe 10 |
| D5 | Kliknięcie wiersza helikoptera, by wyznaczyć trasę | **otwiera zwykłe wyznaczanie trasy Skyridera**, wg rekomendacji |
| D6 | Czy uszkodzenie blokuje też lot? | **tak: 100–50 może latać, 49 i mniej odmowa lotu** do czasu naprawy do 50 |

## 6. Rozmiar i ryzyko

Duża zmiana: nowy rodzaj żołnierza w drużynie gracza, który nigdy nie może
trafić do taktyki, zmieniony model obrażeń, reguły naprawy, zawartość zapisu.
Trzeba sprawdzić wiele pętli po `OUR_TEAM` / `SOLDIER_VEHICLE` pod kątem
helikoptera. Szacunek: 4 etapy, każdy do zbudowania i przetestowania w grze
przed następnym.

## 7. Wdrożenie (branch `Vehicles`)

| Etap | Commit | Co |
|---|---|---|
| 1 | 805e498fd | postać helikoptera (`CreateHelicopterSoldier`, `IsHelicopterSoldier` / `GetHelicopterSoldier` w `Vehicles.cc`), trzymana poza taktyką (`UpdateMercInSector`, `InternalAddSoldierToSector`), sektor zgodny z helikopterem, kliknięcie wiersza wyznacza trasę Skyridera, okienko ruchu ją pomija, linia statusu tylko ze stanem |
| 2 | e3760854f | trafienie SAM −25 stanu, skutek wg stanu po trafieniu (`HELI_SAM_HIT_DAMAGE`, `HELI_MINOR/MAJOR_DAMAGE_MIN_CONDITION`) |
| 3 | 0a3d90095 | naprawa po wylądowaniu w sektorze najemnika (`IsVehicleInRepairReach`), `HELICOPTER_REPAIR_POINTS_DIVISOR` 20, blokada lotu poniżej 50 (`IsHelicopterTooDamagedToFly`) z `pSkyriderText[3]`; menu naprawy przyjmuje każdy numer pojazdu poniżej `MAX_VEHICLES` |
| 4 | (ten etap) | portret helikoptera; ten dokument zaktualizowany do stanu po wdrożeniu |

Powiązana poprawka znaleziona przy testach: ed5a82d49 (ekran mapy nie kończy
się crashem na odwiedzonym sektorze bez mapy radaru, np. przy testowym
odsłonięciu całej mapy).
