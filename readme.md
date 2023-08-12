# Innullifiability
This project is an attempt to learn more about a very specific mathsy-
feeling problem I came across playing an arithmetic game with a friend.
I'm not sure if it really has much to do with maths, and it could very
well be thoroughly in the 'computing' side of things. But I'm trying to
find a bunch of solutions, essentially, and try and find patterns and
interesting things.

Here's the problem written out very concisely: Find a set of positive
integers from which it is impossible to construct an arithmetic
expression, using each value only once, that evaluates to zero. Here, an
arithmetic expression can utilize bracketing, addition, subtraction,
multiplication, and division in the case where no remainder is
generated. Such a set is 'innullifiable'. If, on the other hand, it is
possible to construct a null-evaluating expression, a set is
'nullifiable'.

## Basic Logic
Through some basic reasoning, we can know that a set is nullifiable if
and only if there exist two non-overlapping expressions evaluating to
the same number: since we're using positive integers, the only way to
have zero be the result of a computation is to subtract a value from
itself, and once you get zero anywhere, you simply multiply by the
remaining values to nullify the set. With this simplified test for
nullifiability, we can easily say any set that contains the same value
twice is nullifiable, so we'll only look at sets without repetition. We
also know that any superset of a nullifiable set is also nullifiable.

This program is for finding all the innullifiable sets in a given search
space. Running an exhaustive test for every set in a search space would
be quite costly, so the approach actually used here is to generate new
nullifiable sets from smaller ones. There are two 'expansion phases' for
generating new sets from already-computed smaller ones: making
supersets, and introducing 'mutations' to the values. These mutations
essentially replace values with pairs of different values, from which a
simple expression evaluating to the original value can be made, thus
creating a completely new set that is still nullifiable. Importantly,
these mutations need not be introduced to sets generated through a
superset phase, as for those the original set would've undergone all
those same mutations too, making additional ones redundant.
