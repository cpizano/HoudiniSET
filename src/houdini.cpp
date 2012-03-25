#include "houdini.h"

#include <Windows.h>

struct Houdini::State {
  ScreenOutput* so;
};

Houdini::Houdini(ScreenOutput* so) : state_(new State) {
  state_->so = so;
  so->Output("type \\cf1 help \\cf0 for available commands \\line");
}

// \\cf1 %d \\cf0 tests to run. Time is %d:%d.%d \\line

Houdini::~Houdini() {
  delete state_;
}

void Houdini::InputCommand(const char* command) {

}

