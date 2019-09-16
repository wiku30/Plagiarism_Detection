#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

unsigned binary_get(istream& ins)
{
	unsigned res=0;
	for(int i=0;i<4;i++)
	{
		res*=256;
		res+=(unsigned)ins.get();
	}
	return res;
}

void load(const char* path, vector<int>& target)
{
	target.clear();
	ifstream inf(path,ios_base::binary);
	int num=binary_get(inf);
	for(int n=0;n<num;n++)
	{
		target.push_back(binary_get(inf));
	}
}

int count(const vector<int> &a, const vector<int> &b)
{
	unsigned i1=0,i2=0;
	int res=0;
	while(1)
	{
		if(i1>=a.size() || i2>=b.size())
		{
			return res;
		}
		if(a[i1]==b[i2])
		{
			res++;
			i1++;
			i2++;
		}
		else if(a[i1]<b[i2])
			i1++;
		else
			i2++;
	}
}

int main(int argc, char ** argv)
{
	ofstream ouf("log.txt");
	if(argc!=3)
	{
		ouf<<"请导入两个index文件供测试！！"<<endl;
		return 0;
	}
	vector<int> v1,v2;
	int s1,s2,res;
	double min_len;
	load(argv[1],v1);
	load(argv[2],v2);
	s1=v1.size();
	s2=v2.size();
	min_len=s1<s2 ? s1 : s2;
	res=count(v1,v2);
	ouf<<"两文件分别含有"<<s1<<"和"<<s2<<"个标记，其中"<<res<<"个重合。"<<endl;
	ouf<<"重合率为"<<int(res*100/min_len+0.5)<<"%"<<endl;
	return 0;
}