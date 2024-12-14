#ifndef __PROCESS_H__
#define __PROCESS_H__
#include <string>

class CProcess
{
public:
	virtual bool start(const std::string &app,
		const std::string &workDir,
		const std::string &paramters) = 0;
	virtual bool resume() = 0;
	virtual bool readP(void *address, std::string &readBuffer) = 0;
	virtual bool writeP(void *address, std::string &writeBuffer) = 0;
	virtual void* allocP(size_t sz, unsigned flag) = 0;
	virtual bool freeP(void *p) = 0;
	virtual unsigned protectP(void *address, size_t sz, unsigned flag) = 0;
};

#endif