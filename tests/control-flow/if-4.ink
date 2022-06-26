// @test multiline
// 20
// 17
// true
// 14
// false
// false
// 12
// false
// 7
// false
// 5

fn foo(int i, bool b) : bool
{
	print_uint32(i);
	if (i > 10)
	{
		if (b == true)
		{
			print_bool(b);
		}

		if (i == 14)
		{
			print_bool(b);
		}

		if (i > 15)
		{
			return false;
		}
	}

	if (b == false)
	{
		print_bool(b);
	}
}

fn main() : int
{
	foo(20, false);
	foo(17, true);
	foo(14, false);
	foo(12, false);
	foo(7, false);
	foo(5, true);

	return 0;
}