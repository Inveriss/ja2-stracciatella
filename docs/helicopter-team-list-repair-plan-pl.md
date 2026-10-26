# Plan: helikopter na liście drużyny, z naprawą (Repair)

Polska wersja dokumentu [helicopter-team-list-repair-plan.md](helicopter-team-list-repair-plan.md).

Status: **tylko plan**, niewdrożony (zlecony 2026-10-26, gdy użytkownik
pracował przez Remote Control; zmiana jest zbyt duża, by robić ją bez testów).
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

- Zastąpić `gubHelicopterHitsTaken` przez `bLife` pustego żołnierza (decyzja D2
  co do liczb). Propozycja z zachowaniem oryginalnego charakteru: każde
  trafienie SAM −34 → 66 (kwestia „lekkie uszkodzenie”), 32 (kwestia „poważne
  uszkodzenie”, powrót do bazy), poniżej `OKLIFE` → zniszczenie.
- Progi są liczone ze stanu, więc naprawiony helikopter może znów przyjąć
  trafienia.
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
- `RepairVehicle` już działa na `bLife` (10 punktów naprawy = 1 punkt stanu,
  decyzja D4).
- Zaokrąglanie punktów naprawy w `Faces.cc` już obsługuje pojazdy.

### Etap 4 — limity, teksty, testy

- Miejsce w drużynie: pusty żołnierz zajmuje jedno miejsce `PLAYER_TEAM_SIZE`
  jak Hummer (`PlayerTeamFull` liczy pojazdy) — decyzja D1.
- Linia statusu: tylko stan, bez paliwa (helikopter nie ma modelu paliwa) —
  jak u robota.
- Tłumaczenia: tylko jeśli dojdą nowe komunikaty (np. „helikopter musi
  wylądować, żeby go naprawić”).
- Testy w grze: zdobyć Skyridera, sprawdzić wiersz; przelecieć nad sektorem SAM,
  żeby dostać trafienia; wylądować; naprawić mechanikiem; zniszczyć helikopter
  (3. trafienie) i sprawdzić, że wiersz znika; zapis/odczyt.

## 5. Decyzje dla użytkownika

| # | Pytanie | Propozycja |
|---|---|---|
| D1 | Czy helikopter zajmuje miejsce w drużynie (jak Hummer)? | tak, jak Hummer |
| D2 | Obrażenia na trafienie SAM i progi | −34 na trafienie; < 67 kwestia lekka, < 34 kwestia poważna + powrót do bazy, < `OKLIFE` zniszczenie |
| D3 | Gdzie można go naprawiać? | tylko po wylądowaniu, najemnik w tym samym sektorze (dowolny sektor, nie tylko miejsce tankowania) |
| D4 | Szybkość naprawy | jak Hummer: 10 punktów naprawy = 1 punkt stanu |
| D5 | Kliknięcie wiersza helikoptera, by wyznaczyć trasę | otwiera zwykłe wyznaczanie trasy Skyridera |
| D6 | Czy uszkodzenie blokuje też lot (np. poniżej 34 nie wystartuje do naprawy)? | tak — poniżej 34 helikopter stoi na ziemi do czasu naprawy |

## 6. Rozmiar i ryzyko

Duża zmiana: nowy rodzaj żołnierza w drużynie gracza, który nigdy nie może
trafić do taktyki, zmieniony model obrażeń, reguły naprawy, zawartość zapisu.
Trzeba sprawdzić wiele pętli po `OUR_TEAM` / `SOLDIER_VEHICLE` pod kątem
helikoptera. Szacunek: 4 etapy, każdy do zbudowania i przetestowania w grze
przed następnym.
