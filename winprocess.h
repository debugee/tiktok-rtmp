#ifndef __WINPROCESS_H__
#define __WINPROCESS_H__
#include "process.h"
#include <Windows.h>

class CWinProcess : public CProcess
{
public:
	~CWinProcess();
	CWinProcess();
	CWinProcess(unsigned pid);
	bool start(const std::string &app,
		const std::string &workDir,
		const std::string &paramters);
	bool resume();
	bool readP(void *address, std::string &readBuffer);
	bool writeP(void *address, std::string &writeBuffer);
	unsigned protectP(void *address, size_t sz, unsigned flag);
	void* allocP(size_t sz, unsigned flag);
	bool freeP(void *p);

	void* getEP();
	void* getBase();
	void* findMiddleSpace(size_t sz);
	std::string getShellCode(const std::string &dll);
private:
	HANDLE m_hProcess;
	HANDLE m_hThread;
};

#endif
