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
check '1/0' 'division by zero'
check '2pi' '▸ 6.28318530717959'
check '3(4+1)' '▸ 15'
check 'sqrt(-4)' '▸ 2i'
check '(2+3i)*(1-i)' '▸ 5 + i'
check '0xFF+1' '▸ 256'
check 'f(x)=x^2+1' 'defined f(x)'
check 'solve 2*x+3=11' 'x = 4'
check 'solve x^2-5*x+6=0' 'x2 = 3'
check 'solve x^2+1=0' 'x2 = i'
check 'solve x^3-6*x^2+11*x-6=0' 'x3 = 3'
check 'solve x^3-3x^2+3x-1=0' '▸ x = 1'
check 'solve x+y=3, x-y=1' 'y = 1'
check 'solve x+1=x+2' 'no solution'
check 'solve cos(x)=x' 'x = 0.739085133'
check 'diff(x^3, 2)' '▸ 12'
check 'integral(x^2, 0, 3)' '▸ 9'
check 'limit(sin(x)/x, 0)' '▸ 1'
check 'limit(1/x^2, 0)' '▸ inf'
check 'sum(k^2, k, 1, 10)' '▸ 385'
check 'prod(k, k, 1, 5)' '▸ 120'
check 'findroot(cos(x)-x, 1)' '0.7390851332'
check 'nCr(5,2)' '▸ 10'
check 'gcd(12,18)' '▸ 6'
check 'isprime(97)' '▸ 1'
check 'factor 360' '2^3 × 3^2 × 5'
check 'mean(1,2,3,4)' '▸ 2.5'
check 'det([1,2;3,4])' '▸ -2'
check '[1,2;3,4]*[5,6;7,8]' '[ 19  22 ]'
check 'linreg([1,2,3],[2,4,6])' '[ 2  0  1 ]'
check '[2,1;1,3]\[3;5]' '[ 0.8 ]'
check 'rref([1,2,3;4,5,6;7,8,9])' '[ 1  0  -1 ]'
check 'nullspace([1,2,3;4,5,6;7,8,9])' '[  1 ]'
check '[1,2;2,4]\[1;3]' 'no solution'
check '[1,2,3].*[4,5,6]' '[ 4  10  18 ]'
check '2.^[1,2,3]' '[ 2  4  8 ]'
check 'A=[5,6;7,8]' '[ 5  6 ]'
check 'expm([0,1;0,0])' '[ 1  1 ]'
check 'cond([1,2;2,4])' '▸ inf'
check 'diag([1,2,3])' '[ 1  0  0 ]'
check 'sub([1,2,3;4,5,6;7,8,9],2,3,1,2)' '[ 4  5 ]'
check 'hex 255' '0xFF'
check 'base 255 7' '513'
check 'convert 100 km to mi' '62.137119'
check 'bigfact 30' '265252859812191058636308480000000'
check 'plot sin(x)' 'plot  sin(x)'

exit $fail
