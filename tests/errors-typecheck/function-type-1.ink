// @test error

fn_type Foo = (int) : int

fn Func(float x) : int
{
	return 0;
}

fn main() : int
{
	Foo x = Func;
	return 0;
}