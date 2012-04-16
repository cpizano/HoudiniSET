#include <Windows.h>

#include <map>
#include <string>
#include <memory>

namespace houdini {

class ScreenOutput;
struct ProcessTracker;

struct State {
  SRWLOCK rwlock;
  ScreenOutput* so;
  std::map<HANDLE, std::unique_ptr<ProcessTracker> > processes;
  std::map<TP_WAIT*, HANDLE> reg_ob_waits;

  State(ScreenOutput* so_i) {
    so = so_i;
    ::InitializeSRWLock(&rwlock);
  }
};

}  // namespace