#include "CrashHandler.h"

#include "GameLoop.h"
#include "GameVersion.h"
#include "JAScreens.h"
#include "Logger.h"
#include "MapScreen.h"
#include "ScreenIDs.h"
#include "Sys_Globals.h"
#include "UILayout.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <typeinfo>
#include <vector>

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	define PSAPI_VERSION 2
#	include <windows.h>
#	include <dbghelp.h>
#	include <psapi.h>
#	include <tlhelp32.h>
#	include <intrin.h>
#	include <signal.h>
#	include <stdlib.h>
#else
#	include <csignal>
#	include <execinfo.h>
#	include <fcntl.h>
#	include <unistd.h>
#endif

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Portable part: state providers, recent input, report layout
// ---------------------------------------------------------------------------

namespace {

constexpr int kMaxProviders = 16;
constexpr unsigned kProviderBuffer = 1024;
constexpr int kInputRing = 64;
constexpr size_t kLogTailLines = 200;
constexpr size_t kLogHeadLines = 40;
constexpr size_t kSummaryLogLines = 15;
constexpr size_t kSummaryFrames = 10;
constexpr int kDefaultKeepReports = 5;

struct ProviderEntry
{
	const char*        name;
	CrashStateProvider fn;
};

ProviderEntry    g_providers[kMaxProviders];
std::atomic<int> g_providerCount{0};

struct InputEvent
{
	long long ms;
	int       kind;
	int       a;
	int       b;
	int       c;
};

InputEvent g_input[kInputRing];
std::atomic<unsigned> g_inputPos{0};

// 0 = running, 1 = a fatal report is being (or has been) written
#ifdef _WIN32
volatile LONG g_fatalState = 0;
#else
volatile sig_atomic_t g_fatalState = 0;
#endif

char g_assertMessage[1024] = "";

long long NowMs()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

long long const g_startMs = NowMs();

std::string EnvString(const char* name)
{
	const char* v = std::getenv(name);
	return v ? std::string(v) : std::string();
}

const char* ScreenIdName(ScreenID id)
{
	switch (id)
	{
		case EDIT_SCREEN:                return "EDIT_SCREEN";
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

// Full path to ja2.log, via the same Rust logger that created it.
std::string GetLogFilePath()
{
	RustPointer<char> logPath(Logger_getFilePath("ja2.log"));
	if (logPath.get() == NULL) return std::string();
	return std::string(logPath.get());
}

// Directory of ja2.log.
std::string GetLogDir()
{
	std::string const logFilePath = GetLogFilePath();
	size_t const pos = logFilePath.find_last_of("/\\");
	return (pos == std::string::npos) ? std::string(".") : logFilePath.substr(0, pos);
}

// Where reports go: $JA2_CRASH_DIR (set per session by the launcher), else
// next to ja2.log.
std::string GetDiagnosticsDir()
{
	std::string dir = EnvString("JA2_CRASH_DIR");
	if (dir.empty())
	{
		std::string const logFilePath = GetLogFilePath();
		size_t const pos = logFilePath.find_last_of("/\\");
		dir = (pos == std::string::npos) ? std::string(".") : logFilePath.substr(0, pos);
	}
	std::error_code ec;
	fs::create_directories(fs::path(dir), ec);
	return dir;
}

void AppendLogHead(std::ostream& out, const std::string& logFilePath, size_t const maxLines)
{
	std::ifstream in(logFilePath.c_str());
	if (!in)
	{
		out << "(could not open " << logFilePath << ")\n";
		return;
	}
	std::string line;
	for (size_t i = 0; i < maxLines && std::getline(in, line); ++i) out << line << "\n";
}

std::deque<std::string> ReadLogTail(const std::string& logFilePath, size_t const maxLines)
{
	std::deque<std::string> lines;
	std::ifstream in(logFilePath.c_str());
	if (!in) return lines;

	std::string line;
	while (std::getline(in, line))
	{
		lines.push_back(std::move(line));
		if (lines.size() > maxLines) lines.pop_front();
	}
	return lines;
}

#ifdef _MSC_VER
bool SafeCallProvider(CrashStateProvider fn, char* buf, unsigned size)
{
	__try
	{
		fn(buf, size);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}
#else
bool SafeCallProvider(CrashStateProvider fn, char* buf, unsigned size)
{
	fn(buf, size);
	return true;
}
#endif

// Lines describing the game itself. One line per fact, so the same text can
// be used in the full report and in the summary.
std::vector<std::string> CollectGameState()
{
	std::vector<std::string> lines;
	auto add = [&lines](std::string const& s) { lines.push_back(s); };

	add(std::string("Current screen: ") + std::to_string((int)guiCurrentScreen) + " (" + ScreenIdName(guiCurrentScreen) + ")");
	add(std::string("Pending screen: ") + std::to_string((int)guiPendingScreen) + " (" + ScreenIdName(guiPendingScreen) + ")");
	add("Resolution: " + std::to_string(SCREEN_WIDTH) + "x" + std::to_string(SCREEN_HEIGHT));
	add(std::string("In map mode: ") + (fInMapMode ? "yes" : "no"));
	add("Game loop cycles: " + std::to_string(guiGameCycleCounter));

	int const n = g_providerCount.load();
	for (int i = 0; i < n && i < kMaxProviders; ++i)
	{
		char buf[kProviderBuffer];
		buf[0] = '\0';
		if (!SafeCallProvider(g_providers[i].fn, buf, sizeof(buf)))
		{
			add(std::string(g_providers[i].name) + ": (state provider failed)");
			continue;
		}
		buf[sizeof(buf) - 1] = '\0';
		add(std::string(g_providers[i].name) + ": " + buf);
	}
	return lines;
}

void AppendRecentInput(std::ostream& out)
{
	unsigned const pos = g_inputPos.load();
	long long const now = NowMs();
	int shown = 0;
	for (int i = 1; i <= kInputRing && shown < 24; ++i)
	{
		if (pos < (unsigned)i) break;
		InputEvent const& e = g_input[(pos - i) % kInputRing];
		if (e.ms == 0) continue;
		char line[160];
		char const* kindName = "?";
		switch (e.kind)
		{
			case 1: kindName = "key down"; break;
			case 2: kindName = "mouse down"; break;
			case 3: kindName = "mouse up"; break;
			case 4: kindName = "mouse wheel"; break;
		}
		if (e.kind == 1)
		{
			std::snprintf(line, sizeof(line), "  -%6.2fs  %s, keycode %d, modifiers 0x%x\n", (now - e.ms) / 1000.0, kindName, e.a, e.b);
		}
		else
		{
			std::snprintf(line, sizeof(line), "  -%6.2fs  %s, x=%d y=%d button/delta=%d\n", (now - e.ms) / 1000.0, kindName, e.a, e.b, e.c);
		}
		out << line;
		++shown;
	}
	if (shown == 0) out << "  (none recorded)\n";
}

// ---- Report file management ------------------------------------------------

// A single crash: what the writers need to know. Filled by the platform
// specific code below.
struct CrashInfo
{
	const char*              kind = "crash";   // file prefix: crash or hang
	std::string              reason;           // one line
	std::string              details;          // several lines (exception, registers, ...)
	std::vector<std::string> stack;            // formatted frames, innermost first
	std::string              extraSections;    // platform specific (threads, modules, ...)
};

void RotateReports(const std::string& dir)
{
	int keep = kDefaultKeepReports;
	std::string const keepEnv = EnvString("JA2_CRASH_KEEP");
	if (!keepEnv.empty()) keep = std::max(1, std::atoi(keepEnv.c_str()));

	struct Entry { fs::file_time_type time; std::string stem; };
	std::vector<Entry> reports;

	std::error_code ec;
	for (fs::directory_iterator it(fs::path(dir), ec), end; !ec && it != end; it.increment(ec))
	{
		std::string const name = it->path().filename().string();
		bool const isReport = (name.rfind("ja2_crash_", 0) == 0 || name.rfind("ja2_hang_", 0) == 0);
		if (!isReport) continue;
		if (it->path().extension() != ".txt") continue;
		if (name.size() > 12 && name.compare(name.size() - 12, 12, ".summary.txt") == 0) continue;

		std::error_code ec2;
		auto const t = fs::last_write_time(it->path(), ec2);
		reports.push_back({ec2 ? fs::file_time_type::min() : t, it->path().stem().string()});
	}

	if ((int)reports.size() <= keep) return;
	std::sort(reports.begin(), reports.end(), [](Entry const& l, Entry const& r) { return l.time > r.time; });
	for (size_t i = keep; i < reports.size(); ++i)
	{
		fs::path const base = fs::path(dir) / reports[i].stem;
		std::error_code ec3;
		fs::remove(fs::path(base.string() + ".txt"), ec3);
		fs::remove(fs::path(base.string() + ".summary.txt"), ec3);
		fs::remove(fs::path(base.string() + ".dmp"), ec3);
	}
}

#ifdef _WIN32
std::string ProcessDetailsText();       // defined below
std::string SystemInfoText();
std::string RecentCppExceptionsText();
#else
std::string ProcessDetailsText() { return std::string(); }
std::string SystemInfoText() { return std::string(); }
std::string RecentCppExceptionsText() { return std::string(); }
#endif

void AppendHeader(std::ostream& out, const char* title, const std::string& reason)
{
	out << title << "\n";
	out << "Timestamp: " << FormatTimestamp() << "\n";
	out << "Reason: " << reason << "\n";
	out << "Version: " << g_version_label << " (this file compiled " << __DATE__ << " " << __TIME__ << ")\n";
	out << "Session uptime: " << (NowMs() - g_startMs) / 1000 << " s\n";
	std::string const session = EnvString("JA2_SESSION_ID");
	if (!session.empty()) out << "Session id: " << session << "\n";
}

void WriteFullReport(const std::string& path, const char* title, CrashInfo const& info, bool includeLogTail)
{
	// Unbuffered: if a crash happens while writing, what is already there survives.
	std::ofstream out;
	out.rdbuf()->pubsetbuf(nullptr, 0);
	out.open(path.c_str(), std::ios::out | std::ios::trunc);
	if (!out) return;

	AppendHeader(out, title, info.reason);
	out << "\n";

	if (!info.details.empty()) out << "Details:\n" << info.details << "\n";

	if (!info.stack.empty())
	{
		out << "Call stack (innermost first):\n";
		for (std::string const& f : info.stack) out << f << "\n";
		out << "\n";
	}

	out << "Game state:\n";
	for (std::string const& l : CollectGameState()) out << "  " << l << "\n";
	out << "\n";

	out << "Recent input (newest first):\n";
	AppendRecentInput(out);
	out << "\n";

	out.flush();
	std::string const cpp = RecentCppExceptionsText();
	if (!cpp.empty()) out << cpp << "\n";

	std::string const proc = ProcessDetailsText();
	if (!proc.empty()) out << proc << "\n";

	std::string const sys = SystemInfoText();
	if (!sys.empty()) out << sys << "\n";

	if (!info.extraSections.empty()) out << info.extraSections << "\n";

	if (includeLogTail)
	{
		std::string const logFilePath = GetLogFilePath();
		out << "---- first lines of " << logFilePath << " (start-up configuration) ----\n";
		AppendLogHead(out, logFilePath, kLogHeadLines);
		out << "\n---- last lines of " << logFilePath << " ----\n";
		for (std::string const& l : ReadLogTail(logFilePath, kLogTailLines)) out << l << "\n";
	}
}

// Short text the launcher copies into ja2-launcher.log.
void WriteSummary(const std::string& path, CrashInfo const& info, const std::string& reportPath, const std::string& dumpPath)
{
	std::ofstream out(path.c_str(), std::ios::out | std::ios::trunc);
	if (!out) return;

	AppendHeader(out, "JA2 Stracciatella crash summary", info.reason);

	// details: first few lines only (exception code, address, access type)
	{
		std::istringstream in(info.details);
		std::string line;
		int n = 0;
		while (n < 4 && std::getline(in, line))
		{
			if (line.empty()) continue;
			out << line << "\n";
			++n;
		}
	}

	if (!info.stack.empty())
	{
		out << "Top of the call stack:\n";
		for (size_t i = 0; i < info.stack.size() && i < kSummaryFrames; ++i) out << info.stack[i] << "\n";
	}

	out << "Game state:\n";
	{
		std::vector<std::string> const state = CollectGameState();
		for (std::string const& l : state) out << "  " << l << "\n";
	}

	out << "Full report: " << reportPath << "\n";
	if (!dumpPath.empty()) out << "Minidump:    " << dumpPath << "\n";

	std::deque<std::string> const tail = ReadLogTail(GetLogFilePath(), kSummaryLogLines);
	out << "Last " << tail.size() << " log lines:\n";
	for (std::string const& l : tail) out << "  " << l << "\n";
}


// Platform specific: dump writing, defined below.
bool WriteMiniDumpImpl(void* exceptionPointers, unsigned threadId, const std::string& path);

// Writes dump (Windows), full report and summary, then prunes old reports.
void WriteReportSet(CrashInfo& info, void* exceptionPointers, unsigned threadId)
{
	std::string const dir = GetDiagnosticsDir();
	std::string const stem = std::string("ja2_") + info.kind + "_" + FormatTimestamp();
	std::string const base = (fs::path(dir) / stem).string();

	// Dump first: it's the reliable artifact and needs the fewest
	// allocations of our own, in case the heap is already unhappy.
	std::string dumpPath = base + ".dmp";
	if (!WriteMiniDumpImpl(exceptionPointers, threadId, dumpPath)) dumpPath.clear();

	WriteFullReport(base + ".txt", "JA2 Stracciatella diagnostic report", info, true);
	WriteSummary(base + ".summary.txt", info, base + ".txt", dumpPath);
	RotateReports(dir);
}

// Reports a fatal condition detected by our own code (abort, terminate,
// failed assertion, ...). Only the first fatal report of a session is
// written; later ones (e.g. the SIGABRT that follows an assertion) are
// duplicates. `skipFrames` hides the handler's own frames from the stack.
void ReportFatal(const char* reason, const std::string& details, unsigned skipFrames);

} // namespace

// ---------------------------------------------------------------------------
// Public, platform independent API
// ---------------------------------------------------------------------------

void RegisterCrashStateProvider(const char* name, CrashStateProvider provider)
{
	int const i = g_providerCount.load();
	if (i >= kMaxProviders) return;
	g_providers[i] = {name, provider};
	g_providerCount.store(i + 1);
}

void CrashStateAppend(char* out, unsigned outSize, const char* fmt, ...)
{
	size_t const used = std::strlen(out);
	if (used + 1 >= outSize) return;
	va_list args;
	va_start(args, fmt);
	std::vsnprintf(out + used, outSize - used, fmt, args);
	va_end(args);
}

void CrashHandlerRecordInput(int kind, int a, int b, int c)
{
	unsigned const i = g_inputPos.fetch_add(1) % kInputRing;
	g_input[i] = {NowMs(), kind, a, b, c};
}

void WriteExceptionDiagnosticReport(const char* what)
{
	ReportFatal("uncaught C++ exception", what ? what : "", 2);
}

void CrashHandlerAssertFailed(const char* file, const char* message)
{
	std::snprintf(g_assertMessage, sizeof(g_assertMessage), "%s: %s", file ? file : "?", message ? message : "");
	ReportFatal("assertion failed", std::string("Assertion failed in ") + g_assertMessage, 2);
}

void WriteCleanExitDiagnosticReport()
{
	CrashInfo info;
	info.reason = "clean exit";
	WriteFullReport((fs::path(GetLogDir()) / "ja2_last_session.txt").string(), "JA2 Stracciatella diagnostic report", info, true);
}

#ifdef _WIN32

// ---------------------------------------------------------------------------
// Windows
// ---------------------------------------------------------------------------

namespace {

constexpr int kExceptionRing = 16;
constexpr unsigned kMaxFrames = 64;

struct CppExceptionRecord
{
	long long ms;
	DWORD     threadId;
	char      type[128];
	char      what[200];
	void*     frames[16];
	unsigned  frameCount;
};

CppExceptionRecord    g_exceptions[kExceptionRing];
std::atomic<unsigned> g_exceptionPos{0};

bool g_symbolsReady = false;

void EnsureSymbols()
{
	if (g_symbolsReady) return;
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_FAIL_CRITICAL_ERRORS);
	g_symbolsReady = SymInitialize(GetCurrentProcess(), nullptr, TRUE) != FALSE;
}

const char* ExceptionName(DWORD code)
{
	switch (code)
	{
		case EXCEPTION_ACCESS_VIOLATION:         return "ACCESS_VIOLATION";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "ARRAY_BOUNDS_EXCEEDED";
		case EXCEPTION_BREAKPOINT:               return "BREAKPOINT";
		case EXCEPTION_DATATYPE_MISALIGNMENT:    return "DATATYPE_MISALIGNMENT";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "FLT_DIVIDE_BY_ZERO";
		case EXCEPTION_FLT_INVALID_OPERATION:    return "FLT_INVALID_OPERATION";
		case EXCEPTION_ILLEGAL_INSTRUCTION:      return "ILLEGAL_INSTRUCTION";
		case EXCEPTION_IN_PAGE_ERROR:            return "IN_PAGE_ERROR";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "INT_DIVIDE_BY_ZERO";
		case EXCEPTION_INT_OVERFLOW:             return "INT_OVERFLOW";
		case EXCEPTION_PRIV_INSTRUCTION:         return "PRIV_INSTRUCTION";
		case EXCEPTION_STACK_OVERFLOW:           return "STACK_OVERFLOW";
		case 0xC0000409:                         return "STACK_BUFFER_OVERRUN / FAST_FAIL";
		case 0xC0000374:                         return "HEAP_CORRUPTION";
		case 0xC0000420:                         return "ASSERTION_FAILURE";
		case 0x40000015:                         return "FATAL_APP_EXIT";
		case 0xE06D7363:                         return "C++ exception";
		default:                                 return "unknown";
	}
}

std::string BaseName(const std::string& path)
{
	size_t const pos = path.find_last_of("/\\");
	return pos == std::string::npos ? path : path.substr(pos + 1);
}

// "module.exe" and the offset of `addr` inside it, or "" when unknown.
std::string ModuleOf(DWORD64 addr, DWORD64* offset)
{
	HMODULE hm = nullptr;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)addr, &hm) || !hm)
	{
		*offset = 0;
		return std::string();
	}
	char path[MAX_PATH];
	DWORD const n = GetModuleFileNameA(hm, path, MAX_PATH);
	*offset = addr - (DWORD64)hm;
	return n ? BaseName(std::string(path, n)) : std::string();
}

std::string FormatFrame(unsigned index, DWORD64 addr)
{
	char head[64];
	std::snprintf(head, sizeof(head), "  #%-2u 0x%016llx  ", index, (unsigned long long)addr);
	std::string s = head;

	DWORD64 modOffset = 0;
	std::string const mod = ModuleOf(addr, &modOffset);

	bool haveSymbol = false;
	if (g_symbolsReady)
	{
		alignas(SYMBOL_INFO) char symBuf[sizeof(SYMBOL_INFO) + 256];
		SYMBOL_INFO* const symbol = (SYMBOL_INFO*)symBuf;
		std::memset(symbol, 0, sizeof(SYMBOL_INFO));
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = 256;
		DWORD64 displacement = 0;
		if (SymFromAddr(GetCurrentProcess(), addr, &displacement, symbol))
		{
			char off[32];
			std::snprintf(off, sizeof(off), " +0x%llx", (unsigned long long)displacement);
			s += (mod.empty() ? std::string() : mod + "!") + symbol->Name + off;
			haveSymbol = true;

			DWORD lineDisplacement = 0;
			IMAGEHLP_LINE64 line;
			std::memset(&line, 0, sizeof(line));
			line.SizeOfStruct = sizeof(line);
			if (SymGetLineFromAddr64(GetCurrentProcess(), addr, &lineDisplacement, &line))
			{
				s += std::string("  (") + line.FileName + ":" + std::to_string(line.LineNumber) + ")";
			}
		}
	}
	if (!haveSymbol)
	{
		char off[40];
		std::snprintf(off, sizeof(off), "+0x%llx", (unsigned long long)modOffset);
		s += mod.empty() ? std::string("(unknown module)") : mod + off;
	}
	return s;
}

// Walks the stack of `thread` starting at `ctx` (the faulting context for
// crashes, so the handler's own frames are not in the trace).
std::vector<std::string> CollectStack(HANDLE thread, CONTEXT ctx, unsigned skip, unsigned maxFrames)
{
	std::vector<std::string> frames;
#if defined(_M_X64)
	STACKFRAME64 sf;
	std::memset(&sf, 0, sizeof(sf));
	sf.AddrPC.Offset    = ctx.Rip;
	sf.AddrPC.Mode      = AddrModeFlat;
	sf.AddrFrame.Offset = ctx.Rsp;
	sf.AddrFrame.Mode   = AddrModeFlat;
	sf.AddrStack.Offset = ctx.Rsp;
	sf.AddrStack.Mode   = AddrModeFlat;

	HANDLE const process = GetCurrentProcess();
	unsigned index = 0;

	// A call through a null function pointer leaves Rip at 0 and the return
	// address on top of the stack; the walker can't start from there.
	if (ctx.Rip == 0 && ctx.Rsp != 0)
	{
		DWORD64 returnAddress = 0;
		SIZE_T bytesRead = 0;
		if (ReadProcessMemory(process, (LPCVOID)ctx.Rsp, &returnAddress, sizeof(returnAddress), &bytesRead) && bytesRead == sizeof(returnAddress))
		{
			frames.push_back(FormatFrame(0, 0) + "  (call through a null function pointer)");
			ctx.Rip = returnAddress;
			ctx.Rsp += sizeof(returnAddress);
			sf.AddrPC.Offset = ctx.Rip;
			sf.AddrStack.Offset = ctx.Rsp;
			sf.AddrFrame.Offset = ctx.Rsp;
		}
	}

	while (frames.size() < maxFrames &&
	       StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &sf, &ctx, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
	{
		if (sf.AddrPC.Offset == 0) break;
		if (index++ < skip) continue;
		frames.push_back(FormatFrame((unsigned)frames.size(), sf.AddrPC.Offset));
	}
#else
	(void)thread; (void)ctx;
	void* raw[kMaxFrames];
	USHORT const n = CaptureStackBackTrace(skip, std::min<unsigned>(maxFrames, kMaxFrames), raw, nullptr);
	for (USHORT i = 0; i < n; ++i) frames.push_back(FormatFrame(i, (DWORD64)raw[i]));
#endif
	return frames;
}

DWORD64 ContextIp(const CONTEXT& ctx)
{
#if defined(_M_X64)
	return ctx.Rip;
#else
	(void)ctx;
	return 0;
#endif
}

std::string RegistersText(const CONTEXT& ctx)
{
#if defined(_M_X64)
	char buf[640];
	std::snprintf(buf, sizeof(buf),
		"Registers:\n"
		"  rip=%016llx rsp=%016llx rbp=%016llx eflags=%08lx\n"
		"  rax=%016llx rbx=%016llx rcx=%016llx rdx=%016llx\n"
		"  rsi=%016llx rdi=%016llx r8 =%016llx r9 =%016llx\n"
		"  r10=%016llx r11=%016llx r12=%016llx r13=%016llx\n"
		"  r14=%016llx r15=%016llx\n",
		(unsigned long long)ctx.Rip, (unsigned long long)ctx.Rsp, (unsigned long long)ctx.Rbp, (unsigned long)ctx.EFlags,
		(unsigned long long)ctx.Rax, (unsigned long long)ctx.Rbx, (unsigned long long)ctx.Rcx, (unsigned long long)ctx.Rdx,
		(unsigned long long)ctx.Rsi, (unsigned long long)ctx.Rdi, (unsigned long long)ctx.R8, (unsigned long long)ctx.R9,
		(unsigned long long)ctx.R10, (unsigned long long)ctx.R11, (unsigned long long)ctx.R12, (unsigned long long)ctx.R13,
		(unsigned long long)ctx.R14, (unsigned long long)ctx.R15);
	return buf;
#else
	(void)ctx;
	return std::string();
#endif
}

// ---- Loaded modules, PDB identity ------------------------------------------

#ifdef _MSC_VER
bool ReadPdbInfo(HMODULE hm, char* out, size_t outSize)
{
	__try
	{
		auto const* dos = (const IMAGE_DOS_HEADER*)hm;
		if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
		auto const* nt = (const IMAGE_NT_HEADERS*)((const char*)hm + dos->e_lfanew);
		if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
		IMAGE_DATA_DIRECTORY const dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
		if (dir.VirtualAddress == 0) return false;
		auto const* dbg = (const IMAGE_DEBUG_DIRECTORY*)((const char*)hm + dir.VirtualAddress);
		for (DWORD i = 0; i < dir.Size / sizeof(IMAGE_DEBUG_DIRECTORY); ++i)
		{
			if (dbg[i].Type != IMAGE_DEBUG_TYPE_CODEVIEW) continue;
			struct Rsds { DWORD sig; GUID guid; DWORD age; char path[1]; };
			auto const* cv = (const Rsds*)((const char*)hm + dbg[i].AddressOfRawData);
			if (cv->sig != 0x53445352) continue; // "RSDS"
			GUID const& g = cv->guid;
			std::snprintf(out, outSize, "PDB %08lX%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%lu  %s  (timestamp 0x%08lx)",
				(unsigned long)g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7],
				(unsigned long)cv->age, cv->path, (unsigned long)nt->FileHeader.TimeDateStamp);
			return true;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
	return false;
}
#else
bool ReadPdbInfo(HMODULE, char*, size_t) { return false; }
#endif

std::string ThreadName(DWORD tid)
{
	using GetThreadDescriptionFn = HRESULT(WINAPI*)(HANDLE, PWSTR*);
	static GetThreadDescriptionFn const fn = (GetThreadDescriptionFn)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetThreadDescription");
	if (!fn) return std::string();
	HANDLE const h = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, tid);
	if (!h) return std::string();
	std::string name;
	PWSTR w = nullptr;
	if (SUCCEEDED(fn(h, &w)) && w)
	{
		char buf[128];
		int const n = WideCharToMultiByte(CP_UTF8, 0, w, -1, buf, sizeof(buf), nullptr, nullptr);
		if (n > 0) name = buf;
		LocalFree(w);
	}
	CloseHandle(h);
	return name;
}

std::string ProcessDetailsText()
{
	std::ostringstream out;
	char exe[MAX_PATH];
	DWORD const n = GetModuleFileNameA(nullptr, exe, MAX_PATH);
	out << "Process:\n";
	out << "  Executable: " << std::string(exe, n) << "\n";
	out << "  Command line: " << GetCommandLineA() << "\n";
	out << "  Process id: " << GetCurrentProcessId() << ", reporting thread id: " << GetCurrentThreadId() << "\n\n";

	out << "Threads:\n";
	HANDLE const snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (snap != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 te;
		te.dwSize = sizeof(te);
		if (Thread32First(snap, &te))
		{
			do
			{
				if (te.th32OwnerProcessID != GetCurrentProcessId()) continue;
				std::string const name = ThreadName(te.th32ThreadID);
				out << "  " << te.th32ThreadID << (te.th32ThreadID == GetCurrentThreadId() ? " (reporting)" : "")
				    << (name.empty() ? "" : "  ") << name << "\n";
			} while (Thread32Next(snap, &te));
		}
		CloseHandle(snap);
	}
	out << "  (the stacks of all threads are in the .dmp file)\n\n";

	out << "Loaded modules:\n";
	HMODULE mods[512];
	DWORD needed = 0;
	if (K32EnumProcessModules(GetCurrentProcess(), mods, sizeof(mods), &needed))
	{
		size_t const count = std::min<size_t>(needed / sizeof(HMODULE), 512);
		for (size_t i = 0; i < count; ++i)
		{
			char path[MAX_PATH] = "";
			MODULEINFO mi{};
			K32GetModuleFileNameExA(GetCurrentProcess(), mods[i], path, MAX_PATH);
			K32GetModuleInformation(GetCurrentProcess(), mods[i], &mi, sizeof(mi));
			char line[MAX_PATH + 96];
			std::snprintf(line, sizeof(line), "  %016llx  %8lu KB  %s\n", (unsigned long long)(uintptr_t)mi.lpBaseOfDll, (unsigned long)(mi.SizeOfImage / 1024), path);
			out << line;
			if (i == 0)
			{
				char pdb[400];
				if (ReadPdbInfo(mods[i], pdb, sizeof(pdb))) out << "      " << pdb << "\n";
			}
		}
	}
	return out.str();
}

std::string SystemInfoText()
{
	std::ostringstream out;
	out << "System:\n";

	// RtlGetVersion is not subject to the compatibility shims GetVersionEx has.
	using RtlGetVersionFn = LONG(WINAPI*)(OSVERSIONINFOEXW*);
	auto const rtlGetVersion = (RtlGetVersionFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion");
	OSVERSIONINFOEXW vi{};
	vi.dwOSVersionInfoSize = sizeof(vi);
	if (rtlGetVersion && rtlGetVersion(&vi) == 0)
	{
		out << "  Windows " << vi.dwMajorVersion << "." << vi.dwMinorVersion << " build " << vi.dwBuildNumber << "\n";
	}

	int regs[4] = {0, 0, 0, 0};
	char brand[49] = {};
	__cpuid(regs, 0x80000000);
	if ((unsigned)regs[0] >= 0x80000004)
	{
		for (unsigned i = 0; i < 3; ++i)
		{
			__cpuid(regs, 0x80000002 + i);
			std::memcpy(brand + i * 16, regs, 16);
		}
	}
	SYSTEM_INFO si;
	GetNativeSystemInfo(&si);
	out << "  CPU: " << (brand[0] ? brand : "unknown") << ", " << si.dwNumberOfProcessors << " logical processors\n";

	MEMORYSTATUSEX ms;
	ms.dwLength = sizeof(ms);
	if (GlobalMemoryStatusEx(&ms))
	{
		out << "  Physical memory: " << ms.ullTotalPhys / (1024 * 1024) << " MB total, " << ms.ullAvailPhys / (1024 * 1024) << " MB free\n";
		out << "  Address space: " << ms.ullAvailVirtual / (1024 * 1024) << " MB free of " << ms.ullTotalVirtual / (1024 * 1024) << " MB\n";
	}
	PROCESS_MEMORY_COUNTERS pmc{};
	if (K32GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
	{
		out << "  Process working set: " << pmc.WorkingSetSize / (1024 * 1024) << " MB (peak " << pmc.PeakWorkingSetSize / (1024 * 1024)
		    << " MB), pagefile usage " << pmc.PagefileUsage / (1024 * 1024) << " MB\n";
	}
	return out.str();
}

bool IsThrowMachineryModule(const std::string& module)
{
	for (const char* name : { "ntdll.dll", "kernelbase.dll", "vcruntime140.dll", "vcruntime140_1.dll" })
	{
		if (_stricmp(module.c_str(), name) == 0) return true;
	}
	return false;
}

std::string RecentCppExceptionsText()
{
	std::ostringstream out;
	out << "Recent C++ exceptions thrown (newest first; most are handled and harmless):\n";
	unsigned const pos = g_exceptionPos.load();
	long long const now = NowMs();
	int shown = 0;
	for (int i = 1; i <= kExceptionRing && shown < 8; ++i)
	{
		if (pos < (unsigned)i) break;
		CppExceptionRecord const& e = g_exceptions[(pos - i) % kExceptionRing];
		if (e.ms == 0) continue;
		char head[96];
		std::snprintf(head, sizeof(head), "  -%6.2fs  thread %lu  ", (now - e.ms) / 1000.0, (unsigned long)e.threadId);
		out << head << e.type;
		if (e.what[0]) out << ": " << e.what;
		out << "\n";
		if (shown < 3)
		{
			// frame 0 is our own handler; then skip the OS/CRT throw machinery
			unsigned printed = 0;
			bool leading = true;
			for (unsigned f = 1; f < e.frameCount && printed < 8; ++f)
			{
				DWORD64 off = 0;
				std::string const m = ModuleOf((DWORD64)e.frames[f], &off);
				if (leading && IsThrowMachineryModule(m)) continue;
				leading = false;
				out << "    " << FormatFrame(printed++, (DWORD64)e.frames[f]) << "\n";
			}
		}
		++shown;
	}
	if (shown == 0) out << "  (none)\n";
	return out.str();
}

// ---- Minidump ----------------------------------------------------------------

bool WriteMiniDumpImpl(void* exceptionPointers, unsigned threadId, const std::string& path)
{
	HANDLE const file = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE) return false;

	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = threadId ? threadId : GetCurrentThreadId();
	mei.ExceptionPointers = (PEXCEPTION_POINTERS)exceptionPointers;
	mei.ClientPointers = FALSE;

	// Small by default (tens of MB): all stacks, module list, globals and
	// what the stacks point to. $JA2_CRASH_FULL_DUMP=1 adds the whole
	// process memory (hundreds of MB) for the hardest cases.
	std::string const full = EnvString("JA2_CRASH_FULL_DUMP");
	int type = MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo |
	           MiniDumpWithUnloadedModules | MiniDumpWithHandleData | MiniDumpWithProcessThreadData;
	if (!full.empty() && full != "0") type |= MiniDumpWithFullMemory;

	BOOL const ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, (MINIDUMP_TYPE)type,
	                                  exceptionPointers ? &mei : nullptr, nullptr, nullptr);
	CloseHandle(file);
	return ok != FALSE;
}

// ---- Exception details -------------------------------------------------------

std::string ExceptionDetailsText(const EXCEPTION_POINTERS* ep, DWORD threadId)
{
	EXCEPTION_RECORD const& er = *ep->ExceptionRecord;
	std::ostringstream out;
	char buf[256];

	std::snprintf(buf, sizeof(buf), "Exception: 0x%08lX (%s)\n", (unsigned long)er.ExceptionCode, ExceptionName(er.ExceptionCode));
	out << buf;

	DWORD64 off = 0;
	std::string const mod = ModuleOf((DWORD64)er.ExceptionAddress, &off);
	std::snprintf(buf, sizeof(buf), "Address: 0x%016llx  (%s+0x%llx)\n", (unsigned long long)(DWORD64)er.ExceptionAddress,
	              mod.empty() ? "unknown module" : mod.c_str(), (unsigned long long)off);
	out << buf;

	if ((er.ExceptionCode == EXCEPTION_ACCESS_VIOLATION || er.ExceptionCode == EXCEPTION_IN_PAGE_ERROR) && er.NumberParameters >= 2)
	{
		const char* what = er.ExceptionInformation[0] == 0 ? "reading" : er.ExceptionInformation[0] == 1 ? "writing" : "executing (DEP)";
		std::snprintf(buf, sizeof(buf), "Access violation %s address 0x%016llx%s\n", what, (unsigned long long)er.ExceptionInformation[1],
		              er.ExceptionInformation[1] < 0x10000 ? "  (null pointer or small offset from one)" : "");
		out << buf;
	}
	std::snprintf(buf, sizeof(buf), "Faulting thread id: %lu\n", (unsigned long)threadId);
	out << buf;
	out << RegistersText(*ep->ContextRecord);
	return out.str();
}

void FillNativeCrashInfo(CrashInfo& info, EXCEPTION_POINTERS* ep, HANDLE thread, DWORD threadId)
{
	char reason[128];
	std::snprintf(reason, sizeof(reason), "unhandled native exception 0x%08lX (%s)", (unsigned long)ep->ExceptionRecord->ExceptionCode,
	              ExceptionName(ep->ExceptionRecord->ExceptionCode));
	info.reason = reason;
	info.details = ExceptionDetailsText(ep, threadId);
	info.stack = CollectStack(thread, *ep->ContextRecord, 0, kMaxFrames);
}

// ---- Handlers ------------------------------------------------------------------

LONG WINAPI NativeCrashFilter(EXCEPTION_POINTERS* ep)
{
	// A second crash while reporting the first: give up quietly.
	if (InterlockedCompareExchange(&g_fatalState, 1, 0) != 0) return EXCEPTION_EXECUTE_HANDLER;

	CrashInfo info;
	FillNativeCrashInfo(info, ep, GetCurrentThread(), GetCurrentThreadId());
	WriteReportSet(info, ep, GetCurrentThreadId());
	return EXCEPTION_EXECUTE_HANDLER;
}

void ReportFatal(const char* reason, const std::string& details, unsigned skipFrames)
{
	if (InterlockedCompareExchange(&g_fatalState, 1, 0) != 0) return;

	CrashInfo info;
	info.reason = reason;
	info.details = details;

	CONTEXT ctx;
	std::memset(&ctx, 0, sizeof(ctx));
	RtlCaptureContext(&ctx);
	// +1: this function's own frame
	info.stack = CollectStack(GetCurrentThread(), ctx, skipFrames + 1, kMaxFrames);

	// Give the dump a context to open at, even though there was no SEH exception.
	EXCEPTION_RECORD er;
	std::memset(&er, 0, sizeof(er));
	er.ExceptionCode = 0x40000015; // STATUS_FATAL_APP_EXIT
	er.ExceptionAddress = (PVOID)ContextIp(ctx);
	EXCEPTION_POINTERS ep;
	ep.ExceptionRecord = &er;
	ep.ContextRecord = &ctx;

	WriteReportSet(info, &ep, GetCurrentThreadId());
}

// abort(): reached for failed assertions (already reported, see
// CrashHandlerAssertFailed) and for any abort() elsewhere. Returning lets the
// CRT go on terminating the process as it normally would.
void AbortSignalHandler(int)
{
	if (g_fatalState) return;
	ReportFatal("abort() called", g_assertMessage[0] ? std::string("Last assertion: ") + g_assertMessage : std::string(), 2);
}

void TerminateHandler()
{
	std::string what = "no active exception";
	if (auto ex = std::current_exception())
	{
		try { std::rethrow_exception(ex); }
		catch (std::exception const& e) { what = std::string(typeid(e).name()) + ": " + e.what(); }
		catch (...) { what = "unknown exception type"; }
	}
	ReportFatal("std::terminate called", what, 2);
	std::abort();
}

void InvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned line, uintptr_t)
{
	auto narrow = [](const wchar_t* w) {
		if (!w) return std::string();
		char buf[256];
		int const n = WideCharToMultiByte(CP_UTF8, 0, w, -1, buf, sizeof(buf), nullptr, nullptr);
		return n > 0 ? std::string(buf) : std::string();
	};
	std::ostringstream d;
	d << "Invalid parameter passed to a C runtime function\n  expression: " << narrow(expression) << "\n  function: " << narrow(function)
	  << "\n  file: " << narrow(file) << ":" << line;
	ReportFatal("invalid CRT parameter", d.str(), 2);
	TerminateProcess(GetCurrentProcess(), 0xC000000D);
}

void PureCallHandler()
{
	ReportFatal("pure virtual function call", std::string(), 2);
	TerminateProcess(GetCurrentProcess(), 0xC0000025);
}

// ---- C++ exception history (first-chance, via vectored handler) --------------------

struct ThrowInfo_ { unsigned attributes; int pmfnUnwind; int pForwardCompat; int pCatchableTypeArray; };
struct CatchableTypeArray_ { int nCatchableTypes; int arrayOfCatchableTypes[1]; };
struct Pmd_ { int mdisp; int pdisp; int vdisp; };
struct CatchableType_ { unsigned properties; int pType; Pmd_ thisDisplacement; int sizeOrOffset; int copyFunction; };
struct TypeDescriptor_ { const void* pVFTable; void* spare; char name[1]; };

#if defined(_WIN64) && defined(_MSC_VER)
// Reads the type name and, for std::exception based types, what() out of a
// MSVC C++ exception record. Everything is untrusted memory: any fault is
// swallowed.
void DecodeCppException(const EXCEPTION_RECORD* er, char* typeOut, size_t typeSize, char* whatOut, size_t whatSize)
{
	__try
	{
		if (er->NumberParameters < 4) return;
		char const* const base = (const char*)er->ExceptionInformation[3];
		auto const* ti = (const ThrowInfo_*)er->ExceptionInformation[2];
		char const* const object = (const char*)er->ExceptionInformation[1];
		auto const* arr = (const CatchableTypeArray_*)(base + ti->pCatchableTypeArray);
		for (int i = 0; i < arr->nCatchableTypes && i < 16; ++i)
		{
			auto const* ct = (const CatchableType_*)(base + arr->arrayOfCatchableTypes[i]);
			auto const* td = (const TypeDescriptor_*)(base + ct->pType);
			if (i == 0) std::snprintf(typeOut, typeSize, "%s", td->name);
			if (std::strcmp(td->name, ".?AVexception@std@@") == 0 && ct->thisDisplacement.pdisp == -1)
			{
				auto const* e = (const std::exception*)(object + ct->thisDisplacement.mdisp);
				std::snprintf(whatOut, whatSize, "%s", e->what());
				break;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}
#else
void DecodeCppException(const EXCEPTION_RECORD*, char*, size_t, char*, size_t) {}
#endif

void RecordCppException(const EXCEPTION_RECORD* er)
{
	unsigned const slot = g_exceptionPos.fetch_add(1) % kExceptionRing;
	CppExceptionRecord& rec = g_exceptions[slot];
	rec.ms = 0;
	rec.type[0] = '\0';
	rec.what[0] = '\0';
	DecodeCppException(er, rec.type, sizeof(rec.type), rec.what, sizeof(rec.what));
	rec.threadId = GetCurrentThreadId();
	rec.frameCount = RtlCaptureStackBackTrace(1, 16, rec.frames, nullptr);
	rec.ms = NowMs();
}

// ---- Stack overflow --------------------------------------------------------------------

EXCEPTION_RECORD g_overflowRecord;
CONTEXT          g_overflowContext;
DWORD            g_overflowThreadId = 0;

DWORD WINAPI StackOverflowReportThread(LPVOID)
{
	EXCEPTION_POINTERS ep;
	ep.ExceptionRecord = &g_overflowRecord;
	ep.ContextRecord = &g_overflowContext;

	HANDLE const crashed = OpenThread(THREAD_ALL_ACCESS, FALSE, g_overflowThreadId);

	CrashInfo info;
	FillNativeCrashInfo(info, &ep, crashed ? crashed : GetCurrentThread(), g_overflowThreadId);
	WriteReportSet(info, &ep, g_overflowThreadId);

	if (crashed) CloseHandle(crashed);
	return 0;
}

void HandleStackOverflow(EXCEPTION_POINTERS* ep)
{
	if (InterlockedCompareExchange(&g_fatalState, 1, 0) != 0) return;

	g_overflowRecord = *ep->ExceptionRecord;
	g_overflowContext = *ep->ContextRecord;
	g_overflowThreadId = GetCurrentThreadId();

	// The overflowing thread has almost no stack left: report from a fresh one.
	HANDLE const t = CreateThread(nullptr, 4 * 1024 * 1024, StackOverflowReportThread, nullptr, 0, nullptr);
	if (t)
	{
		WaitForSingleObject(t, 60000);
		CloseHandle(t);
	}
	TerminateProcess(GetCurrentProcess(), EXCEPTION_STACK_OVERFLOW);
}

LONG CALLBACK VectoredHandler(EXCEPTION_POINTERS* ep)
{
	DWORD const code = ep->ExceptionRecord->ExceptionCode;
	if (code == 0xE06D7363) RecordCppException(ep->ExceptionRecord);
	else if (code == EXCEPTION_STACK_OVERFLOW) HandleStackOverflow(ep);
	return EXCEPTION_CONTINUE_SEARCH;
}

// ---- Hang watchdog --------------------------------------------------------------------------

std::atomic<unsigned> g_heartbeat{0};
std::atomic<bool>     g_watchdogPaused{false};
std::atomic<bool>     g_watchdogStarted{false};
HANDLE                g_mainThread = nullptr;
DWORD                 g_mainThreadId = 0;
unsigned              g_watchdogSeconds = 60;

void ReportHang(unsigned stalledSeconds)
{
	CONTEXT ctx;
	std::memset(&ctx, 0, sizeof(ctx));
	ctx.ContextFlags = CONTEXT_FULL;
	bool haveContext = false;
	if (SuspendThread(g_mainThread) != (DWORD)-1)
	{
		haveContext = GetThreadContext(g_mainThread, &ctx) != FALSE;
		ResumeThread(g_mainThread);
	}

	CrashInfo info;
	info.kind = "hang";
	char reason[160];
	std::snprintf(reason, sizeof(reason), "possible hang: the main loop made no progress for %u s", stalledSeconds);
	info.reason = reason;
	info.details = "The game is still running; this report is a snapshot of the main thread.\nMain thread id: " + std::to_string(g_mainThreadId) + "\n";

	EXCEPTION_RECORD er;
	std::memset(&er, 0, sizeof(er));
	er.ExceptionCode = 0x40000015;
	EXCEPTION_POINTERS ep;
	ep.ExceptionRecord = &er;
	ep.ContextRecord = &ctx;
	if (haveContext)
	{
		er.ExceptionAddress = (PVOID)ContextIp(ctx);
		info.details += RegistersText(ctx);
		info.stack = CollectStack(g_mainThread, ctx, 0, kMaxFrames);
	}
	WriteReportSet(info, haveContext ? &ep : nullptr, g_mainThreadId);
}

DWORD WINAPI WatchdogThread(LPVOID)
{
	unsigned lastBeat = g_heartbeat.load();
	long long lastChange = NowMs();
	bool reported = false;

	while (!g_fatalState)
	{
		Sleep(1000);
		unsigned const beat = g_heartbeat.load();
		long long const now = NowMs();
		if (beat != lastBeat || g_watchdogPaused.load())
		{
			lastBeat = beat;
			lastChange = now;
			reported = false;
			continue;
		}
		unsigned const stalled = (unsigned)((now - lastChange) / 1000);
		if (!reported && stalled >= g_watchdogSeconds)
		{
			reported = true;
			ReportHang(stalled);
			SLOGE("Possible hang: the main loop made no progress for {} s, diagnostic report written next to this log", stalled);
		}
	}
	return 0;
}

} // namespace

void InstallCrashHandler()
{
	static bool installed = false;
	if (installed) return;
	installed = true;

	EnsureSymbols();

	SetUnhandledExceptionFilter(NativeCrashFilter);
	AddVectoredExceptionHandler(1, VectoredHandler);
	signal(SIGABRT, AbortSignalHandler);
	std::set_terminate(TerminateHandler);
	_set_invalid_parameter_handler(InvalidParameterHandler);
	_set_purecall_handler(PureCallHandler);
}

void CrashHandlerStartWatchdog()
{
	if (g_watchdogStarted.exchange(true)) return;

	std::string const env = EnvString("JA2_WATCHDOG_SECONDS");
	if (!env.empty()) g_watchdogSeconds = (unsigned)std::atoi(env.c_str());
	if (g_watchdogSeconds == 0) return;

	g_mainThreadId = GetCurrentThreadId();
	if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &g_mainThread, 0, FALSE, DUPLICATE_SAME_ACCESS)) return;

	HANDLE const t = CreateThread(nullptr, 0, WatchdogThread, nullptr, 0, nullptr);
	if (t) CloseHandle(t);
}

void CrashHandlerHeartbeat() { g_heartbeat.fetch_add(1, std::memory_order_relaxed); }

void CrashHandlerPauseWatchdog(bool paused) { g_watchdogPaused.store(paused); }

#else // !_WIN32

// ---------------------------------------------------------------------------
// POSIX
// ---------------------------------------------------------------------------

namespace {

bool WriteMiniDumpImpl(void*, unsigned, const std::string&) { return false; }

void ReportFatal(const char* reason, const std::string& details, unsigned skipFrames)
{
	if (g_fatalState) return;
	g_fatalState = 1;

	CrashInfo info;
	info.reason = reason;
	info.details = details;

	void* frames[64];
	int const n = backtrace(frames, 64);
	char** const symbols = backtrace_symbols(frames, n);
	for (int i = (int)skipFrames; i < n; ++i)
	{
		info.stack.push_back("  #" + std::to_string(i - (int)skipFrames) + "  " + (symbols ? symbols[i] : "?"));
	}
	std::free(symbols);

	WriteReportSet(info, nullptr, 0);
}

void NativeCrashSignalHandler(int sig)
{
	if (!g_fatalState)
	{
		ReportFatal(("fatal signal " + std::to_string(sig)).c_str(), std::string(), 2);
	}

	// Restore the default handler and re-raise, so the OS still does
	// whatever it would normally do (core dump, standard exit code, etc.).
	signal(sig, SIG_DFL);
	raise(sig);
}

void TerminateHandler()
{
	std::string what = "no active exception";
	if (auto ex = std::current_exception())
	{
		try { std::rethrow_exception(ex); }
		catch (std::exception const& e) { what = std::string(typeid(e).name()) + ": " + e.what(); }
		catch (...) { what = "unknown exception type"; }
	}
	ReportFatal("std::terminate called", what, 2);
	std::abort();
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
	std::set_terminate(TerminateHandler);
}

void CrashHandlerStartWatchdog() {}
void CrashHandlerHeartbeat() {}
void CrashHandlerPauseWatchdog(bool) {}

#endif
