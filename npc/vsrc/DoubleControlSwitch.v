module DoubleControlSwitch(
	input a,
	input b,
	output f
);
	initial begin
		if ($test$plusargs("trace") != 0) begin
			$dumpfile("vlt_dump.vcd");
			$dumpvars();
		end
	end
	assign f = a ^ b;
endmodule
