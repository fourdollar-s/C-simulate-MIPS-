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

Pipeline::Pipeline(string f) //string f��input file�ɦW
{
	string k;

	// ==========================��l��==========================
	//reg and mem
	for (int i = 0; i < 32; i++) {
		reg[i][0] = 1;//���Ѯv�ݵ��G���Ȧs���A$0��0�A��l��1
		reg[i][1] = 0;
		mem[i] = 1;//32word���O�Ь�1
	}
	reg[0][0] = 0;

	//temp_reg
	for (int temp = 0; temp < 3; temp++) {
		ID_EXE[temp] = -1;
		EXE_MEM[temp] = -1;
		MEM_WB[temp] = -1;
	}//��l��stage�����Ȧs���A��Ȭ��Ȧs���s���A��l�Ȭ�-1

	// signals 
	//�Ĥ@��string�Okey�A�ĤG��string�Omapping value	
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

	//Ū��&�r��B�z
	file.open(f, ios::in);
	//cout <<"filenum: "<< f[-1]<<endl;
	int i = 0;
	while (getline(file, k))//�@�y�yŪ
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
	//�����o�Ҧ����O�A��iinsturctions�}�C��

	// start the cycle
	cycleNum = 1;//�ĴX��cycle
	in = 0;
	total = i;//���O�`��
	do_stall = false;
	if_beq = false;
	beq_stall = false;
	cycle();

	do_forwarding = false;

	file.close();//����
}

void Pipeline::cycle() {
	int first, second, third, current, next, d, s, s1;//s, s1 are operands
	stringstream ss;
	fstream write;
	write.open("result.txt", ios::out);//�g�ɡA�N���G�g�bresult.txt��

	//pipeline�Ҭ�-1�Bin=1�ɵ���while�j��
	//��i�ӷ|�Opipeline all=(-1)&&in=0
	while (!(pipeline["IF"] == -1 && pipeline["ID"] == -1 && pipeline["EX"] == -1 && pipeline["MEM"] == -1 && pipeline["WB"] == -1) || !in)
	{
		// ��X�W�@cycle �U�Ӧ�m�����O
		//pipeline[]=�ĴX�����O
		first = pipeline["EX"];
		second = pipeline["WB"];
		third = pipeline["MEM"];
		current = pipeline["ID"];


		//set temp regsiter
		for (int t = 0; t < 3; t++) {
			//set ID/EXE reg
			ss.str("");//�M��ss
			ss.clear();//���]ss��internal error state flags
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

		//�o�̦b�]�w����register�����
		// �٭� reg WB wb��d_reg ���٭�
		if (second != -1) {//WB stage�������O�A�P�_�Oadd�Bsub��lw(���ӫ��O���u���o�T�ӷ|�ݭn�N�Ȧs�^�Ȧs��)
			if (instructions[second][0] == "add" || instructions[second][0] == "sub" || instructions[second][0] == "lw") {
				ss.str("");//�M��ss
				ss.clear();//���]ss��internal error state flags
				ss.str(instructions[second][1]);//ins[][0]=���O,ins[][1]=rs,ins[][2]=rt,ins[][3]=rd
				ss >> d;//d=rs

				reg[d][1] = 0;//�N��i�H��
			}
		}
		// ex �� reg �[�J
		if (pipeline["EX"] != -1) {
			if (instructions[first][0] == "add" || instructions[first][0] == "sub" || instructions[first][0] == "lw") {
				ss.str("");
				ss.clear();
				ss.str(instructions[first][1]);
				ss >> d;

				reg[d][1] = 1;
			}
		}
		// mem �� reg �[�J
		if (pipeline["MEM"] != -1) {
			if (instructions[third][0] == "add" || instructions[third][0] == "sub" || instructions[third][0] == "lw") {
				ss.str("");
				ss.clear();
				ss.str(instructions[third][1]);
				ss >> d;

				reg[d][1] = 1;
			}
		}

		//���D���ǼȦs������Ϋ�A�}�l�ݦ��S������
		// �}�l�P�_�o���ݤ��ݭnstall
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

			//��source����
			//s,s1���Ȧs���s��
			if (reg[s][1] == 1 || reg[s1][1] == 1) {
				judge_forwarding(s, s1, first, third, current);
				if (do_forwarding == false) {//����forwarding�A�u��stall�F
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

				if (do_forwarding == false) {//����forwarding�A�u��stall�F
					stall();
				}
			}
		}

		/*next = pipeline["IF"];
		if (next != -1) {//�̭�����
			if (instructions[next][0] == "beq") {//�P�_�L�U�@�ӬObeq
				int now_reg;

				ss.str("");
				ss.clear();
				ss.str(instructions[current][1]);//�ت��a���Ȧs��
				ss >> now_reg;


				ss.str("");
				ss.clear();
				ss.str(instructions[next][1]);
				ss >> s;//s=rs
				ss.str("");
				ss.clear();
				ss.str(instructions[next][2]);
				ss >> s1;//s1=rt
				if (now_reg == s || now_reg == s1) {//����
					beq_stall = true;
				}
				if (instructions[current][0] == "add" || instructions[current][0] == "sub") {//�p�G�L�Oadd�άOsub �N�i�H�bex���N����
					if (pipeline["EX"] != -1) {
						forwarding_add_sub(current);
						beq_stall = false;
					}
				}
				else if (instructions[current][0] == "sw") {//�p�G�L�Olw�άOsw �n�bmem��~������
					if (pipeline["MEM"] != -1) {
						forwarding_sw(current);
						beq_stall = false;
					}
				}
				else if (instructions[current][0] == "lw") {//�p�G�L�Olw�άOsw �n�bmem��~������
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
				if (instructions[current][0] == "beq" && (instructions[third][0] == "lw" || instructions[third][0] == "sw")) {//�{�b�b����beq ���Olw�ݭn���쥦MEM/WB �ҥH�h���@��
					stall();
				}//���ݭn�P�_�n���nforwarding �@�w�n����lw��WB�^�ӵ���

				else {
					judge_forwarding(s, s1, first, third, current);
					if (do_forwarding == false) {//����forwarding�A�u��stall�F
						stall();
					}
				}
			}
		}
		// wb and mem ��ƨ�s
		pipeline["WB"] = pipeline["MEM"];//��i�����O�qmem���q�i�Jwb���q
		pipeline["MEM"] = pipeline["EX"];//��i+1�����O�qex���q�i�Jmem���q
		// if id instruction == beq

		int a, b;
		ss.str("");
		ss.clear();
		ss.str(instructions[current][1]);//current=ID stage�A�ҥHinstructions[current][1]�N��bIDstage�o�����O��rs�Ȧs���s��
		ss >> a;//rs
		ss.str("");
		ss.clear();
		ss.str(instructions[current][2]);
		ss >> b;//rt


		if (instructions[current][0] == "beq" && reg[a][1] == 0 && reg[b][1] == 0) {
			if_beq = true;//�i�H���F
		}
		// beq ���B�z
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

			if (reg[a][0] == reg[b][0])//�Y��ӼȦs���ȬۦP
			{
				ss.str("");
				ss.clear();
				ss.str(instructions[current][3]);//beg���O���Ains[3]���n���L�h��label
				ss >> c;
				pipeline["EX"] = current;//id�������O�]��U�@���q
				pipeline["ID"] = -1;//�ҥHid���q�{�b�S�����O�F
				if (pipeline["EX"] + c + 1 < total) {//=�ثe�����O�s��(�ĴX��)+label+1�p�G�p���`���O�ơApipeline+1=$pc+4=�S�������ܤU�@�ӫ��O����}�Ac=lable�A
					pipeline["IF"] = pipeline["EX"] + c + 1;//pipeline[IF]=����L�h�����O�s��(�ĴX��)
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


		// stall ���B�z
		if (!do_stall && !if_beq) {//���ݭnstall�]���ݭnbeq
			pipeline["EX"] = pipeline["ID"];//���U����
			pipeline["ID"] = pipeline["IF"];//���U����
			if (!beq_stall) {//�����U�@��
				if (in < total) {//�p�G�٨S�쩳
					pipeline["IF"] = in;//��U�@�ӧ�i��
					in++;
				}
				else pipeline["IF"] = -1;
			}
		}

		//if (beq_stall) pipeline["ID"] = -1;

		if (do_stall) pipeline["EX"] = -1;

		if (!(pipeline["IF"] == -1 && pipeline["ID"] == -1 && pipeline["EX"] == -1 && pipeline["MEM"] == -1 && pipeline["WB"] == -1))//�����@���q�������O�A�i��B�z
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

	write << "�`�@Cycle�� : " << cycleNum - 2 << endl;
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

void Pipeline::calculate(int c) {//c�OWB���q�����O�s��(�ĴX��)
	std::stringstream ss;
	int a, b, d;
	if (c < 0) return;//WB���S�����O�s�b
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
		if (d != 0 && instructions[c][0] == "add") reg[d][0] = reg[a][0] + reg[b][0];//d���O$0
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
	//first�OEX
	//third�OMEM
	//current�OID


	bool cannot_forward = false;
	if ((instructions[first][0] == "add" || instructions[first][0] == "sub") && instructions[current][0] != "beq") {//add��sub�bex���N�i�H�i�H����
		if (EXE_MEM[0] != 0 && (EXE_MEM[0] == s || EXE_MEM[0] == s1)) {
			do_forwarding = true;
			forwarding_add_sub(first);
		}
	}//r format
	else if (instructions[first][0] == "lw") {
		if (EXE_MEM[0] != 0 && (EXE_MEM[0] == s || EXE_MEM[0] == s1)) {//lw��sw�bex���ɭ��ٮ�����
			cannot_forward = true;
		}
	}
	else if (instructions[first][0] == "sw") {
		if (EXE_MEM[2] == s || EXE_MEM[2] == s1) {
			cannot_forward = true;
		}
	}

	//MEM hazard
	if (!cannot_forward) {//�P�_MEM�i���i�Hforwarding

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