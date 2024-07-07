// 2 bit 4 to 1 mux
module lab1(
    input[1:0] X0, 
    input[1:0] X1, 
    input[1:0] X2, 
    input[1:0] X3, 
    input[1:0] Y, 
    output[1:0] F
    );
    wire[1:0] f00, f01, f10, f11;
    assign f00 = X0 & {2{&(Y^~2'b00)}};
    assign f01 = X1 & {2{&(Y^~2'b01)}};
    assign f10 = X2 & {2{&(Y^~2'b10)}};
    assign f11 = X3 & {2{&(Y^~2'b11)}};

    assign F = f00 | f01 | f10 | f11;
endmodule
