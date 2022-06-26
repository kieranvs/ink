// @test 3.500000

// Verify that f32 can be passed and returned

fn foo(f32 a) : f32
{
	return a;
}

fn main() : int
{
	print_float32(foo(3.5));
	return 0;
}