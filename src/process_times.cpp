#include "houdini_command.h"


class ProcessTimes : public houdini::Command {
public:
  virtual ~ProcessTimes() {};

protected:
  virtual bool OnCommand(houdini::ScreenOutput* so,
                         const std::vector<std::string>& tokens) override {
    return true;
  }

  virtual void OnHelp(houdini::ScreenOutput* so,
                      const std::string& topic) override {

  }

};

houdini::CmdDescriptor MakeProcessTimesCommand() {
  houdini::CmdDescriptor cd = {
    "ptimes", 
    "some ptimes help here",
    new ProcessTimes()
  };
  return cd;
}
