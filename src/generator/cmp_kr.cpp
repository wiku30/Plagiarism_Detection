#include "cmp_kr.h"


long long cmp_kr::coef=2333333;
long long cmp_kr::mod=19971019; //a prime

long long cmp_kr::_hash(std::string _str, int h_len) const
{
	long long res=0;
	for(int i=0;i<h_len;i++)
	{
		res*=coef;
		res+=_str[i];
		res%=mod;
	}
	return res;
}

cmp_kr::cmp_kr(std::string _str): cmp_base(_str)
{
	index=_hash(str,str.length());
	iter_const=mod-1; //to be added rather than subtracted
	for(int i=0;i<length()-1;i++) //no need for quick power, I think
	{
		iter_const*=coef;
		iter_const%=mod;
	}
}

cmp_kr::cmp_kr(int _length)
{
	iter_const=mod-1; //to be added rather than subtracted
	for(int i=0;i<_length-1;i++) //no need for quick power, I think
	{
		iter_const*=coef;
		iter_const%=mod;
	}
}

int cmp_kr::check(source src) const
{
	long long hash=_hash(src,length());
	for(int place=0;place<=src.length()-length();place++)
	{
		if(hash==index) //probably found
		{
			int flag=1;
			for(int i=0;i<length();i++)
			{
				if(src[place+i]!=str[i])
				{
					flag=0;
					break;
				}
			}
			if(flag)
				return place; //found
		}

		//iterate hash
		//      |   [pl]   [pl+1]   ... [pl+n-1]   [pl+n]
		//      | q^(n-1)  q^(n-2)  ...    1
		// ---> |          q^(n-1)  ...    q         1

		if(place<src.length()-length())
		{
			hash+=src[place]*iter_const;
			hash%=mod;
			hash*=coef;
			hash%=mod;
			hash+=src[place+length()];
			hash+=mod;  //to avoid negative hash
			hash%=mod;
		}
	}
	return -1;
}
