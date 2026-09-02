module top(
  input clk,
  input rst,
  output [31:0] pc_out,
  output [31:0] inst_out
);
	assign pc_out = pc;
	assign inst_out = inst;
	import "DPI-C" function void npc_trap(int code, int pc);
	import "DPI-C" function int pmem_read(input int raddr);
	import "DPI-C" function void pmem_write(
	input int waddr, input int wdata, input byte wmask);

	// 实例化读写模块
	reg [31:0] alu_result;
	wire [31:0] rdata1;
	wire [31:0] rdata2;
	reg [31:0] imm_ext;
	reg [31:0] immS_ext;
	reg [31:0] next_pc;
	RegisterFile instance1 (
		.clk(clk),
		.wdata(alu_result),
		.waddr(rd),
		.raddr1(rs1),
		.raddr2(rs2),
		.wen(opcode == 7'b0010011 || opcode == 7'b1100111 || opcode == 7'b0110011 || opcode == 7'b0110111 || opcode == 7'b0000011),
		.rdata1(rdata1),
		.rdata2(rdata2)
	);

	// 计算立即数和地址
	reg [31:0] mem_addr;
	always @(*) begin
		imm_ext = {{20{imm[11]}}, imm};
		immS_ext = {{20{immS[11]}}, immS};
		if (opcode == 7'b0100011)  // S 型 (store)
			mem_addr = rdata1 + immS_ext;
		else  // I 型 (load)
			mem_addr = rdata1 + imm_ext;
	end

	// 访存逻辑（独立的块）
	reg mem_read_valid;
	reg mem_write_valid;
	reg [31:0] mem_rdata;
	reg [7:0] mem_wmask;

	always @(*) begin
		mem_read_valid = (opcode == 7'b0000011);
		mem_write_valid = (opcode == 7'b0100011);
		
		if (mem_read_valid) begin
			mem_rdata = pmem_read(mem_addr);
		end
		else begin
			mem_rdata = 0;
		end
		
		case(funct3)
			3'b000: mem_wmask = 8'b0000_0001 << mem_addr[1:0];              // sb
			3'b001: mem_wmask = mem_addr[1] ? 8'b0000_1100 : 8'b0000_0011; // sh
			3'b010: mem_wmask = 8'b0000_1111;                              // sw
			default: mem_wmask = 8'b0000_0000;
		endcase
	end

	wire is_ebreak = (inst == 32'h00100073);
	reg [31:0] pc = 32'h80000000;;
	always @(posedge clk) begin
		if(rst) pc <= 32'h80000000;
		else if(!is_ebreak) pc <= next_pc;
	end
	reg [31:0] inst_mem [0:31];
	initial begin
		// ==========================================
		// 0x80000000 <_start>
		// ==========================================
		// [0] 0x80000000: lui sp, 0x80001 (sp = 0x80001000, 栈顶)
		inst_mem[0]  = 32'h80001137;

		// [1] 0x80000004: addi sp, sp, 0 (确认 sp 值)
		inst_mem[1]  = 32'h00010113;

		// [2] 0x80000008: addi a0, zero, 42 (a0 = 0x2a)
		inst_mem[2]  = 32'h02a00513;

		// [3] 0x8000000c: sb a0, 0(sp) (mem[sp] = 0x2a)
		inst_mem[3]  = 32'h00a10023;

		// [4] 0x80000010: addi a1, zero, -1 (a1 = 0xffffffff)
		inst_mem[4]  = 32'hfff00593;

		// [5] 0x80000014: sw a1, 4(sp) (mem[sp+4] = 0xffffffff)
		inst_mem[5]  = 32'h00b12223;

		// [6] 0x80000018: lb a2, 0(sp) (a2 = 0x2a)
		inst_mem[6]  = 32'h00010603;

		// [7] 0x8000001c: lbu a3, 0(sp) (a3 = 0x2a)
		inst_mem[7]  = 32'h00014683;

		// [8] 0x80000020: lw a4, 4(sp) (a4 = 0xffffffff)
		inst_mem[8]  = 32'h00412703;

		// [9] 0x80000024: lui t0, 0x80000 (t0 = 0x80000000, 存放基地址)
		inst_mem[9]  = 32'h800002b7;

		// [10] 0x80000028: add a5, a2, a3 (a5 = 42 + 42 = 84 = 0x54)
		inst_mem[10] = 32'h00d607b3;

		// [11] 0x8000002c: jalr ra, 56(t0) (跳转到 0x80000038 <func>, ra = 0x80000030)
		inst_mem[11] = 32'h038280e7;

		// ==========================================
		// 0x80000030 <halt> (对应 index 12 = 0x30 / 4)
		// ==========================================
		// [12] 0x80000030: ebreak (停机)
		inst_mem[12] = 32'h00100073;

		// [13] 0x80000034: nop (填充占位)
		inst_mem[13] = 32'h00000013;

		// ==========================================
		// 0x80000038 <func> (对应 index 14 = 0x38 / 4)
		// ==========================================
		// [14] 0x80000038: addi a5, a5, 100 (a5 = 0x54 + 0x64 = 0xb8 = 184)
		inst_mem[14] = 32'h06478793;

		// [15] 0x8000003c: lui a6, 0x12345 (a6 = 0x12345000)
		inst_mem[15] = 32'h12345837;

		// [16] 0x80000040: jalr zero, 0(ra) (返回到 ra 指向的 0x80000030 <halt>)
		inst_mem[16] = 32'h00008067;
	end


	wire [31:0] inst = pmem_read(pc);

	// I 型指令
	wire [6:0] opcode = inst[6:0];
	wire [4:0] rd = inst[11:7];
	wire [2:0] funct3 = inst[14:12];
	wire [4:0] rs1 = inst[19:15];
	wire [11:0] imm = inst[31:20];

	// R 型指令
	wire [6:0] funct7 = inst[31:25];
	wire [4:0] rs2 = inst[24:20];

	// U 型指令
	wire [19:0] immU = inst[31:12];

	// S 型指令
	wire [11:0] immS = {inst[31:25], inst[11:7]};

	// 调试
	// always @(posedge clk) begin
	//	if (!is_ebreak)
	//		$display("PC=%h, inst=%h, opcode=%b, rd=%d, rs1=%d(=%h), rs2=%d(=%h)", 
	//				pc, inst, opcode, rd, rs1, rdata1, rs2, rdata2);
	// end

	wire [7:0] load_byte = (mem_addr[1:0] == 2'b00) ? mem_rdata[7:0]   :
                       (mem_addr[1:0] == 2'b01) ? mem_rdata[15:8]  :
                       (mem_addr[1:0] == 2'b10) ? mem_rdata[23:16] : mem_rdata[31:24];

	always @(*) begin
		alu_result = 32'b0;
		next_pc = pc + 4;
		case(opcode)
			7'b0010011: begin
				alu_result = rdata1 + imm_ext; // addi
			end
			7'b1100111: begin
				alu_result = pc + 4;
				next_pc = (rdata1 + imm_ext) & 32'hfffffffe; // jalr
			end
			7'b0110011: begin
				if(funct7 == 7'b0) alu_result = rdata1 + rdata2; // add
				else if(funct7 == 7'b0100000) alu_result = rdata1 - rdata2; // sub
			end
			7'b0110111: begin
				alu_result = {immU, 12'b0}; // lui
			end
			7'b0000011: begin // load
				case(funct3) 
					3'b000: alu_result = {{24{load_byte[7]}}, load_byte}; // lb
					3'b010: alu_result = mem_rdata;                       // lw
					3'b100: alu_result = {24'b0, load_byte};              // lbu
					default: alu_result = 32'b0;
				endcase
			end
			7'b0100011: begin // store
				// 前面已经处理
			end
			default: begin
			end
		endcase
	end

	// store 指令写内存
	wire [31:0] store_data = (funct3 == 3'b000) ? {4{rdata2[7:0]}} : rdata2;

	always @(posedge clk) begin
		if (mem_write_valid && !is_ebreak) begin
			pmem_write(mem_addr, store_data, mem_wmask);
		end
	end

	function int breakout();
		if (inst == 32'h00100073) return 1;
		else return 0;
	endfunction

	wire [31:0] a0 = instance1.rf[10];
	always @(posedge clk) begin
		if (is_ebreak) begin
			npc_trap(a0, pc);
			$finish; // 告诉仿真器可以结束了
		end
	end
endmodule