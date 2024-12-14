#include <windows.h>
#include <Shlwapi.h>
#include <atlstr.h>
#include "injectlib.h"
#include "charset.h"

int _tmain(int argc, TCHAR **argv)
{
#ifndef NDEBUG
	MessageBox(NULL, _T("epinject"), NULL, MB_OK);
#endif
	if (argc != 3) {
		printf("epinject.exe app_full_path dll_full_path\n");
		return 0;
	}
	CString szAppPath;
	szAppPath.Format(_T("%s"), argv[1]);
	CString szPath;
	szPath.Format(_T("%s"), argv[2]);//full path

	std::string app;
	std::string dll;
	charset::u16ToU8Str(szAppPath.GetString(), app);
	charset::u16ToU8Str(szPath.GetString(), dll);
	epInjectDll(app,
		"",
		"",
		dll);

    return 0;
}

