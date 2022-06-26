// @test 32

fn foo() : int
{
	return 32;
}

fn main() : int
{
	int x = foo();
	print_uint32(x);
	return 0;
}