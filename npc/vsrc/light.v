module light(
  input clk,
  input rst,
  output reg [15:0] led
);
  initial begin
    if ($test$plusargs("trace") != 0) begin
      $dumpfile("light.fst");
      $dumpvars();
    end
  end

  reg [31:0] count;
  always @(posedge clk) begin
    if (rst) begin led <= 1; count <= 0; end
    else begin
      if (count == 0) led <= {led[14:0], led[15]};
      count <= (count >= 5000000 ? 32'b0 : count + 1);
    end
  end
endmodule

