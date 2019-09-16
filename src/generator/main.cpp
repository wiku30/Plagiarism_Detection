#include "output.h"
#include "digest.h"
#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
	for(int i=1;i<argc;i++)
	{
		execute(argv[i]);
	}
	return 0;
}