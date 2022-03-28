// @test multiline
// 1
// 2
// 6
// 24
// 120
// 720
// 5040
// 40320
// 362880
// 3628800

fn fact(int i) : int
{
	if (i == 1)
	{
		return 1;
	}
	else
	{
		return i * fact(i - 1);
	}
}

fn main() : int
{
	for (int i = 1; i <= 10; i = i + 1)
	{
		print_uint32(fact(i));
	}
	return 0;
}