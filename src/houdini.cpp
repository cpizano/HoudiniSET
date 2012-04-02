#include "houdini.h"

#include <Windows.h>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>

namespace {

void locase_str(std::string &str) {
	std::transform(str.begin(), str.end(), str.begin(), std::tolower);
}

template < class ContainerT >
void tokenize(const std::string& str, ContainerT& tokens,
              const std::string& delimiters = " ", const bool trimEmpty = false) {
   std::string::size_type pos, lastPos = 0;
   while(true) {
      pos = str.find_first_of(delimiters, lastPos);
      if(pos == std::string::npos) {
         pos = str.length();

         if(pos != lastPos || !trimEmpty)
            tokens.push_back(ContainerT::value_type(str.data()+lastPos,
                  (ContainerT::value_type::size_type)pos-lastPos ));

         break;
      } else {
         if(pos != lastPos || !trimEmpty)
            tokens.push_back(ContainerT::value_type(str.data()+lastPos,
                  (ContainerT::value_type::size_type)pos-lastPos ));
      }

      lastPos = pos + 1;
   }
};

}

struct Houdini::State {
  ScreenOutput* so;
};


void OnHelp(Houdini::State* state, std::vector<std::string>& tokens) {
  if (tokens.size() == 1) {
    state->so->Output("\\cf1 commands are");
    state->so->NewLine();
    state->so->Output("\\cf1 help");
    state->so->NewLine();
    state->so->Output("\\cf1 quit");
    state->so->NewLine();
  }
  else {
    state->so->Output("no command specific help yet");
    state->so->NewLine();
  }
}

Houdini::Houdini(ScreenOutput* so) : state_(new State) {
  state_->so = so;
  so->Output("type \\cf1 help \\cf0 for available commands");
  so->NewLine();
}

// \\cf1 %d \\cf0 tests to run. Time is %d:%d.%d \\line

Houdini::~Houdini() {
  delete state_;
}

void Houdini::InputCommand(const char* command) {
  // Echo the command in the results pane.
  std::string comm(command);
  locase_str(comm);
  state_->so->Output(">");
  state_->so->Output(comm.c_str());
  state_->so->NewLine();
  // tokenize and then use the first token to route.
  std::vector<std::string> tokens;
  tokenize(comm, tokens, " ", true);
  if (tokens.size() == 0)
    return;
  const std::string& verb = tokens[0];
  if (verb == "help") {
    OnHelp(state_, tokens);
  } else if (verb == "quit") {
    // quitting is hard.
  } else {
    state_->so->Output("\\cf1 huh?");
    state_->so->NewLine();
  }
}

