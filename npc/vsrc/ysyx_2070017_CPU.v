`ifdef VERILATOR
import "DPI-C" function void set_exit_status(input int status);
import "DPI-C" function int pmem_read(input int raddr);
import "DPI-C" function void pmem_write(input int waddr, input int wdata, input byte wmask);
`endif

module ysyx_2070017_CPU(
    input clk,
    input rst,
    input [31:0] inst, // Instruction
	output [`ysyx_24070017_WORD_TYPE] pc
);

`ifdef VERILATOR

`ifdef ysyx_2070017_VERILATOR_TRACE
initial begin
	$dumpfile("cpu.fst");
	$dumpvars();
end
`endif

reg [`ysyx_24070017_WORD_TYPE] memory_read;
wire [`ysyx_24070017_WORD_TYPE] memory_write;
always @(*) begin
	if (inst == 32'h00100073) set_exit_status(rf_data[(10+1)*`ysyx_24070017_WORD_LENGTH-1:10*`ysyx_24070017_WORD_LENGTH]); // ebreak
end
always @(*) begin
	if (opcode == `ysyx_24070017_OPCODE_LOAD) begin 
		memory_read = pmem_read(alu_result);
	end else begin
		memory_read = 0;
	end
	if (opcode == `ysyx_24070017_OPCODE_STORE && funct3 == 3'b000) pmem_write(alu_result, rs2_data, 8'b0001);
	if (opcode == `ysyx_24070017_OPCODE_STORE && funct3 == 3'b001) pmem_write(alu_result, rs2_data, 8'b0011);
	if (opcode == `ysyx_24070017_OPCODE_STORE && funct3 == 3'b010) pmem_write(alu_result, rs2_data, 8'b1111);
end
`endif

wire [`ysyx_24070017_WORD_TYPE] load;
ysyx_24070017_MuxKey #(5, 3, `ysyx_24070017_WORD_LENGTH) load_mux (load, funct3, {
	3'b000, {{`ysyx_24070017_WORD_LENGTH-8{memory_read[7]}}, memory_read[7:0]},
	3'b001, {{`ysyx_24070017_WORD_LENGTH-16{memory_read[15]}}, memory_read[15:0]},
	3'b010, memory_read,
	3'b100, {{`ysyx_24070017_WORD_LENGTH-8{1'b0}}, memory_read[7:0]},
	3'b101, {{`ysyx_24070017_WORD_LENGTH-16{1'b0}}, memory_read[15:0]}
});


// wire [`ysyx_24070017_WORD_TYPE] pc;
wire [`ysyx_24070017_WORD_TYPE] snpc; // static next PC
assign snpc = pc + 4;
wire [`ysyx_24070017_WORD_TYPE] dnpc; // dynamic next PC 

wire branch;
wire signed [`ysyx_24070017_WORD_TYPE] rs1_data_signed, rs2_data_signed;
assign rs1_data_signed = rs1_data;
assign rs2_data_signed = rs2_data;
ysyx_24070017_MuxKey #(6, 3, 1) branch_mux (branch, funct3, {
	3'b000,  rs1_data == rs2_data,
	3'b001,  rs1_data != rs2_data,
	3'b100,  rs1_data_signed < rs2_data_signed,
	3'b101,  rs1_data_signed >= rs2_data_signed,
	3'b110,  rs1_data < rs2_data,
	3'b111,  rs1_data >= rs2_data
});

wire is_jump_inst;
wire is_ecall, is_mret;
assign is_ecall = (inst == 32'h00000073);
assign is_mret = (inst == 32'h30200073);
assign is_jump_inst = (
	(opcode==`ysyx_24070017_OPCODE_BRANCH && branch) ||
	(opcode==`ysyx_24070017_OPCODE_JALR)             ||
	(opcode==`ysyx_24070017_OPCODE_JAL)
);

assign dnpc = (
	(snpc & {`ysyx_24070017_WORD_LENGTH{(!(is_jump_inst | is_ecall | is_mret))}})    | 
	(alu_result & {`ysyx_24070017_WORD_LENGTH{is_jump_inst}}) |
	(mtvec & {`ysyx_24070017_WORD_LENGTH{is_ecall}})          |
	(mepc & {`ysyx_24070017_WORD_LENGTH{is_mret}})
);

ysyx_24070017_Reg #(`ysyx_24070017_WORD_LENGTH, 32'h80000000) pc_reg (
	.clk(clk),
	.rst(rst),
	.wen(1'b1),
	.din(dnpc),
	.dout(pc)
);

wire [6:0] opcode;
wire [4:0] rd, rs1, rs2;
wire [2:0] funct3;
wire [6:0] funct7;
wire [`ysyx_24070017_WORD_TYPE] immI, immS, immB, immU, immJ;
ysyx_2070017_IDU idu(
	.inst(inst),
	.opcode(opcode),
    .rd(rd),
    .rs1(rs1),
    .rs2(rs2),
    .funct3(funct3),
    .funct7(funct7),
    .immI(immI),
    .immS(immS),
    .immB(immB),
    .immU(immU),
    .immJ(immJ)
);

// 可以尝试使用逻辑表达式或其他方式实现译码器, 对比逻辑门使用量
wire [`ysyx_24070017_RF_REG_NUM-1:0] rfwe; // Reg File Write Enable
wire [`ysyx_24070017_RF_REG_NUM-1:0] rd_decode;
wire need_rd;
assign rd_decode = 1'b1 << rd; // decoder
assign need_rd = (
	(opcode == `ysyx_24070017_OPCODE_AUIPC)  |
	(opcode == `ysyx_24070017_OPCODE_LUI)    |
	(opcode == `ysyx_24070017_OPCODE_JAL)    |
	(opcode == `ysyx_24070017_OPCODE_JALR)   |
	(opcode == `ysyx_24070017_OPCODE_LOAD)   |
	(opcode == `ysyx_24070017_OPCODE_OP_IMM) |
	(opcode == `ysyx_24070017_OPCODE_OP)     |
	((opcode == `ysyx_24070017_OPCODE_SYSTEM) && (funct3 != 3'b000))
);
assign rfwe = rd_decode & {`ysyx_24070017_RF_REG_NUM{need_rd}};

wire [`ysyx_24070017_RF_REG_NUM * `ysyx_24070017_WORD_LENGTH - 1:0] rf_data;
wire [`ysyx_24070017_WORD_TYPE] write_data, rs1_data, rs2_data;
ysyx_24070017_RF rf(
	.clk(clk),
	.rst(rst),
	.we(rfwe),
	.wdata({`ysyx_24070017_RF_REG_NUM{write_data}}),
	.rdata(rf_data)
);
wire [`ysyx_24070017_RF_REG_NUM * `ysyx_24070017_WORD_LENGTH - 1:0] rf_data, rs1_data_t, rs2_data_t;
assign rs1_data_t = rf_data>>(rs1*`ysyx_24070017_WORD_LENGTH);
assign rs2_data_t = rf_data>>(rs2*`ysyx_24070017_WORD_LENGTH);
assign rs1_data = rs1_data_t[`ysyx_24070017_WORD_TYPE];
assign rs2_data = rs2_data_t[`ysyx_24070017_WORD_TYPE];

ysyx_24070017_MuxKey #(8, 7, `ysyx_24070017_WORD_LENGTH) rd_mux (write_data, opcode, {
	`ysyx_24070017_OPCODE_LUI,    immU,
	`ysyx_24070017_OPCODE_AUIPC,  alu_result,
	`ysyx_24070017_OPCODE_JAL,    snpc,
	`ysyx_24070017_OPCODE_JALR,   snpc,
	`ysyx_24070017_OPCODE_OP_IMM, alu_result,
	`ysyx_24070017_OPCODE_OP,     alu_result,
	`ysyx_24070017_OPCODE_LOAD,   load,
	`ysyx_24070017_OPCODE_SYSTEM, csr_read
});

wire [`ysyx_24070017_WORD_TYPE] alu_src1, alu_src2, alu_result;

ysyx_24070017_MuxKey #(8, 7, `ysyx_24070017_WORD_LENGTH) alu_src1_mux (alu_src1, opcode, {
	`ysyx_24070017_OPCODE_AUIPC,  pc,
	`ysyx_24070017_OPCODE_JAL,    pc,
	`ysyx_24070017_OPCODE_JALR,   rs1_data,
	`ysyx_24070017_OPCODE_BRANCH, pc,
	`ysyx_24070017_OPCODE_OP_IMM, rs1_data,
	`ysyx_24070017_OPCODE_OP,     rs1_data,
	`ysyx_24070017_OPCODE_LOAD,   rs1_data,
	`ysyx_24070017_OPCODE_STORE,  rs1_data
});
ysyx_24070017_MuxKey #(8, 7, `ysyx_24070017_WORD_LENGTH) alu_src2_mux (alu_src2, opcode, {
	`ysyx_24070017_OPCODE_AUIPC,  immU,
	`ysyx_24070017_OPCODE_JAL,    immJ,
	`ysyx_24070017_OPCODE_JALR,   immI,
	`ysyx_24070017_OPCODE_BRANCH, immB,
	`ysyx_24070017_OPCODE_OP_IMM, immI,
	`ysyx_24070017_OPCODE_OP,     rs2_data,
	`ysyx_24070017_OPCODE_LOAD,   immI,
	`ysyx_24070017_OPCODE_STORE,  immS
});

ysyx_2070017_ALU alu(
	.opcode(opcode),
	.funct3(funct3 & {3{opcode==`ysyx_24070017_OPCODE_OP_IMM||opcode==`ysyx_24070017_OPCODE_OP}}), // 仅当opcode=OP-IMM或OP时才启用funct3，否则funct3是始终=0,alu将仅执行src1+src2
	.funct7(funct7),
	.src1(alu_src1),
	.src2(alu_src2),
	.result(alu_result)
);


wire [`ysyx_24070017_WORD_TYPE] csr_read, csr_write, csr_write_mask;
ysyx_24070017_MuxKey #(4, 12, `ysyx_24070017_WORD_LENGTH) csr_read_mux (csr_read, immI[11:0], {
	12'h300,  mstatus,
	12'h305,  mtvec,
	12'h341,  mepc,
	12'h342,  mcause
});
assign csr_write_mask = funct3[2] ? {{(`ysyx_24070017_WORD_LENGTH-5){1'b0}},rs1} : rs1_data;
ysyx_24070017_MuxKey #(3, 2, `ysyx_24070017_WORD_LENGTH) csr_write_mux (csr_write, funct3[1:0], {
	2'b01,  csr_write_mask,
	2'b10,  csr_read | (csr_write_mask),
	2'b11,  csr_read & (~csr_write_mask)
});

wire [`ysyx_24070017_WORD_TYPE] mstatus, mtvec, mepc, mcause;
ysyx_24070017_Reg #(`ysyx_24070017_WORD_LENGTH, 32'h00000000) mstatus_r (
	.clk(clk),
	.rst(rst),
	.wen(opcode==`ysyx_24070017_OPCODE_SYSTEM && immI[11:0] == 12'h300 && (funct3 != 3'b000)),
	.din(csr_write),
	.dout(mstatus)
);
ysyx_24070017_Reg #(`ysyx_24070017_WORD_LENGTH, 32'h00000000) mtvec_t (
	.clk(clk),
	.rst(rst),
	.wen(opcode==`ysyx_24070017_OPCODE_SYSTEM && immI[11:0] == 12'h305 && (funct3 != 3'b000)),
	.din(csr_write),
	.dout(mtvec)
);
ysyx_24070017_Reg #(`ysyx_24070017_WORD_LENGTH, 32'h00000000) mepc_r (
	.clk(clk),
	.rst(rst),
	.wen((opcode==`ysyx_24070017_OPCODE_SYSTEM && immI[11:0] == 12'h341 && (funct3 != 3'b000)) || is_ecall),
	.din(is_ecall ? pc : csr_write),
	.dout(mepc)
);
ysyx_24070017_Reg #(`ysyx_24070017_WORD_LENGTH, 32'h00000000) mcause_r (
	.clk(clk),
	.rst(rst),
	.wen((opcode==`ysyx_24070017_OPCODE_SYSTEM && immI[11:0] == 12'h342 && (funct3 != 3'b000)) || is_ecall),
	.din(is_ecall ? 32'hb : csr_write),
	.dout(mcause)
);

endmodule
