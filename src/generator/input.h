#pragma once

#include <string>
#include <iostream>

class source
{
protected:
	std::string str;
public:
	source(){};
	source(const std::string _str);
	source(std::istream& ins);
	virtual void load(std::istream& ins);
	virtual void output(std::ostream& ous) const;
	operator std::string() const;
	std::string tostr() const;
	friend std::ostream& operator <<(std::ostream& os, source src);
	
	char& operator [](int pl){return str[pl];}
	const char& operator [](int pl) const {return str[pl];}
	int length() const {return str.size();}
};