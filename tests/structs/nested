// @test multiline
// 1
// 2
// 3
// 4
// 5
// 6

struct Foo
{
	int x;
	int y;
	int z;
}

struct Bar
{
	Foo a;
	Foo b;
}

fn main() : int
{
	Bar p;
	p.a.x = 1;
	p.a.y = 2;
	p.a.z = 3;
	p.b.x = 4;
	p.b.y = 5;
	p.b.z = 6;

	print_uint32(p.a.x);
	print_uint32(p.a.y);
	print_uint32(p.a.z);
	print_uint32(p.b.x);
	print_uint32(p.b.y);
	print_uint32(p.b.z);

	return 0;
}