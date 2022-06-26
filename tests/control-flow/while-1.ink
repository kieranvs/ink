// @test multiline
// 0
// 1
// 2
// 3
// 4
// 5

fn main() : int
{
	int x = 0;
	while (x < 6)
	{
		print_uint32(x);
		x = x + 1;
	}

	return 0;
}