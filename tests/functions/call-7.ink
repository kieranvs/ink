// @test true

fn get_bool(bool b) : bool
{
	return b;
}

fn main() : int
{
	bool b = get_bool(true);
	print_bool(b);
	return 0;
}