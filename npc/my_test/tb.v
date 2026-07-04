module tb;
    reg a;
    reg b;
    wire out;

    top u_dut (
        .a(a),
        .b(b),
        .f(out)
    );

    initial begin
        $dumpfile("wave.vcd");
        $dumpvars(0, tb); // 记录所有内容。Verilator 忽略参数，只记录所有内容。
        
        a = 1;
        b = 0;
        #15 a = 0;
        #100;
        $finish;
    end

endmodule