#pragma once

class ScreenOutput {
public:
  virtual void Output(const char* text) = 0;
  virtual void NewLine() = 0;
};

class Houdini {
public:
  struct State;

  Houdini(ScreenOutput* output);
  ~Houdini();
  void InputCommand(const char* command);

private:
  State* state_;
};

