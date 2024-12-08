#include "AntiDebug.h"
#include "hooklib.h"
#include <atlstr.h>
#include <vector>
#include <sstream>


static HOOK_INFO hkInfo;

typedef LONG(WINAPI *PFN_SetInformationThread)(IN HANDLE ThreadHandle,
                                               IN DWORD ThreadInformaitonClass,
                                               IN PVOID ThreadInformation,
                                               IN ULONG ThreadInformationLength);

LONG WINAPI pfnZwSetInformationThread(IN HANDLE ThreadHandle,
                                      IN DWORD ThreadInformaitonClass,
                                      IN PVOID ThreadInformation,
                                      IN ULONG ThreadInformationLength)
{
    PFN_SetInformationThread pfn_old = (PFN_SetInformationThread)GetNewAddress(&hkInfo);
    if (ThreadInformaitonClass == 17)
    {
        ThreadInformaitonClass = 123123;
    }
    return pfn_old(ThreadHandle, ThreadInformaitonClass,
                   ThreadInformation, ThreadInformationLength);
}

BOOL AntiDebug()
{
    FARPROC pfnSetInformationThread = GetProcAddress(LoadLibraryA("ntdll.dll"), "ZwSetInformationThread");
    HookByAddress(pfnSetInformationThread, &hkInfo,
                  &pfnZwSetInformationThread);

    return TRUE;
}

extern "C" __declspec(dllexport) void asdfsadf()
{

}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        AntiDebug();
        break;
    }
    return TRUE;
}
