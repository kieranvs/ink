// @test Hello

// Verify that pointers can be returned

fn foo() : char*
{
	return "Hello";
}

fn main() : int
{
	print_string(foo());
	return 0;
}