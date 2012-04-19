#include <string>
#include <vector>

namespace houdini {

class ScreenOutput;

class Command {
public:
  virtual ~Command() {};
  virtual void OnCommand(ScreenOutput*, const std::vector<std::string>& tokens) = 0;
  virtual void OnHelp(ScreenOutput*, const std::string& topic) = 0;
};

struct CmdDescriptor {
  std::string name;
  std::string desc;
  Command* cmd;
};

}  // namespace