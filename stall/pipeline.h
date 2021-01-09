
#ifndef PIPLINE_H
#define PIPLINE_H

#include <string>
#include <map>
#include <fstream>;
using namespace std;

class Pipeline {
public:
	Pipeline(string);
	void cycle();
	void stall();
	void calculate(int);
	int filenum;

private:
	std::map<string, int> pipeline;
	fstream file;
	int reg[32][2];
	int mem[32];
	string instructions[20][4];
	map<string, string> signals;
	int cycleNum;
	int in;
	int total;
	bool do_stall;
	bool if_beq;
};

#endif
