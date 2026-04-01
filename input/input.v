module carry(a, b, c, cout, a_equal_b, a_shift_b);
	input a, b, c;
	output cout, a_equal_b, a_shift_b;
	wire x;
	assign a_shift_b = a & b;
	assign a_equal_b = a | b;
	assign x = a & b & c;
	assign cout = x | c | b;
endmodule