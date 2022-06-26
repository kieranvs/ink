// @test 32

fn foo() : int
{
	return 32;
}

fn main() : int
{
	print_uint32(foo());
	return 0;
}