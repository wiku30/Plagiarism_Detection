#pragma once

#include <vector>
#include "cmp_base.h"

class cmp_kmp: public cmp_base
{
protected:
	std::vector<int> next;
public:
	cmp_kmp(std::string _str);
	void initialize();
	virtual int check(source src) const;
};