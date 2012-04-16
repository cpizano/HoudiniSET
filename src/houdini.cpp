#include "houdini.h"

#include <Windows.h>
#include <Psapi.h>

#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <sstream>
#include <map>

#include "houdini_state.h"

#pragma comment(lib, "psapi.lib")

#define RCLR(x) "\\cf"#x" "

using namespace houdini;

namespace {

void locase_str(std::string &str) {
	std::transform(str.begin(), str.end(), str.begin(), std::tolower);
}


template < class ContainerT >
void tokenize(const std::string& str, ContainerT& tokens,
              const std::string& delimiters = " ", const bool trimEmpty = false) {
   std::string::size_type pos, lastPos = 0;
   while(true) {
      pos = str.find_first_of(delimiters, lastPos);
      if(pos == std::string::npos) {
         pos = str.length();

         if(pos != lastPos || !trimEmpty)
            tokens.push_back(ContainerT::value_type(str.data()+lastPos,
                  (ContainerT::value_type::size_type)pos-lastPos ));

         break;
      } else {
         if(pos != lastPos || !trimEmpty)
            tokens.push_back(ContainerT::value_type(str.data()+lastPos,
                  (ContainerT::value_type::size_type)pos-lastPos ));
      }

      lastPos = pos + 1;
   }
};

class ScopedReadLock {
public:
  ScopedReadLock(SRWLOCK* rwlock) : rwlock_(rwlock)  {
    ::AcquireSRWLockShared(rwlock_);
  }

  ~ScopedReadLock()  {
    ::ReleaseSRWLockShared(rwlock_);
  }

private:
  ScopedReadLock(const ScopedReadLock&);
  SRWLOCK* rwlock_;
};

class ScopedWriteLock {
public:
  ScopedWriteLock(SRWLOCK* rwlock) : rwlock_(rwlock)  {
    ::AcquireSRWLockExclusive(rwlock_);
  }

  ~ScopedWriteLock()  {
    ::ReleaseSRWLockExclusive(rwlock_);
  }

private:
  ScopedWriteLock(const ScopedWriteLock&);
  SRWLOCK* rwlock_;
};

HANDLE OpenProcess(const char* process_spec,
                   DWORD access, DWORD fallback_access,
                   houdini::ScreenOutput* so) {
  char p_or_h = 0;
  unsigned int pid = 0;
  std::istringstream iss(process_spec);
  std::ostringstream oss;
  iss >> p_or_h;
  if (p_or_h != 'p') {
    so->Output(RCLR(1)"invalid process id format, use p[pid]");
    so->NewLine();
    return NULL;
  }
  iss >> pid;
  HANDLE handle = ::OpenProcess(access, FALSE, pid);
  if (handle) {
    return handle;
  }
  if (fallback_access != -1) {
    handle = ::OpenProcess(fallback_access, FALSE, pid);
    if (handle) {
      return handle;
    }
  }

  DWORD gle = ::GetLastError();
  oss << RCLR(1)"failed to open process "RCLR(2) << pid;
  oss << RCLR(1)" requesting "RCLR(2) << fallback_access << RCLR(1)" access.";
  oss << " win32 error "RCLR(2) << gle;
  so->Output(oss.str().c_str());
  so->NewLine();
  return NULL;   
}

DWORD ListProcesses(const char* filter, houdini::ScreenOutput* so) {
  DWORD pids[2048];
  DWORD needed;
  if (!::EnumProcesses(pids, sizeof(pids), &needed)) {
    DWORD gle = ::GetLastError();
    so->Output(RCLR(1)"plist enumproc failed!");
    return gle;
  }

  {
    needed = needed / sizeof(pids[0]);
    std::ostringstream oss;
    oss << RCLR(1)"total of "RCLR(2) << needed << RCLR(1)" processes found";
    so->Output(oss.str().c_str());
    so->NewLine();
  }
 
  for (size_t ix = 0; ix != needed; ++ix) {
    if (pids[ix] == 0)
      continue;
    std::ostringstream oss;
    HANDLE handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pids[ix]);
    if (!handle && !filter) {
      oss << RCLR(2) << pids[ix] << RCLR(1) << " ";
      DWORD gle = ::GetLastError();
      if (gle == ERROR_ACCESS_DENIED) {
        oss << "[access denied]";
      } else {
        oss << "[error " << gle << " ]";
      }
    } else {
      char path[300];
      DWORD nc = GetProcessImageFileNameA(handle, path, _countof(path));
      if (!nc && !filter) {
        oss << "[no name] ---";
      } else {
        std::string name(path);
        if (filter) {
          size_t ffp = name.find(filter);
          if (ffp == std::string::npos) {
            continue;
          }
        }
        oss << RCLR(2) << pids[ix] << RCLR(1) << " ";
        size_t p1 = name.find_last_of('\\');
        oss << RCLR(0) << name.substr(p1 + 1, std::string::npos) << RCLR(1);
        size_t p2 = name.find_last_of('\\', p1 - 1);
        oss << " [" << name.substr(p2 + 1,  (p1 - p2)) << "]";
      }
      ::CloseHandle(handle);
    }
    so->Output(oss.str().c_str());
    so->NewLine();
  }

  return 0;
}

DWORD ListProcessTimes(HANDLE process, houdini::ScreenOutput* so) {
  FILETIME creation;
  FILETIME exit;
  FILETIME kernel;
  FILETIME user = {0};
  if (!::GetProcessTimes(process, &creation, &exit, &kernel, &user)) {
    return ::GetLastError();
  }
  std::ostringstream oss;
  ULARGE_INTEGER exit_ul = {exit.dwLowDateTime, exit.dwHighDateTime};
  ULARGE_INTEGER creation_ul = {creation.dwLowDateTime, creation.dwHighDateTime};
  oss << RCLR(1)"times ["RCLR(2) << creation_ul.QuadPart << RCLR(1)"]";
  oss << "["RCLR(2) << exit_ul.QuadPart << RCLR(1)"]";
  if (exit_ul.QuadPart != 0) {
    double delta_s = double(exit_ul.QuadPart - creation_ul.QuadPart) / double(10000000) ;
    oss << "("RCLR(2) << delta_s << RCLR(1)")";
  }
  so->Output(oss.str().c_str());
  return 0;
}

}  // namespace

namespace houdini {

struct ProcessTracker {
  DWORD pid;
  std::string how;
  ULONGLONG when;

  ProcessTracker() : pid(0), when(0) {}

  ProcessTracker(DWORD pid_i, const std::string& how_i) 
      : pid(pid_i), how(how_i) {
    when = ::GetTickCount64();
  }

};

} // namespace

struct PoolWaitContext {
  houdini::State* state;
  HANDLE handle;
  PoolWaitContext(houdini::State* state_i, HANDLE handle_i)
    : state(state_i), handle(handle_i) {}
};

void CALLBACK PoolWaitCallback(TP_CALLBACK_INSTANCE* instance,
                               void* param,
                               TP_WAIT* wait,
                               DWORD wait_result) {
  ULONGLONG time = ::GetTickCount64();
  PoolWaitContext ctx = *reinterpret_cast<PoolWaitContext*>(param);
  delete reinterpret_cast<PoolWaitContext*>(param);

  std::ostringstream oss;
  { // read lock
    ScopedReadLock lock(&ctx.state->rwlock);
    auto it  = ctx.state->processes.find(ctx.handle);
    if (it == ctx.state->processes.end()) {
      oss << RCLR(1)"signaled handle "RCLR(2) << ctx.handle << RCLR(1)" unknown!";
      ctx.state->so->Output(oss.str().c_str());
      return;
    } else {
      oss << RCLR(1)"process "RCLR(2) << it->second->pid << RCLR(1)" terminated,";
      DWORD exit_code;
      if (::GetExitCodeProcess(it->first, &exit_code)) {
        oss << " exit code "RCLR(2) << exit_code;
      }
      oss << RCLR(1)" tracked for "RCLR(2) << (time - it->second->when) << "ms ";
      ctx.state->so->Output(oss.str().c_str());
      ctx.state->so->NewLine();
      ListProcessTimes(it->first, ctx.state->so);
      ctx.state->so->NewLine();
    }
  }  // read lock end
  {  // write lock
    ScopedWriteLock lock(&ctx.state->rwlock);
    ctx.state->processes.erase(ctx.handle);
    ::CloseHandle(ctx.handle);
    ctx.state->reg_ob_waits.erase(wait);
    ::SetThreadpoolWait(wait, NULL, NULL);
    ::CloseThreadpoolWait(wait);
  }  // write lock end
}

void OnHelp(houdini::State* state, std::vector<std::string>& tokens) {
  if (tokens.size() == 1) {
    state->so->Output(RCLR(1)"commands are");
    state->so->NewLine();
    state->so->Output("help ");
    state->so->NewLine();
    state->so->Output("quit "RCLR(1)"or"RCLR(0)" exit");
    state->so->NewLine();
    state->so->Output("track "RCLR(1)":get notified when a process exits");
    state->so->NewLine();
    state->so->Output("plist "RCLR(1)":get the list of running processes");
    state->so->NewLine();
    state->so->Output("ptimes "RCLR(1)":get the start and end times of a process");
    state->so->NewLine();
  }
  else {
    state->so->Output("no command specific help yet");
    state->so->NewLine();
  }
}

void OnTrack(houdini::State* state, std::vector<std::string>& tokens) {
  if (tokens.size() == 1) {
    state->so->Output(RCLR(1)"use track ? for help");
    state->so->NewLine();
    return;
  }
  
  if (tokens[1] == "?") {
    state->so->Output(RCLR(1)"track p[pid]");
    state->so->NewLine();
  } else {
    ScopedWriteLock lock(&state->rwlock);
    std::ostringstream oss;
    HANDLE handle = OpenProcess(tokens[1].c_str(), 
                                SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, SYNCHRONIZE,
                                state->so);
    if (!handle)
      return;
    DWORD pid = ::GetProcessId(handle);

    state->processes[handle].reset(new ProcessTracker(pid, "track"));
    PoolWaitContext* pwc = new PoolWaitContext(state, handle);
    TP_WAIT* wait_object = ::CreateThreadpoolWait(&PoolWaitCallback, pwc, NULL);
    state->reg_ob_waits[wait_object] = handle;

    ::SetThreadpoolWait(wait_object, handle, NULL);
    oss << RCLR(1)"tracking process "RCLR(2) << pid << RCLR(1)" with handle "RCLR(2)"0x" << handle;
    state->so->Output(oss.str().c_str());
    state->so->NewLine();
  }
}

void OnPList(houdini::State* state, std::vector<std::string>& tokens) {
  if (tokens.size() == 1) {
    ListProcesses(NULL, state->so);
  } else if (tokens.size() == 2) {
    if (tokens[1] == "?") {
      state->so->Output(RCLR(1)"plist [filter]");
      state->so->NewLine();
    } else {
      ListProcesses(tokens[1].c_str(), state->so);
    }
  } else {
    state->so->Output(RCLR(1)"unknown param");
    state->so->NewLine();
  }
}

void OnPTimes(houdini::State* state, std::vector<std::string>& tokens) {
 if (tokens.size() == 1) {
    state->so->Output(RCLR(1)"use ptimes ? for help");
    state->so->NewLine();
    return;
  }
  
  if (tokens[1] == "?") {
    state->so->Output(RCLR(1)"ptimes p[pid]");
    state->so->NewLine();
    return;
  } 

  HANDLE process = OpenProcess(tokens[1].c_str(),
                               PROCESS_QUERY_LIMITED_INFORMATION, -1,
                               state->so);
  if (!process)
    return;
  ListProcessTimes(process, state->so);
  state->so->NewLine();
}

void OnConvert(houdini::State* state, std::vector<std::string>& tokens) {
  if (tokens.size() == 1) {
    state->so->Output(RCLR(1)"use conv ? for help");
    state->so->NewLine();
    return;
  } else if (tokens[1] == "?") {
    state->so->Output(RCLR(1)"conv filetime number");
    state->so->NewLine();
    return;
  } else if (tokens.size() == 3) {
    if (tokens[1] == "filetime") {
      std::istringstream iss(tokens[2]);
      ULARGE_INTEGER uli;
      iss >> uli.QuadPart;
      FILETIME ft = {uli.LowPart, uli.HighPart};
      FILETIME lft = {0};
      if (!::FileTimeToLocalFileTime(&ft, &lft)) {
        state->so->Output(RCLR(1)"invalid filetime #1");
        state->so->NewLine();
        return;
      }
      SYSTEMTIME st = {0};
      if (!::FileTimeToSystemTime(&lft, &st)) {
        state->so->Output(RCLR(1)"invalid filetime #2");
        state->so->NewLine();
        return;
      }
      std::ostringstream oss;
      oss << RCLR(1) << st.wYear << "." << st.wMonth << "." << st.wDay << " ";
      oss << st.wHour << ":" << st.wMinute << "." << st.wSecond;
      state->so->Output(oss.str().c_str());
      state->so->NewLine();
    }
  }
}

Houdini::Houdini(ScreenOutput* so) : state_(new State(so)) {
  // Done with initialization, signal user to start working.
  so->Output(RCLR(1)"type "RCLR(0)"help"RCLR(1)" for available commands");
  so->NewLine();
}

Houdini::~Houdini() {
  delete state_;
}

void Houdini::InputCommand(const char* command) {
  // Echo the command in the results pane.
  std::string comm(command);
  state_->so->Output(">");
  state_->so->Output(comm.c_str());
  state_->so->NewLine();
  // tokenize and then use the first token to route.
  std::vector<std::string> tokens;
  tokenize(comm, tokens, " ", true);
  if (tokens.size() == 0)
    return;
  const std::string& verb = tokens[0];
  if (verb == "help") {
    OnHelp(state_, tokens);
  } else if (verb == "quit" || verb == "exit") {
    // quitting is hard. for now just crap out.
    ::ExitProcess(0);
  } else if (verb == "track") {
    // Track the lifetime of a given process.
    OnTrack(state_, tokens);
  } else if (verb == "plist") {
    // List the currently running processes.
    OnPList(state_, tokens);
  } else if (verb == "ptimes") {
    // List a particular process times
    OnPTimes(state_, tokens);
  } else if (verb == "conv") {
    // Coverts filetime
    OnConvert(state_, tokens);
  } else {
    state_->so->Output(RCLR(1)"wot? type "RCLR(0)"help"RCLR(1)" next time");
    state_->so->NewLine();
  } 
}

