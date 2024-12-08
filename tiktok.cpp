#include <iostream>
#include <tchar.h>
#include <windows.h>
#include <sstream>
#include "hooklib.h"

HOOK_INFO hkInfoURL;
HOOK_INFO hkInfoStream;

typedef int (*PFN_RTMP_SetupURL)(void *r, char *url);
typedef int (*PFN_RTMP_AddStream)(void *r, const char *playpath);

bool sendMsg(const char *msg, ULONG_PTR type)
{
    HWND hwnd = ::FindWindow(NULL, _T("标题"));
    if (NULL != hwnd)
    {
        COPYDATASTRUCT data;
        data.dwData = type;
        data.cbData = std::strlen(msg) + 1;
        data.lpData = const_cast<char *>(msg);
        ::SendMessage(hwnd, WM_COPYDATA, 0, (LPARAM)&data);
    }

    return true;
}

int setupURL_Proxy(void *r, char *url)
{
    PFN_RTMP_SetupURL pfn = (PFN_RTMP_SetupURL)GetNewAddress(&hkInfoURL);
#ifndef NDEBUG
    MessageBoxA(NULL, url, "test", MB_OK);
#endif
    sendMsg(url, 0);
    return pfn(r, url);
}

int addStream_Proxy(void *r, const char *playpath)
{
    PFN_RTMP_AddStream pfn = (PFN_RTMP_AddStream)GetNewAddress(&hkInfoStream);
#ifndef NDEBUG
    MessageBoxA(NULL, playpath, "test", MB_OK);
#endif
    sendMsg(playpath, 1);
    return pfn(r, playpath);
}

void InstallHook()
{
    FARPROC pfnSetupURL = GetProcAddress(LoadLibraryA("librtmp.dll"), "RTMP_SetupURL");
    HookByAddress((void *)pfnSetupURL, &hkInfoURL,
                  (void *)&setupURL_Proxy);
    FARPROC pfnAddStream = GetProcAddress(LoadLibraryA("librtmp.dll"), "RTMP_AddStream");
    HookByAddress((void *)pfnAddStream, &hkInfoStream,
                  (void *)&addStream_Proxy);
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        std::ostringstream ostr;
        ostr << "tiktok_mutex_";
        ostr << GetCurrentProcessId();
        HANDLE hMutex = CreateMutexA(NULL, FALSE, ostr.str().c_str());
        if (hMutex)
        {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_ALREADY_EXISTS)
            {
                CloseHandle(hMutex);
                break;
            }
        }
        InstallHook();
        break;
    }
    return TRUE;
}
