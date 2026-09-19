# Merc profile data

The named characters (A.I.M., M.E.R.C., I.M.P., R.P.C., N.P.C., vehicles; 170 profiles)
are defined by JSON files in `assets/externalized`:

| File | Content |
|------|---------|
| `mercs-profile-info.json` | Every profile: type, stats, traits, contract prices, inventory, dialogue parameters, ... |
| `mercs-profile-names-<language>.json` | Full name and nickname in one game language (`-eng` is provided) |
| `mercs-relations.json` | Opinions, friends and enemies of each profile |

The original `prof.dat` is no longer needed for the English version. It is only read

* for a language that has no `mercs-profile-names-<language>.json` (the names then come
  from it; everything else comes from the JSON files), or
* when the environment variable `JA2_USE_PROF_DAT=1` is set (the JSON files are then merged
  into `prof.dat` as they used to be).

Inventory slots in `prof.dat` use the original 19-slot order; they are mapped to the current
slots when it is read.

## Generating the files

`JA2_DUMP_MERC_PROFILES=<directory>` makes the game write the fully merged profiles as
`mercs-profile-info.json`, `mercs-profile-names-<language>.json` and `mercs-relations.json`
into that directory and continue starting up (stop it afterwards). Use it with
`JA2_USE_PROF_DAT=1` and the unmodified `prof.dat` of the game version to produce the files
for another language, or after editing profiles to normalize the JSON.

## Adding a language

Run the dump with the game data of that language (and `JA2_USE_PROF_DAT=1`) and copy only
the `mercs-profile-names-<language>.json` file; the other two files are language independent.
