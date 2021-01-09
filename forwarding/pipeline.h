
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

	//判別指令種類
	void judge_forwarding(int, int, int, int, int);
	//判別出撞車的指令種類與動作後
	void forwarding_add_sub(int);
	void forwarding_add_sub_mem(int);
	void forwarding_sw(int);
	void forwarding_lw(int);

private:
	std::map<string, int> pipeline;
	fstream file;
	int reg[32][2];//reg[][1]為flag，為1時代表不能用，為0代表可用
	int mem[32];
	string instructions[20][4];
	map<string, string> signals;
	int cycleNum;
	int in;
	int total;
	bool do_stall;
	bool if_beq;

	bool do_forwarding;
	bool cond3;
	bool beq_stall;
	int ID_EXE[3];
	int EXE_MEM[3];
	int MEM_WB[3];
};

#endif
