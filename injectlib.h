#ifndef __INJECTLIB_H__
#define __INJECTLIB_H__
#include <string>

std::string reloc(const std::string &opcode, ptrdiff_t diff);
bool epInjectDll(const std::string &app, const std::string &paramters,
	const std::string &workDir, const std::string &dll);
#endif
