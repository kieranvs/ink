// @test 16

fn foo(int x) : int
{
	return x * x;
}

fn main() : int
{
	print_uint32(foo(4));
	return 0;
}