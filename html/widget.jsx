'use strict';

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
function SetupBoard({pieces, onPiecesChange}) {
  const [selectedFieldIndex, setSelectedFieldIndex] = React.useState(-1);

  function handlePieceSelect(piece, i) {
    setSelectedFieldIndex(-1);
    onPiecesChange(replacePiece(pieces, i, piece));
  }

  function renderField(i) {
    return <Piece piece={pieces[i]}/>;
  }

  function handleFieldClick(i) {
    setSelectedFieldIndex((j) => j === i ? -1 : i);
  }

  return (
    <Board renderField={renderField} isSelectable={() => true} onFieldClick={handleFieldClick}>
      <PiecePalette fieldIndex={selectedFieldIndex} onSelect={handlePieceSelect} />
    </Board>
  );
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

  React.useEffect(
    () => {
      if (pieceAnimations == null) return;
      for (const [dst, keyframes, options] of pieceAnimations) {
        if (pieceRefs[dst].current != null) {
          pieceRefs[dst].current.animate(keyframes, options);
        }
      }
    },
    [pieceAnimations]);

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
        <div className="piece-holder" ref={pieceRefs[i]}>
          <Piece piece={pieces[i]}></Piece>
        </div>
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

// Status bar shown while playing moves.
function PlayStatusBar({text, moves, index}) {
  const permText =
      moves.length === 1 ? '1 move left' :
      moves.length === 2 ? '0 moves left' :
      moves.length > 0 || index == null ? 'Index indeterminate' :
      String(index  );
  return (
    <div className="status">
      <div className="status-text">{text}</div>
      <div className="perm-index">{permText}</div>
    </div>
  );
}

// Enhanced version of the status bar that is used during setup.
function SetupStatusBar({text, index, onStartClick, onInvertColorsClick, onIndexClick}) {
  return (
    <div className="status">
      <div className="status-text">{text}</div>
      <div className="status-links">
        {onStartClick &&
            <div className="status-link clickable" onClick={onStartClick}>Start moving</div>}
        <div className="status-link clickable" onClick={onInvertColorsClick}>Invert colors</div>
      </div>
      <div className="clickable" onClick={onIndexClick}>{index == null ? 'Index indeterminate' : String(index)}</div>
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
    function getPieceValue(i) {
      if (i === 0) return NO_PIECE;
      if (i === 1) return RED_MOVER;
      if (i === 2) return RED_PUSHER;
      if (i === 3) return BLUE_MOVER;
      if (i === 4) return BLUE_PUSHER;
      if (i === 5) return BLUE_ANCHOR;
    }
    const perm = permAtIndex(index);
    const pieces = perm.map(getPieceValue);
    return [pieces];
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
    return [undefined, "Don't know how to parse: " + result];
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

function SetUpComponent({onStart}) {
  const [pieces, setPieces] = React.useState(INITIAL_PIECES);
  const {validity, error, winner, index} = validatePieces(pieces);

  function changePermutation() {
    const response = prompt('Permutation string or index:', index);
    if (response == null) return;
    const [pieces, error] = parsePieces(response);
    if (pieces != null) {
      setPieces(pieces);
    } else {
      alert(error);
    }
  }

  const statusMessage =
    validity === PiecesValidity.STARTED ? 'Valid starting position.' :
    validity === PiecesValidity.IN_PROGRESS ? 'Valid position.' :
    validity === PiecesValidity.FINISHED ? 'Finished position. ' + ['Red', 'Blue'][winner] + ' won.' :
      error || 'Unknown error!';

  const isStartEnabled = validity === PiecesValidity.STARTED || validity === PiecesValidity.IN_PROGRESS;

  return (
    <React.Fragment>
      <SetupStatusBar
          text={statusMessage}
          index={index}
          onIndexClick={changePermutation}
          onInvertColorsClick={() => setPieces((pieces) => invertColors(pieces))}
          onStartClick={isStartEnabled ? () => onStart(pieces) : null}
      />
      <div className="board-holder">
        <SetupBoard pieces={pieces} onPiecesChange={setPieces}/>
      </div>
    </React.Fragment>
  );
}

function History({firstPlayer, turns, moves, undoEnabled, playEnabled, onUndoClick, onPlayClick}) {
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
    <div className="tab-panel">
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

function generatePieceAnimations(pieces, ...moves) {
  const animations = [];
  let lastMove = null;
  let startTime = 0;
  for (const move of moves) {
    const [src, dst] = move;
    if (src === dst) {
      continue;
    }
    if (lastMove != null) {
      pieces = applyMoves(pieces, lastMove);
    }
    lastMove = move;

    const dr = FIELD_ROW[dst] - FIELD_ROW[src];
    const dc = FIELD_COL[dst] - FIELD_COL[src];

    if (pieces[dst] === 0) {
      // Move.
      // TODO: pathfinding
      const keyframes = [
        { left: -100*dc + '%', top: -100*dr + '%'},
        { left: 0, top: 0},
      ];
      animations.push([dst, keyframes, {delay: startTime, duration: 1000, fill: 'backwards'}]);
      startTime += 1000;
    } else {
      // Push.
      const keyframes = [
        { left: -100*dc + '%', top: -100*dr + '%'},
        { left: 0, top: 0},
      ];
      let i = src;
      let r = FIELD_ROW[i];
      let c = FIELD_COL[i];
      console.log('a', i, r, c, getPieceType(pieces[i]));
      while (getPieceType(pieces[i]) !== PieceType.NONE) {
        console.log('b', i, r, c, getPieceType(pieces[i]));
        r += dr;
        c += dc;
        i = getFieldIndex(r, c);
        // FIXME: this doesn't allow us to animate the piece that's pushed off the board :/
        if (i === -1) break;
        animations.push([i, keyframes, {delay: startTime, duration: 1000, fill: 'backwards'}]);
      }
      startTime += 1000;
    }
  }
  console.log('c', animations);
  return animations;
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

      // List of animations to apply to pieces.
      // Each element is a triple: destination index, keyframes, options
      pieceAnimations: [],
    };

    this.handleMove = this.handleMove.bind(this);
    this.handlePush = this.handlePush.bind(this);
    this.handleUndo = this.handleUndo.bind(this);
    this.handleAutoPlay = this.handleAutoPlay.bind(this);
    this.playFullTurn = this.playFullTurn.bind(this);
  }

  handleMove(src, dst) {
    this.setState(({pieces, moves}) => {
      // Generating animations must be done before we update the moves.
      const pieceAnimations = generatePieceAnimations(pieces, [src, dst]);
      moves = [...moves];  // copy
      if (moves.length > 0 && moves[moves.length - 1][1] === src) {
        const lastMove = moves.pop();
        pieces = applyMoves(pieces, [lastMove[1], lastMove[0]]);  // undo last move
        src = lastMove[0];
      }
      if (src !== dst) {
        pieces = applyMoves(pieces, [src, dst]);
        moves.push([src, dst]);
      }
      return {
        pieces,
        moves,
        pieceAnimations: pieceAnimations,
      };
    });
  }

  handlePush(src, dst) {
    this.setState(({pieces, turns, moves, piecesAtTurnStart}) => {
      const push = Object.freeze([src, dst]);
      const newPieces = Object.freeze(applyMoves(pieces, push));
      return {
        pieces: newPieces,
        turns: [...turns, [...moves, push]],
        moves: [],
        piecesAtTurnStart: [...piecesAtTurnStart, newPieces],
        pieceAnimations: generatePieceAnimations(pieces, push),
      };
    });
  }

  // Plays the given turn, overriding any partial moves played so far.
  playFullTurn(turn) {
    this.setState(({turns, piecesAtTurnStart}) => {
      const oldPieces = piecesAtTurnStart[turns.length];
      let newPieces = Object.freeze(applyMoves(oldPieces, ...turn));
      return {
        pieces: newPieces,
        turns: [...turns, turn],
        moves: [],
        piecesAtTurnStart: [...piecesAtTurnStart, newPieces],
        pieceAnimations: generatePieceAnimations(oldPieces, ...turn),
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
        <PlayStatusBar text={statusMessage} moves={moves} index={index} />
        <div className="board-holder">
          <div className="board-with-panel">
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
        </div>
      </React.Fragment>
    );
  }
}

function RootComponent() {
  const [initialPieces, setInitialPieces] = React.useState(null);

  return (
    <React.StrictMode>
    {
      initialPieces == null ?
        <SetUpComponent onStart={setInitialPieces}/> :
        <PlayComponent initialPieces={initialPieces}/>
    }
    </React.StrictMode>
  );
}

// Global initialization.
{
  const root = ReactDOM.createRoot(document.getElementById('root'));
  root.render(React.createElement(RootComponent));
}
