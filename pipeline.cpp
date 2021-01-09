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

Pipeline::Pipeline(string f) //string f為input file檔名
{
	string k;

	// ==========================初始化==========================
	//reg and mem
	for (int i = 0; i < 32; i++) {
		reg[i][0] = 1;//給老師看結果的暫存器，$0為0，其餘為1
		reg[i][1] = 0;
		mem[i] = 1;//32word的記憶為1
	}
	reg[0][0] = 0;

	//temp_reg
	for (int temp = 0; temp < 3; temp++) {
		ID_EXE[temp] = -1;
		EXE_MEM[temp] = -1;
		MEM_WB[temp] = -1;
	}//初始化stage間的暫存器，其值為暫存器編號，初始值為-1

	// signals 
	//第一個string是key，第二個string是mapping value	
	signals.insert(pair<string, string>("lw", "01 010 11"));
	signals.insert(pair<string, string>("sw", "X1 001 0X"));
	signals.insert(pair<string, string>("beq", "X0 100 0X"));
	signals.insert(pair<string, string>("add", "10 000 10"));//R type
	signals.insert(pair<string, string>("sub", "10 000 10"));//R type

	// pipeline
	//map<string,int>,string=key,int=value
	pipeline["IF"] = -1;
	pipeline["ID"] = -1;
	pipeline["EX"] = -1;
	pipeline["MEM"] = -1;
	pipeline["WB"] = -1;

	//讀檔&字串處理
	file.open(f, ios::in);
	//cout <<"filenum: "<< f[-1]<<endl;
	int i = 0;
	while (getline(file, k))//一句句讀
	{
		k = str_replace(k, "$", "");
		k = str_replace(k, ")", "");
		k = str_replace(k, ",", " ");
		k = str_replace(k, "(", " ");
		//lw 2  8 0  add 6  4  5
		int first = k.find(" ");
		instructions[i][0] = k.substr(0, first);//instructions[i][0]=lw
		k = k.substr(first + 1);
		first = k.find(" ");
		instructions[i][1] = k.substr(0, first);//ins[i][1]=2
		k = k.substr(first + 2);
		first = k.find(" ");
		instructions[i][2] = k.substr(0, first);//ins[i][2]=8
		instructions[i][3] = k.substr(first, k.size() - first);//ins[i][3]=0

		i++;
	}
	//先取得所有指令，放進insturctions陣列中

	// start the cycle
	cycleNum = 1;//第幾個cycle
	in = 0;
	total = i;//指令總數
	do_stall = false;
	if_beq = false;
	beq_stall = false;
	cycle();

	do_forwarding = false;

	file.close();//關檔
}

void Pipeline::cycle() {
	int first, second, third, current, next, d, s, s1;//s, s1 are operands
	stringstream ss;
	fstream write;
	write.open("result.txt", ios::out);//寫檔，將結果寫在result.txt中

	//pipeline皆為-1且in=1時結束while迴圈
	//剛進來會是pipeline all=(-1)&&in=0
	while (!(pipeline["IF"] == -1 && pipeline["ID"] == -1 && pipeline["EX"] == -1 && pipeline["MEM"] == -1 && pipeline["WB"] == -1) || !in)
	{
		// 找出上一cycle 各個位置的指令
		//pipeline[]=第幾條指令
		first = pipeline["EX"];
		second = pipeline["WB"];
		third = pipeline["MEM"];
		current = pipeline["ID"];


		//set temp regsiter
		for (int t = 0; t < 3; t++) {
			//set ID/EXE reg
			ss.str("");//清空ss
			ss.clear();//重設ss的internal error state flags
			ss.str(instructions[current][t + 1]);
			ss >> ID_EXE[t];
			//set EXE/MEM reg
			ss.str("");
			ss.clear();
			ss.str(instructions[first][t + 1]);
			ss >> EXE_MEM[t];
			//set MEM/WB reg
			ss.str("");
			ss.clear();
			ss.str(instructions[third][t + 1]);
			ss >> MEM_WB[t];
		}
		//finish set temp

		//這裡在設定那些register不能動
		// 還原 reg WB wb的d_reg 先還原
		if (second != -1) {//WB stage中有指令，判斷是add、sub或lw(五個指令中只有這三個會需要將值存回暫存器)
			if (instructions[second][0] == "add" || instructions[second][0] == "sub" || instructions[second][0] == "lw") {
				ss.str("");//清空ss
				ss.clear();//重設ss的internal error state flags
				ss.str(instructions[second][1]);//ins[][0]=指令,ins[][1]=rs,ins[][2]=rt,ins[][3]=rd
				ss >> d;//d=rs

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

		//知道哪些暫存器不能用後，開始看有沒有撞車
		// 開始判斷這輪需不需要stall
		//cout<< pipeline["IF"];
		//system("pause");


		if (instructions[current][0] == "add" || instructions[current][0] == "sub" && current != -1) {// ID stage
			ss.str("");
			ss.clear();
			ss.str(instructions[current][2]);
			ss >> s;//s=rt
			ss.str("");
			ss.clear();
			ss.str(instructions[current][3]);
			ss >> s1;//s1=rd

			//有source撞車
			//s,s1為暫存器編號
			if (reg[s][1] == 1 || reg[s1][1] == 1) {
				judge_forwarding(s, s1, first, third, current);
				if (do_forwarding == false) {//不能forwarding，只能stall了
					stall();
				}
			}
		}
		else if (instructions[current][0] == "lw" || instructions[current][0] == "sw" && current != -1) {
			ss.str("");
			ss.clear();
			ss.str(instructions[current][3]);
			ss >> s;//s=rd
			ss.str("");
			ss.clear();
			ss.str(instructions[current][1]);
			ss >> s1;//s1=rs

			if (reg[s][1] == 1 || reg[s1][1] == 1) {
				judge_forwarding(s, s1, first, third, current);

				if (do_forwarding == false) {//不能forwarding，只能stall了
					stall();
				}
			}
		}

		/*next = pipeline["IF"];
		if (next != -1) {//裡面有值
			if (instructions[next][0] == "beq") {//判斷他下一個是beq
				int now_reg;

				ss.str("");
				ss.clear();
				ss.str(instructions[current][1]);//目的地的暫存器
				ss >> now_reg;


				ss.str("");
				ss.clear();
				ss.str(instructions[next][1]);
				ss >> s;//s=rs
				ss.str("");
				ss.clear();
				ss.str(instructions[next][2]);
				ss >> s1;//s1=rt
				if (now_reg == s || now_reg == s1) {//撞到
					beq_stall = true;
				}
				if (instructions[current][0] == "add" || instructions[current][0] == "sub") {//如果他是add或是sub 就可以在ex完就拿到
					if (pipeline["EX"] != -1) {
						forwarding_add_sub(current);
						beq_stall = false;
					}
				}
				else if (instructions[current][0] == "sw") {//如果他是lw或是sw 要在mem後才拿的到
					if (pipeline["MEM"] != -1) {
						forwarding_sw(current);
						beq_stall = false;
					}
				}
				else if (instructions[current][0] == "lw") {//如果他是lw或是sw 要在mem後才拿的到
					if (pipeline["MEM"] != -1) {
						forwarding_lw(current);
						beq_stall = false;
					}
				}
			}
		}*/
		else if (current != -1)//beq
		{
			ss.str("");
			ss.clear();
			ss.str(instructions[current][1]);
			ss >> s;//s=rs
			ss.str("");
			ss.clear();
			ss.str(instructions[current][2]);
			ss >> s1;//s1=rt

			if (reg[s][1] == 1 || reg[s1][1] == 1) {
				if (instructions[current][0] == "beq" && (instructions[third][0] == "lw" || instructions[third][0] == "sw")) {//現在在執行beq 但是lw需要等到它MEM/WB 所以多等一次
					stall();
				}//不需要判斷要不要forwarding 一定要等到lw的WB回來給它

				else {
					judge_forwarding(s, s1, first, third, current);
					if (do_forwarding == false) {//不能forwarding，只能stall了
						stall();
					}
				}
			}
		}
		// wb and mem 資料刷新
		pipeline["WB"] = pipeline["MEM"];//第i條指令從mem階段進入wb階段
		pipeline["MEM"] = pipeline["EX"];//第i+1條指令從ex階段進入mem階段
		// if id instruction == beq

		int a, b;
		ss.str("");
		ss.clear();
		ss.str(instructions[current][1]);//current=ID stage，所以instructions[current][1]代表在IDstage這條指令的rs暫存器編號
		ss >> a;//rs
		ss.str("");
		ss.clear();
		ss.str(instructions[current][2]);
		ss >> b;//rt


		if (instructions[current][0] == "beq" && reg[a][1] == 0 && reg[b][1] == 0) {
			if_beq = true;//可以跳了
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

			if (reg[a][0] == reg[b][0])//若兩個暫存器值相同
			{
				ss.str("");
				ss.clear();
				ss.str(instructions[current][3]);//beg指令中，ins[3]為要跳過去的label
				ss >> c;
				pipeline["EX"] = current;//id中的指令跑到下一階段
				pipeline["ID"] = -1;//所以id階段現在沒有指令了
				if (pipeline["EX"] + c + 1 < total) {//=目前的指令編號(第幾個)+label+1如果小於總指令數，pipeline+1=$pc+4=沒有跳的話下一個指令的位址，c=lable，
					pipeline["IF"] = pipeline["EX"] + c + 1;//pipeline[IF]=跳轉過去的指令編號(第幾個)
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
		if (!do_stall && !if_beq) {//不需要stall也不需要beq
			pipeline["EX"] = pipeline["ID"];//往下執行
			pipeline["ID"] = pipeline["IF"];//往下執行
			if (!beq_stall) {//不能抓下一個
				if (in < total) {//如果還沒到底
					pipeline["IF"] = in;//把下一個抓進來
					in++;
				}
				else pipeline["IF"] = -1;
			}
		}

		//if (beq_stall) pipeline["ID"] = -1;

		if (do_stall) pipeline["EX"] = -1;

		if (!(pipeline["IF"] == -1 && pipeline["ID"] == -1 && pipeline["EX"] == -1 && pipeline["MEM"] == -1 && pipeline["WB"] == -1))//有任一階段中有指令再進行處理
			write << "Cycle " << cycleNum << endl;
		if (pipeline["WB"] != -1) write << instructions[pipeline["WB"]][0] << ": WB " << signals[instructions[pipeline["WB"]][0]].substr(7) << endl;
		if (pipeline["MEM"] != -1) write << instructions[pipeline["MEM"]][0] << ": MEM " << signals[instructions[pipeline["MEM"]][0]].substr(3) << endl;
		if (pipeline["EX"] != -1) write << instructions[pipeline["EX"]][0] << ": EX " << signals[instructions[pipeline["EX"]][0]] << endl;
		if (pipeline["ID"] != -1) write << instructions[pipeline["ID"]][0] << ": ID" << endl;
		if (pipeline["IF"] != -1) write << instructions[pipeline["IF"]][0] << ": IF" << endl;

		cycleNum++;
		do_stall = false;
		if_beq = false;
		do_forwarding = false;
		beq_stall = false;
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

void Pipeline::calculate(int c) {//c是WB階段的指令編號(第幾個)
	std::stringstream ss;
	int a, b, d;
	if (c < 0) return;//WB中沒有指令存在
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
		if (d != 0 && instructions[c][0] == "add") reg[d][0] = reg[a][0] + reg[b][0];//d不是$0
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
			if (d != 0)
				reg[d][0] = mem[b + a / 4];
		}
		if (instructions[c][0] == "sw") {
			if (d != 0) mem[b + a / 4] = reg[d][0];
		}
	}
}


//forwarding
void Pipeline::forwarding_add_sub(int ins) {
	if (instructions[ins][0] == "add") {
		reg[EXE_MEM[0]][0] = reg[EXE_MEM[1]][0] + reg[EXE_MEM[2]][0];
	}
	else if (instructions[ins][0] == "sub") {
		reg[EXE_MEM[0]][0] = reg[EXE_MEM[1]][0] - reg[EXE_MEM[2]][0];
	}
}

void Pipeline::forwarding_add_sub_mem(int ins) {
	if (instructions[ins][0] == "add") {
		reg[EXE_MEM[0]][0] = reg[EXE_MEM[1]][0] + reg[EXE_MEM[2]][0];
	}
	else if (instructions[ins][0] == "sub") {
		reg[EXE_MEM[0]][0] = reg[EXE_MEM[1]][0] - reg[EXE_MEM[2]][0];
	}
}

void Pipeline::forwarding_sw(int ins) {
	mem[MEM_WB[2] + MEM_WB[1] / 4] = reg[MEM_WB[0]][0];
}

void Pipeline::forwarding_lw(int ins) {
	reg[MEM_WB[0]][0] = mem[MEM_WB[2] + MEM_WB[1] / 4];
}


void Pipeline::judge_forwarding(int s, int s1, int first, int third, int current) {
	//EX hazard
	//first是EX
	//third是MEM
	//current是ID


	bool cannot_forward = false;
	if ((instructions[first][0] == "add" || instructions[first][0] == "sub") && instructions[current][0] != "beq") {//add跟sub在ex完就可以可以拿到
		if (EXE_MEM[0] != 0 && (EXE_MEM[0] == s || EXE_MEM[0] == s1)) {
			do_forwarding = true;
			forwarding_add_sub(first);
		}
	}//r format
	else if (instructions[first][0] == "lw") {
		if (EXE_MEM[0] != 0 && (EXE_MEM[0] == s || EXE_MEM[0] == s1)) {//lw跟sw在ex的時候還拿不到
			cannot_forward = true;
		}
	}
	else if (instructions[first][0] == "sw") {
		if (EXE_MEM[2] == s || EXE_MEM[2] == s1) {
			cannot_forward = true;
		}
	}

	//MEM hazard
	if (!cannot_forward) {//判斷MEM可不可以forwarding

		if (instructions[third][0] == "add" || instructions[third][0] == "sub") {
			if (MEM_WB[0] != 0 && (MEM_WB[0] == s || MEM_WB[0] == s1)) {
				do_forwarding = true;
				forwarding_add_sub_mem(third);
			}
		}

		else if (instructions[third][0] == "lw") {
			if (MEM_WB[0] != 0 && (MEM_WB[0] == s || MEM_WB[0] == s1)) {
				do_forwarding = true;
				forwarding_lw(third);
			}
		}
		else if (instructions[third][0] == "sw") {
			if (MEM_WB[2] == s || MEM_WB[2] == s1) {
				//(MEM_WB[MEM_WB[2] + MEM_WB[1] / 4] != 0 &&
				//(MEM_WB[MEM_WB[2] + MEM_WB[1] / 4] == s || MEM_WB[MEM_WB[2] + MEM_WB[1] / 4] == s1)) {
				do_forwarding = true;
				forwarding_sw(third);
			}
		}
	}
}