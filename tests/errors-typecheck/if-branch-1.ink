// @test error

// Verify that type checking recurses into if branch of if statement

fn main() : int
{
	if (true)
	{
		int x = false;
	}
	else
	{
		int x = 3;
	}

	return 0;
}