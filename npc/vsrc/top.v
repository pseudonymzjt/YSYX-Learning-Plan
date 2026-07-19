module top(
  input clk,
  input rst
);
	import "DPI-C" function void npc_trap(int code);
	reg [31:0] pc;
	wire [31:0] next_pc;
	always @(posedge clk) begin
		if(rst) pc <= 32'h80000000;
		else pc <= next_pc;
	end
	reg [31:0] inst_mem [0:15];
	initial begin
	// 00000000 <_start>:
		// 1. addi	a0,zero,20
		inst_mem[0] = 32'h01400513; 
		// 2. jalr	ra,16(zero) # 10 <fun>
		inst_mem[1] = 32'h010000e7; 
		// 3. jalr	ra,12(zero) # c <halt>
		inst_mem[2] = 32'h00c000e7; 
	// 0000000c <halt>:
		// 4. ebreak
		inst_mem[3] = 32'h00100073;
	// 00000010 <fun>:
		// 5. addi	a0,a0,10
		inst_mem[4] = 32'h00a50513;
		// 6. jalr	zero,0(ra)
		inst_mem[5] = 32'h00008067;
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
	reg [31:0] alu_result;
	wire [31:0] rdata1;
	wire [31:0] rdata2;
	reg [31:0] imm_ext;
	reg [31:0] next_pc;
	RegisterFile instance1 (
		.clk(clk),
		.wdata(alu_result),
		.waddr(rd),
		.raddr1(rs1),
		.raddr2(rs2),
		.wen(opcode == 7'b0010011 || opcode == 7'b1100111),
		.rdata1(rdata1),
		.rdata2(rdata2)
	);
	always @(*) begin
		imm_ext = {{20{imm[11]}}, imm};
		next_pc = pc + 4;
		alu_result = 32'b0;
		case(opcode)
			7'b0010011: begin
				alu_result = rdata1 + imm_ext;
			end
			7'b1100111: begin
				alu_result = pc + 4;
				next_pc = (rdata1 + imm_ext) & 32'hfffffffe;
			end
			default: begin
			end
		endcase
	end
	function int breakout();
		if (inst == 32'h00100073) return 1;
		else return 0;
	endfunction

	always @(posedge clk) begin
		if (inst == 32'h00100073) begin
			npc_trap(0);
			$finish; // 告诉仿真器可以结束了
		end
	end
endmodule