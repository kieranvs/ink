// @test multiline
// 45
// 45

// This test catches a problem where part of an expression is generated into a register which is
// then clobbered by the function call

fn foo() : int
{
	return 42;
}

fn main() : int
{
	int a = foo() + 3;
	print_uint32(a);

	int b = 3 + foo();
	print_uint32(b);

	return a;
}