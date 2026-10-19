#!/bin/sh
set -u
BIN=./mooncalc
fail=0

check() {
    input=$1
    expected=$2
    out=$(printf '%s\nquit\n' "$input" | NO_COLOR=1 $BIN 2>&1)
    case "$out" in
        *"$expected"*) printf 'ok    %s\n' "$input" ;;
        *) printf 'FAIL  %s (expected: %s)\n' "$input" "$expected"; fail=1 ;;
    esac
}

check '2+3*4' '▸ 14'
check '2^3^2' '▸ 512'
check '-2^2' '▸ -4'
check '5!' '▸ 120'
check 'sin(pi/2)' '▸ 1'
check 'sqrt(-1)' 'sqrt of a negative number'
check '1/0' 'division by zero'
check 'solve 2*x+3=11' 'x = 4'
check 'solve x^2-5*x+6=0' 'x2 = 3'
check 'solve x^2+1=0' 'x2 = 1i'
check 'solve x+1=x+2' 'no solution'
check 'solve x+1=x+1' 'infinitely many solutions'
check 'solve cos(x)=x' 'x = 0.739085133215161'

exit $fail
