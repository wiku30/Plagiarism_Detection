#pragma once

#include <string>
#include "input.h"

//As a abstract base class for distinct kinds of string comparing

class cmp_base: public source
{
public:
	cmp_base (std::string _str): source(_str){}
	cmp_base (){};
	virtual int check(source src) const=0 ;
};