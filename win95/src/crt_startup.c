
#include <windows.h>

extern int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

void WinMainCRTStartup(void) {
    STARTUPINFOA si;
    si.cb = sizeof(si);
    GetStartupInfoA(&si);
    int show = (si.dwFlags & STARTF_USESHOWWINDOW) ? si.wShowWindow : SW_SHOWDEFAULT;
    int ret = WinMain(GetModuleHandleA(NULL), NULL, GetCommandLineA(), show);
    ExitProcess(ret);
}
