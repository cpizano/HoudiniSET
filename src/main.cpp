#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include "resource.h"

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

void SetTitle(HWND hwnd) {
  wchar_t buf[60];
  wsprintf(buf, L"Houdini %s  (pid:%d)", kProdVersion, ::GetCurrentProcessId());
  ::SetWindowTextW(hwnd, buf);
}

BOOL OnInitDialog(HWND hwnd, HWND hwnd_focus, LPARAM lParam) {
  SetTitle(hwnd);

  return TRUE;
}

void OnCommand(HWND hwnd, int id, HWND ctl, UINT notify) {
  switch (id) {

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
