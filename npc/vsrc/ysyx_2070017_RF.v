module ysyx_24070017_RF (
    input clk,
    input rst,
    input [`ysyx_24070017_RF_REG_NUM * `ysyx_24070017_WORD_LENGTH - 1:0] wdata, // Write Data
    input [`ysyx_24070017_RF_REG_NUM-1:0] we, // Write Enable
    output [`ysyx_24070017_RF_REG_NUM * `ysyx_24070017_WORD_LENGTH - 1:0] rdata // Read Data
);

assign rdata[`ysyx_24070017_WORD_TYPE] = {`ysyx_24070017_WORD_LENGTH{1'b0}};

genvar i;
generate
    for(i=1; i<`ysyx_24070017_RF_REG_NUM; i=i+1) begin: xreg
        ysyx_24070017_Reg #(`ysyx_24070017_WORD_LENGTH, {`ysyx_24070017_WORD_LENGTH{1'b0}}) xreg (
            .clk(clk),
            .rst(rst),
            .wen(we[i]),
            .din(wdata[(i+1)*`ysyx_24070017_WORD_LENGTH-1:i*`ysyx_24070017_WORD_LENGTH]),
            .dout(rdata[(i+1)*`ysyx_24070017_WORD_LENGTH-1:i*`ysyx_24070017_WORD_LENGTH])
        );
    end
endgenerate

endmodule
