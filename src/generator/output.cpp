#include "output.h"
#include <string>
#include <fstream>
using namespace std;

void execute(const char* _path)
{
	string path(_path);
	string target=path + ".index";
	ifstream inf(path.c_str());
	if(!inf)
	{
		return;
	}
	digest dig;
	dig.load(inf);
	ofstream ouf(target.c_str(),ios_base::binary);
	output op(dig.getlist());
	op(ouf);
}

void binary_output(std::ostream& ous, unsigned data)
{
	for(int bias=24;bias>=0;bias-=8)
	{
		ous.put((data>>bias)%256);
	}
}

void output::set(const std::vector<long long> &_list)
{
	plist=&_list;
}

output::output(const std::vector<long long> &_list)
{
	plist=&_list;
}

void output::operator () (std::ostream& ous) const
{
	binary_output(ous, plist->size());
	for(int i=0; i<plist->size(); i++)
	{
		binary_output(ous, unsigned((*plist)[i]));
	}
}