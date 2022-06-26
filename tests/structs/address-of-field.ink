// @test 4

struct Foo
{
	int x;
	int y;
	int z;
}

fn main() : int
{
	Foo f;
	f.y = 4;
	int* p = &f.y;
	print_uint32(*p);
	return 0;
}