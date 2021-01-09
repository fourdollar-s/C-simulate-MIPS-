#include "pipeline.h"
#include <iostream>
#include <sstream>
using namespace std;

string str_replace(string s, string a, string b)
{
	int first = s.find(a);
	string g = b;
	string t = "";

	while (first != -1) {
		t.append(s.substr(0, first));
		s = s.substr(first + 1);
		s = b.append(s);
		b = g;
		first = s.find(a);
	}
	t.append(s);
	return t;
}
Pipeline::Pipeline(string f)
{
	string k;

	// ==========================初始化==========================
	//reg and mem
	for (int i = 0; i < 32; i++) {
		reg[i][0] = 1;
		reg[i][1] = 0;
		mem[i] = 1;
	}
	reg[0][0] = 0;


	// signals 
	signals.insert(pair<string, string>("lw", "01 010 11"));
	signals.insert(pair<string, string>("sw", "X1 001 0X"));
	signals.insert(pair<string, string>("beq", "X0 100 0X"));
	signals.insert(pair<string, string>("add", "10 000 10"));//R type
	signals.insert(pair<string, string>("sub", "10 000 10"));//R type

	// pipeline
	pipeline["IF"] = -1;
	pipeline["ID"] = -1;
	pipeline["EX"] = -1;
	pipeline["MEM"] = -1;
	pipeline["WB"] = -1;

	//讀檔&字串處理
	file.open(f, ios::in);
	//cout <<"filenum: "<< f[-1]<<endl;
	int i = 0;
	while (getline(file, k))
	{
		k = str_replace(k, "$", "");
		k = str_replace(k, ")", "");
		k = str_replace(k, ",", " ");
		k = str_replace(k, "(", " ");
		//lw 2  8 0  add 6  4  5
		int first = k.find(" ");
		instructions[i][0] = k.substr(0, first);
		k = k.substr(first + 1);
		first = k.find(" ");
		instructions[i][1] = k.substr(0, first);
		k = k.substr(first + 2);
		first = k.find(" ");
		instructions[i][2] = k.substr(0, first);
		instructions[i][3] = k.substr(first, k.size() - first);

		i++;
	}

	// start the cycle
	cycleNum = 1;
	in = 0;
	total = i;//有幾個指令
	do_stall = false;
	if_beq = false;
	cycle();

	file.close();
}

void Pipeline::cycle() {
	int first, second, third, current, d, s, s1;
	stringstream ss;
	fstream write;
	write.open("result.txt", ios::out);

	while (!(pipeline["IF"] == -1 && pipeline["ID"] == -1 && pipeline["EX"] == -1 && pipeline["MEM"] == -1 && pipeline["WB"] == -1) || !in)
	{
		// 找出上一cycle 各個位置的指令
		first = pipeline["EX"];
		second = pipeline["WB"];
		third = pipeline["MEM"];
		current = pipeline["ID"];

		// 還原 reg WB wb的d_reg 先還原
		if (second != -1) {
			if (instructions[second][0] == "add" || instructions[second][0] == "sub" || instructions[second][0] == "lw") {
				ss.str("");
				ss.clear();
				ss.str(instructions[second][1]);
				ss >> d;

				reg[d][1] = 0;//代表可以用
			}
		}
		// ex 的 reg 加入
		if (pipeline["EX"] != -1) {
			if (instructions[first][0] == "add" || instructions[first][0] == "sub" || instructions[first][0] == "lw") {
				ss.str("");
				ss.clear();
				ss.str(instructions[first][1]);
				ss >> d;

				reg[d][1] = 1;
			}
		}
		// mem 的 reg 加入
		if (pipeline["MEM"] != -1) {
			if (instructions[third][0] == "add" || instructions[third][0] == "sub" || instructions[third][0] == "lw") {
				ss.str("");
				ss.clear();
				ss.str(instructions[third][1]);
				ss >> d;

				reg[d][1] = 1;
			}
		}
		// 開始判斷這輪需不需要stall
		if (instructions[current][0] == "add" || instructions[current][0] == "sub" && current != -1) {
			ss.str("");
			ss.clear();
			ss.str(instructions[current][2]);
			ss >> s;
			ss.str("");
			ss.clear();
			ss.str(instructions[current][3]);
			ss >> s1;

			if (reg[s][1] == 1 || reg[s1][1] == 1) stall();
		}
		else if (instructions[current][0] == "lw" || instructions[current][0] == "sw" && current != -1) {
			ss.str("");
			ss.clear();
			ss.str(instructions[current][3]);
			ss >> s;
			ss.str("");
			ss.clear();
			ss.str(instructions[current][1]);
			ss >> s1;

			if (reg[s][1] == 1 || reg[s1][1] == 1) stall();
		}
		else if (current != -1)//beq
		{
			ss.str("");
			ss.clear();
			ss.str(instructions[current][1]);
			ss >> s;
			ss.str("");
			ss.clear();
			ss.str(instructions[current][2]);
			ss >> s1;

			if (reg[s][1] == 1 || reg[s1][1] == 1) stall();
		}
		// wb and mem 資料刷新
		pipeline["WB"] = pipeline["MEM"];
		pipeline["MEM"] = pipeline["EX"];
		// if id instruction == beq
		int a, b;
		ss.str("");
		ss.clear();
		ss.str(instructions[current][1]);
		ss >> a;
		ss.str("");
		ss.clear();
		ss.str(instructions[current][2]);
		ss >> b;

		if (instructions[current][0] == "beq" && reg[a][1] == 0 && reg[b][1] == 0) {
			if_beq = true;
		}
		// beq 的處理
		if (if_beq) {
			int a, b, c;
			ss.str("");
			ss.clear();
			ss.str(instructions[current][1]);
			ss >> a;
			ss.str("");
			ss.clear();
			ss.str(instructions[current][2]);
			ss >> b;

			if (reg[a][0] == reg[b][0])
			{
				ss.str("");
				ss.clear();
				ss.str(instructions[current][3]);
				ss >> c;
				pipeline["EX"] = current;
				pipeline["ID"] = -1;
				if (pipeline["EX"] + c + 1 < total) {
					pipeline["IF"] = pipeline["EX"] + c + 1;
					in = pipeline["IF"] + 1;
				}
				else {
					pipeline["IF"] = -1;
					in = total;
				}
			}
			else {
				if_beq = false;
			}
		}
		// stall 的處理
		if (!do_stall && !if_beq) {
			pipeline["EX"] = pipeline["ID"];
			pipeline["ID"] = pipeline["IF"];
			if (in < total) {
				pipeline["IF"] = in;
				in++;
			}
			else pipeline["IF"] = -1;
		}

		if (do_stall) pipeline["EX"] = -1;

		if (!(pipeline["IF"] == -1 && pipeline["ID"] == -1 && pipeline["EX"] == -1 && pipeline["MEM"] == -1 && pipeline["WB"] == -1))
			write << "Cycle " << cycleNum << endl;
		if (pipeline["WB"] != -1) write << instructions[pipeline["WB"]][0] << ": WB " << signals[instructions[pipeline["WB"]][0]].substr(7) << endl;
		if (pipeline["MEM"] != -1) write << instructions[pipeline["MEM"]][0] << ": MEM " << signals[instructions[pipeline["MEM"]][0]].substr(3) << endl;
		if (pipeline["EX"] != -1) write << instructions[pipeline["EX"]][0] << ": EX " << signals[instructions[pipeline["EX"]][0]] << endl;
		if (pipeline["ID"] != -1) write << instructions[pipeline["ID"]][0] << ": ID" << endl;
		if (pipeline["IF"] != -1) write << instructions[pipeline["IF"]][0] << ": IF" << endl;

		cycleNum++;
		do_stall = false;
		if_beq = false;
		calculate(pipeline["WB"]);
	}

	write << "總共Cycle數 : " << cycleNum - 2 << endl;
	// write the register values
	for (int i = 0; i < 32; i++) {
		write << "$" << i << " ";
	}
	write << endl;
	for (int i = 0; i < 32; i++) {
		if (i < 10) write << " " << reg[i][0] << " ";
		else write << "  " << reg[i][0] << " ";
	}
	write << endl;
	// write mem values
	for (int i = 0; i < 32; i++) {
		write << "W" << i << " ";
	}
	write << endl;
	for (int i = 0; i < 32; i++) {
		if (i < 10) write << " " << mem[i] << " ";
		else write << "  " << mem[i] << " ";
	}

	write.close();
}

void Pipeline::stall() {
	do_stall = true;
}

void Pipeline::calculate(int c) {
	std::stringstream ss;
	int a, b, d;
	if (c < 0) return;
	if (instructions[c][0] == "add" || instructions[c][0] == "sub") {
		ss.str("");
		ss.clear();
		ss.str(instructions[c][2]);
		ss >> a;
		ss.str("");
		ss.clear();
		ss.str(instructions[c][3]);
		ss >> b;
		ss.str("");
		ss.clear();
		ss.str(instructions[c][1]);
		ss >> d;
		if (d != 0 && instructions[c][0] == "add") reg[d][0] = reg[a][0] + reg[b][0];
		if (d != 0 && instructions[c][0] == "sub") reg[d][0] = reg[a][0] - reg[b][0];
	}
	else if (instructions[c][0] == "lw" || instructions[c][0] == "sw") {
		ss.str("");
		ss.clear();
		ss.str(instructions[c][2]);
		ss >> a;
		ss.str("");
		ss.clear();
		ss.str(instructions[c][3]);
		ss >> b;
		ss.str("");
		ss.clear();
		ss.str(instructions[c][1]);
		ss >> d;
		if (instructions[c][0] == "lw") {
			if (d != 0) reg[d][0] = mem[b + a / 4];
		}
		if (instructions[c][0] == "sw") {
			if (d != 0) mem[b + a / 4] = reg[d][0];
		}
	}
}

