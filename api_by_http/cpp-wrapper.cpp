// cpp-wrapper.cpp : Defines the entry point for the console application.
//

#include "BTCChinaAPI.h"
#include <fstream>

using namespace std;

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		cout << "usage: cpp-wrapper.exe acc-filename" << endl
    		 << "first line is accesskey and second line is secretkey" << endl;
		return -1;
	}
	
	ifstream ifs(argv[1]);
	if (ifs.bad())
	{
		cout << "error open file:" << argv[1] << endl;
		return -1;
	}
	else
	{
		string accesskey, secretkey;
		getline(ifs, accesskey);
		getline(ifs, secretkey);
		CBTCChinaAPI testAPI(accesskey, secretkey);
		string result1, result2;
		cout << clock() << endl;
		testAPI.getAccountInfo(result1);
		cout << clock() << endl;
		testAPI.getOrders(result2, false);
		cout << result2 << endl;
		cout << result1 << endl;
		return 0;
	}
}

