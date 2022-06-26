// @test multiline
// 0
// 2

struct Foo
{
	int x;
	int y;
}

Foo a;

fn main() : int
{
	print_uint32(a.x);
	a.x = 2;
	print_uint32(a.x);
	return 0;
}
