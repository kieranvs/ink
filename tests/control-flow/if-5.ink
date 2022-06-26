// @test 3

// If the if statement's scope isn't properly nested within the parent scope, the two variables will
// get allocated the same offset and y will stomp x

fn main() : int
{
	int x = 3;
	if (x == 3)
	{
		int y = 5;
	}

	print_uint32(x);
	return 0;
}