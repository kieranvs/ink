// @test error

// Check that for loop initialiser doesn't get into parent scope

fn main() : int
{
	for (int i = 0; i < 4; i = i + 1)
	{
		int y = 0;
	}

	return i;
}