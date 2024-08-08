module ysyx_2070017_CPU(
    input clk,
    input rst,
    input inst, // Instruction
	output [ysyx_24070017_WORD_TYPE] pc,
);
initial begin
	if ($test$plusargs("trace") != 0) begin
		$dumpfile("cpu.fst");
		$dumpvars();
	end
end

// wire [ysyx_24070017_WORD_TYPE] pc;
wire [ysyx_24070017_WORD_TYPE] snpc; // static next PC
assign snpc = pc + 4;
wire [ysyx_24070017_WORD_TYPE] dnpc; // dynamic next PC 
assign dnpc = snpc;
ysyx_24070017_Reg #(ysyx_24070017_WORD_LENGTH, 32'h80000000) pc_r (
	.clk(clk),
	.rst(rsy),
	.wen(1'b1),
	.din(dnpc),
	.dout(pc).
);

wire [6:0] opcode;
wire [4:0] rd, rs1, rs2;
wire [2:0] funct3;
wire [6:0] funct7;
wire [ysyx_24070017_WORD_TYPE] immI. immS, immB, immU, immJ;
ysyx_2070017_IDU idu(
	.inst(inst),
	.opcode(opcode)
    .rd(rd),
    .rs1(rs1),
    .rs2(rs1),
    .funct3(funct3),
    .funct7(funct7),
    .immI,
    .immS,
    .immB,
    .immU,
    .immJ,
)

// 可以尝试使用逻辑表达式或其他方式实现译码器, 对比逻辑门使用量
wire [ysyx_24070017_RF_REG_NUM-1:0] rfwe; // Reg File Write Enable
wire [ysyx_24070017_RF_REG_NUM-1:0] rd_decode;
wire need_rd;
assign rd_decode = ysyx_24070017_RF_REG_NUM'b1 << rd; // decoder
assign need_rd = (
	(opcode == 7'b0110111) |
	(opcode == 7'b0010011)
)
assign rfwe = rd_decode & {ysyx_24070017_RF_REG_NUM{need_rd}}

wire [ysyx_24070017_RF_REG_NUM * ysyx_24070017_WORD_LENGTH - 1:0] rf_data;
wire [ysyx_24070017_WORD_TYPE] write_data, rs1_data, rs2_data;
ysyx_24070017_RF rf(
	.clk(clk),
	.rst(rst),
	.we(rfwe)
	.wdata({ysyx_24070017_RF_REG_NUM{write_data}}),
	.rdata(rf_data),
)
assign rs1_data = rf_data[(rs1+1)*ysyx_24070017_WORD_TYPE-1:rs1*ysyx_24070017_WORD_TYPE];
assign rs2_data = rf_data[(rs2+1)*ysyx_24070017_WORD_TYPE-1:rs2*ysyx_24070017_WORD_TYPE];

wire is_op_imm;
assign is_op_imm = opcode == 7'b0010011;
wire [ysyx_24070017_WORD_TYPE] result;
ysyx_2070017_ALU alu(
	.opcode(opcode),
	.funct3(funct3),
	.funct7(funct7),
	.src1(rs1_data),
	.src2((rs2_data&{ysyx_24070017_WORD_LENGTH{!is_op_imm}}) | (immI&{ysyx_24070017_WORD_LENGTH{is_op_imm}}))
	.result(result),
)
assign write_data = result;

endmodule
