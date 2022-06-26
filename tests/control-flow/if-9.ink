// @test 99

// Check that if branch works when there is an else branch

fn main() : int
{
	int x = 4;
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