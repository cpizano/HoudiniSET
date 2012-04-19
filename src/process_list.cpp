#include "houdini_command.h"


class ProcessList : public houdini::Command {
public:
  virtual ~ProcessList() {};

protected:
  virtual void OnCommand(houdini::ScreenOutput* so,
                         const std::vector<std::string>& tokens) override {

  }

  virtual void OnHelp(houdini::ScreenOutput* so,
                      const std::string& topic) override {

  }

};

houdini::CmdDescriptor MakeProcessListCommand() {
  houdini::CmdDescriptor cd = {
    "plist", 
    "some plist help here",
    new ProcessList()
  };
  return cd;
}
