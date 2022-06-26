// @test multiline
// false
// true
// true
// false
// true
// false
// false
// true
// true
// true
// false
// true

fn main() : int
{
	float z = 20.0;
	bool a = z == 10.0;
	bool b = z == 20.0;
	bool c = z != 10.0;
	bool d = z != 20.0;
	bool e = z > 10.0;
	bool f = z > 20.0;
	bool g = z < 20.0;
	bool h = z < 30.0;
	bool i = z >= 10.0;
	bool j = z >= 20.0;
	bool k = z <= 10.0;
	bool l = z <= 20.0;
	print_bool(a);
	print_bool(b);
	print_bool(c);
	print_bool(d);
	print_bool(e);
	print_bool(f);
	print_bool(g);
	print_bool(h);
	print_bool(i);
	print_bool(j);
	print_bool(k);
	print_bool(l);
	return 0;
}