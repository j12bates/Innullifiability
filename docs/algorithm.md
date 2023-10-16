# Algorithm

When it comes to actually enumerating innullifiabile sets, there are a
few different plausible methods with varying complexity and runtime. The
simple and naive approach would be to enumerate all possible sets and
run an exhaustive test for nullifiability, checking every possible
arithmetic operation until zero is reached or everything's been checked.
But we'll see that this method is quite redundant and inefficient.

When we're talking about finding all the innullifiable sets, we do have
to set some restrictions to define our search space. Typically we'll
look at sets of one particular length, $`N`$, whose highest value falls
within a certain range. We call a set's highest value its M-value, so
this range will be the M-range. It's these two parameters which define
a search space of sets.

So, going back to algorithms, we could run a test over all sets in our
search space. This test would have to try out every possible way of
merging and reducing the input set before it can say it's innullifiable.
There'd be between 3 and 4 ways to merge two values, and that'd repeat
all the way down. And when the source set size goes up, the number of
pairs to test increases as well, meaning this test would be very costly.

Instead of this kind-of *top-down* approach, I'm taking a more
*bottom-up* approach.
