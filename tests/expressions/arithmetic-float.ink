// @test multiline
// 4.000000
// 1092.000000
// 0.500000
// 1.250000
// 100.000000
// 2.000000
// 6.000000
// 413999.000000
// 6.000000
// 2048.000000
// 160.000000
// 64.000000
// 51.200000
// 1.250000

fn main() : int
{
	print_float(1.0 + 3.0);
	print_float(101.0 + 991.0);
	print_float(0.2 + 0.3);
	print_float(1.125 + 0.125);
	print_float(99.5 + 0.5);

	print_float(3.0 - 1.0);
	print_float(8.5 - 2.5);
	print_float(414090.2 - 91.2);

	print_float(2.0 * 3.0);
	print_float(32.0 * 64.0);
	print_float(2.5 * 64.0);

	print_float(128.0 / 2.0);
	print_float(128.0 / 2.5);
	print_float(5.0 / 4.0);

	return 0;
}