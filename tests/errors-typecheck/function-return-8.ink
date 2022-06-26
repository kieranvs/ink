// @test error

// Attempt to use return value from function with no return type

fn foo()
{
	return 4;
}

fn main() : int
{
	int x = foo();
	return 0;
}