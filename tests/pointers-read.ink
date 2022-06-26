// @test multiline
// false
// true
// false
// true
// true
// false
// true
// false
// false
// false
// false
// false
// true
// false
// true

// Test pointer read stuff

fn main() : int
{
	bool a = false;
	bool b = true;
	bool c = false;

	bool* pa = &a;
	bool* pb = &b;
	bool* pc = &c;

	print_bool(*pa);
	print_bool(*pb);
	print_bool(*pc);

	a = *pb;

	print_bool(*pa);
	print_bool(*pb);
	print_bool(*pc);

	b = false;

	print_bool(*pa);
	print_bool(*pb);
	print_bool(*pc);

	pa = pc;

	print_bool(*pa);
	print_bool(*pb);
	print_bool(*pc);

	c = true;

	print_bool(*pa);
	print_bool(*pb);
	print_bool(*pc);
}