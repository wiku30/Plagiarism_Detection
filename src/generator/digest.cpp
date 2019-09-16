#include "digest.h"
#include <string>
#include <cstdlib>
#include <algorithm>

using namespace std;

const int digest::hash_length=32;
const int digest::window_range=96;

digest::digest() : cmp_kr(hash_length)
{
	head=0;
}

const vector<long long>& digest::getlist() const
{
	return list;
}

int digest::size() const
{
	return list.size();
}

void digest::load(const source& src)
{
	window.clear();
	list.clear();
	if(src.length()<hash_length)
	{
		cout<<"Source too short!"<<endl;
		exit(-1);
	}
	long long now_min=mod*2;
	int now_min_pl=-1;

	long long hash=_hash(src, hash_length);
	for(int place=0;place<=src.length()-hash_length;place++)
	{
		int ifpush=0;
		window.push_back(hash);
		if(window.size()>window_range)
		{
			window.erase(window.begin());
			head++;
		}
		if(now_min_pl<head)
		{
			int flag=0;
			now_min=(int)mod*2;
			for(unsigned i=0;i<window.size();i++)
			{
				if(window[i]<=now_min)
				{
					now_min=window[i];
					now_min_pl=i+head;
					flag=1;
				}
			}
			if(flag && window.size()>=window_range)
			{
				ifpush=1;
			}
		}
		else
		{
			if(hash<=now_min)
			{
				now_min=hash;
				now_min_pl=place;
				if(window.size()>=window_range)
					ifpush=1;
			}
		}
		if(place==window_range-1)
			ifpush=1;
		if(ifpush)
		{
			list.push_back(now_min);
		}
////////////////////////////////////////////////////////////////////
		//get a new hash
		//      |   [pl]   [pl+1]   ... [pl+n-1]   [pl+n]
		//      | q^(n-1)  q^(n-2)  ...    1
		// ---> |          q^(n-1)  ...    q         1

		if(place<src.length()-hash_length)
		{
			hash+=(src[place]*iter_const)%mod;
			hash%=mod;
			hash*=coef;
			hash%=mod;
			hash+=src[place+hash_length];
			hash+=mod;  //to avoid negative hash
			hash%=mod;
		}

	}
	sort(list.begin(),list.end());
}
