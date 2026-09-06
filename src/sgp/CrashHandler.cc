#include "CrashHandler.h"

#include "JAScreens.h"
#include "Logger.h"
#include "ScreenIDs.h"
#include "MapScreen.h"
#include "UILayout.h"

#include <cstring>
#include <ctime>
#include <deque>
#include <fstream>
#include <sstream>
#include <string>

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <dbghelp.h>
#else
#	include <csignal>
#	include <execinfo.h>
#	include <fcntl.h>
#	include <unistd.h>
#endif

namespace {

const char* ScreenIdName(ScreenID id)
{
	switch (id)
	{
		case EDIT_SCREEN:               return "EDIT_SCREEN";
		case ERROR_SCREEN:               return "ERROR_SCREEN";
		case INIT_SCREEN:                return "INIT_SCREEN";
		case GAME_SCREEN:                return "GAME_SCREEN";
		case PALEDIT_SCREEN:             return "PALEDIT_SCREEN";
		case DEBUG_SCREEN:               return "DEBUG_SCREEN";
		case MAP_SCREEN:                 return "MAP_SCREEN";
		case LAPTOP_SCREEN:              return "LAPTOP_SCREEN";
		case LOADSAVE_SCREEN:            return "LOADSAVE_SCREEN";
		case MAPUTILITY_SCREEN:          return "MAPUTILITY_SCREEN";
		case FADE_SCREEN:                return "FADE_SCREEN";
		case MSG_BOX_SCREEN:             return "MSG_BOX_SCREEN";
		case MAINMENU_SCREEN:            return "MAINMENU_SCREEN";
		case AUTORESOLVE_SCREEN:         return "AUTORESOLVE_SCREEN";
		case SAVE_LOAD_SCREEN:           return "SAVE_LOAD_SCREEN";
		case OPTIONS_SCREEN:             return "OPTIONS_SCREEN";
		case SHOPKEEPER_SCREEN:          return "SHOPKEEPER_SCREEN";
		case SEX_SCREEN:                 return "SEX_SCREEN";
		case GAME_INIT_OPTIONS_SCREEN:   return "GAME_INIT_OPTIONS_SCREEN";
		case DEMO_EXIT_SCREEN:           return "DEMO_EXIT_SCREEN";
		case INTRO_SCREEN:               return "INTRO_SCREEN";
		case CREDIT_SCREEN:              return "CREDIT_SCREEN";
		case QUEST_DEBUG_SCREEN:         return "QUEST_DEBUG_SCREEN";
		case MAX_SCREENS:                return "MAX_SCREENS";
		case NO_PENDING_SCREEN:          return "NO_PENDING_SCREEN";
		default:                         return "UNKNOWN";
	}
}

// YYYYMMDD_HHMMSS, used both for report filenames and inside their content.
std::string FormatTimestamp()
{
	std::time_t t = std::time(nullptr);
	std::tm tmv;
#ifdef _WIN32
	localtime_s(&tmv, &t);
#else
	localtime_r(&t, &tmv);
#endif
	char buf[32];
	std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tmv);
	return std::string(buf);
}

// Full path to ja2.log, via the same Rust logger that created it -- see
// Logger_getFilePath() usage in Launcher.cc for the established pattern.
std::string GetLogFilePath()
{
	RustPointer<char> logPath(Logger_getFilePath("ja2.log"));
	if (logPath.get() == NULL) return std::string();
	return std::string(logPath.get());
}

std::string GetDiagnosticsDir(const std::string& logFilePath)
{
	size_t const pos = logFilePath.find_last_of("/\\");
	return (pos == std::string::npos) ? std::string(".") : logFilePath.substr(0, pos);
}

void AppendLogTail(std::ostream& out, const std::string& logFilePath, size_t const maxLines = 200)
{
	std::ifstream in(logFilePath.c_str());
	if (!in)
	{
		out << "(could not open " << logFilePath << ")\n";
		return;
	}

	std::deque<std::string> lines;
	std::string line;
	while (std::getline(in, line))
	{
		lines.push_back(std::move(line));
		if (lines.size() > maxLines) lines.pop_front();
	}
	for (std::string const& l : lines) out << l << "\n";
}

// Shared report body for all three ways a session can end -- see
// WriteExceptionDiagnosticReport()/WriteCleanExitDiagnosticReport() below
// and the native-crash handlers further down. `details` is caller-supplied
// free text (exception message, native exception code/address/stack trace,
// or nullptr for a clean exit); `includeLogTail` appends the recent tail of
// ja2.log so everything relevant lives in one file.
void WriteReportFile(std::string const& path, const char* reason, const char* details, bool includeLogTail)
{
	std::ofstream out(path.c_str(), std::ios::out | std::ios::trunc);
	if (!out) return;

	out << "JA2 Stracciatella diagnostic report\n";
	out << "Timestamp: " << FormatTimestamp() << "\n";
	out << "Reason: " << reason << "\n\n";

	if (details && details[0] != '\0')
	{
		out << "Details:\n" << details << "\n\n";
	}

	// Best-effort snapshot of a few, already-globally-visible state
	// variables. Deliberately kept small and read-only -- add more fields
	// here later if a future crash needs them.
	out << "Game state snapshot:\n";
	out << "  Current screen ID: " << (int)guiCurrentScreen << " (" << ScreenIdName(guiCurrentScreen) << ")\n";
	out << "  Resolution: " << SCREEN_WIDTH << "x" << SCREEN_HEIGHT << "\n";
	out << "  In map mode: " << (fInMapMode ? "yes" : "no") << "\n\n";

	if (includeLogTail)
	{
		std::string const logFilePath = GetLogFilePath();
		out << "---- last lines of " << logFilePath << " ----\n";
		AppendLogTail(out, logFilePath);
	}
}

} // namespace


#ifdef _WIN32

namespace {

void WriteMiniDump(EXCEPTION_POINTERS* info, std::string const& path)
{
	HANDLE const file = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE) return;

	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = GetCurrentThreadId();
	mei.ExceptionPointers = info;
	mei.ClientPointers = FALSE;

	// As deep as practically possible: full process memory (so any
	// variable/container can be inspected in Visual Studio/WinDbg later),
	// plus handle and thread info. Produces a large file, by design.
	MINIDUMP_TYPE const type = (MINIDUMP_TYPE)(
		MiniDumpWithFullMemory |
		MiniDumpWithHandleData |
		MiniDumpWithThreadInfo |
		MiniDumpWithUnloadedModules);

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type, info ? &mei : nullptr, nullptr, nullptr);
	CloseHandle(file);
}

// Best-effort: symbol/line resolution only succeeds if matching PDBs are
// findable (e.g. a Debug build, or Release with PDBs alongside the exe).
// Even without symbols, the raw addresses are still useful for manual
// module+offset lookup, and the minidump remains the authoritative artifact.
void AppendStackTrace(std::ostringstream& out)
{
	constexpr int kMaxFrames = 64;
	void* frames[kMaxFrames];
	USHORT const captured = CaptureStackBackTrace(0, kMaxFrames, frames, nullptr);

	HANDLE const process = GetCurrentProcess();
	bool const symbolsReady = SymInitialize(process, nullptr, TRUE) != FALSE;

	out << "Stack trace (" << captured << " frames):\n";
	for (USHORT i = 0; i < captured; ++i)
	{
		DWORD64 const addr = (DWORD64)(frames[i]);
		out << "  #" << i << "  0x" << std::hex << addr << std::dec;

		if (symbolsReady)
		{
			alignas(SYMBOL_INFO) char symBuf[sizeof(SYMBOL_INFO) + 256];
			SYMBOL_INFO* const symbol = (SYMBOL_INFO*)symBuf;
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol->MaxNameLen = 256;
			DWORD64 displacement = 0;
			if (SymFromAddr(process, addr, &displacement, symbol))
			{
				out << "  " << symbol->Name << " +0x" << std::hex << displacement << std::dec;
			}

			DWORD lineDisplacement = 0;
			IMAGEHLP_LINE64 line{};
			line.SizeOfStruct = sizeof(line);
			if (SymGetLineFromAddr64(process, addr, &lineDisplacement, &line))
			{
				out << "  (" << line.FileName << ":" << line.LineNumber << ")";
			}
		}
		out << "\n";
	}

	if (symbolsReady) SymCleanup(process);
}

LONG WINAPI NativeCrashFilter(EXCEPTION_POINTERS* info)
{
	std::string const logFilePath = GetLogFilePath();
	std::string const dir = GetDiagnosticsDir(logFilePath);
	std::string const base = dir + "\\ja2_crash_" + FormatTimestamp();

	// Dump first: it's the reliable artifact and needs the fewest
	// allocations of our own, in case the heap is already unhappy.
	WriteMiniDump(info, base + ".dmp");

	std::ostringstream details;
	details << "Unhandled native exception 0x" << std::hex << info->ExceptionRecord->ExceptionCode << std::dec
	        << " at address " << info->ExceptionRecord->ExceptionAddress << "\n";
	AppendStackTrace(details);

	WriteReportFile(base + ".txt", "native crash (unhandled SEH exception)", details.str().c_str(), true);

	return EXCEPTION_EXECUTE_HANDLER;
}

} // namespace

void InstallCrashHandler()
{
	static bool installed = false;
	if (installed) return;
	installed = true;

	SetUnhandledExceptionFilter(NativeCrashFilter);
}

#else // !_WIN32

namespace {

// Deliberately minimal and POSIX-only-safe (no C++ allocations): a fatal
// signal handler can't safely assume the heap is in a usable state. No
// minidump equivalent, no log tail/game-state snapshot here -- just enough
// to know it crashed and where.
void NativeCrashSignalHandler(int sig)
{
	std::string const logFilePath = GetLogFilePath();
	std::string const dir = GetDiagnosticsDir(logFilePath);
	std::string const path = dir + "/ja2_crash_" + FormatTimestamp() + ".txt";

	int const fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd >= 0)
	{
		dprintf(fd, "JA2 Stracciatella diagnostic report\nReason: native crash (signal %d)\n\nStack trace:\n", sig);
		void* frames[64];
		int const n = backtrace(frames, 64);
		backtrace_symbols_fd(frames, n, fd);
		close(fd);
	}

	// Restore the default handler and re-raise, so the OS still does
	// whatever it would normally do (core dump, standard exit code, etc.).
	signal(sig, SIG_DFL);
	raise(sig);
}

} // namespace

void InstallCrashHandler()
{
	static bool installed = false;
	if (installed) return;
	installed = true;

	struct sigaction sa {};
	sa.sa_handler = NativeCrashSignalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	for (int const sig : { SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS })
	{
		sigaction(sig, &sa, nullptr);
	}
}

#endif


void WriteExceptionDiagnosticReport(const char* what)
{
	std::string const logFilePath = GetLogFilePath();
	std::string const dir = GetDiagnosticsDir(logFilePath);
	std::string const path = dir + "/ja2_crash_" + FormatTimestamp() + ".txt";

	WriteReportFile(path, "uncaught C++ exception", what, true);
}

void WriteCleanExitDiagnosticReport()
{
	std::string const logFilePath = GetLogFilePath();
	std::string const dir = GetDiagnosticsDir(logFilePath);
	std::string const path = dir + "/ja2_last_session.txt";

	WriteReportFile(path, "clean exit", nullptr, true);
}
