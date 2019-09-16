#include "cmp_kmp.h"

cmp_kmp::cmp_kmp(std::string _str) :cmp_base(_str)
{
	initialize();
}

//I referred to the website http://www.cnblogs.com/10jschen/archive/2012/08/21/2648451.html
//for the optimal implementation of next[]

void cmp_kmp::initialize()
{
	next.push_back(-1);
	int i=0,j=-1;
	while(i<length())
	{
		if(j==-1 || str[i]==str[j])
		{
			i++;
			j++;
			next.push_back(j);
		}
		else
		{
			j=next[j];
		}
	}
}

int cmp_kmp::check(source src) const
{
	int place=0;
	while(place<=src.length()-length())
	{
		int flag=1;
		for(int i=0;i<length();i++)
		{
			if(str[i]!=src[place+i])
			{
				place+=(i-next[i]);
				flag=0;
				break;
			}
		}
		if(flag)
			return place;
	}
	return -1;
}