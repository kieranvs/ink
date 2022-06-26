// @test error

// Check that bool* can't be assigned to int*

fn main() : int
{
	int x = 5;
	bool b = true;
	int* y = &b;
	return 0;
}