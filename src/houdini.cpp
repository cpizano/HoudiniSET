#include "houdini.h"

#include <Windows.h>
#include <Psapi.h>

#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <sstream>
#include <map>

#pragma comment(lib, "psapi.lib")

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

DWORD ListProcesses(const char* filter, houdini::ScreenOutput* so) {
  DWORD pids[2048];
  DWORD needed;
  if (!::EnumProcesses(pids, sizeof(pids), &needed)) {
    DWORD gle = ::GetLastError();
    so->Output("\\cf1 plist enumproc failed!");
    return gle;
  }

  {
    needed = needed / sizeof(pids[0]);
    std::ostringstream oss("\\cf2 ");
    oss << needed << " \\cf1 processes";
    so->Output(oss.str().c_str());
    so->NewLine();
  }
 
  for (size_t ix = 0; ix != needed; ++ix) {
    if (pids[ix] == 0)
      continue;
    std::ostringstream oss;
    oss << "\\cf2 " << pids[ix] << " \\cf1 ";
    HANDLE handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pids[ix]);
    if (!handle) {
      DWORD gle = ::GetLastError();
      if (gle == ERROR_ACCESS_DENIED) {
        oss << "[access denied]";
      } else {
        oss << "[error " << gle << " ]";
      }
    } else {
      char path[300];
      DWORD nc = GetProcessImageFileNameA(handle, path, _countof(path));
      if (!nc) {
        oss << "[no name]";
      } else {
        std::string name(path);
        size_t p1 = name.find_last_of('\\');
        oss << name.substr(p1 + 1, std::string::npos);
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


}  // namespace

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

struct Houdini::State {
  SRWLOCK rwlock;
  ScreenOutput* so;
  std::map<HANDLE, ProcessTracker> processes;
  std::map<TP_WAIT*, HANDLE> reg_ob_waits;

  State(ScreenOutput* so_i) {
    so = so_i;
    ::InitializeSRWLock(&rwlock);
  }
};

struct PoolWaitContext {
  Houdini::State* state;
  HANDLE handle;
  PoolWaitContext(Houdini::State* state_i, HANDLE handle_i)
    : state(state_i), handle(handle_i) {}
};

void CALLBACK PoolWaitCallback(TP_CALLBACK_INSTANCE* instance,
                               void* param,
                               TP_WAIT* wait,
                               DWORD wait_result) {
  ULONGLONG time = ::GetTickCount64();
  PoolWaitContext ctx = *reinterpret_cast<PoolWaitContext*>(param);
  delete reinterpret_cast<PoolWaitContext*>(param);

  std::ostringstream oss("\\cf1 ");
  { // read lock
    ScopedReadLock lock(&ctx.state->rwlock);
    auto it  = ctx.state->processes.find(ctx.handle);
    if (it == ctx.state->processes.end()) {
      oss << "signaled handle \\cf2 " << ctx.handle << " \\cf1 unknown!";
      ctx.state->so->Output(oss.str().c_str());
      return;
    } else {
      oss << "\\cf1 process \\cf2 " << it->second.pid << " \\cf1 terminated,";
      DWORD exit_code;
      if (::GetExitCodeProcess(it->first, &exit_code)) {
        oss << " exit code \\cf2 " << exit_code;
      }
      oss << " \\cf1 tracked for \\cf2 " << (time - it->second.when) << "ms";
      ctx.state->so->Output(oss.str().c_str());
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

void OnHelp(Houdini::State* state, std::vector<std::string>& tokens) {
  if (tokens.size() == 1) {
    state->so->Output("\\cf1 commands are");
    state->so->NewLine();
    state->so->Output("\\cf1 help");
    state->so->NewLine();
    state->so->Output("\\cf1 quit or exit");
    state->so->NewLine();
    state->so->Output("\\cf1 track");
    state->so->NewLine();
    state->so->Output("\\cf1 plist");
    state->so->NewLine();
  }
  else {
    state->so->Output("no command specific help yet");
    state->so->NewLine();
  }
}

void OnTrack(Houdini::State* state, std::vector<std::string>& tokens) {
  if (tokens.size() == 1) {
    state->so->Output("\\cf1 use track ? for help");
    state->so->NewLine();
    return;
  }
  
  if (tokens[1] == "?") {
    state->so->Output("\\cf1 track p[pid]");
    state->so->NewLine();
  } else {
    ScopedWriteLock lock(&state->rwlock);
    char p_or_h = 0;
    unsigned int pid = 0;
    std::istringstream iss(tokens[1]);
    std::ostringstream oss;
    iss >> p_or_h;
    if (p_or_h != 'p') {
      state->so->Output("\\cf1 invalid 1st parameter");
      state->so->NewLine();
      return;
    }
    iss >> pid;
    HANDLE handle = ::OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!handle) {
      handle = ::OpenProcess(SYNCHRONIZE, FALSE, pid);
      if (!handle) {
        DWORD gle = ::GetLastError();
        oss << "\\cf1 failed to open process \\cf2 " << pid << " \\cf1 requesting SYNCHRONIZE access.";
        oss << " error \\cf2 " << gle;
        state->so->Output(oss.str().c_str());
        state->so->NewLine();
        return;
      }
    }
    state->processes[handle] = ProcessTracker(pid, "track");
    PoolWaitContext* pwc = new PoolWaitContext(state, handle);
    TP_WAIT* wait_object = ::CreateThreadpoolWait(&PoolWaitCallback, pwc, NULL);
    ::SetThreadpoolWait(wait_object, handle, NULL);
    state->reg_ob_waits[wait_object] = handle;
    oss << "\\cf1 tracking process \\cf2 " << pid << " \\cf1 with handle \\cf2 0x" << handle;
    state->so->Output(oss.str().c_str());
    state->so->NewLine();
  }
}

void OnPList(Houdini::State* state, std::vector<std::string>& tokens) {
  if (tokens.size() == 1) {
    ListProcesses(NULL, state->so);
  } else if (tokens[1] == "?") {
    state->so->Output("\\cf1 plist");
    state->so->NewLine();
  } else {
    state->so->Output("\\cf1 unknown param");
    state->so->NewLine();
  }
}

Houdini::Houdini(ScreenOutput* so) : state_(new State(so)) {
  // Done with initialization, signal user to start working.
  so->Output("type \\cf1 help \\cf0 for available commands");
  so->NewLine();
}

// \\cf1 %d \\cf0 tests to run. Time is %d:%d.%d \\line

Houdini::~Houdini() {
  delete state_;
}

void Houdini::InputCommand(const char* command) {
  // Echo the command in the results pane.
  std::string comm(command);
  locase_str(comm);
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
  } else {
    state_->so->Output("\\cf1 huh?");
    state_->so->NewLine();
  } 
}

