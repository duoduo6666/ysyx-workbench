`ifndef ysyx_24070017_DEFINES
`define ysyx_24070017_DEFINES

`define ysyx_2070017_VERILATOR_TRACE

// `define ysyx_24070017_RV32I
`define ysyx_24070017_RV32E

`define ysyx_24070017_WORD_LENGTH 32
`define ysyx_24070017_WORD_TYPE `ysyx_24070017_WORD_LENGTH-1:0


`ifdef ysyx_24070017_RV32I
`define ysyx_24070017_RF_REG_NUM 32
`endif

`ifdef ysyx_24070017_RV32E
`define ysyx_24070017_RF_REG_NUM 16
`endif

`endif
