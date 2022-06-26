// @test error

// Missing return expression

fn foo() : int
{
	return;
}

fn main() : int
{
	return foo();
}