// @test multiline
// Test string
// 

// Verify that libc functions can be called

#link "libc"

external fn puts(char* str) : int

fn main() : int
{
	puts("Test string");
	return 0;
}