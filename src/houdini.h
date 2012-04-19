#pragma once

#include <vector>

namespace houdini {

struct State;
struct CmdDescriptor;

class ScreenOutput {
public:
  virtual void Output(const char* text) = 0;
  virtual void NewLine() = 0;
};

class Houdini {
public:
  Houdini(ScreenOutput* output);
  ~Houdini();
  void InputCommand(const char* command);

private:
  State* state_;
  std::vector<CmdDescriptor*> cmd_;
};


}  // namespace houdini
