#include <Windows.h>
#include <sstream>

#include "houdini_command.h"
#include "houdini.h"

void FileTimeConvert(houdini::ScreenOutput* so, const std::vector<std::string>& tokens) {
  std::istringstream iss(tokens[2]);
  ULARGE_INTEGER uli;
  iss >> uli.QuadPart;
  FILETIME ft = {uli.LowPart, uli.HighPart};
  FILETIME lft = {0};
  if (!::FileTimeToLocalFileTime(&ft, &lft)) {
    so->Output(RCLR(1)"invalid filetime #1");
    so->NewLine();
    return;
  }
  SYSTEMTIME st = {0};
  if (!::FileTimeToSystemTime(&lft, &st)) {
    so->Output(RCLR(1)"invalid filetime #2");
    so->NewLine();
    return;
  }
  std::ostringstream oss;
  oss << RCLR(1) << st.wYear << "." << st.wMonth << "." << st.wDay << " ";
  oss << st.wHour << ":" << st.wMinute << "." << st.wSecond;
  so->Output(oss.str().c_str());
  so->NewLine();  
}


class Conversions : public houdini::Command {
public:
  virtual ~Conversions() {};

protected:
  virtual bool OnCommand(houdini::ScreenOutput* so,
                         const std::vector<std::string>& tokens) override {
    if ((tokens.size() != 3) || (tokens[1] != "filetime")) {
      return false;
    }

    FileTimeConvert(so, tokens);
    return true;
  }

  virtual void OnHelp(houdini::ScreenOutput* so,
                      const std::string& topic) override {
    if (topic != "filetime") {
      so->Output(RCLR(1)"converts windows types to human readable formats. Use:"RLF \
                 RCLR(0)"conv filetime qword"RCLR(1)" to convert from FILETIME");
    } else {
      if (topic == "filetime") {
        so->Output(RCLR(1)"got topic, but no help yet");
      }
    }

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
