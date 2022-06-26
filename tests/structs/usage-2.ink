// @test multiline
// 1
// false
// E
// Hello

struct Foo
{
	int x;
	bool y;
	char z;
	char* w;
}

fn main() : int
{
	Foo foo;
	foo.x = 1;
	foo.y = false;
	foo.z = 'E';
	foo.w = "Hello";
	print_uint32(foo.x);
	print_bool(foo.y);
	print_char(foo.z);
	print_string(foo.w);
	return 0;
}