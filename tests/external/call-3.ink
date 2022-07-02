// @test 3.200000

// Verify that external function can return

#link "libc"
#link "libext.a"

external fn sum_doubles(float x, float y) : float

fn main() : int
{
	float s = sum_doubles(2.4, 0.8);
	print_float(s);
	return 0;
}