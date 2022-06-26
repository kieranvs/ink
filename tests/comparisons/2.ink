// @test multiline
// false
// true
// true
// false

fn main() : int
{
	bool z = true;
	bool a = z == false;
	bool b = z == true;
	bool c = z != false;
	bool d = z != true;
	print_bool(a);
	print_bool(b);
	print_bool(c);
	print_bool(d);
	return 0;
}