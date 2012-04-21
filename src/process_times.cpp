#include <Windows.h>
#include <sstream>

#include "houdini_command.h"
#include "houdini.h"

class ProcessTimes : public houdini::Command {
public:
  virtual ~ProcessTimes() {};

protected:
  virtual bool OnCommand(houdini::ScreenOutput* so,
                         const std::vector<std::string>& tokens) override {
    if (tokens.size() != 2) {
      return false;
    }

    HANDLE process = houdini::OpenProcessSpec(tokens[1].c_str(),
        PROCESS_QUERY_LIMITED_INFORMATION, -1, so);
    if (process) {
      houdini::PrintProcessTimes(process, so);
    }
    return true;
  }

  virtual void OnHelp(houdini::ScreenOutput* so,
                      const std::string& topic) override {
    so->Output(RCLR(1)"Shows a process start and end time in FILETIME units. Use:"RLF \
               RCLR(0)"ptimes p<pid>"RCLR(1)" where pid is the process id.");
  }

};

houdini::CmdDescriptor MakeProcessTimesCommand() {
  houdini::CmdDescriptor cd = {
    "ptimes", 
    "prints process start and end times",
    new ProcessTimes()
  };
  return cd;
}

