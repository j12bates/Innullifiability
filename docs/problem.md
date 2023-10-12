# Problem Explanation

You know the "four 4's" puzzle? This is a game where you're supposed to
find a way to compute as many whole numbers as possible, each using only
arithmetic operations and four instances of the number 4. Here are some
examples of how this might work:

- $`0 = 4 - 4 + 4 - 4`$
- $`1 = 4 / 4 * 4 / 4`$
- $`2 = 4 * 4 / (4 + 4)`$
- $`3 = (4 * 4 - 4) / 4`$
- $`4 = 4 + 4 * (4 - 4)`$
- $`5 = (4 * 4 + 4) / 4`$

It's maybe worth pointing out that there are many variations on this
puzzle: for example, some allow concatenation, letting you use '44' as a
valid number. But this version is purely arithmetic. In fact, we could
rephrase this "four 4's problem" like this: *construct an arithmetic
expression which evaluates to a given whole number, using only four
instances of the value 4.* Here, an *arithmetic expression* means an
expression using the four basic arithmetic operations, where the values
can be rearranged and bracketed.[^1]

[^1]: Formally, it's probably unnecessary to make this provision, as
bracketing is only used because of standard infix notation. In other
notations, notably pre/postfix, there is no need for bracketing at all,
as the order for specific operations is specified. So, in a pure sense,
bracketing doesn't really exist.

Similar to this is the 'numbers puzzle' from the British game show
'Countdown,' in which contestants are given a set of six values, as well
as a 'target value.' Their goal is similar to the four 4's problem, in
that they're attempting to use arithmetic to get their six numbers equal
to the target value, using each value only once. There's one important
difference though, namely that it's not required that a solution uses
every value, unlike the four 4's problem which requires all four values
be used, even if it makes obtaining a solution harder.[^2]

[^2]: There are some additional details about this game, like that the
target number is always three-digits and there's a distinction between
'big' and 'small' numbers in the source set, but we can ignore that
here.

Here's an example 'Countdown problem':
- $`S: [1, 3, 7, 10, 25, 50]`$
- $`T: 765`$

Typically, a solution would be given as a series of operations,
performed on two unused values, each of which may be from either the set
or the result of a previous operation, until the target number is
reached, like this:
1. $`25 - 10 = 15`$
2. $`1 + 50 = 51`$
3. $`15 * 51 = 765`$

Notice that as we perform these operations, one by one, we merge two of
our possible input values into one, reducing our total set of input
values by one each time, until the value we want appears. We can apply
this metaphor to the four 4's problem as well, so that instead of
constructing an expression, we're incrementally merging values, and a
solution would mean we've reduced it down to a single value that is the
target whole number. We can also apply the metaphor of arithmetic
expressions to this, so that a solution would be an expression that uses
each given value up to once, evaluating to the target.

So far, we've looked at two different puzzle-type games, both with this
idea of having a source set of numbers and a target result, where the
player must find a way to arithmetically bring them together. We've seen
that they can be phrased in terms of constructing an arithmetic
expression evaluating to the target, or by doing operations one at a
time to reduce the input until the target appears. Just to show that
these metaphors are identical, let's look at a more general version of
these puzzles, where there is an arbitrary set of N whole numbers and a
whole number target, and we'll also add the requirement from four 4's
that all values be used once and only once:

$`[2, 9, 13, 16]`$\
Target: $`21`$

Using the 'merge and reduce' method:

$`2 * 9 = 18`$\
$`\Rightarrow [13, 16, 18]`$

$`16 - 13 = 3`$\
$`\Rightarrow [3, 18]`$

$`3 + 18 = 21`$\
$`\Rightarrow [21]`$\
singleton member is target, solved

Rewriting as an arithmetic expression:

$`(2 * 9) + (16 - 13) = 21`$

Now I will introduce 'Nullifiability,' the actual subject of my
research, which I like to think of as a trivial case to this generalized
puzzle.

Obviously, some puzzles will be naturally unsolvable, and this can
depend a lot on the target value. If it's something like a high prime
number, it's less likely an arbitrary set can be solved for it. So then,
what would be the most solvable target? The answer is, trivially, zero.
I'm pretty sure, at least. I did a bit of number crunching for one
particular case, but I feel like this is probably general. To me at
least, this makes intuitive sense, and I'll explain a bit of why I think
so.

The interesting thing is that solving to get zero will always follow one
particular pattern: if there's no zero initially, there'll be a way to
get two of the same value, then those are subtracted to get zero, and
the zero is multiplied by all the remaining values to end off with zero.
The only way to get zero through arithmetic is to subtract a number from
itself: there's no way to add or multiply positive numbers to get zero,
and for a quotient to be zero the dividend must be zero to start with.
Because of the ability to multiply any remaining values by zero once
zero is initially reached, there is no distinction made by the 'once and
only once' rule when dealing with a target of zero.

Here are some examples:
- $`[2, 6, 7, 8]: (8 - 2 - 6) * 7 = 0`$
- $`[4, 7, 12, 15]: (7 - 4) - (15 - 12) = 0`$
- $`[5, 9, 15, 17, 18]: (15 - 5) - (9 + 18 - 17) = 0`$

So, this is 'Arithmetic Nullification': find a way to take a given input
set and arithmetically get it to zero. This can be expressed either by
constructing an arithmetic expression or by combining and reducing, and
it needn't be specified whether or not to require all values be used
explicitly. A set with which this can be done will have the property of
being 'Nullifiable.'

Nullifiability is a common property with sets. In fact, every set with a
repeated value is trivially nullifiable. So, what I'm particularly
interested in are sets which don't have this property, the *unusual*
ones, in a sense. For these sets, the *Innullifiable* sets, there is
absolutely *no way* to get zero. Since getting zero requires getting two
of the same value, this also means there's no way to get the same value
two different ways.

Trying to nullify an innullifiable set can feel sorta 'almost-y,' in
that trying different operations will produce results very close to each
other, but never quite equal. Here are some examples of innullifiable
sets, and I encourage the reader to try to find a way to get zero:
- $`[1, 4, 6, 8]`$
- $`[8, 10, 13, 16, 17]`$
- $`[3, 7, 8, 16, 22]`$
- $`[12, 19, 23, 27, 32, 33]`$

So to summarize, Nullifiability is when a set can be rearranged to get
zero, specifically when a null-evaluating (equal to zero) arithmetic
expression can be constructed, or when values can be merged together
using those operations to get zero. Since this is a rather common
property, I'm interested in the sets that don't follow this, or are
Innullifiable.
