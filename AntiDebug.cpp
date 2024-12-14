#include "antidebug.h"
#include <vector>
#include <sstream>
#include <thread>
#include "hooklib.h"

#pragma comment(lib, "ws2_32.lib")

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

BOOL ConnectTimeOut(WORD dwPort)
{
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in svr;
	svr.sin_addr.s_addr = inet_addr("127.0.0.1");
	svr.sin_port = htons(dwPort);
	svr.sin_family = AF_INET;

	unsigned long mode = 1;
	ioctlsocket(s, FIONBIO, &mode);
	int err = connect(s, (sockaddr *)&svr, sizeof(svr));
	if (err == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			closesocket(s);
			return TRUE;
		}
	}
	mode = 0;
	ioctlsocket(s, FIONBIO, &mode);
	fd_set fd_write = { 0 };
	timeval	time_out = { 0 };
	time_out.tv_sec = 0;
	time_out.tv_usec = 1000 * 200;
	FD_SET(s, &fd_write);
	int ret = select(0, NULL, &fd_write, NULL, &time_out);
	if (ret <= 0)
	{
		closesocket(s);
		return FALSE;
	}
	closesocket(s);
	return TRUE;
}

WORD SelectDevToolPort()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	WORD i = 10000 + GetCurrentProcessId() % 10000;
	for (int j = 0; j < 100; j++)
	{
		BOOL bRet = ConnectTimeOut(i + j);
		if (!bRet)
			return i + j;
	}
	return 0;
}

HOOK_INFO hkinfo_create_process;
typedef BOOL(WINAPI *PFN_CreateProcessW)(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation);

BOOL NewCreateProcessW(LPCWSTR lpApplicationName,
                       LPWSTR lpCommandLine,
                       LPSECURITY_ATTRIBUTES lpProcessAttributes,
                       LPSECURITY_ATTRIBUTES lpThreadAttributes,
                       BOOL bInheritHandles,
                       DWORD dwCreationFlags,
                       LPVOID lpEnvironment,
                       LPCWSTR lpCurrentDirectory,
                       LPSTARTUPINFOW lpStartupInfo,
                       LPPROCESS_INFORMATION lpProcessInformation)
{
    PFN_CreateProcessW pfn_old = (PFN_CreateProcessW)GetNewAddress(&hkinfo_create_process);
    LPCWSTR lpWhereCommandLine = nullptr;
    LPCWSTR lpEmptyCommandLine = L"";
    BOOL appPathIncludeDevProcName = FALSE;

    std::vector<std::wstring> devProcessName;
    devProcessName.push_back(L"直播伴侣.exe");

    int nArgs = 0;
    LPWSTR *szArglist = nullptr;

    if (lpCommandLine)
        lpWhereCommandLine = lpCommandLine;
    else
        lpWhereCommandLine = lpEmptyCommandLine;

    if (lpApplicationName)
        szArglist = CommandLineToArgvW(lpApplicationName, &nArgs);
    else if (lpCommandLine)
        szArglist = CommandLineToArgvW(lpCommandLine, &nArgs);

    if (szArglist && nArgs > 0)
    {
        std::wstring appPath = szArglist[0];
        for (auto appName = devProcessName.begin();
             appName != devProcessName.end(); appName++)
        {
            if (std::wstring::npos != appPath.find(*appName))
            {
                appPathIncludeDevProcName = TRUE;
                break;
            }
        }
        LocalFree(szArglist);
    }
    std::vector<wchar_t> newCommandLine;
    if (lpWhereCommandLine && appPathIncludeDevProcName)
    {
        std::wstring commandLineAppend = lpWhereCommandLine;
        WORD port = SelectDevToolPort();
        std::wostringstream sstr(L" --remote-debugging-port=", std::ios_base::app|std::ios_base::out);
        sstr << SelectDevToolPort();
        commandLineAppend += sstr.str();
        newCommandLine.assign(commandLineAppend.begin(), commandLineAppend.end());
        newCommandLine.push_back(0);

        lpCommandLine = newCommandLine.data();
    }

    return pfn_old(lpApplicationName, lpCommandLine,
                   lpProcessAttributes, lpThreadAttributes,
                   bInheritHandles, dwCreationFlags,
                   lpEnvironment, lpCurrentDirectory,
                   lpStartupInfo, lpProcessInformation);
}

// dev tool
BOOL hookCreateProcess()
{
    return HookAPIByName("kernel32.dll", "CreateProcessW",
                         &hkinfo_create_process, NewCreateProcessW);
}


BOOL AntiDebug()
{
    FARPROC pfnSetInformationThread = GetProcAddress(LoadLibraryA("ntdll.dll"), "ZwSetInformationThread");
    HookByAddress((void *)pfnSetInformationThread, &hkInfo,
                  (void *)&pfnZwSetInformationThread);

    //hookCreateProcess();
    std::thread threadObj([]{
        //34AAD0 DevToolsManagerDelegate::StartHttpHandler()
        typedef void (*PFN_StartHttpHandler)();
        PFN_StartHttpHandler pfn = reinterpret_cast<PFN_StartHttpHandler>(reinterpret_cast<uintptr_t>(GetModuleHandle(NULL)) + 0x34AAD0);
        std::ostringstream sstr;
        sstr << GetCurrentProcessId();
        MessageBoxA(NULL, sstr.str().c_str(), "click to open devtools", MB_OK);
        pfn();
    });
    threadObj.detach();
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
