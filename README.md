# ink

`inkc` is a hobby/educational compiler for a simple, C-like language `ink`, which compiles to `x86_64`.

Currently, the compiler supports Linux and macOS. It has one dependency, on the [yasm assembler](https://yasm.tortall.net/).

## Development Quick Start

Build:
1. Make sure `yasm` is available on the `PATH`. (e.g. on archlinux, `pacman -S yasm`)
1. `mkdir build && cd build`
1. `cmake ..`
1. `make`

Run the unit tests:
1. `./testing`

Run the compiler:

```rust
// File hello.ink:

fn main() : int
{
	print_string("Hello world!");
	return 0;
}
```

`./inkc hello.ink`
