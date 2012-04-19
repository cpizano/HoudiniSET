#include "houdini_command.h"


class ProcessList : public houdini::Command {
public:
  virtual ~ProcessList() {};

protected:
  virtual void OnCommand(houdini::ScreenOutput* so, const std::vector<std::string>& tokens) override {

  }

  virtual void OnHelp(houdini::ScreenOutput* so, const std::string& topic) override {

  }

};

houdini::CmdDescriptor MakeProcessListCommand() {
  ProcessList* pl = new ProcessList();
  houdini::CmdDescriptor cd = {"plist", "some help here", pl};
  return cd;
}
