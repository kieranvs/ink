// @test multiline
// 0
// 2

int* x;

fn main() : int
{
	int y;
	x = &y;
	print_uint32(*x);
	y = 2;
	print_uint32(*x);
	return 0;
}
