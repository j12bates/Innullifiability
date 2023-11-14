# Algorithm

When we're talking about finding all the innullifiable sets, we do have
to set some restrictions to define our search space. Typically we'll
look at sets of one particular length, $`N`$. Then we'll also restrict
the range of values that can appear in a set, by setting a maximum
value, $`M`$. It's these two parameters which define a search space of
sets. The program works with 'set records,' which are arrays that hold a
status for every set in a search space.[^1]

[^1]: The way it actually works in the program

The sets are represented like this:

$`[2, 3, 5]`$

There are three elements, or numbers, so we say $`N = 3`$. Let's make a
record of these sets with $`M = 5`$:

| Set           | Nullifiable?  |
| ---           | ---           |
| $`[1, 2, 3]`$ | Yes           |
| $`[1, 2, 4]`$ | No            |
| $`[1, 3, 4]`$ | Yes           |
| $`[2, 3, 4]`$ | No            |
| $`[1, 2, 5]`$ | No            |
| $`[1, 3, 5]`$ | No            |
| $`[2, 3, 5]`$ | Yes           |
| $`[1, 4, 5]`$ | Yes           |
| $`[2, 4, 5]`$ | No            |
| $`[3, 4, 5]`$ | No            |

The sets are in a *lexicographic order,* so we can compute the actual
numeric representation at each index using an algorithm, meaning we
don't need to store all those numbers and scan through them just to
access a simple bit. This also lets us change $`M`$ however we want by
simply growing or shrinking the record.

To find innullifiable sets, what we'll do is create a record and mark in
every *nullifiable* set, leaving the innullifiable ones behind. What we
could do is simply scan over a record, testing every set we come across.
A test would just have to go through every possible operation, returning
true once it gets zero, or false if it exhausts everything without ever
getting zero.

> diagram of scanning a record, looking at set $`[2, 4, 5, 7]`$, running
> it through a test, returning true, marking it back in

But what's happening with this test?

> diagram of merge-and-reducing the set, getting lots of smaller sets
> that then must be recursed on

The exhaustive test has to check absolutely *everything* until it finds
zero. There's no way to know ahead of time what'll work, as this is
computation, and it has to just keep on recursively testing stuff. This
sort of 'top-down' approach is very inefficient, and there's going to be
a lot of redundant testing too since there are far fewer reduced sets.

A better approach would be to go 'bottom-up,' or starting from known
nullifiable sets and algorithmically working out what sets would reduce
to them. How could we do this? Well, it's effectively the inverse of
merge-and-reduce, and we'll be basically taking a value and substituting
it with pairs of values that, when operated on, give the original back.

> cyclic arrow diagram: $`[2, 3, 5]`$ -> substitute '3' with '4' and '7'
> -> $`[2, 4, 5, 7]`$ (MUTATED set) -> $`7 - 4 = 3`$, so we can reduce

By sort-of 'un-merging' a value, we can find a bunch of sets that can
reduce to the original, and are thus more nullifiable sets. The pairs of
values we replace a single value with are what I call 'equivalent
pairs,' and the process of inserting them into a set is what I call
'mutation.'

> diagram of equivalent pairs of 3

Another thing we have to realize about nullifiable sets is that any
superset of one is also nullifiable. Why? Because if we have extra
values left over after getting zero, we can multiply them away and still
have zero.

> diagram: nullifying $`[2, 3, 5]`$ and then $`[2, 3, 5, 11]`$

So in this expansion process, we should also include supersets alongside
mutated sets, for completeness.

Now, we have a process for taking in nullifiable sets and expand them by
one element to get more, larger nullifiable sets. We can apply it across
a base case of length-3 nullifiable sets to get length-4 ones, and then
keep expanding as far up as we please.

> diagram: similar to naive scan, there's a source record where the set
> we're on gets piped to the expand program, creating mutations and
> supersets, which get marked in the destination

> continue: talk about how this isn't flawless bc of input sets, the
> optimization(s)
