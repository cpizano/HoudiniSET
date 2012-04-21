#pragma once
#include <Windows.h>

#include <string>
#include <vector>

// This is the RTF font color command.
#define RCLR(x) "\\cf"#x" "
// This is the RTF new line command.
#define RLF     "\\line "

namespace houdini {

class ScreenOutput;

class Command {
public:
  virtual ~Command() {};
  virtual bool OnCommand(ScreenOutput*, const std::vector<std::string>& tokens) = 0;
  virtual void OnHelp(ScreenOutput*, const std::string& topic) = 0;
};

struct CmdDescriptor {
  std::string name;
  std::string desc;
  Command* cmd;
};


HANDLE OpenProcessSpec(const char* process_spec,
                       DWORD access, DWORD fallback_access,
                       ScreenOutput* so);

DWORD PrintProcessTimes(HANDLE process, ScreenOutput* so);
 

}  // namespace