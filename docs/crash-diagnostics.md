# Crash diagnostics

When the game ends abnormally it leaves a report behind. Nothing has to be
started by hand: the game installs the handlers itself
(`src/sgp/CrashHandler.cc`) and the launcher shows the result.

## What is written

For every crash (`ja2_crash_<time>`) or detected hang (`ja2_hang_<time>`):

| File | Content |
|------|---------|
| `.summary.txt` | Short summary (exception, location, top of the call stack, game state, last log lines). The launcher copies it into `ja2-launcher.log` and into its logs tab. |
| `.txt` | Full report: exception code/address/access type, registers, call stack with symbols, game state, recent input, recent C++ exceptions, threads, loaded modules (with PDB id), OS/CPU/memory, start and end of `ja2.log`. |
| `.dmp` | Minidump (Windows). Small by default; open it in Visual Studio/WinDbg together with the matching `ja2.exe` and `.pdb`. |

Files go to the per-session directory the launcher creates
(`%LOCALAPPDATA%\JA2\crashes\<session>\`, passed in `JA2_CRASH_DIR`), or next
to `ja2.log` when the game is started without the launcher. Only the newest 5
reports are kept there (`JA2_CRASH_KEEP`); the launcher keeps the newest 20
session directories.

## What is covered

Unhandled SEH exceptions (access violations, ...), stack overflows,
`abort()` and failed assertions (`SLOGA`/`Assert`, with the assertion
message), `std::terminate`, invalid CRT parameters, pure virtual calls,
uncaught C++ exceptions, and hangs (the main loop makes no progress for 60 s).
Fast-fail aborts (`0xC0000409` raised directly by the CRT/`/GS`) cannot be
intercepted in-process; start the launcher with `JA2_WER_LOCALDUMPS=1` to have
Windows Error Reporting write a dump of `ja2.exe` into
`%LOCALAPPDATA%\JA2\crashes\wer` (this sets a per-user registry value under
`HKCU\Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\ja2.exe`).

## Environment variables

| Variable | Effect |
|----------|--------|
| `JA2_CRASH_DIR` | Where reports are written (set by the launcher). |
| `JA2_CRASH_FULL_DUMP=1` | Include the whole process memory in the minidump (hundreds of MB). |
| `JA2_CRASH_KEEP=<n>` | How many reports to keep (default 5). |
| `JA2_WATCHDOG_SECONDS=<n>` | Hang detection threshold, `0` disables (default 60). |
| `JA2_WER_LOCALDUMPS=1` | Launcher only: enable Windows Error Reporting local dumps for `ja2.exe`. |

## Adding game state to the report

Register a small read-only function with `RegisterCrashStateProvider()`; see
`src/game/CrashStateProviders.cc`. It runs inside the crash handler, so it
must only read globals and never allocate.

## Symbols

Reports contain function names and source lines only if the matching `.pdb`
is next to `ja2.exe` (build `RelWithDebInfo`). The full report always lists the
PDB id of the executable, so a report can be matched to the right symbols later.

## Privacy

Reports and dumps contain file paths (including the user name) and process
memory. Nothing is uploaded automatically.
