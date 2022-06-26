// @test 3.500000

// Verify that floats can be passed and returned

fn foo(float a, float b) : float
{
	return a + b;
}

fn main() : int
{
	print_float(foo(1.0, 2.5));
	return 0;
}