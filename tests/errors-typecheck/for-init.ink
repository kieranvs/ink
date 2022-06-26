// @test error

// Verify that type checking recurses into init branch of for statement

fn main() : int
{
	for (int x = false; x < 5; x = x + 1)
	{
		print_bool(false);
	}

	return 0;
}