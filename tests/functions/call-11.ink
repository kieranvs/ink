// @test 4.125000

// Verify that floats can be returned

fn foo() : float
{
	return 4.125;
}

fn main() : int
{
	print_float(foo());
	return 0;
}