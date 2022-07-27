#!/bin/sh

DIR=$(readlink -f $(dirname "$0"))
VERSION=1.0.0
BASENAME=pushfight
DIRNAME=$BASENAME-$VERSION
ZIPNAME=$BASENAME-$VERSION-windows.zip

set -e -E -o pipefail

# Build windows binaries
echo 'Building executables...'
make clean
make -f Makefile.mingw -j lookup-min pushfight-standalone-server

# Build statically served files (HTML, JS, etc.)
echo 'Building static files...'
(cd html && npm run build)

echo 'Populating temporary directory...'
TEMPDIR=$(mktemp -d)
trap "rm -R '$TEMPDIR'" EXIT
mkdir "$TEMPDIR/$DIRNAME"
cp lookup-min.exe pushfight-standalone-server.exe "$TEMPDIR/$DIRNAME/"
cp -R html/dist "$TEMPDIR/$DIRNAME/static"

cat >"$TEMPDIR/$DIRNAME/README.txt" <<EOF
Push Fight analyzer version $VERSION for Windows

Copyright 2022 by Maks Verver.

For more information, or to download the source code, please visit:
https://github.com/maksverver/pushfight/


WHAT IS PUSH FIGHT?
-------------------

Push Fight is an abstract board game for two players designed by Brett Picotte.
For more information about the game, or if you want to buy a physical copy to
play with, visit: https://pushfightgame.com/


RULES OF THE GAME
-----------------

Push Fight is a game for two players on a board with 4 rows and 8 columns,
with some squares missing. Rows are labeled 1 through 4 (from bottom to top)
and columns are labeled a through h (from left to right), as in Chess.

    a b c d e f g h
        ---------
  4     . . . . .   4
  3 . . . . . . . . 3
  2 . . . . . . . . 2
  1   . . . . .     1
      ---------
    a b c d e f g h

Each player controls five pieces: two round pieces and three square ones. The
game starts with the first player (in my version, red) placing their pieces
anywhere on the left half of the board (columns a through d), then the second
player placing their pieces on the right half of the board (columns e through
h), and then the red player plays the first turn.

During a turn, a player can make 0, 1 or 2 moves and then must end with exactly
1 push. If a player cannot push, he or she loses the game immediately. A move
consists of taking a piece of the player's color (either a square or a round
one) and moving it to any free square that is reachable through a sequence of
horizontal and vertical moves. Finally, a push involves taking a square piece
of the player's color and moving it exactly 1 space up, down, left or right,
pushing pieces that are in the way in the same direction. At least 1 other
piece must be pushed, though it doesn't matter what color or shape the pushed
pieces are.

Pieces cannot be pushed past the railing at the top and bottom of the board.
However, pieces can be pushed off the board on the left and right side (and off
the top or bottom of squares like b3 and g2). The first player who manages to
push an opponent's piece off the board, wins.

There is one final rule. After a player has performed their push move, a special
piece called an anchor is placed on the pushing piece. The opponent may not push
that piece during the next turn.


COMPLETING THE INSTALLATION
---------------------------

To use the executables in this distribution, you need to download a data file
called minimized.bin.xz (4.3 GB) and place it in this directory. The file is
available here: https://styx.verver.ch/~maks/pushfight/minimized.bin.xz

If that URL no longer works, check https://github.com/maksverver/pushfight
for updated instructions.

Note that although .xz is a compression format, you do not have to extract the
contents of the file; it can be used directly. However, extracting it is an
option, and may lead to faster analysis. Note however, that if you choose to
extract this file, it will take around 68 GB of disk space. You can extract .xz
files with XZ Utils (https://tukaani.org/xz/) or 7-Zip (https://www.7-zip.org/).


AVAILABLE EXECUTABLES
---------------------

This release contains two executables:

  1. lookup-min.exe, which can be used to analyze any position on the command
     line. For a given position, it prints all successors sorted by valuation.

  2. pushfight-standalone-server.exe, which can be used to run a standalone
     version of the web app that is also available at:
     https://styx.verver.ch/pushfight/

     After downloading minimized.bin.xz, start pushfight-standalone-server.exe,
     and then open http://localhost:8080/ in your browser to interact with the
     app.

     Run "pushfight-standalone-server.exe --help" to get a list of command line
     options, including which host and port to serve on.

     Note that this server is meant for local use only. It's not meant to serve
     large amounts of production traffic.
EOF

echo 'Ready to archive the following files:'
(cd "$TEMPDIR" && find "$DIRNAME")

(cd "$TEMPDIR" && zip -r -9 "$DIR"/"$ZIPNAME" "$DIRNAME")
