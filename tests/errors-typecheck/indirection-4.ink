// @test error

// Check that int can't be dereferenced

fn main() : int
{
	int x = 5;
	int* y = *x;
	return 0;
}