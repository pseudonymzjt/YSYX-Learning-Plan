module top(
  input clk,
  input rst
);
	reg [31:0] pc;
	always @(posedge clk) begin
		if(rst) pc <= 32'h80000000;
		else pc <= pc + 4;
	end
	reg [31:0] inst_mem [0:15];
	initial begin
    // 1. addi x1, x0, 5
    inst_mem[0] = 32'h00500093; 
    // 2. addi x2, x1, 10
    inst_mem[1] = 32'h00a08113; 
    // ebreak
    inst_mem[2] = 32'h00100073; 
	end
	wire [31:0] inst = inst_mem[pc[5:2]];

	// I 型指令
	wire [6:0] opcode = inst[6:0];
	wire [4:0] rd = inst[11:7];
	wire [2:0] funct3 = inst[14:12];
	wire [4:0] rs1 = inst[19:15];
	wire [11:0] imm = inst[31:20];

	wire [4:0] rs2 = inst[24:20];
	// 实例化读写模块
	wire [31:0] alu_result;
	wire [31:0] rdata1;
	wire [31:0] rdata2;
	wire [31:0] imm_ext;
	RegisterFile instance1 (
		.clk(clk),
		.wdata(alu_result),
		.waddr(rd),
		.raddr1(rs1),
		.raddr2(rs2),
		.wen(opcode == 7'b0010011),
		.rdata1(rdata1),
		.rdata2(rdata2)
	);
	assign imm_ext = {{20{imm[11]}}, imm};
	assign alu_result = rdata1 + imm_ext;

	always @(posedge clk) begin
		if (inst == 32'h00100073) begin
			$display("Hit ebreak! Simulation successfully finished.");
			$finish; // 告诉仿真器可以结束了
		end
	end
endmodule