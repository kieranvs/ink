// @test error

fn get_bool(bool b) : bool
{
	return b;
}

fn main() : int
{
	bool b = get_bool(80);
	return 20;
}