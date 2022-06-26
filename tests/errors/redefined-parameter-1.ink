// @test error

fn foo(int x) : int
{
	int x = 9;
	return x;
}

fn main() : int
{
	return foo(4);
}