// @test error

fn main() : int
{
	for (int x = 0; x; x = x + 1)
	{
		print_uint32(x);
	}

	return 1;
}