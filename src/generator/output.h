#include "digest.h"
#include <iostream>

#pragma once

void execute(const char* path);

class output
{
protected:
	const std::vector<long long> *plist;
public:
	output(){}
	output(const std::vector<long long> &_list);
	void set(const std::vector<long long> &_list);
	void operator () (std::ostream& ous) const;
};