# Solving Push Fight!
This repository contains code used to solve Push Fight, an abstract board game for
designed by Brett Picotte, available from https://pushfightgame.com/

## Rules of the Game

Push Fight is a game for two players on a board with 4 rows and 8 columns, with some
squares missing. Rows are labeled 1 through 4 (from bottom to top) and columns
are labeled *a* through *h* (from left to righ), as in Chess. See below:

![The empty board](images/empty-board.gif)

Each player controls five pieces: two round pieces and three square ones. The game
starts with the first player (in my version, red) placing their pieces anywhere on
the left half of the board (columns *a* through *d*), then the second player placing
their pieces on the right half of the board (columns *e* through *h*), and then the
first player moves first.

Below are two possible starting positions:

![One starting position](images/start1.gif)

![Another starting position](images/start2.gif)

During a turn, a player can make 0, 1 or 2 *moves* and then must end with exactly
1 *push*. If a player cannot push, he or she loses the game immediately. A move
consists of taking a piece of the player's color (either a square or a round one)
and moving it to any free square that is reachable through a sequence of horizontal
and vertical moves. Finally, a push involves taking a square piece of the player's
color and moving it exactly 1 space up, down, left or right, pushing pieces that are
in the way in the same direction. At least 1 piece other piece must be pushed, though it
doesn't matter if that piece belongs to the player or his or her opponent.

Below is an example of the first turn for the red player, starting from the second
starting positon above. (The image below is an animated GIF. You may have to click the
play button to view it.)

![Another starting position](images/start2-move1.gif)

Pieces cannot be pushed past the railing at the top and bottom of the board. However,
pieces can be pushed off the board on the left and right side (and off the top or bottom
of squares like *b3*  and *g2*). The first player who manages to push an opponent's
piece off the board, wins.

There is one final rule. After a player has performed their push move, a special piece
called an *anchor* is placed on the pushing piece. The opponent may not move that piece
during the next turn. This prevents obvious stalemates with players pushing the same
row of pieces back and forth.

## Notation of positions and moves
For convenience, let's define human-readable textual representation for
positions and moves. It's fairly easy to encode the board in text:

```
  abcdefgh
4   .OX..  4
3 ...oxY.. 3
2 ..Oox... 2
1  ..OX.   1
  abcdefgh
```

In this notation, `o` is a red circle (mover), `O` is a red square (pusher),
`x` is a blue circle (mover), `X` is a blue square (pusher), and `Y` is a blue
square with an anchor on top. Some tools also use `P` for a red square with
an anchor, but as will be explained above, we can usually assume without loss
of generality that the anchor is on a black square.

If we want a more compact representation, we can remove the board coordinates:

```
   .OX..
 ...oxY..
 ..Oox...
  ..OX.
```

and put all characters on a single line:

`.OX.....oxY....Oox.....OX.`

Now, we've reduced the board to a single 26-character string.

For moves, we can use a chess-like notation. For example, `c2-d2` represents a
push to the right, and `d1-a2,d3-c3,c2-d2` represents two moves followed by a
final push. Note that the push is necessarily the last move in the sequence,
and it's also clear from the board, since moves always end in an empty square
while pushes must move a piece to an occupied square.

## Number of positions
Push Fight is an interesting game because of how constrained it is: the board
consists of only 26 fields, with 10 total pieces (5 for each player). Although
it's possible to push a piece off the board, this immediately ends the game,
so in any intermediate position all 10 pieces will be present.

If we ignore the anchor for a moment, that means all positions can be 
represented as permutations of a 26-character string that consists of
16 `.` (empty spaces), 2 `o`s, 3 `O`s, 2 `x`s, and 3 `X`s. The total number
of permutations is:

$26! / 16! / 3! / 2! / 3! / 2! = 133,855,722,000$

where x! represents the [factorial](https://en.wikipedia.org/wiki/Factorial)
of x. However, this doesn't account for the placing of the anchor, which can
be on any of the 6 square pieces or not in play at all. Seemingly, this would
multiple the number of positions by 7. However, if we're willing to treat the
first turn specially and we only want to be able to represent positions from
turn 2 and on, we can assume the anchor is in place.

Additionally, we can assume without loss of generality that the anchor is
always on one of the second player's squares, since the game board is
symmetric, so we can always swap the colors of the pieces. So without loss
of generality, we will assume it's the red player's turn to move, and the
anchor is on one of the blue player's squares. This means we will replace
one of the `X`s with an `Y`, and all positions are now some permutation of
the string:

`................ooOOOxxXXY`

The total number of such permutations is:

$26! / 16! / 3! / 2! / 2! / 2! / 1! = 401,567,166,000$

## Solving the game

To solve the game, we need, at a minimum, to decide for each position if it's
winning, losing, or tying for the next player. Ideally, we would also like to
record either a winning move or the number of turns left until a win/loss 
occurs, so we can enumerate the different options.

Note that a position is tying if neither player can force a win. In that case,
the game would go indefinitely (assuming optimal play from both sides). It's
not clear if tying positions exist, but it seems likely.
