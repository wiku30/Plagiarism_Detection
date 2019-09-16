#pragma once

#include "cmp_base.h"

class cmp_kr: public cmp_base
{
protected:
	static long long mod;
	static long long coef;
	long long iter_const; //-coef^(length-1)
	long long index;
	long long _hash(std::string _str, int h_len) const;
public:
	cmp_kr(std::string _str);
	cmp_kr(int _length);
	int check(source src) const; 
};
