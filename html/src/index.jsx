import React from "react";
import ReactDOM from 'react-dom/client';

import {
  totalPerms, indexOfPerm, permAtIndex,
} from './perms.js';

import {
  H, W, DR, DC, FIELD_COUNT, FIELD_INDEX, getFieldIndex, FIELD_ROW, FIELD_COL,
  PieceType, getPieceType, getPlayerColor, makePiece,
  EMPTY_PIECES, INITIAL_PIECES, validatePieces, PiecesValidity,
  RED_PLAYER, BLUE_PLAYER, NO_PIECE,
  RED_MOVER, RED_PUSHER, RED_ANCHOR,
  BLUE_MOVER, BLUE_PUSHER, BLUE_ANCHOR,
  invertColors,
  permToPieces, piecesToPerm,
} from './board.js';


import {
  generateMoveDestinations, findPushDestinations,
  parseField, parseMove, parseTurn,
  formatField, formatMove, formatTurn,
} from './moves.js';

const reactStrictMode = true;

const LOOKUP_URL = window.location.hostname === 'localhost' ? 'http://localhost:8003/' : 'lookup/';

async function analyzePosition(pieces) {
  const url = LOOKUP_URL + '?perm=' + encodeURIComponent(piecesToNormalizedString(pieces));
  const response = await fetch(url);
  if (response.status !== 200) {
    const error = await response.text();
    throw new Error(`${error} (HTTP status ${response.status})`);
  }
  const obj = await response.json();
  // Note: this only partially validates the response object.
  if (typeof obj !== 'object' || typeof obj.status != 'string' || !Array.isArray(obj.successors)) {
    throw new Error('Invalid response format.');
  }
  return obj;
}

class Board extends React.Component {
  constructor(props) {
    super(props);

    this.rowCoordinates = [];
    for (let r = 0; r < H; ++r) {
      for (let c of [1, W + 2]) {
        this.rowCoordinates.push(
            <div key={`${r},${c}`} className="coord"
                style={{gridRow: r + 2, gridColumn: c}}>
              {String(4 - r)}
            </div>
        );
      }
    }

    this.columnCoordinates = [];
    for (let c = 0; c < W; ++c) {
      for (let r of [1, H + 2]) {
        this.columnCoordinates.push(
          <div key={`${r},${c}`} className="coord"
              style={{gridRow: r, gridColumn: c + 2}}>
            {String.fromCharCode('a'.charCodeAt(0) + c)}
          </div>
        );
      }
    }
  }

  render() {
    const {selectedFieldIndex, isSelectable, renderField, onFieldClick} = this.props;
    const fields = [];
    for (let i = 0; i < FIELD_COUNT; ++i) {
      const r = FIELD_ROW[i];
      const c = FIELD_COL[i];
      // Possible optimization: cache field click handlers to avoid unnecessary
      // updates (assuming Field is implemented as a PureComponent).
      fields.push(
        <Field key={`${r},${c}`} r={r} c={c} i={i}
            selectable={isSelectable(i)}
            selected={selectedFieldIndex === i}
            onClick={() => onFieldClick(i, r, c)}>
          {renderField(i, r, c)}
        </Field>
      );
    }

    return (
      <div className="board">
        {this.rowCoordinates}
        {this.columnCoordinates}
        {fields}
        {this.props.children}
      </div>
    );
  }
}

// Version of the board used during setup.
function SetupBoard({pieces, onPiecesChange, onStart}) {
  const [selectedFieldIndex, setSelectedFieldIndex] = React.useState(-1);

  function handleChangePermutation() {
    const response = prompt('Permutation index or string:');
    if (response == null) return;
    const [pieces, error] = parsePieces(response);
    if (pieces != null) {
      onPiecesChange(pieces);
    } else {
      alert(error);
    }
  }

  function handleInvertColors() {
    onPiecesChange(invertColors(pieces));
  }

  function handleRandomize() {
    const index = Math.floor(Math.random() * totalPerms);
    onPiecesChange(permToPieces(permAtIndex(index)));
  }

  function handlePieceSelect(piece, i) {
    setSelectedFieldIndex(-1);
    onPiecesChange(replacePiece(pieces, i, piece));
  }

  function handleFieldClick(i) {
    setSelectedFieldIndex((j) => j === i ? -1 : i);
  }

  function renderField(i) {
    return <Piece piece={pieces[i]}/>;
  }

  return (
    <div className="board-holder">
      <Board renderField={renderField} isSelectable={() => true} onFieldClick={handleFieldClick}>
        <PiecePalette fieldIndex={selectedFieldIndex} onSelect={handlePieceSelect} />
      </Board>
      <div className="setup-panel side-panel">
        <h1>Push Fight Setup</h1>
        <p>
          Change the initial placement of the pieces by clicking on fields.
          Press the <em>Start game</em> button to start playing from the chosen position.
        </p>
        <h4>Permutation string</h4>
        <code>{piecesToString(pieces)}</code>
        <p className="buttons">
          <button onClick={handleChangePermutation}>Enter permutation</button>
          <button onClick={handleInvertColors}>Invert colors</button>
          <button onClick={handleRandomize}>Randomize</button>
          <button disabled={onStart == null} onClick={onStart}>Start game</button>
        </p>
      </div>
    </div>
  );
}

// Component intended to be a simple div that wraps a piece on the board,
// while supporting animation.
//
// The animation prop is an object with two keys: "keyframes" and "options".
// These are the arguments passed to animate() on the div whenever the prop
// changes.
class PieceHolder extends React.PureComponent {
  ref = React.createRef();

  // The last value of the animation prop.
  lastAnimation = null;

  // The Animation object returned from animate()
  currentAnimation = null;

  constructor(props) {
    super(props);
  }

  componentDidMount() {
    this.maybeAnimate();
  }

  componentDidUpdate() {
    this.maybeAnimate();
  }

  maybeAnimate() {
    const {animation} = this.props;
    if (animation == this.lastAnimation) return;
    if (this.currentAnimation) {
      this.currentAnimation.finish();
    }
    this.lastAnimation = animation;
    if (animation == null) {
      this.currentAnimation = null;
    } else {
      const {keyframes, options} = animation;
      this.currentAnimation = this.ref.current.animate(keyframes, options);
    }
  }

  render() {
    return (
      <div className="piece-holder" ref={this.ref}>
        {this.props.children}
      </div>
    );
  }
}

// Version of the board used when playing.
function PlayBoard({pieces, nextPlayer, moves, push, pieceAnimations, onMove, onPush}) {
  const [selectedFieldIndex, setSelectedFieldIndex] = React.useState(-1);

  const pieceRefs = [];
  for (let i = 0; i < FIELD_COUNT; ++i) pieceRefs.push(React.useRef());

  const isMoveTarget = {};
  const isPushTarget = {};
  if (selectedFieldIndex !== -1 && push == null) {
    if (moves.length < 2) {
      for (const field of generateMoveDestinations(pieces, selectedFieldIndex)) {
        isMoveTarget[field] = true;
      }
    } else if (selectedFieldIndex === moves[moves.length - 1][1]) {
      isMoveTarget[moves[moves.length - 1][0]] = true;
    }
    if (getPieceType(pieces[selectedFieldIndex]) === PieceType.PUSHER) {
      for (const field of findPushDestinations(pieces, selectedFieldIndex)) {
        isPushTarget[field] = true;
      }
    }
  }

  // Clear the current selection whenever the pieces change, because
  // moves may no longer be valid.
  React.useEffect(
    () => setSelectedFieldIndex(-1),
    [piecesToString(pieces)]);

  function isSelectable(i) {
    if (isMoveTarget[i] || isPushTarget[i]) return true;
    const type = getPieceType(pieces[i]);
    if (type === PieceType.NONE || getPlayerColor(pieces[i]) !== nextPlayer) return false;
    if (push != null) return false;  // turn over
    if (type === PieceType.PUSHER) return true;
    return moves.length < 2 ||
        // allow undoing last move
        i === moves[moves.length - 1][1];
  }

  function renderField(i) {
    return (
      <React.Fragment>
        {isMoveTarget[i] && <div className="move-target"></div>}
        {isPushTarget[i] && <div className="push-target"></div>}
        <PieceHolder animation={pieceAnimations[i]}>
          <Piece piece={pieces[i]}></Piece>
        </PieceHolder>
      </React.Fragment>
    );
  }

  function handleFieldClick(i) {
    if (isMoveTarget[i]) {
      onMove(selectedFieldIndex, i);
      setSelectedFieldIndex(-1);
      return;
    }

    if (isPushTarget[i]) {
      onPush(selectedFieldIndex, i);
      setSelectedFieldIndex(-1);
      return;
    }

    if (i !== selectedFieldIndex &&
        getPieceType(pieces[i]) !== PieceType.NONE &&
        getPlayerColor(pieces[i]) === nextPlayer) {
      setSelectedFieldIndex(i);
      return;
    }

    setSelectedFieldIndex(-1);
  }

  return (
    <Board
      selectedFieldIndex={selectedFieldIndex}
      isSelectable={isSelectable}
      renderField={renderField}
      onFieldClick={handleFieldClick}/>
  );
}

function PiecePalette({fieldIndex, onSelect}) {
  if (fieldIndex < 0) return null;

  const r = FIELD_ROW[fieldIndex];
  const c = FIELD_COL[fieldIndex];

  return (
    <div className="palette" style={{gridRow: r + 2, gridColumn: c + 2, zIndex: 1, position: 'relative'}}>
      <div className="cell" style={{position: 'absolute', width: '25%', height: '25%', left: '20.0%', top:  '7.5%'}}
          onClick={() => onSelect(RED_MOVER, fieldIndex)}><Piece piece={RED_MOVER}/></div>
      <div className="cell" style={{position: 'absolute', width: '25%', height: '25%', left:  '5.0%', top: '37.5%'}}
          onClick={() => onSelect(RED_PUSHER, fieldIndex)}><Piece piece={RED_PUSHER}/></div>
      <div className="cell" style={{position: 'absolute', width: '25%', height: '25%', left: '20.0%', top: '67.5%'}}
          onClick={() => onSelect(RED_ANCHOR,fieldIndex)}><Piece piece={RED_ANCHOR}/></div>
      <div className="cell" style={{position: 'absolute', width: '25%', height: '25%', left: '55.0%', top:  '7.5%'}}
          onClick={() => onSelect(BLUE_MOVER, fieldIndex)}><Piece piece={BLUE_MOVER}/></div>
      <div className="cell" style={{position: 'absolute', width: '25%', height: '25%', left: '70.0%', top: '37.5%'}}
          onClick={() => onSelect(BLUE_PUSHER, fieldIndex)}><Piece piece={BLUE_PUSHER}/></div>
      <div className="cell" style={{position: 'absolute', width: '25%', height: '25%', left: '55.0%', top: '67.5%'}}
          onClick={() => onSelect(BLUE_ANCHOR, fieldIndex)}><Piece piece={BLUE_ANCHOR}/></div>
      <div className="cell icon" style={{position: 'absolute', width: '25%', height: '25%', left: '37.5%', top: '37.5%'}}
          onClick={() => onSelect(NO_PIECE, fieldIndex)}>🗙</div>
    </div>
  );
}

class Field extends React.Component {
  constructor(props) {
    super(props);
  }

  render() {
    const {children, r, c, selectable, selected, onClick} = this.props;
    let className = 'field';
    if (r === 0) className += ' border-top';
    if (r === H - 1) className += ' border-bottom';
    if (selectable) className += ' clickable';
    if (selected) className += ' selected';
    return (
      <div className={className} style={{gridRow: r + 2, gridColumn: c + 2}}
          onClick={onClick}>
        {(r !== 0 && FIELD_INDEX[r - 1][c] === -1) && <div className="danger top"/>}
        {(c === 0 || FIELD_INDEX[r][c - 1] === -1) && <div className="danger left"/>}
        {(r !== H - 1 && FIELD_INDEX[r + 1][c] === -1) && <div className="danger bottom"/>}
        {(c === W - 1 || FIELD_INDEX[r][c + 1] === -1) && <div className="danger right"/>}
        {children}
      </div>
    );
  }
}

function Piece({piece}) {
  const type = getPieceType(piece);
  const color = getPlayerColor(piece);

  if (type == PieceType.NONE) return null;

  const shapeClass = type == PieceType.MOVER ? 'circle' : 'square';
  const colorClass = ['red', 'blue'][color];
  const classNames = ['piece', shapeClass, colorClass];
  const className = classNames.join(' ');
  return (
    <div className={className}>
      {type === PieceType.ANCHOR ? '+' : ''}
    </div>
  );
}

// Status bar shown at the top of the screen.
function StatusBar({text, moves, index}) {
  const moveCount = moves == null ? 0 : moves.length;
  const permText =
      moveCount === 1 ? '1 move left' :
      moveCount === 2 ? '0 moves left' :
      moveCount > 0 || index == null ? 'Index indeterminate' :
      String(index);
  return (
    <div className="status">
      <div className="status-text">{text}</div>
      <div className="perm-index" title="Permutation index">{permText}</div>
    </div>
  );
}

// Returns a pair of [pieces, error] where exactly one element is set.
function parsePieces(string) {
  if (string.match(/^\d+$/)) {
    // Parse as permutation index.
    const index = Number(string);
    if (index >= totalPerms) {
      return [undefined, "Permutation index out of range: " + string];
    }
    return [permToPieces(permAtIndex(index))];
  } else if (string.match(/^[.oOPxXY]{26}$/)) {
    function getPieceValue(i) {
      if (i === '.') return NO_PIECE;
      if (i === 'o') return RED_MOVER;
      if (i === 'O') return RED_PUSHER;
      if (i === 'P') return RED_ANCHOR;
      if (i === 'x') return BLUE_MOVER;
      if (i === 'X') return BLUE_PUSHER;
      if (i === 'Y') return BLUE_ANCHOR;
    }
    const pieces = [...string].map(getPieceValue);
    return [pieces];
  } else {
    return [undefined, "Don't know how to parse: " + string];
  }
}

// Converts pieces to a 26-character permutation string.
function piecesToString(pieces) {
  function getChar(p) {
    if (p === NO_PIECE)    return '.';
    if (p === RED_MOVER)   return 'o';
    if (p === RED_PUSHER)  return 'O';
    if (p === RED_ANCHOR)  return 'P';
    if (p === BLUE_MOVER)  return 'x';
    if (p === BLUE_PUSHER) return 'X';
    if (p === BLUE_ANCHOR) return 'Y';
  }
  return pieces.map(getChar).join('');
}

// Converts pieces to a 26-character permutation string, possibly inverting
// the players so that red is the next player to move.
function piecesToNormalizedString(pieces) {
  let redAnchors = 0;
  let blueAnchors = 0;
  for (const piece of pieces) {
    if (piece === RED_ANCHOR) ++redAnchors;
    if (piece === BLUE_ANCHOR) ++blueAnchors;
  }
  return piecesToString(redAnchors > blueAnchors ? invertColors(pieces) : pieces);
}

function SetUpComponent({initialPieces, onStart}) {
  const [pieces, setPieces] = React.useState(initialPieces);
  const {validity, error, winner, index} = validatePieces(pieces);

  const statusMessage =
    validity === PiecesValidity.STARTED ? 'Valid starting position.' :
    validity === PiecesValidity.IN_PROGRESS ? 'Valid position.' :
    validity === PiecesValidity.FINISHED ? 'Finished position. ' + ['Red', 'Blue'][winner] + ' won.' :
      error || 'Unknown error!';

  const isStartEnabled = validity === PiecesValidity.STARTED || validity === PiecesValidity.IN_PROGRESS;

  return (
    <React.Fragment>
      <StatusBar text={statusMessage} index={index} />
      <SetupBoard
          pieces={pieces}
          onPiecesChange={setPieces}
          onStart={isStartEnabled ? () => onStart(pieces) : null}
      />
    </React.Fragment>
  );
}

function History({firstPlayer, turns, moves, undoEnabled, playEnabled,
    onUndoClick, onPlayClick, onExitClick}) {
  const turnStrings = turns.map(formatTurn);
  if (moves != null) {
    turnStrings.push(formatTurn(moves) + '...');
  }

  const rows = [];
  for (let row = 0; 2*row < turnStrings.length; ++row) {
    rows.push(
      <tr key={row}>
        <th>{row}.</th>
        <td>{turnStrings[2*row]}</td>
        <td>{turnStrings[2*row + 1]}</td>
      </tr>
    );
  }

  return (
    <div className="history">
      <table>
        <thead>
          <tr>
            <td></td>
            <th className={["red", "blue"][firstPlayer]}>
              {["Red","Blue"][firstPlayer]}
            </th>
            <th className={["red", "blue"][1 - firstPlayer]}>
              {["Red", "Blue"][1 - firstPlayer]}
            </th>
            <td></td>
          </tr>
        </thead>
        <tbody>{rows}</tbody>
      </table>
      <div className="buttons">
        <button title="Undos the last turn"
            disabled={!undoEnabled} onClick={onUndoClick}>
          Undo turn
        </button>
        <button title="Automatically plays an optimal move"
            disabled={!playEnabled} onClick={onPlayClick}>
          Play best
        </button>
        <button onClick={onExitClick}>
          Return to setup
        </button>
      </div>
    </div>
  );
}

// Formats status as a human-readable string (e.g. "Win in 2").
// Assumes status is formatted like 'T', 'L123', or 'W123'.
function statusDisplayName(status) {
  return status.replace('T', 'Tie').replace('L', 'Loss in ').replace('W', 'Win in ');
}

// Successors grouped by status. Used in the analysis tab.
function SuccessorGroup({status, turns, expanded, onExpand, onSelectTurn}) {
  function handleTurnClick(turnString) {
    const turn = parseTurn(turnString);
    if (turn == null) {
      console.error('Failed to parse turn string!', turn);
      return;
    }
    onSelectTurn(turn);
  }

  return  (
  <div className="group" key={status}>
    <div className="header clickable" onClick={onExpand} title={expanded ? 'Collapse' : 'Expand'}>
      {statusDisplayName(status)}
      <span className="float-right">
        {turns.length}&nbsp;{expanded ? '▲' : '▼'}
      </span>
    </div>
    {
      expanded &&
        <div className="turns">
          {
            turns.map((turn) =>
              <div
                  key={turn}
                  className="turn clickable"
                  onClick={() => handleTurnClick(turn)}>
                {turn}
              </div>
            )
          }
        </div>
    }
    </div>
  );
}

function Analysis({pieces, onSelectTurn}) {

  const [error, setError] = React.useState(null);
  const [successors, setSuccessors] = React.useState(null);
  const [expandedStatus, setExpandedStatus] = React.useState(null);

  const {validity} = validatePieces(pieces);

  // Recalculate contents every time `pieces` changes.
  React.useEffect(() => {
    setError(null);
    setSuccessors(null);
    if (validity === PiecesValidity.INVALID) return;
    if (validity === PiecesValidity.FINISHED) return;
    // FIXME: there is a race condition here. In theory, it's possible for
    // `pieces` to change while analyzePositions() is in progress.
    analyzePosition(pieces).then(
      (obj) => {
        let successors = obj.successors;
        if (successors.length === 0) {
          // Fake entry for the case where there are no moves.
          successors = [{status: obj.status, moves: []}];
        }
        setSuccessors(successors);
      },
      (error) => setError(String(error)));
  }, [pieces]);

  const toggleExpand = (status) =>
    setExpandedStatus((currentStatus) => status === currentStatus ? null : status);

  return (
    <div className="analysis">
    {
      validity === PiecesValidity.INVALID ?
          <p>Position is invalid!</p> :
      validity === PiecesValidity.FINISHED ?
          <p>Game is finished.</p> :
      error != null ?
          <p>Analysis failed: {String(error)}</p> :
      successors == null ?
          <p>Loading...</p> :
      successors.map(({status, moves}) =>
          <SuccessorGroup
              key={status}
              status={status}
              turns={moves}
              expanded={status === expandedStatus}
              onExpand={() => toggleExpand(status)}
              onSelectTurn={onSelectTurn}
          />)
    }
    </div>
  );
}

// Side panel that contains the move history and position analysis tabs.
function PlayPanel({renderTab}) {
  const [selectedTab, setSelectedTab] = React.useState('history');

  return (
    <div className="tab-panel side-panel">
      <div className="tabs">
        <div className="tab clickable history-tab"
            onClick={() => setSelectedTab('history')}>Moves</div>
        <div className="tab clickable analysis-tab"
            onClick={() => setSelectedTab('analysis')}>Analysis</div>
      </div>
      {renderTab(selectedTab)}
    </div>
  );
}

// This executes moves, similar to applyMoves(), but it also calculates
// piece animations as a side effect.
//
// Returns a list [animations, pieces], where `animations` is an object
// with field indexes as keys and animation objects as values, while `pieces`
// is an updated copy of the pieces array after executing the given moves.
function generatePieceAnimations(oldPieces, ...moves) {
  const moveTime = 200; // milliseconds
  const pushTime = 800; // milliseconds
  const pieces = [...oldPieces];

  // Uses breath-first search to find a path from `src` to `dst` that only covers
  // empty fields. Returns undefined if no such path exists. The result is an array
  // representing the path, starting with `src` and ending with `dst`.
  function findShortestPath(src, dst) {
    const visited = new Map();
    visited.set(src, src);
    const todo = [src];
    for (let i = 0; i < todo.length; ++i) {
      const i1 = todo[i];
      const r1 = FIELD_ROW[i1];
      const c1 = FIELD_COL[i1];
      for (let d = 0; d < 4; ++d) {
        const r2 = r1 + DR[d];
        const c2 = c1 + DC[d];
        const i2 = getFieldIndex(r2, c2);
        if (i2 >= 0 && getPieceType(pieces[i2]) === PieceType.NONE && !visited.has(i2)) {
          visited.set(i2, i1);
          if (i2 === dst) {
            // Solution found!
            const path = [];
            let i = dst;
            for (;;) {
              path.push(i);
              if (i === src) break;
              i = visited.get(i);
            }
            path.reverse();
            return path;
          }
          todo.push(i2);
        }
      }
    }
  }

  // For each piece, we keep a list of [time, position] pairs.
  const movements = oldPieces.map(p => getPieceType(p) === PieceType.NONE ? null : []);

  let time = 0;
  for (const move of moves) {
    const [src, dst] = move;
    if (src === dst) {
      continue;
    }

    if (pieces[dst] === 0) {
      // Move.
      const path = findShortestPath(src, dst) || [src, dst];
      for (let i = 0; i + 1 < path.length; ++i) {
        const a = path[i], b = path[i + 1];
        pieces[b] = pieces[a];
        pieces[a] = 0;
        movements[b] = movements[a];
        movements[a] = null;
        movements[b].push([time, a], [time += moveTime, b]);
      }
    } else {
      // Push.
      const dr = FIELD_ROW[dst] - FIELD_ROW[src];
      const dc = FIELD_COL[dst] - FIELD_COL[src];

      // Remove old anchor.
      for (let i = 0; i < pieces.length; ++i) {
        if (getPieceType(pieces[i]) === PieceType.ANCHOR) {
          pieces[i] = makePiece(getPlayerColor(pieces[i]), PieceType.PUSHER);
        }
      }

      let i = src;
      let r = FIELD_ROW[i];
      let c = FIELD_COL[i];
      let p = pieces[i];
      let m = movements[i];
      p = makePiece(getPlayerColor(p), PieceType.ANCHOR);  // place anchor
      pieces[i] = NO_PIECE;
      movements[i] = null;
      for (;;) {
        const j = getFieldIndex(r += dr, c += dc);
        if (j === -1) {
          // FIXME: this piece is pushed off the board! Also support this.
          break;
        }
        m.push([time, i], [time + pushTime, j]);
        const q = pieces[j];
        const n = movements[j];
        pieces[j] = p;
        movements[j] = m;
        if (getPieceType(q) === PieceType.NONE) {
          // End of push.
          break;
        }
        p = q;
        m = n;
        i = j;
      }
      time += pushTime;
    }
  }

  // Calculate animations from movements.
  const animations = {};
  for (let i = 0; i < FIELD_COUNT; ++i) {
    const m = movements[i];
    if (m == null || m.length === 0) continue;
    const [tStart, iStart] = m[0];
    const [tEnd, iEnd] = m[m.length - 1];
    const duration = tEnd - tStart;
    const rEnd = FIELD_ROW[iEnd];
    const cEnd = FIELD_COL[iEnd];
    const keyframes = [];
    for (let [t, i] of m) {
      keyframes.push({
        left: -100*(cEnd - FIELD_COL[i]) + '%',
        top: -100*(rEnd - FIELD_ROW[i]) + '%',
        offset: (t - tStart) / duration,
      });
    }
    const options = { delay: tStart, duration: duration, fill: 'backwards' };
    animations[iEnd] = { keyframes, options };
  }

  return [animations, pieces];
}

class PlayComponent extends React.Component {

  constructor(props) {
    super(props);
    this.state = {
      // Positions of pieces on the board.
      pieces: props.initialPieces,

      // Turns played so far, not including the current turn.
      // Each turn is a list of moves followed by one push.
      turns: [],

      // Moves played in the current turn.
      moves: [],

      // Pieces at the start of each turn. This has length 1 greater
      // than `turns`. Used to implement undo.
      piecesAtTurnStart: [props.initialPieces],

      // Animations to apply to pieces.
      // Keys are destination field indices, values are lists of objects
      // with two keys: `keyframes` and `options`, which are pased to
      // animate().
      pieceAnimations: {},
    };

    this.handleMove = this.handleMove.bind(this);
    this.handlePush = this.handlePush.bind(this);
    this.handleUndo = this.handleUndo.bind(this);
    this.handleAutoPlay = this.handleAutoPlay.bind(this);
    this.handleExit = this.handleExit.bind(this);
    this.playFullTurn = this.playFullTurn.bind(this);
  }

  handleMove(src, dst) {
    this.setState(({pieces, moves}) => {
      // Generating animations must be done before we update the moves.
      const [pieceAnimations, newPieces] = generatePieceAnimations(pieces, [src, dst]);
      moves = [...moves];  // copy
      const lastMove = moves.length > 0 ? moves[moves.length - 1] : undefined;
      if (lastMove != null && lastMove[1] === src) {
        // For convenience: moving the last-moved piece updates the last move.
        // This allows undoing part of a turn without undoing the entire turn.
        lastMove[1] = dst;
        if (lastMove[0] === lastMove[1]) moves.pop();
      } else {
        moves.push([src, dst]);
      }
      return {
        pieces: Object.freeze(newPieces),
        moves,
        pieceAnimations: pieceAnimations,
      };
    });
  }

  handlePush(src, dst) {
    this.setState(({pieces, turns, moves, piecesAtTurnStart}) => {
      const push = Object.freeze([src, dst]);
      const [pieceAnimations, newPieces] = generatePieceAnimations(pieces, push);
      return {
        pieces: Object.freeze(newPieces),
        turns: [...turns, [...moves, push]],
        moves: [],
        piecesAtTurnStart: [...piecesAtTurnStart, newPieces],
        pieceAnimations: pieceAnimations,
      };
    });
  }

  // Plays the given turn, overriding any partial moves played so far.
  playFullTurn(turn) {
    this.setState(({turns, piecesAtTurnStart}) => {
      const oldPieces = piecesAtTurnStart[turns.length];
      const [pieceAnimations, newPieces] = generatePieceAnimations(oldPieces, ...turn);
      return {
        pieces: newPieces,
        turns: [...turns, turn],
        moves: [],
        piecesAtTurnStart: [...piecesAtTurnStart, newPieces],
        pieceAnimations: pieceAnimations,
      };
    });
  }

  handleAutoPlay() {
    const {turns, piecesAtTurnStart} = this.state;
    analyzePosition(piecesAtTurnStart[turns.length]).then(
      (value) => {
        if (value.successors.length === 0) {
          alert('No moves available!');
          return;
        }
        // Randomly pick one of the best moves to play.
        const bestMoves = value.successors[0].moves;
        const randomTurn = parseTurn(bestMoves[Math.floor(Math.random() * bestMoves.length)]);
        this.playFullTurn(randomTurn);
      },
      (error) => {
        alert('Failed to analyze position! ' + error);
      }
    );
  }

  handleUndo() {
    this.setState(({moves, turns, piecesAtTurnStart}) => {
      if (moves.length > 0) {
        // Undo only the moves in the current turn.
        return {
          pieces: piecesAtTurnStart[turns.length],
          moves: [],
        };
      }
      if (turns.length > 0) {
        // Undo the current turn.
        return {
          pieces: piecesAtTurnStart[turns.length - 1],
          turns: turns.slice(0, turns.length - 1),
          piecesAtTurnStart: piecesAtTurnStart.slice(0, turns.length),
        };
      }
    });
  }

  handleExit() {
    if (this.state.turns.length == 0 ||
        confirm('Are you sure? The move history will be lost.')) {
      this.props.onExit(this.state.piecesAtTurnStart[this.state.turns.length]);
    }
  }

  render() {
    const {pieces, turns, moves, piecesAtTurnStart, pieceAnimations} = this.state;
    const {validity, error, nextPlayer, winner, index} = validatePieces(pieces);

    const isUnfinished = validity === PiecesValidity.STARTED || validity === PiecesValidity.IN_PROGRESS;
    const statusMessage =
        isUnfinished ? ['Red', 'Blue'][nextPlayer] + ' to move.' :
        validity === PiecesValidity.FINISHED ? ['Red', 'Blue'][winner] + ' won.' :
        error || 'Unknown error!';

    const firstPlayer = this.props.initialPieces.includes(RED_ANCHOR) ? BLUE_PLAYER : RED_PLAYER;

    // Can undo undo if there is a (partial) turn to be undo.
    const undoEnabled = turns.length > 0 || moves.length > 0;

    // Note: this is an arrow function so we can use `this` inside to refer
    // to the PlayComponent instance.
    const renderTab = (tab) => {
      if (tab === 'history') {
        return (
          <History
              firstPlayer={firstPlayer}
              turns={turns}
              moves={isUnfinished ? moves : undefined}
              undoEnabled={undoEnabled}
              playEnabled={isUnfinished}
              onUndoClick={this.handleUndo}
              onPlayClick={this.handleAutoPlay}
              onExitClick={this.handleExit}
            />
        );
      }
      if (tab === 'analysis') {
        return (
          <Analysis
              pieces={piecesAtTurnStart[turns.length]}
              onSelectTurn={this.playFullTurn}
          />
        );
      }
    }

    return (
      <React.Fragment>
        <StatusBar text={statusMessage} moves={moves} index={index} />
        <div className="board-holder">
          <PlayBoard
            pieces={pieces}
            nextPlayer={nextPlayer}
            moves={moves}
            push={null}
            onMove={this.handleMove}
            onPush={this.handlePush}
            pieceAnimations={pieceAnimations}/>
          <PlayPanel renderTab={renderTab}/>
        </div>
      </React.Fragment>
    );
  }
}

class RootComponent extends React.Component {
  state = {
    initialPieces: INITIAL_PIECES,
    setupComplete: false,
  };

  constructor(props) {
    super(props);
    this.handleStart = this.handleStart.bind(this);
    this.handleExit = this.handleExit.bind(this);
  }

  handleStart(pieces) {
    this.setState({initialPieces: pieces, setupComplete: true});
  }

  handleExit(pieces) {
    this.setState({initialPieces: pieces, setupComplete: false});
  }

  render() {
    const {initialPieces, setupComplete} = this.state;
    let content = !setupComplete ?
        <SetUpComponent initialPieces={initialPieces} onStart={this.handleStart}/> :
        <PlayComponent initialPieces={initialPieces} onExit={this.handleExit}/>;
    if (reactStrictMode) {
      content = <React.StrictMode>{content}</React.StrictMode>;
    }
    return content;
  }
}

// Global initialization.
{
  const root = ReactDOM.createRoot(document.getElementById('root'));
  root.render(React.createElement(RootComponent));
}