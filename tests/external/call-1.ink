// @test Hello

#link "libc"
#link "libext.a"

external fn print_msg()

fn main() : int
{
	print_msg();
	return 0;
}