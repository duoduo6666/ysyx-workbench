module ysyx_2070017_ALU(
    input [6:0] opcode,
    input [2:0] funct3,
    input [6:0] funct7,
    input [ysyx_24070017_WORD_TYPE] src1,
    input [ysyx_24070017_WORD_TYPE] src2,
    output [ysyx_24070017_WORD_TYPE] result,
)

// mode 0 即 funct7=0 或 opcode=OP-IMM(0010011), 这时OP-IMM和OP执行的运算相同
// mode 1 即 funct7=0100000 且 opcode=OP, 这时部分指令的运算不相同
wire mode;
assign mode = funct7==7'b0100000

ysyx_24070017_MuxKey #(1, 3, ysyx_24070017_WORD_LENGTH) mux(result, funct3, {
    3'b000, ((src1+src2) & {ysyx_24070017_WORD_LENGTH{!mode}}) | ((src1-src2) & {ysyx_24070017_WORD_LENGTH{mode}}), // addi, add, sub
    // TODO 更多命令
}
);

endmodule