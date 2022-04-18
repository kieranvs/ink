// @test 0

// x and y are at the same stack offset. Check if zero initialisation occurred for y

struct Foo
{
	int a;
}

fn main() : int
{
	if (true)
	{
		Foo x;
		x.a = 42;
	}

	if (true)
	{
		Foo y;
		print_uint32(y.a);
	}
	return 0;
}