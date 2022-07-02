// @test Hello

// Verify that external function can be called

#link "libc"
#link "libext.a"

external fn print_msg()

fn main() : int
{
	print_msg();
	return 0;
}