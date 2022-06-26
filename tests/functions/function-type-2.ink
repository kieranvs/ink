// @test 42

fn foo() : int
{
	return 32;
}

fn_type FuncType = () : int

fn example(FuncType f)
{
	return;
}

fn main() : int
{
	example(foo);
	print_uint32(42);
	return 0;
}