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
'Countdown,' in which contestants are given a set of six numbers, some
large, some small, as well as a 'target value.' Their goal is similar to
the four 4's problem, in that they're attempting to use arithmetic to
get their six numbers equal to the target value, using each value only
once. There's one important difference though, namely that it's not
required that a solution uses every value, unlike the four 4's problem
which requires all four values be used, even if it makes obtaining a
solution harder. <<maybe footnote this?>>

Anyways, here's an example 'countdown problem':
$$
S: [1, 3, 7, 10, 25, 50]
T: 765
$$

Typically, a solution would be given as a series of binary operations,
each operand being an unused value from either the set or the result of
a previous operation, like this:
$$
25 - 10 = 15
1 + 50 = 51
15 * 51 = 765
$$

Notice that as we perform these operations, one by one, we merge two of
our possible input values into one, reducing our total set of input
values by one each time, until the value we want appears. We can apply
this metaphor to the four 4's problem as well, so that instead of
constructing an expression, we're incrementally merging values, and a
solution would mean we've reduced it down to a single value, which is
the target whole number.


