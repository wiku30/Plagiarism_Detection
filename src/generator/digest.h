#pragma once

#include "cmp_kr.h"
#include <vector>

class digest: public cmp_kr
{
private:
	const static int hash_length;
	const static int window_range;
	std::vector<long long> window; 
	std::vector<long long> list;
	int length() const{return -1;}
	int head;
	bool isvalid;
public:
	digest();
	void load(const source& input);
	const std::vector<long long> & getlist() const;
	int size() const;
};