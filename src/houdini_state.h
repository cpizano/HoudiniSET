#include <Windows.h>

#include <map>
#include <string>

namespace houdini {

class ScreenOutput;
struct ProcessTracker;

struct State {
  SRWLOCK rwlock;
  ScreenOutput* so;
  std::map<HANDLE, ProcessTracker*> processes;
  std::map<TP_WAIT*, HANDLE> reg_ob_waits;

  State(ScreenOutput* so_i) {
    so = so_i;
    ::InitializeSRWLock(&rwlock);
  }
};

}  // namespace