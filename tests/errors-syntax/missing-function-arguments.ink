// @test error

fn foo() : int
{
	return 4;
}

fn main() : int
{
	int x = foo;
	return 2;
}