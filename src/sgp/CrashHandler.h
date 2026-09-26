#ifndef __CRASH_HANDLER_
#define __CRASH_HANDLER_

// Post-mortem diagnostics. Installs handlers for every way a session can
// end abnormally (unhandled SEH exceptions, stack overflows, abort()/failed
// asserts, std::terminate, invalid CRT parameters/pure virtual calls, fatal
// signals on POSIX) and a watchdog for hangs. For each of them it writes,
// next to ja2.log (or into $JA2_CRASH_DIR when the launcher provides one):
//
//   ja2_crash_<time>.txt          the full report (state, registers, stack
//                                 with symbols, threads, modules, recent
//                                 C++ exceptions/input, log head and tail)
//   ja2_crash_<time>.summary.txt  a short summary the launcher copies into
//                                 its own log
//   ja2_crash_<time>.dmp          a minidump (Windows). Small by default,
//                                 full memory if $JA2_CRASH_FULL_DUMP is set
//
// Hangs use the ja2_hang_ prefix. Only the newest few reports are kept.
//
// Call InstallCrashHandler() once, as early as possible in main() (right
// after Logger_initialize(), so the report can be written next to the log
// file and its own tail included).

/** Install the native crash handlers. Safe to call multiple times; only the
 * first call has an effect. */
void InstallCrashHandler();

/** Write a diagnostic report for an uncaught C++ exception. `what` is the
 * exception's own message (already formatted by the caller), or an empty
 * string if unknown. */
void WriteExceptionDiagnosticReport(const char* what);

/** Called by SLOGA()/Assert() right before the process is aborted. Writes a
 * report and dump that contain the assertion message and the call stack
 * of the failing assertion. */
void CrashHandlerAssertFailed(const char* file, const char* message);

/** Write a lightweight diagnostic report for a normal, successful game
 * shutdown. Overwrites ja2_last_session.txt next to ja2.log (unlike crash
 * reports, which each get their own timestamped file) so a clean exit
 * doesn't litter the temp directory run after run. */
void WriteCleanExitDiagnosticReport();

// ---- Game state ----------------------------------------------------------

/** Subsystems register a small read-only function that appends a few
 * "key: value" lines describing their state. Called from inside the crash
 * handler (guarded against faults), so keep it allocation-light and never
 * follow pointers that may be dangling without checking them. */
typedef void (*CrashStateProvider)(char* out, unsigned outSize);
void RegisterCrashStateProvider(const char* name, CrashStateProvider provider);

/** Append text to a provider's output buffer (printf-like). */
void CrashStateAppend(char* out, unsigned outSize, const char* fmt, ...);

/** Remember the most recent user input (kind: 1 key down, 2 mouse button
 * down, 3 mouse button up, 4 mouse wheel). Cheap enough to call per event. */
void CrashHandlerRecordInput(int kind, int a, int b, int c);

// ---- Hang detection ------------------------------------------------------

/** Start the hang watchdog for the calling (main loop) thread. Fires when
 * CrashHandlerHeartbeat() has not been called for $JA2_WATCHDOG_SECONDS
 * (default 60, 0 disables). */
void CrashHandlerStartWatchdog();

/** Call once per main loop iteration. */
void CrashHandlerHeartbeat();

/** The main loop is deliberately blocked (e.g. window in the background). */
void CrashHandlerPauseWatchdog(bool paused);

#endif
