# MoonCalc

An interactive terminal scientific calculator written in C++17, with a black and purple interface, rounded borders and a built-in solver for basic algebraic equations.

```
╭─ MoonCalc ────────────────────────────────────╮
│  scientific calculator in C++17              │
│                                              │
│  help  ·  solve x^2-4=0  ·  quit             │
╰──────────────────────────────────────────────╯
rad ❯ solve x^2-5*x+6=0
  quadratic equation
  ▸ x1 = 2
  ▸ x2 = 3
```

## Requirements

| Requirement | Details |
|---|---|
| Operating system | macOS or Linux. Uses `<unistd.h>` (`isatty`); on Windows use WSL. |
| Compiler | Any C++17 compiler (`clang++` 5+, `g++` 7+). |
| Build tool | `make` (optional). |
| Terminal | UTF-8 and 24-bit (truecolor) support, e.g. iTerm2, Kitty, Alacritty, GNOME Terminal, VS Code. A dark background looks best. |
| Dependencies | None beyond the C++ standard library. |

Notes:

- **Colors** are disabled automatically when output is not a terminal (pipes, redirects) or when the `NO_COLOR` environment variable is set.
- **Precision:** calculations use `long double`. On x86-64 Linux this gives about 18 digits; on Apple Silicon `long double` equals `double` (about 15-16 digits). Results are shown with 15 significant digits.
- **Input:** MoonCalc is a REPL reading lines from standard input until `quit` or end of input (Ctrl-D).
- It is not intended for critical computation (financial, medical, safety-related).

## Build and run

```bash
make
./mooncalc
```

Without `make`:

```bash
c++ -std=c++17 -O2 main.cpp -o mooncalc
```

Run the smoke tests:

```bash
make test
```

## Usage

### Basics

| Feature | Syntax |
|---|---|
| Operators | `+ - * / %` and `^` (power, right-associative: `2^3^2 = 512`) |
| Factorial | `5!` or `fact(5)` |
| Constants | `pi`, `e`, `ans` (last result) |
| Variables | `x = 5*2`, then `x^2` |
| Scientific notation | `2e3`, `1.5e-4` |
| Angle mode | `deg` / `rad` (the prompt shows the active mode) |

`-2^2` evaluates to `-4`. There is no implicit multiplication: write `2*pi`, not `2pi`.

### Functions

- Trigonometric: `sin cos tan asin acos atan atan2(y,x)`
- Hyperbolic: `sinh cosh tanh`
- Powers and logarithms: `sqrt cbrt exp ln log log2 logb(x,base) root(x,n) pow(a,b) mod(a,b)`
- Other: `abs floor ceil round fact min max` (`min` and `max` accept several arguments)

### Solving equations

```
solve 2*x+3=11            ->  x = 4
solve x^2-5*x+6=0         ->  x1 = 2, x2 = 3
solve x^2+1=0             ->  x1 = -1i, x2 = 1i
solve y: 3*y-1=y+7        ->  y = 4        (explicit unknown)
solve cos(x)=x            ->  x = 0.739085133215161
```

- Linear and quadratic equations are solved exactly. A negative discriminant yields complex roots.
- Contradictions (`x+1=x+2`) and identities (`x+1=x+1`) are detected.
- Any other equation (cubics, trigonometric, exponential, ...) is handled by a numeric search for **real** roots in `[-100, 100]`. It can miss double roots and roots that are extremely close together.
- The unknown is detected automatically when exactly one identifier is undefined. Otherwise specify it with `solve <var>: equation`.

### Commands

`help`, `vars` (list variables), `deg`, `rad`, `quit` / `exit`.

## Project layout

```
calculator.hpp   engine: parser, functions, equation solver
main.cpp         terminal interface (colors, panels, REPL)
Makefile         build and test targets
tests/smoke.sh   end-to-end smoke tests
```

## License

MIT. See [LICENSE](LICENSE).
