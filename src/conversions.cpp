#include "houdini_command.h"


class Conversions : public houdini::Command {
public:
  virtual ~Conversions() {};

protected:
  virtual void OnCommand(houdini::ScreenOutput* so,
                         const std::vector<std::string>& tokens) override {

  }

  virtual void OnHelp(houdini::ScreenOutput* so,
                      const std::string& topic) override {

  }

};

houdini::CmdDescriptor MakeConversionsCommand() {
  houdini::CmdDescriptor cd = {
    "conv", 
    "some conv help here",
    new Conversions()
  };
  return cd;
}
