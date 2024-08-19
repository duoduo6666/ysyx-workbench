module ysyx_2070017_ALU(
    input [6:0] opcode,
    input [2:0] funct3,
    input [6:0] funct7,
    input [`ysyx_24070017_WORD_TYPE] src1,
    input [`ysyx_24070017_WORD_TYPE] src2,
    output [`ysyx_24070017_WORD_TYPE] result
);

// mode 0 默认
// mode 1 funct7=0100000 且 opcode=OP(0110011), 这时执行减法
wire add_mode;
assign add_mode = (funct7==7'b0100000) && (opcode==7'b0110011);

wire [`ysyx_24070017_WORD_TYPE] add;
assign add = (src1+((src2&{`ysyx_24070017_WORD_LENGTH{!add_mode}} ) | (-src2& {`ysyx_24070017_WORD_LENGTH{add_mode}})));

wire [`ysyx_24070017_WORD_TYPE] sra;
assign sra = (src1_signed >>> src2[4:0]);

wire signed [`ysyx_24070017_WORD_TYPE] src1_signed, src2_signed;
assign src1_signed = src1;
assign src2_signed = src2;
ysyx_24070017_MuxKey #(8, 3, `ysyx_24070017_WORD_LENGTH) mux(result, funct3, {
    3'b000,  add,                                                                              // addi, add, sub
    3'b001,  (src1 << src2[4:0]),                                                                              // slli, sll
    3'b010,  {31'd0, src1_signed < src2_signed},                                               // slti, slt
    3'b011,  {31'd0, src1 < src2},                                                             // sltiu, sltu
    3'b100,  src1 ^ src2,                                                                      // xori, xor
    3'b101,  (src1 >> src2[4:0]) | (sra & {`ysyx_24070017_WORD_LENGTH{funct7 == 7'b0100000}}), // srli, srai, srl, sra
    3'b110,  src1 | src2,                                                                      // ori, or
    3'b111,  src1 & src2                                                                       // andi, and
});

endmodule
