module carry(a,b,c,cout,a_equal_b);
	input a, b, c; 
	output cout, a_equal_b;
	wire x; 
	assign a_equal_b = (a == c) & b;
	assign x = a & b;
	assign cout = x | c;
endmodule