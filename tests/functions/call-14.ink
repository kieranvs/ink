// @test 3.140000

// Check that f32 can be returned, especially from a float literal which requires conversion

fn foo() : f32
{
	return 3.14;
}

fn main() : u64
{
	f32 y = foo();
	print_float32(y);

	return 0;
}