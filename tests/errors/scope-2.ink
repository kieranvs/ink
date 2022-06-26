// @test error

// Check that the else branch gets a distinct scope

fn main() : int
{
	if (true)
	{
		int x = 4;
	}
	else
	{
		x = 3;
	}

	return 0;
}