#include <iostream>
#include <stdlib.h>
#include <vector>
void printInt(int cond)
{
  std::cout << "Branch Outcome : " << cond << "\n";
}
__attribute__((noinline)) void printBranch(char* name, int cond, char* n1, char *n2)
{
	char *target;
	if(cond == 0)
		target = n1;
	else
		target = n2;
	std::cout << "B: "<<name << "," << target << "\n";
	//std::cout << "Branch ["<< name << "]: " << cond << " / " << target <<  "\n";	
}

__attribute__((noinline)) void printMem(char *name, bool type, long long addr, int size)
{
	if(type == 0)
		std::cout << "Load ["<< name << "]: " << type << " / " << addr << " / " << size <<  "\n";	
	else if(type == 1)
		std::cout << "Store ["<< name << "]: " << type << " / " << addr << " / " << size <<  "\n";	
}
__attribute__((noinline)) void printSw(char *name, int value, char *def, int n, ...)
{
	va_list vl;
	std::vector<int> vals;
	std::vector<char*> strs;
	char* target;
	va_start(vl,n);
	for(int i=0; i<n; i++)
	{
		int val = va_arg(vl, int);
		char *bbname = va_arg(vl, char*);
		vals.push_back(val);
		strs.push_back(bbname);
	}
	va_end(vl);
	bool found = false;
	for(int i=0; i<vals.size(); i++) {
		if(vals.at(i) == value) {
			target = strs.at(i);
			found = true;
			break;
		}
	}
	if(!found)
		target = def;
	std::cout << "S: " <<name << "," << target <<"\n";
	//std::cout << "Switch [" << name << "]: " << value << " / " << target << "\n";
}