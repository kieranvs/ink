// @test error

// Verify that type checking recurses into increment branch of for statement

fn main() : int
{
	for (int x = 0; x < 5; x = x + false)
	{
		print_bool(false);
	}

	return 0;
}