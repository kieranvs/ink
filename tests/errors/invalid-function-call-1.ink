// @test error

fn foo(int x, int y) : int
{
	return x + y;
}

fn main() : int
{
	return foo(4);
}