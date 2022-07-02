// @test Hello

#link "libc"
#link "libext.a"

external fn _print_msg()

fn main() : int
{
	_print_msg();
	return 0;
}