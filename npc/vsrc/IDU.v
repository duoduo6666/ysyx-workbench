module ysyx_2070017_IDU(
    input inst, // Instruction
    output [6:0] opcode,
    output [4:0] rd,
    output [4:0] rs1,
    output [4:0] rs2,
    output [2:0] funct3,
    output [6:0] funct7,
    output [ysyx_24070017_WORD_TYPE] immI,
    output [ysyx_24070017_WORD_TYPE] immS,
    output [ysyx_24070017_WORD_TYPE] immB,
    output [ysyx_24070017_WORD_TYPE] immU,
    output [ysyx_24070017_WORD_TYPE] immJ,
)
assign rd = inst[11:7];
assign rs1 = inst[19:15];
assign rs2 = inst[24:20];

assign funct3 = inst[14:12];
assign funct7 = inst[31:25];

assign immI[31:11] = {21{inst[31]}};
assign immI[10:0] = inst[30:20];
assign immS[31:11] = {21{inst[31]}};
assign immS[11:0] = {inst[30:25], inst[11:7]};
assign immB[31:12] = {20{inst[31]}};
assign immB[11:0] = {inst[7], inst[30:25], inst[11:8], 1'b0};
assign immU[11:0] = {12{1'b0}};
assign immU[31:12] = inst[31:12];
assign immJ[31:20] = {12{inst[31]}};
assign immJ[19:0] = {inst[19:12], inst[20], inst[30:21], 1'b0};

endmodule