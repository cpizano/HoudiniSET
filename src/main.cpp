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
WNDPROC  g_edit_ctrl_proc;

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

LRESULT CALLBACK EditSubProc2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_KEYDOWN:
      if (wParam == VK_RETURN) {
        ::Beep(440, 40);
      }
    default:
      ;
  };
  return ::CallWindowProcW(g_edit_ctrl_proc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK EditSubProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_GETDLGCODE:
      return (DLGC_WANTALLKEYS |
          CallWindowProc(g_edit_ctrl_proc, hwnd, message, wParam, lParam));
    case WM_CHAR:
      if ((wParam == VK_RETURN) || (wParam == VK_TAB))
        return 0;
      break;
    case WM_KEYDOWN:
      if ((wParam == VK_RETURN) || (wParam == VK_TAB)) {
        // do stuff here
        return FALSE;
      }
      break;
  }
  return CallWindowProc(g_edit_ctrl_proc, hwnd, message, wParam, lParam);
}
				

BOOL OnInitDialog(HWND hwnd, HWND hwnd_focus, LPARAM lParam) {
  SetTitle(hwnd);
  HWND richedit = ::GetDlgItem(hwnd, IDC_RICHEDIT21);
  g_houdini = new Houdini(new RichEditOutput(richedit));

  // Subclass the edit control because we want the return key.
  HWND edit = ::GetDlgItem(hwnd, IDC_EDIT1);
  g_edit_ctrl_proc = reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(edit, GWLP_WNDPROC));
  ::SetWindowLongPtrW(edit, GWLP_WNDPROC, LONG_PTR(EditSubProc));

  return TRUE;
}

void OnEditCtrlChanged(HWND ctrl, UINT notify) {
  switch (notify) {
    case  EN_CHANGE: {
        wchar_t line[80];
        Edit_GetLine(ctrl, 0, line, _countof(line));
        break;
      }
    default:
      ;
  };
}

void OnCommand(HWND hwnd, int id, HWND ctl, UINT notify) {
  switch (id) {
    case IDC_EDIT1:
      OnEditCtrlChanged(ctl, notify);
      break;
    default:
      ;
  };

}

void OnClose(HWND hWnd) {
  EndDialog(hWnd, 0);
}

void OnChar(HWND hwnd, TCHAR ch, int cRepeat) {
  ::Beep(440, 40);
}

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    HANDLE_MSG(hwnd, WM_CLOSE, OnClose);
    HANDLE_MSG(hwnd, WM_CHAR, OnChar);
    HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
    HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
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
