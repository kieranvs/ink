// @test multiline
// 6
// 3

// Check that shadowing works properly

fn main() : int
{
	int x = 3;
	if (x == 3)
	{
		int x = 5;
		x = 6;
		print_uint32(x);
	}

	print_uint32(x);

	return 0;
}