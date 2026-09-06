#ifndef __CRASH_HANDLER_
#define __CRASH_HANDLER_

// Post-mortem diagnostics: installs a handler for native crashes (SEH
// unhandled exceptions on Windows, fatal signals elsewhere) that writes a
// timestamped report (and, on Windows, a full minidump) next to ja2.log,
// and provides the same report-writing logic for the two other ways the
// game can end -- an uncaught C++ exception (see TerminationHandler() in
// SGP.cc) and a normal, successful shutdown -- so that every session
// leaves something behind to look at, not just crashes.
//
// Call InstallCrashHandler() once, as early as possible in main() (right
// after Logger_initialize(), so the report can be written next to the log
// file and its own tail included).

/** Install the native crash handler (SetUnhandledExceptionFilter on
 * Windows, sigaction for fatal signals elsewhere). Safe to call multiple
 * times; only the first call has an effect. */
void InstallCrashHandler();

/** Write a diagnostic report for an uncaught C++ exception. `what` is the
 * exception's own message (already formatted by the caller), or an empty
 * string if unknown. Writes ja2_crash_<timestamp>.txt next to ja2.log. */
void WriteExceptionDiagnosticReport(const char* what);

/** Write a lightweight diagnostic report for a normal, successful game
 * shutdown. Overwrites ja2_last_session.txt next to ja2.log (unlike crash
 * reports, which each get their own timestamped file) so a clean exit
 * doesn't litter the temp directory run after run. */
void WriteCleanExitDiagnosticReport();

#endif
