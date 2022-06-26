// @test multiline
// 23
// 17
// 42
// 29

fn main() : int
{
	int x0 = 3 + 4 * 5;
	int x1 = 3 * 4 + 5;
	int x2 = 3 * 4 + 5 * 6;
	int x3 = 3 + 4 * 5 + 6;

	print_uint32(x0);
	print_uint32(x1);
	print_uint32(x2);
	print_uint32(x3);

	return 0;
}