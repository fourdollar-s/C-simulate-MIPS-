# C-simulate-MIPS-
學校課程期末專題，用C++模擬MIPS CPU

請設計一個pipelined的cpu模擬器
(可以任何語言來模擬 ex:C & C++)
第一部分 ( 70% ) : 用Stall來解決所有的hazard問題，換言之，需要實作hazard的邏輯判斷
第二部分 ( 30% ) : 用Forwarding來解決部分的hazard問題

Input 為一名為memory.txt的文字檔，裡面為MIPS的組合語言程式
Output 請輸出此程式執行結果於一名為result.txt的文字檔

Instructions
◼ lw
◼ sw
◼ add
◼ sub
◼ beq

Register Number
◼ 32 registers

Memory Size
◼ 32 words

Initial Values of Memory and Register
◼ 記憶體中的每個word都是1
◼ $0暫存器的值為0，其他都是1

Output Requirements
◼ 不同clock cycle時，在CPU中執行的指令狀態(顯示各指令在ID、EX、MEM、WB階段即將與未使用的signal值)
◼ 最後顯示
執行該段指令需要多少cycle
記憶體與暫存器執行後的結果
singal輸出順序:
OPcode Stage RegDst ALUSrc Branch MemRead MemWrite RegWrite MemtoReg
lw EX 0 1 0 1 0 1 1

Output範例：
Cycle 1
lw: IF
Cycle 2
lw: ID
lw: IF
Cycle 3
lw: EX 01 010 11
lw: ID
add: IF
Cycle 4
lw: MEM 010 11
lw: EX 01 010 11
add: ID
sw: IF
Cycle 5
lw: WB 11
lw: MEM 010 11
add: ID
sw: IF
Cycle 6
lw: WB 11
add: ID
sw: IF
Cycle 7
add: EX 10 000 10
sw: ID
Cycle 8
add: MEM 000 10
sw: ID
Cycle 9
add: WB 10
sw: ID
Cycle 10
sw: EX X1 001 0X
Cycle 11
sw: MEM 001 0X
Cycle 12
sw: WB 0X
$0 $1 $2 $3 $4 $5 $6 ….
0 1 1 1 2 1 1 ….
W0 W1 W2 W3 W4 W5 W6…
1 1 1 1 1 1 2 …. 
