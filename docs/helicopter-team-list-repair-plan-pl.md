# Plan: helikopter na liście drużyny, z naprawą (Repair)

Polska wersja dokumentu [helicopter-team-list-repair-plan.md](helicopter-team-list-repair-plan.md).

Status: **tylko plan**, niewdrożony (zlecony 2026-10-26, gdy użytkownik
pracował przez Remote Control; zmiana jest zbyt duża, by robić ją bez testów).
Decyzje D1–D6 podjęte przez użytkownika 2026-10-26 (rozdział 5).
Branch `Vehicles`. Tło: [vehicles-current-implementation-pl.md](vehicles-current-implementation-pl.md).

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
  wyznaczania trasy Skyridera albo zablokować (decyzja D5).
- Nazwa i portret: `zVehicleName[HELICOPTER]`, `giCarPortraits[HELICOPTER]`
  (sprawdzić, czy portret istnieje dla indeksu 5).

### Etap 2 — stan techniczny zamiast licznika trafień

- Zastąpić `gubHelicopterHitsTaken` przez `bLife` pustego żołnierza (D2):
  **każde trafienie SAM −25 stanu** (`HELI_SAM_HIT_DAMAGE` 25). Od pełnego stanu:

  | Trafienie | Stan po | Skutek |
  |---|---|---|
  | 1 | 75 | kwestia `HELI_TOOK_MINOR_DAMAGE` |
  | 2 | 50 | kwestia `HELI_TOOK_MINOR_DAMAGE` |
  | 3 | 25 | kwestia `HELI_TOOK_MAJOR_DAMAGE`, `MakeHeliReturnToBase()` |
  | 4 | 0 | oryginalna katastrofa: kwestia `HELI_GOING_DOWN`, dźwięk upadku, `SkyriderDestroyed()` — giną wszyscy na pokładzie |

- Skutek wybiera się ze **stanu po trafieniu**, więc naprawiony (albo częściowo
  naprawiony) helikopter podlega tym samym regułom:
  stan > 25 → kwestia lekka; 1–25 → kwestia poważna + powrót do bazy;
  0 → katastrofa. Stan nigdy nie spada poniżej 0. To zastępuje oryginalną
  regułę liczby trafień (kwestia, powrót, katastrofa przy trafieniach 1, 2, 3).
- Ścieżka obrażeń pojazdu (`VehicleTakeDamage`, która ustawia życie < `OKLIFE`
  na 0) nie jest używana dla trafień SAM; logika helikoptera sama zmienia `bLife`.
- Zapis: `gubHelicopterHitsTaken` zostaje w rekordzie zapisu (nieużywane albo
  wyliczane); zgodnie z zasadą braku zgodności zapisów może być potrzebna nowa
  gra.

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
- Blokada lotu (D6): **poniżej 25 stanu helikopter nie wystartuje**, dopóki nie
  zostanie naprawiony — nowy warunek w `CanHelicopterFly()` /
  `CanHelicopterTakeOff()` z komunikatem Skyridera, dlaczego. Uwaga: po
  dokładnie trzech trafieniach od pełnego stanu ma 25, więc *nie* jest
  zablokowany (i tak już wraca do bazy); blokada dotyczy stanu 1–24,
  osiąganego po trafieniach częściowo naprawionego helikoptera.

### Etap 4 — limity, teksty, testy

- Miejsce w drużynie: pusty żołnierz zajmuje jedno miejsce `PLAYER_TEAM_SIZE`
  jak Hummer (`PlayerTeamFull` liczy pojazdy) — decyzja D1.
- Linia statusu: tylko stan, bez paliwa (helikopter nie ma modelu paliwa) —
  jak u robota.
- Tłumaczenia: tylko jeśli dojdą nowe komunikaty (np. „helikopter musi
  wylądować, żeby go naprawić”).
- Testy w grze: zdobyć Skyridera, sprawdzić wiersz; przelecieć nad sektorem SAM,
  żeby dostać trafienia (75 / 50 / 25 i kwestie, powrót do bazy po 3.);
  wylądować; naprawić mechanikiem (20 punktów na punkt stanu); blokada startu
  poniżej 25; zniszczyć helikopter (4. trafienie od pełnego stanu) i sprawdzić,
  że wiersz znika; zapis/odczyt.

## 5. Decyzje (podjęte 2026-10-26)

| # | Pytanie | Decyzja |
|---|---|---|
| D1 | Czy helikopter zajmuje miejsce w drużynie (jak Hummer)? | **tak**, jak Hummer (wg rekomendacji) |
| D2 | Obrażenia na trafienie SAM i skutki | **−25 na trafienie**; 1. i 2. trafienie: kwestia o lekkim uszkodzeniu; 3. trafienie (stan 25): kwestia o poważnym uszkodzeniu + powrót do bazy; 4. trafienie: oryginalna katastrofa, wszyscy giną (szczegóły w etapie 2) |
| D3 | Gdzie można go naprawiać? | **tylko po wylądowaniu, najemnik w tym samym sektorze** (dowolny sektor, nie tylko miejsce tankowania), wg rekomendacji |
| D4 | Szybkość naprawy | **20 punktów naprawy = 1 punkt stanu** (wolniej niż 10 u Hummera, bo to helikopter) |
| D5 | Kliknięcie wiersza helikoptera, by wyznaczyć trasę | **otwiera zwykłe wyznaczanie trasy Skyridera**, wg rekomendacji |
| D6 | Czy uszkodzenie blokuje też lot? | **tak, poniżej 25 stanu** helikopter nie wystartuje do czasu naprawy |

## 6. Rozmiar i ryzyko

Duża zmiana: nowy rodzaj żołnierza w drużynie gracza, który nigdy nie może
trafić do taktyki, zmieniony model obrażeń, reguły naprawy, zawartość zapisu.
Trzeba sprawdzić wiele pętli po `OUR_TEAM` / `SOLDIER_VEHICLE` pod kątem
helikoptera. Szacunek: 4 etapy, każdy do zbudowania i przetestowania w grze
przed następnym.
