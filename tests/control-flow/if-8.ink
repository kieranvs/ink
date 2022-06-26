// @test 77

// Check that else branch works

fn main() : int
{
	int x = 3;
	if (x == 4)
	{
		print_uint32(99);
	}
	else
	{
		print_uint32(77);
	}

	return 0;
}