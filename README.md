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
