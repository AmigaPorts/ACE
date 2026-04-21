# Fixed-point math

The stock Amiga 500 doesn't have hardware support for floating-point numbers and it's too slow to reasonably use their software version.
Using them starts to be viable with accelerator cards and FPUs, however even on such configurations they aren't the fastest way to deal with fractions.
The solution is to use a fixed-point math instead.

ACE comes bundled with libfixmath, so usage should be quite straightforward.
To use it, you need to include the `<fixmath/fix16.h>` header.

## Limitations of fixed-point math

Contrary to regular n-bit integer that stores its value with bits that represent 2^0 up to 2^(n-1), fixed point numbers use negative powers of two (or 1 repeatedly divided by 2) to represent their value. As such, the typical binary integer `1011` is equal in decimal to `1*1 + 2*1 + 4*0 + 8*1 = 11`, whereas in fixed point that uses 2 bits for fractions, it would be `0.25*1 + 0.5*1 + 1*0 + 2*1 = 2.75`. The conversion between fixed point and regular integers is very fast, as it's a matter of shifting it by the number of fraction bits.

This way of storing numbers has some noticable downsides:

1. In contrast to floating point that adapts its range by changing its precision, the fixed point encoding has constant precision and range.
2. Some operations, such as repeated multiplications, may very quickly render their results inaccurate.

Libfixmath uses signed 16.16 format on a 32-bit type called `fix16_t` to represent its numbers - that means 1 bit for sign, 15 bits for integral part and 16 bits for fractional part, thus precision is limited to `1/65535` and the number range spans from `-32767` to `32767`.
Since it's using 32-bit values, those operations are slower than they would be on a 16-bit counterpart, but offer range and precision acceptable for more use cases.

## Using libfixmath's functions

When using fixed point functions, stick to following rules:

1. Convert your values between fixed point and ints using `fix16_from_int()` and `fix16_to_int()` functions.
1. To convert `const` global values to fixed point in compile-time, use `F16()` macro instead.
1. Adding integers and `fix16_t` variables won't work correctly - be sure to convert either of them to another one's type.
1. It's safest to use fixmath's functions to do arithmetics - `fix16_add()`, `fix16_sub()`, `fix16_mul()`, `fix16_div()`, etc.
1. In some rare scenarios, you can skip those functions to make the code a bit more readable, e.g. when multiplying `fix16_t` and an `int`.
  When in doubt, refer to `fix16_*()` functions sources and see if it makes sense to skip the conversion.

> [!WARNING]
> You should only use `F16()` macro to convert your numbers only in places where `fix16_from_int()` won't work - using it in other contexts will make code noticably slower!

> [!NOTE]
> For performance reasons, `fix16_to_int()` doesn't round the numbers and truncates the fractional part instead.
> If you need rounding, add it on the side of your code, e.g. by doing `fix16_to_int(fValue + fHalf)`.

A sample code that calculates points on a circle is shown below:

```c
#include <fixmath/fix16.h>

// ...

// Prepare circle vertex positions.
// For better accuracy, supply your own precalculated points or more accurate sin/cos table
const fix16_t fHalf = fix16_one / 2;
fix16_t fAngle;
for(UBYTE ubPosIndex = 0; ubPosIndex < STAR_POSITION_COUNT; ++ubPosIndex) {
  fAngle = (fix16_pi*ubPosIndex*2) / STAR_POSITION_COUNT;
  WORD wSin = fix16_to_int(STAR_RADIUS * fix16_sin(fAngle) + fHalf);
  WORD wCos = fix16_to_int(STAR_RADIUS * fix16_cos(fAngle) + fHalf);
  s_pPositions[ubPosIndex].wX = wSin;
  s_pPositions[ubPosIndex].wY = wCos;
}
```

## Speeding up angles and sines

The `fix16_sin()` and other trigonometric functions are accepting their angles in fixed point format, which is a source for some avoidable slowdowns.
Instead, you might want to use *brads* - radians stored on a byte, where 0 means 0&deg; and 255 means almost 360&deg;.
This will limit your angle precision, but will also allow you to cache trigonometric function values in an array.
You can also reduce size of those arrays to your liking by limiting precision even further and making the angle range between `0..127`, `0..63` or some other upper limit of your choice - watch out for overflows and underflows, though.

## Using 16-bit base type for fixed points

If you don't need all the precision and range that `fix16_t` has to offer, you might want to limit it to gain some more speed.
You will need to roll your own functions though, or use another library since libfixmath doesn't offer changing its range or amount of bits in any of its parts.

Rolling your own functions isn't that hard, especially if you only need limited amount of functions, and can get away with only addition and multiplying by integers:

```c
typedef UWORD tFix10p6; // unsigned fixed point integer with 10.6 split

static inline tFix10p6 fix10p6Add(tFix10p6 a, tFix10p6 b) {return a + b; }
static inline tFix10p6 fix10p6FromUword(UWORD x) {return x << 6; }
static inline tFix10p6 fix10p6ToUword(UWORD x) {return x >> 6; }
```
