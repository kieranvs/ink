// @test Hello

// Verify that pointers can be passed as arguments

fn foo(char* msg) : bool
{
	print_string(msg);
	return false;
}

fn main() : int
{
	foo("Hello");
	return 0;
}