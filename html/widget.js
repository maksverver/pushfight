// Implements the Push Fight game board widget.
//
// The widget should support three functions:
//
//  - allow a user to create/edit a position
//  - allow user to select moves to play during a turn
//  - visualize the moves during a turn
//

'use strict';

function initializeWidget() {

  const boardElem = document.getElementById('board');
  const paletteElem = document.getElementById('palette');
  const statusTextElem = document.getElementById('status-text');
  const permIndex = document.getElementById('perm-index');

  // One of: 'setup', 'move'
  let mode = null;

  // In setup mode:
  let currentPermutationIndex = null;

  // In move mode: player to move and number of moves left before push.
  let nextPlayer = -1;
  let movesLeft = -1;

  let currentTurn = [];  // list of source->destination pairs
  let pastTurns = [];

  let fields = [];
  let pieces = Array.from(INITIAL_PIECES);
  let selectedField = -1;
  let moveTargets = [];
  let pushTargets = [];

  function setPaletteVisible(visible) {
    paletteElem.style.display = visible ? null : 'none';
  }

  function isSelectable(i) {
    return (
      nextPlayer !== -1 &&
      getPieceType(pieces[i]) !== PieceType.NONE &&
      getColor(pieces[i]) === nextPlayer
    );
  }

  function setMoveTargets(newMoveTargets) {
  for (let i of moveTargets) fields[i].setMoveTarget(false);
    moveTargets = newMoveTargets;
    for (let i of moveTargets) fields[i].setMoveTarget(true);
  }

  function setPushTargets(newPushTargets) {
    for (let i of pushTargets) fields[i].setPushTarget(false);
    pushTargets = newPushTargets;
    for (let i of pushTargets) fields[i].setPushTarget(true);
  }

  function setSelectedField(i) {
    if (selectedField !== -1) fields[selectedField].setSelected(false);
    selectedField = i;
    if (selectedField !== -1) fields[selectedField].setSelected(true);

    if (mode === 'setup') {
      setPaletteVisible(selectedField !== -1);
    }
    if (mode === 'move') {
      setMoveTargets(i === -1 || currentTurn.length >= 2 ? [] :
            Array.from(generateMoveDestinations(pieces, i)));
      setPushTargets(i === -1 || getPieceType(pieces[i]) !== PieceType.PUSHER ? [] :
          Array.from(findPushDestinations(pieces, i)));
    }
  }

  function handleFieldClick(i) {
    if (selectedField === -1 || (mode === 'setup' && selectedField !== i)) {
      if (fields[i].isSelectable) {
        setSelectedField(i);
      }
    } else if (i === selectedField) {
      // Clicking the currently-selected field deselects it.
      setSelectedField(-1);
    } else if (fields[i].isPushTarget) {
      const r = FIELD_ROW[selectedField];
      const c = FIELD_COL[selectedField];
      for (let d = 0; d < 4; ++d) {
        if (getFieldIndex(r + DR[d], c + DC[d]) === i) {
          addMove(selectedField, i);  // push
          break;
        }
      }
    } else if (fields[i].isMoveTarget) {
      addMove(selectedField, i);  // move
    }
  }

  class Field {
    constructor(div) {
      this.div = div;
      this.moveTargetDiv = document.createElement('div');
      this.moveTargetDiv.className = 'move-target';
      this.moveTargetDiv.style.display = 'none';
      this.div.appendChild(this.moveTargetDiv);
      this.pushTargetDiv = document.createElement('div');
      this.pushTargetDiv.className = 'push-target';
      this.pushTargetDiv.style.display = 'none';
      this.div.appendChild(this.pushTargetDiv);
      this.pieceDiv = document.createElement('div');
      this.pieceDiv.className = 'piece';
      this.pieceDiv.style.display = 'none';
      this.div.appendChild(this.pieceDiv);
      this.isSelectable = false;
      this.isMoveTarget = false;
      this.isPushTarget = false;
    }

    setSelected(selected) {
      this.div.classList.toggle('selected', selected);
    }

    reset(piece, selectable) {
      this.isSelectable = selectable;
      const pieceType = getPieceType(piece);
      if (pieceType === PieceType.NONE) {
        this.pieceDiv.style.display = 'none';
      } else {
        const color = getColor(piece);
        this.pieceDiv.style.display = null;
        this.pieceDiv.classList.toggle('circle', pieceType === PieceType.MOVER);
        this.pieceDiv.classList.toggle('square', pieceType === PieceType.PUSHER);
        this.pieceDiv.classList.toggle('red', color === 0);
        this.pieceDiv.classList.toggle('blue', color === 1);
        this.pieceDiv.textContent = pieceType === PieceType.ANCHOR ? '+' : '';
      }
      this.updateSelectable();
      // TODO: also append a push-origin child (only if not a move/push-target selected?)
    }

    setMoveTarget(value) {
      this.isMoveTarget = value;
      this.moveTargetDiv.style.display = value ? null : 'none';
      this.updateSelectable();
    }

    setPushTarget(value) {
      this.isPushTarget = value;
      this.pushTargetDiv.style.display = value ? null : 'none';
      this.updateSelectable();
    }

    updateSelectable() {
      this.div.style.cursor = this.isSelectable || this.isPushTarget || this.isMoveTarget ? 'pointer' : null;
    }
  }

  function addMove(src, dst) {
    const isPush = pieces[dst] !== 0;
    currentTurn.push([src, dst]);
    executeMove(pieces, src, dst);
    if (isPush) {
      pastTurns.push(currentTurn);
      currentTurn = [];
      nextPlayer = 1 - nextPlayer;
    }
    reset('move');
  }

  function createDiv(className) {
    const div = document.createElement('div');
    div.className = className;
    return div;
  }

  for (let r = 0; r < H; ++r) {
    for (let c = 0; c < W; ++c) {
      const i = FIELD_INDEX[r][c];
      if (i !== -1) {
        const fieldDiv = document.createElement('div');
        if (r !== 0     && FIELD_INDEX[r - 1][c] === -1) fieldDiv.appendChild(createDiv('danger top'));
        if (c === 0     || FIELD_INDEX[r][c - 1] === -1) fieldDiv.appendChild(createDiv('danger left'));
        if (r !== H -1  && FIELD_INDEX[r + 1][c] === -1) fieldDiv.appendChild(createDiv('danger bottom'));
        if (c === W - 1 || FIELD_INDEX[r][c + 1] === -1) fieldDiv.appendChild(createDiv('danger right'));

        fieldDiv.className = 'field';
        fieldDiv.style.gridRowStart = r + 1;
        fieldDiv.style.gridColumnStart = c + 1;
        fieldDiv.classList.toggle('border-top', r === 0);
        fieldDiv.classList.toggle('border-bottom', r === H - 1);
        fieldDiv.onclick = () => handleFieldClick(i);
        boardElem.appendChild(fieldDiv);
        fields.push(new Field(fieldDiv));
      }
    }
  }

  function reset(newMode) {
    mode = newMode;
    if (mode === 'setup') {
      nextPlayer = -1;
    }
    setPaletteVisible(false);
    setSelectedField(-1);
    for (let i = 0; i < fields.length; ++i) {
      fields[i].reset(pieces[i], mode === 'setup' || isSelectable(i));
    }

    if (nextPlayer !== -1) {
      const historyList = document.getElementById('history-list');
      historyList.replaceChildren();
      for (const turn of pastTurns) {
        const li = document.createElement('li');
        li.textContent = turn.map(([src, dst]) => formatMove(src, dst)).join(',');
        historyList.appendChild(li);
      }
      const li = document.createElement('li');
      li.textContent =
          currentTurn.length === 0 ? '…' :
          currentTurn.map(([src, dst]) => formatMove(src, dst)).join(',');
      historyList.appendChild(li);
    }

    updateStatus();
  }

  window.choosePiece = function(color, type) {
    if (selectedField >= 0) {
      pieces[selectedField] = type === PieceType.NONE ? 0 : makePiece(color, type);
      fields[selectedField].reset(pieces[selectedField], true);
      updateStatus();
    }
  }

  function showStartMoving(show) {
    document.getElementById('moving-link').style.display = show ? 'inline' : 'none';
  }

  function updateNextPlayer() {
    for (const piece of pieces) {
      if (getPieceType(piece) == PieceType.ANCHOR) {
        nextPlayer = 1 - getColor(piece);
      }
    }
  }

  function updateStatus() {
    let redMovers = 0;
    let redPushers = 0;
    let redAnchors = 0;
    let bluePushers = 0;
    let blueMovers = 0;
    let blueAnchors = 0;
    for (let piece of pieces) {
      switch (getPieceType(piece)) {
        case PieceType.MOVER:
          if (getColor(piece) === 0) ++redMovers; else ++blueMovers;
          break;
        case PieceType.PUSHER:
          if (getColor(piece) === 0) ++redPushers; else ++bluePushers;
          break;
        case PieceType.ANCHOR:
          if (getColor(piece) === 0) ++redAnchors; else ++blueAnchors;
          break;
      }
    }
    let error = null;
    if (redMovers + redPushers + redAnchors + bluePushers + blueMovers + blueAnchors == 0) {
      error = 'Place pieces to create the initial position.';
    } else if (redMovers < 2) {
      error = 'Too few red movers.';
    } else if (redMovers > 2) {
      error = 'Too many red movers.';
    } else if (redPushers + redAnchors < 3) {
      error = 'Too few red pushers.';
    } else if (redPushers + redAnchors > 3) {
      error = 'Too many red pushers.';
    } else if (blueMovers < 2) {
      error = 'Too few blue movers.';
    } else if (blueMovers > 2) {
      error = 'Too many blue movers.';
    } else if (bluePushers + blueAnchors < 3) {
      error = 'Too few blue pushers.';
    } else if (bluePushers + blueAnchors > 3) {
      error = 'Too many blue pushers.';
    } else if (redAnchors + blueAnchors !== 1) {
      error = 'Exactly one pusher should be anchored.';
    }
    showStartMoving(error == null && nextPlayer === -1);
    if (error != null) {
      statusTextElem.innerHTML = error;
      permIndex.innerHTML = 'Index indeterminate';
      currentPermutationIndex = '';
      nextPlayer = -1;
    } else {
      statusTextElem.innerHTML = 'Position is valid!';

      // Calculate permutation.
      const nextPlayer = redAnchors > 0 ? 1 : 0;
      function getPermValue(piece) {
        const type = getPieceType(piece);
        if(type === PieceType.NONE) return 0;
        const color = getColor(piece);
        return color === nextPlayer ? type : type + 2;
      }
      const perm = pieces.map(getPermValue);

      currentPermutationIndex = indexOfPerm(perm);
      permIndex.innerHTML = currentPermutationIndex;
    }
  }

  window.changePermutation = function() {
    const result = prompt('Permutation string or index:', currentPermutationIndex);
    if (result == null) return;

    if (result.match(/^\d+$/)) {
      const index = Number(result);
      if (index >= totalPerms) {
        alert("Permutation index out of range: " + result);
        return;
      }

      function getPieceValue(i) {
        if (i === 0) return makePiece(0, PieceType.NONE);
        if (i === 1) return makePiece(0, PieceType.MOVER);
        if (i === 2) return makePiece(0, PieceType.PUSHER);
        if (i === 3) return makePiece(1, PieceType.MOVER);
        if (i === 4) return makePiece(1, PieceType.PUSHER);
        if (i === 5) return makePiece(1, PieceType.ANCHOR);
      }
      const perm = permAtIndex(index);
      pieces = perm.map(getPieceValue);
    } else if (result.match(/^[.oOPxXY]{26}$/)) {
      function getPieceValue(i) {
        if (i === '.') return makePiece(0, PieceType.NONE);
        if (i === 'o') return makePiece(0, PieceType.MOVER);
        if (i === 'O') return makePiece(0, PieceType.PUSHER);
        if (i === 'P') return makePiece(0, PieceType.ANCHOR);
        if (i === 'x') return makePiece(1, PieceType.MOVER);
        if (i === 'X') return makePiece(1, PieceType.PUSHER);
        if (i === 'Y') return makePiece(1, PieceType.ANCHOR);
      }
      pieces = [...result].map(getPieceValue);
    } else {
      alert("Don't know how to parse: " + result);
      return;
    }
    reset('setup');
  }

  window.flipColors = function() {
    for (let i = 0; i < pieces.length; ++i) {
      const type = getPieceType(pieces[i]);
      if (type !== PieceType.NONE) {
        pieces[i] = makePiece(1 - getColor(pieces[i]), type);
      }
    }
    reset('setup');
  }

  window.startMoving = function() {
    document.getElementById('history').style.display = 'block';
    updateNextPlayer();
    currentTurn = [];
    reset('move');
  }

  reset('setup');
}
