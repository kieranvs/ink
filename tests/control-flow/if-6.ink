// @test 23

// Check that the parent scope is given enough stack space for the nested scopes

fn main() : int
{
	int x = 3;
	if (x == 3)
	{
		int y = 5;
	}

	if (x == 3)
	{
		int y = 6;
		int z = 23;

		print_uint32(z);
	}

	return 0;
}