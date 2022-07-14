'use strict';

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
function PlayBoard({pieces, nextPlayer, moves, push, onMove, onPush}) {
  const [selectedFieldIndex, setSelectedFieldIndex] = React.useState(-1);

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
        <Piece piece={pieces[i]}></Piece>
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
    <div className="new-palette" style={{gridRow: r + 2, gridColumn: c + 2, zIndex: 1, position: 'relative'}}>
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
      <div class="board-holder">
        <SetupBoard pieces={pieces} onPiecesChange={setPieces}/>
      </div>
    </React.Fragment>
  );
}

function History({turns}) {
  function formatTurn(parts) {
    return parts.length === 0 ? '...' :
        parts.map(([src, dst]) => formatMove(src, dst)).join(',');
  }

  return (
    <div className="history">
      <ol>
        {turns.map((turn, i) => <li key={i}>{formatTurn(turn)}</li>)}
      </ol>
    </div>
  );
}

class PlayComponent extends React.Component {
  state = {
    pieces: undefined,  // set in constructor
    turns: [],
    moves: [],
  }

  constructor(props) {
    super(props);
    this.state.pieces = props.initialPieces;
    this.handleMove = this.handleMove.bind(this);
    this.handlePush = this.handlePush.bind(this);
  }

  handleMove(src, dst) {
    this.setState(({pieces, moves}) => {
      moves = [...moves];  // copy
      if (moves.length > 0 && moves[moves.length - 1][1] === src) {
        const lastMove = moves.pop();
        pieces = applyMove(pieces, lastMove[1], lastMove[0]);  // undo last move
        src = lastMove[0];
      }
      if (src !== dst) {
        pieces = applyMove(pieces, src, dst);
        moves.push([src, dst]);
      }
      return {pieces, moves};
    });
  }

  handlePush(src, dst) {
    this.setState(({pieces, turns, moves, push}) => ({
      pieces: applyMove(pieces, src, dst),
      turns: [...turns, [...moves, [src, dst]]],
      moves: [],
    }));
  }

  render() {
    const {pieces, turns, moves} = this.state;
    const {validity, error, nextPlayer, winner, index} = validatePieces(pieces);

    const isUnfinished = validity === PiecesValidity.STARTED || validity === PiecesValidity.IN_PROGRESS;
    const statusMessage =
        isUnfinished ? ['Red', 'Blue'][nextPlayer] + ' to move.' :
        validity === PiecesValidity.FINISHED ? ['Red', 'Blue'][winner] + ' won.' :
        error || 'Unknown error!';

    return (
      <React.Fragment>
        <PlayStatusBar text={statusMessage} moves={moves} index={index} />
        <div class="board-holder">
          <div className="board-history">
            <PlayBoard
              pieces={pieces}
              nextPlayer={nextPlayer}
              moves={moves}
              push={null}
              onMove={this.handleMove}
              onPush={this.handlePush}/>
            <History turns={isUnfinished ? [...turns, moves]: turns}/>
          </div>
        </div>
      </React.Fragment>
    );
  }
}

function RootComponent() {
  const [initialPieces, setInitialPieces] = React.useState(null);

  return initialPieces == null ?
      <SetUpComponent onStart={setInitialPieces}/> :
      <PlayComponent initialPieces={initialPieces}/>;
}

// Global initialization.
{
  const root = ReactDOM.createRoot(document.getElementById('root'));
  root.render(React.createElement(RootComponent));
}
