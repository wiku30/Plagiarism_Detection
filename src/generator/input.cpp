#include "input.h"
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <cstdlib>

void source::load(std::istream& ins)
{
	if(!ins)
	{
		std::cout<<"Input failed!"<<std::endl;
		exit(-1);
	}
	str.clear();
	std::string buf;
	while(!ins.eof())
	{
		ins>>buf;
		std::transform(buf.begin(),buf.end(),buf.begin(),::tolower);
		str+=buf;
	}
}

source::source(std::istream& ins)
{
	load(ins);
}

source::source(const std::string _str) //perhaps there is a better way
{
	std::ofstream ofs("__tmp");
	ofs<<_str;
	ofs.close();
	std::ifstream ifs("__tmp");
	load(ifs);
	ifs.close();
	remove("__tmp");
}

void source::output(std::ostream& ous) const
{
	ous<<str;
}

source::operator std::string() const
{
	return str;
}

std::string source::tostr() const
{
	return str;
}

std::ostream& operator <<(std::ostream& os, source src)
{
	os<<src.str;
	return os;
}