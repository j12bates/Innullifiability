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
There'd be either 3 or 4 ways to merge two values (addition,
subtraction, multiplication, maybe division), and that'd repeat all the
way down. And when the source set size goes up, the number of pairs to
test increases as well, meaning this test would be very costly. And
there isn't really a way to sort-of 'be smart' about what values are
chosen. This is computation, and we can't know ahead of time what'll
work.

How could we improve this? Well, when we're merge-and-reduce-ing a set,
the set, naturally, ends up with fewer values. And of course, when you
have fewer values, you end up with fewer overall possible sets. This
leads to one reason why the exhaustive-test approach is so bad: a lot of
the work is redundant, since it leads to a much smaller group of sets
that repeat themselves. But we can actually use this to our advantage:
instead of going *top-down*, and starting from a set and trying to get
zero, we can go for a *bottom-up* approach, one where we start from zero
and bootstrap up to sets of the target length.

In order to do this *bottom-up* method, we need to have something that
effectively does the *inverse* of merge-and-reduce. This would be an
algorithm that takes a 'reduced' set, and outputs everything which could
reduce to that. It'd effectively be 'unmerging' values back into pairs
of values, finding every pair of numbers that, when operated on, give
the original value. This process is what I call introducing *mutations*,
and when done on every value in the set, this process effectively acts
as the inverse to merge-and-reduce, with one caveat we'll look at in a
moment.

But there's one more thing to account for: since the 'once and only
once' rule doesn't apply here, meaning when we try nullifying the set we
can choose to ignore values (because in the end they'll be multiplied by
zero anyways), we also have to introduce values that could go unused.
The rule is simple: just enumerate all the supersets of the input set.

> continue talking here about how then it's unnecessary to pipe
> supersets into mutation program, also mention how this will continue
> going up and up until target

> next up... why this isn't perfect and we still have to run exhaustive
> test at the end
