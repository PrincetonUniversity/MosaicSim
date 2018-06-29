#include <iostream>
#include <stdlib.h>
#include <cassert>
#include <vector>
#include <fstream>

std::ofstream f1;
std::ofstream f2;
__attribute__((noinline)) void printBranch(char* name, int cond, char* n1, char *n2)
{
  if(!f1.is_open())
    f1.open("output/ctrl.txt", std::ofstream::out | std::ofstream::trunc);
	char *target;
	if(cond == 0)
		target = n1;
	else
		target = n2;
	f1 << "B,"<<name << "," << target << "\n";
	//std::cout << "Branch ["<< name << "]: " << cond << " / " << target <<  "\n";	
}

__attribute__((noinline)) void printuBranch(char* name, char *n1)
{
	if(!f1.is_open())
    f1.open("output/ctrl.txt", std::ofstream::out | std::ofstream::trunc);
  if(!f1.is_open())
    assert(false);
	f1 << "U,"<< name << "," << n1 << "\n";
	//std::cout << "Branch ["<< name << "]: " << cond << " / " << target <<  "\n";	
}

__attribute__((noinline)) void printMem(char *name, bool type, long long addr, int size)
{
	if(!f2.is_open())
    f2.open("output/mem.txt", std::ofstream::out | std::ofstream::trunc);
	
	if(type == 0)
		//std::cout << "Load ["<< name << "]: " << type << " / " << addr << " / " << size <<  "\n";	
		f2 << "L,"<< name << "," << addr << ","<< size <<"\n";
	else if(type == 1)
		f2 << "S,"<< name << "," << addr << ","<< size <<"\n";
		//std::cout << "Store ["<< name << "]: " << type << " / " << addr << " / " << size <<  "\n";	
}
__attribute__((noinline)) void printSw(char *name, int value, char *def, int n, ...)
{
  if(!f1.is_open())
    f1.open("output/ctrl.txt", std::ofstream::out | std::ofstream::trunc);
	va_list vl;
	std::vector<int> vals;
	std::vector<char*> strs;
	char* target;
	va_start(vl,n);
	for(int i=0; i<n; i++) {
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
	f1 << "S," <<name << "," << target <<"\n";
	//std::cout << "Switch [" << name << "]: " << value << " / " << target << "\n";
}