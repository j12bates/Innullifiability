-- so basically, I want the program to essentially "dream up" sets
-- instead of checking sets one at a time
-- it should start from the set [0], and then come up with sets that
-- can be made equivalent to a particular set member; it will build up
-- and up until it reaches 4 (or however many) elements
-- sets less than the max length can just be checked if they're within
-- other sets

-- When I say "equivalent" when I refer to sets, it means that it can
-- be made equal to all the same numbers (and possibly more) as the
-- original given the allowed arithmetic operations

-- qs
-- quicksort
qs :: Ord a => [a] -> [a]
qs []       = []
qs (x:xs)   = qs lt ++ [x] ++ qs gt
                where   lt = [n | n <- xs, n <= x]
                        gt = [n | n <- xs, n > x]

-- uq
-- get rid of duplicate items in a list, and also quicksort
uq :: Ord a => [a] -> [a]
uq []       = []
uq (x:xs)   = uq lt ++ [x] ++ uq gt
                where   lt = [n | n <- xs, n < x]
                        gt = [n | n <- xs, n > x]

-- has
-- does a set contain a value?
has :: Eq a => a -> [a] -> Bool
has _ []        = False
has a (b:bs)    | a == b    = True
                | otherwise = has a bs

-- superset
-- does a list contain all the elements of another?
superset :: Eq a => [a] -> [a] -> Bool
superset [] _       = True
superset (a:as) bs  | has a bs  = superset as bs
                    | otherwise = False

-- range
-- maximum value
range :: Int
range   = 15

-- safeDiv
-- return x / y if result is whole, otherwise duplicate, no divide by zero error
safeDiv :: Int -> Int -> Int
safeDiv x y | y == 0            = x + y
            | x `mod` y /= 0    = x + y
            | otherwise         = x `div` y

-- canEq
-- is a pair of numbers equivalent to a particular number
canEq :: (Int, Int) -> Int -> Bool
canEq (a, b) n  =  a + b == n
                || a - b == n
                || b - a == n
                || a * b == n
                || safeDiv a b == n
                || safeDiv b a == n

-- eqPairs
-- find all pairs that can be made equivalent to a particular number
eqPairs :: Int -> [[Int]]
eqPairs n   = [[a, b] | a <- [0..range], b <- [(a + 1)..range], canEq (a, b) n, n /= a && n /= b]

-- expandFirst
-- enumerate all equivalent sets expanded by one
expandOne :: [Int] -> [[Int]]
expandOne ns    = [qs (take i ns ++ exp ++ drop (i + 1) ns) | i <- [0..(length ns - 1)], exp <- eqPairs (ns !! i)]

-- expandM
-- enumerate all equivalent sets expanded a certain number of times
expandM :: Int -> [Int] -> [[Int]]
expandM 0 _     = []
expandM 1 ns    = expandOne ns
expandM m ns    = uq (concat [expandM (m - 1) exp | exp <- expandOne ns])
