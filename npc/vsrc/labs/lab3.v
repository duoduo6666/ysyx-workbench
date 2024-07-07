module lab3(
    input[2:0] func,
    input[3:0] A,
    input[3:0] B,
    output reg[3:0] result,
    output carry,
    output zero,
    output overflow
);

    wire[3:0] B0;
    assign B0 = (B ^ {4{func[0]}});

    wire[3:0] f00x;
    wire cout;
    assign {cout, f00x} = A + B0 + {3'b000, func[0]};

    assign carry = func[2:1] == 2'b00 & cout;
    assign zero = ~(|result);
    assign overflow = func[2:1] == 2'b00 & ((A[3] == B0[3]) & (result[3] != A[3]));

    
    always @(*)
        case(func)
            3'b000: result = f00x;
            3'b001: result = f00x;
            3'b010: result = ~A;
            3'b011: result = A & B;
            3'b100: result = A | B;
            3'b101: result = A ^ B;
            3'b110: result = {3'b000, A < B};
            3'b111: result = {3'b000, A == B};
        endcase
endmodule
