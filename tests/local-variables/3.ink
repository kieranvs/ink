// @test 9

fn main() : int
{
	int x = 3;
	int y = x + 2;
	x = 4;
	print_uint32(x + y);
	return 0;
}