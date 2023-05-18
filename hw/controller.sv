// Keys
// KEY[2] = A
// KEY[3] = B
// KEY[0] = SELECT
// KEY[1] = START
// SW[2] = UP
// SW[1] = DOWN
// SW[3] = LEFT
// SW[0] = RIGHT

module controller(input logic clk,
				  input logic reset,
				  input logic read,
				  input logic chipselect,
				  input logic address,
				  input logic [9:0] BOARD_SW,
				  input logic [3:0] BOARD_KEY,
				  output logic [7:0] readdata);
	logic [2:0] button;
	always_ff @(posedge read) begin
		if (address) begin
			readdata = BOARD_KEY[2];
			button = 0;
		end
		else begin
			case (button)
			0 : readdata = {3'b000, 4'b0000, BOARD_KEY[2]};
			1 : readdata = {3'b001, 4'b0000, BOARD_KEY[3]};
			2 : readdata = {3'b010, 4'b0000, BOARD_KEY[0]};
			3 : readdata = {3'b011, 4'b0000, BOARD_KEY[1]};
			4 : readdata = {3'b100, 4'b0000, BOARD_SW[2]};
			5 : readdata = {3'b101, 4'b0000, BOARD_SW[1]};
			6 : readdata = {3'b110, 4'b0000, BOARD_SW[3]};
			7 : readdata = {3'b111, 4'b0000, BOARD_SW[0]};
			endcase
			button = button + 1;
		end
	end

endmodule



