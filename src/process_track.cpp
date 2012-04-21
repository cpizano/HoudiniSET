#include "houdini_command.h"


class ProcessTracker : public houdini::Command {
public:
  virtual ~ProcessTracker() {};

protected:
  virtual bool OnCommand(houdini::ScreenOutput* so,
                         const std::vector<std::string>& tokens) override {
    return true;
  }

  virtual void OnHelp(houdini::ScreenOutput* so,
                      const std::string& topic) override {

  }

};

houdini::CmdDescriptor MakeProcessTrackerCommand() {
  houdini::CmdDescriptor cd = {
    "ptrack", 
    "some ptrack help here",
    new ProcessTracker()
  };
  return cd;
}
