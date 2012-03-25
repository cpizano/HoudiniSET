#include <stdio.h>
#include <string>

#include <windows.h>
#include <windowsx.h>
#include <Richedit.h>

#include "resource.h"
#include "houdini.h"


// Enable Visual Style
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

const wchar_t kProdVersion[] = L"0.0.1";
Houdini* g_houdini;

namespace {

const char kRTFftm[] =
  "{\\rtf1\\ansi\\deff0 "
  "{\\fonttbl{\\f0\\fnil\\fcharset0 Courier New;}}"                   
  "{\\colortbl ;\\red255\\green20\\blue0;\\red0\\green176\\blue80;}"
  "%s }";

LRESULT SetText(HWND hWnd, const wchar_t* text, DWORD flags = ST_DEFAULT) {
	SETTEXTEX ste = {flags, CP_ACP};
	return ::SendMessageW(hWnd, EM_SETTEXTEX,
                        reinterpret_cast<WPARAM>(&ste),
                        reinterpret_cast<LPARAM>(text));
}

void AddRTF(HWND hWnd, const std::string& text1) {
  size_t sz = sizeof(kRTFftm) + text1.size() + 16;
  char* buf = new char[sz];
  sprintf_s(buf, sz, kRTFftm, text1.c_str());
  SetText(hWnd, reinterpret_cast<wchar_t*>(buf), ST_SELECTION);
  delete[] buf;
}

}  // namespace.

class RichEditOutput : public ScreenOutput {
public:
  RichEditOutput(HWND hwnd) : hwnd_(hwnd) {}
  void ScreenOutput::Output(const char* text) override {
    AddRTF(hwnd_, text);
  }

  void ScreenOutput::NewLine() override {
  }

private:
  HWND hwnd_;
};


void SetTitle(HWND hwnd) {
  wchar_t buf[60];
  wsprintf(buf, L"Houdini %s  (pid:%d)", kProdVersion, ::GetCurrentProcessId());
  ::SetWindowTextW(hwnd, buf);
}

BOOL OnInitDialog(HWND hwnd, HWND hwnd_focus, LPARAM lParam) {
  SetTitle(hwnd);
  HWND richedit = ::GetDlgItem(hwnd, IDC_RICHEDIT21);
  g_houdini = new Houdini(new RichEditOutput(richedit));

  return TRUE;
}

void OnCommand(HWND hwnd, int id, HWND ctl, UINT notify) {
  switch (id) {
    default:
      ;
  }
}

void OnClose(HWND hWnd) {
  EndDialog(hWnd, 0);
}

INT_PTR CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    HANDLE_MSG(hWnd, WM_INITDIALOG, OnInitDialog);
    HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
    HANDLE_MSG(hWnd, WM_CLOSE, OnClose);
  default:
      return FALSE;
  }
  return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPWSTR    lpCmdLine,
                      int       nCmdShow) {
  ::LoadLibraryA("RICHED20.DLL");

  return DialogBoxW(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DialogProc);
}
