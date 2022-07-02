// @test 3 7.200000

// Verify that external function with parameters can be called

#link "libc"
#link "libext.a"

external fn print_int_double(int x, float y)

fn main() : int
{
	print_int_double(3, 7.2);
	return 0;
}