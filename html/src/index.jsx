import React from "react";
import ReactDOM from 'react-dom/client';

import * as ai from './ai.js';
import {getFieldIndex} from './board.js';
import {fetchPositionAnalysis, PositionAnalysis, parseStatus} from './analysis.js';
import {generatePieceAnimations} from './animation.js';
import {totalPerms, permAtIndex} from './perms.js';
import {getRandomElement, shuffle} from './random.js';
import {
  H, W, FIELD_COUNT, FIELD_INDEX, FIELD_ROW, FIELD_COL,
  PieceType, getPieceType, getPlayerColor, replacePiece,
  EMPTY_PIECES, INITIAL_PIECES, validatePieces, PiecesValidity,
  RED_PLAYER, BLUE_PLAYER, NO_PIECE,
  RED_MOVER, RED_PUSHER, RED_ANCHOR,
  BLUE_MOVER, BLUE_PUSHER, BLUE_ANCHOR,
  invertColors,
  permToPieces, parsePieces, formatPieces,
} from './board.js';
import {
  generateMoveDestinations, findPushDestinations,
  validateTurn, parseTurn, formatTurn,
} from './moves.js';

// For development: set to `true` to enable React strict mode.
// https://reactjs.org/docs/strict-mode.html
const reactStrictMode = true;

function useEventListener(type, callback, element=document) {
  if (typeof callback !== 'function') {
    throw new Error('useEventListener: callback must be a function');
  }
  React.useEffect(
    () => {
      element.addEventListener(type, callback);
      return () => element.removeEventListener(type, callback);
    },
    [type, callback, element]);
}

function useKeyDownListener(onKeyDown) {
  useEventListener('keydown', onKeyDown);
}

function KeyDownListener({onKeyDown}) {
  useKeyDownListener(onKeyDown);
  return null;
}

class Board extends React.Component {

  // Column selected by typing (see handleKeyDown())
  selectedColumn = -1;

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

    this.handleKeyDown = this.handleKeyDown.bind(this);
  }

  handleKeyDown(ev) {
    if (ev.altKey || ev.ctrlKey || ev.shiftKey || ev.metaKey) return;

    const column = 'abcdefgh'.indexOf(ev.key);
    if (column !== -1) {
      this.selectedColumn = column;
    } else {
      if (this.selectedColumn !== -1) {
        const row = '4321'.indexOf(ev.key);
        if (row !== -1) {
          const i = getFieldIndex(row, this.selectedColumn);
          if (i !== -1 && this.props.isSelectable(i)) {
            this.props.onFieldClick(i, row, this.selectedColumn);
          }
        }
      }
      this.selectedColumn = -1;
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
        <Field key={`${r},${c}`} r={r} c={c}
            selectable={isSelectable(i)}
            selected={selectedFieldIndex === i}
            onClick={() => onFieldClick(i, r, c)}>
          {renderField(i, r, c)}
        </Field>
      );
    }

    return (
      <React.Fragment>
        <div className="board">
          {this.rowCoordinates}
          {this.columnCoordinates}
          {fields}
          {this.props.children}
        </div>
        <KeyDownListener onKeyDown={this.handleKeyDown}/>
      </React.Fragment>
    );
  }
}

function generateRandomStartingPosition() {
  // For a starting position, we'll place four pieces against the starting
  // line, and a final piece in one of the two middle rows in the second column.
  const pieces = [...EMPTY_PIECES];  // copy
  const redFields = [1, 8, 16, 23].concat(getRandomElement([7, 15]));
  const blueFields = [2, 9, 17, 24].concat(getRandomElement([10, 18]));
  shuffle(redFields);
  shuffle(blueFields);
  for (let i = 0; i < 5; ++i) {
    pieces[redFields[i]] = i < 2 ? RED_MOVER : RED_PUSHER;
    pieces[blueFields[i]] = i < 2 ? BLUE_MOVER : BLUE_PUSHER;
  }
  return pieces;
}

// Version of the board used during setup.
function SetupBoard({pieces, onPiecesChange, onStart}) {
  const [selectedFieldIndex, setSelectedFieldIndex] = React.useState(-1);

  function handleChangePermutation() {
    const response = prompt('Permutation index or string:');
    if (response == null) return;
    const [pieces, error] = parsePermutation(response);
    if (pieces != null) {
      onPiecesChange(pieces);
    } else {
      alert(error);
    }
  }

  function handleInvertColors() {
    onPiecesChange(invertColors(pieces));
  }

  function handleClear() {
    onPiecesChange(EMPTY_PIECES);
  }

  function handleRandomStart() {
    onPiecesChange(generateRandomStartingPosition());
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
    return (
      <React.Fragment>
        <Piece piece={pieces[i]}/>
        {getPieceType(pieces[i]) === PieceType.ANCHOR && <Anchor/>}
      </React.Fragment>
    );
  }

  return (
    <div className="board-holder">
      <Board
          isSelectable={() => true}
          renderField={renderField}
          onFieldClick={handleFieldClick}>
        <PiecePalette fieldIndex={selectedFieldIndex} onSelect={handlePieceSelect} />
      </Board>
      <div className="side-panel">
        <div className="setup-panel">
          <h1>Push Fight Setup</h1>
          <p>
            Change the initial placement of the pieces by clicking on fields.
            Press the <em>Start game</em> button to start playing from the chosen position.
          </p>
          <h4>Permutation string</h4>
          <code>{formatPieces(pieces)}</code>
          <p className="buttons">
            <button onClick={handleClear}>Clear board</button>
            <button onClick={handleRandomStart}>Random starting position</button>
            <button onClick={handleRandomize}>Random position</button>
            <button onClick={handleInvertColors}>Invert colors</button>
            <button onClick={handleChangePermutation}>Enter permutation</button>
            <button disabled={onStart == null} onClick={onStart}>Start game</button>
          </p>
        </div>
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
    const style = {
      zIndex: this.props.zIndex,
      visibility: this.props.invisible ? 'hidden' : 'visible',
    };
    return (
      <div className="piece-holder" ref={this.ref} style={style}>
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
    if (moves.length < 2 || selectedFieldIndex === moves[moves.length - 1][1]) {
      for (const field of generateMoveDestinations(pieces, selectedFieldIndex)) {
        isMoveTarget[field] = true;
      }
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
    [formatPieces(pieces)]);

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
    const pushed = pieceAnimations.pushed;
    return (
      <React.Fragment>
        {isMoveTarget[i] && <div className="move-target"></div>}
        {isPushTarget[i] && <div className="push-target"></div>}
        <PieceHolder animation={pieceAnimations[i]} zIndex={1}>
          <Piece piece={pieces[i]}></Piece>
        </PieceHolder>
        {
          getPieceType(pieces[i]) === PieceType.ANCHOR &&
            <PieceHolder animation={pieceAnimations.anchor} zIndex={2}>
              <Anchor/>
            </PieceHolder>
        }
        {
          pushed != null && pushed.position === i &&
            <PieceHolder animation={pushed} zIndex={1} invisible>
              <Piece piece={pushed.piece}></Piece>
            </PieceHolder>
        }
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
          onClick={() => onSelect(RED_ANCHOR,fieldIndex)}><Piece piece={RED_ANCHOR}/><Anchor/></div>
      <div className="cell" style={{position: 'absolute', width: '25%', height: '25%', left: '55.0%', top:  '7.5%'}}
          onClick={() => onSelect(BLUE_MOVER, fieldIndex)}><Piece piece={BLUE_MOVER}/></div>
      <div className="cell" style={{position: 'absolute', width: '25%', height: '25%', left: '70.0%', top: '37.5%'}}
          onClick={() => onSelect(BLUE_PUSHER, fieldIndex)}><Piece piece={BLUE_PUSHER}/></div>
      <div className="cell" style={{position: 'absolute', width: '25%', height: '25%', left: '55.0%', top: '67.5%'}}
          onClick={() => onSelect(BLUE_ANCHOR, fieldIndex)}><Piece piece={BLUE_ANCHOR}/><Anchor/></div>
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
      {color === BLUE_PLAYER ? <div className="color-blind-marker"/> : null}
    </div>
  );
}

function Anchor() {
  return <div className="anchor">+</div>;
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
function parsePermutation(string) {
  if (string.match(/^\d+$/)) {
    // Parse as permutation index.
    const index = Number(string);
    if (index >= totalPerms) {
      return [undefined, "Permutation index out of range: " + string];
    }
    return [permToPieces(permAtIndex(index))];
  }

  const pieces = parsePieces(string);
  if (pieces != null) {
    return [pieces];
  }

  return [undefined, "Don't know how to parse: " + string];
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

function AI({
    visible, strength, aggressive, aiPlayer, autoplayBlocked,
    onChangeStrength, onChangeAggressive, onChangeAiPlayer}) {
  return (
    <div className="ai" style={{display: visible ? undefined : 'none'}}>
      <p>
        <label>
          {'AI Strength: '}
          <input type="range" min={0} max={ai.MAX_STRENGTH} value={strength}
              onChange={(e) => onChangeStrength(parseInt(e.target.value, 10))}/>
          {' ' + String(strength) + ' '}
          <span className="detail">{
              strength === 0 ? '(random play)' :
              strength === ai.MAX_STRENGTH ? '(perfect play)' :
                  `(search depth: ${ai.strengthToMaxDepth(strength)})`
          }</span>
        </label>
      </p>
      <p>
        <label>
          <input
            disabled={strength < 2}
            type="checkbox"
            checked={strength >= 2 && aggressive}
            onChange={(e) => onChangeAggressive(e.target.checked)}
          />
          Play aggressively (slower)
        </label>
      </p>
      <p>
        <label>
          {'AI automatically plays: '}
          <select
              value={['none', 'red', 'blue'][aiPlayer + 1]}
              onChange={(e) => onChangeAiPlayer(['red', 'blue'].indexOf(e.target.value))}>
            <option value="none">Never</option>
            <option value="red">Red</option>
            <option value="blue">Blue</option>
          </select>
        </label>
      </p>
      {autoplayBlocked &&
        <p className="small-warning">
          ⚠️ Autoplay blocked by undo. Use “Play AI Move” to override.
        </p>
      }
    </div>
  );
}

function History({visible, firstPlayer, turns, analyses, moves}) {
  const [showAnalysis, setShowAnalysis] = React.useState(analyses != null);

  const turnStrings = turns.map(formatTurn);
  if (moves != null) {
    turnStrings.push(formatTurn(moves) + '...');
  }

  const renderMove = (i) =>
    <Move
      label={turnStrings[i]}
      analysis={showAnalysis ? analyses[i] : null}
      prevAnalysis={showAnalysis && i > 0 ? analyses[i - 1] : null}
      nextAnalysis={showAnalysis && i + 1 < analyses.length ? analyses[i + 1] : null}
    />

  const rows = [];
  for (let row = 0; 2*row < turnStrings.length; ++row) {
    rows.push(
      <tr key={row}>
        <th>{row + 1}.</th>
        <td>{renderMove(2*row)}</td>
        <td>{2*row + 1 < turnStrings.length && renderMove(2*row + 1)}</td>
      </tr>
    );
  }

  return (
    <div className="history" style={{display: visible ? undefined : 'none'}}>
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
          </tr>
        </thead>
        <tbody>{rows}</tbody>
      </table>
      <p>
        <label>
          <input
              type="checkbox"
              disabled={analyses == null}
              checked={analyses != null && showAnalysis}
              onChange={(e) => setShowAnalysis(e.target.checked)}/>
          Show analysis
        </label>
      </p>
    </div>
  );
}

// Renders a move in the history panel (optionally with analysis).
function Move({label, analysis, prevAnalysis, nextAnalysis}) {
  // A bad move is one that turns a won/tied position into a loss.
  // (We currently don't consider turning a win into a tie a bad move, because
  // some of these are extremely obscure; e.g. missing a win-in-25 would not be
  // a bad move.)
  let badMove =
      analysis?.result?.status && parseStatus(analysis.result.status)[0] >= 0 &&
      nextAnalysis?.result?.status && parseStatus(nextAnalysis.result.status)[0] > 0;
  // A good move is one that preserves a win after the opponent made a bad move.
  // i.e., it is capitalizing on the opponent's mistake.
  let goodMove =
      prevAnalysis?.result?.status && parseStatus(prevAnalysis.result.status)[0] >= 0 &&
      nextAnalysis?.result?.status && parseStatus(nextAnalysis.result.status)[0] < 0;
  return (
    <div>
      <AnalysisChip analysis={analysis}/>
      {label}
      {goodMove && <span className="good-move" title="Good move">!</span>}
      {badMove && <span className="bad-move" title="Bad move">?</span>}
    </div>
  );
}

function AnalysisChip({analysis}) {
  if (analysis == null || analysis.skipped || analysis.isLoading()) return undefined;

  let content = undefined;
  let title = undefined;
  let className = 'analysis-chip';
  if (analysis.failed || analysis.result?.status == null) {
    className += ' failed';
    content = '!';
    title = 'Analysis failed!';
  } else {
    content = analysis.result.status;
    if (content[0] === 'T') className += ' tie';
    if (content[0] === 'L') className += ' loss';
    if (content[0] === 'W') className += ' win';
    title = statusDisplayName(content);
  }
  return <span className={className} title={title}>{content}</span>
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

function Analysis({visible, validity, analysis, onSelectTurn, onRetryAnalysisClick}) {
  const [expandedStatus, setExpandedStatus] = React.useState(null);

  // Collapse expanded status when analysis changes, because it's a bit weird
  // to keep the selection between different positions.
  React.useEffect(() => setExpandedStatus(null), [analysis]);

  const toggleExpand = (status) =>
    setExpandedStatus((currentStatus) => status === currentStatus ? null : status);

  return (
    <div className="analysis" style={{display: visible ? undefined : 'none'}}>
    {
      validity === PiecesValidity.INVALID ?
          <p>Position is invalid!</p> :
      validity === PiecesValidity.FINISHED ?
          <p>Game is finished.</p> :
      analysis.isLoading() ?
          <p>Analysis is loading...</p> :
      analysis.skipped ?
          <p>Analysis was skipped.</p> :
      analysis.error != null ?
          <React.Fragment>
            <p>Analysis failed: {analysis.error.message || String(analysis.error)}</p>
            <p className="buttons"><button onClick={onRetryAnalysisClick}>Retry analysis</button></p>
          </React.Fragment> :
      analysis.result.successors.length === 0 ?
          <p>No moves available!</p> :
      analysis.result.successors.map(({status, moves}) =>
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
function PlayPanel({renderTab, children}) {
  const [selectedTab, setSelectedTab] = React.useState('ai');

  return (
    <div className="side-panel">
      <div className="tab-panel">
        <div>
          <div className="tabs">
            <div className="tab clickable ai-tab"
                onClick={() => setSelectedTab('ai')}>AI</div>
            <div className="tab clickable history-tab"
                onClick={() => setSelectedTab('history')}>Move history</div>
            <div className="tab clickable analysis-tab"
                onClick={() => setSelectedTab('analysis')}>Analysis</div>
          </div>
          {renderTab(selectedTab)}
        </div>
        {children}
      </div>
    </div>
  );
}

function UpdateUrlHash({pieces, turns}) {
  React.useEffect(() => {
    const oldHash = window.location.hash;
    const stateString = formatState(pieces, turns);
    // Note: this assumes all characters in stateString are safe to use in the
    // hash fragment. In particular, they should not contain '&', '#' or '='.
    window.history.replaceState(null, '', '#history=' + stateString);
    return () => {
      window.history.replaceState(null, '', oldHash || '#');
    };
  }, [pieces, turns]);
  return null;
}

function formatState(pieces, turns) {
  let string = formatPieces(pieces);
  for (const turn of turns) {
    string += ';' + formatTurn(turn);
  }
  return string;
}

function updateRedoTurns(redoTurns, turn) {
  if (redoTurns.length === 0) return redoTurns;
  if (formatTurn(redoTurns[0]) === formatTurn(turn)) {
    // Next turn matches the first in redoTurns. Keep the rest.
    return redoTurns.slice(1);
  } else {
    // Next turn doesn't match. Clear redoTurns.
    return [];
  }
}

class PlayComponent extends React.Component {

  constructor(props) {
    super(props);

    this.forceUpdate = this.forceUpdate.bind(this);  // used below!
    this.handleMove = this.handleMove.bind(this);
    this.handlePush = this.handlePush.bind(this);
    this.handleUndo = this.handleUndo.bind(this);
    this.handleRedo = this.handleRedo.bind(this);
    this.handleRetryAnalysis = this.handleRetryAnalysis.bind(this);
    this.handlePlayAiMove = this.handlePlayAiMove.bind(this);
    this.handleExit = this.handleExit.bind(this);
    this.playFullTurn = this.playFullTurn.bind(this);
    this.maybeAutoplayAiMove = this.maybeAutoplayAiMove.bind(this);
    this.handleKeyDown = this.handleKeyDown.bind(this);

    this.state = {
      // Positions of pieces on the board.
      pieces: props.initialPiecesAtTurnStart[props.initialTurns.length],

      // Turns played so far, not including the current turn.
      // Each turn is a list of moves followed by one push.
      turns: props.initialTurns,

      // Moves played in the current turn.
      moves: [],

      // Pieces at the start of each turn. This has length 1 greater
      // than `turns`. Used to implement undo.
      piecesAtTurnStart: props.initialPiecesAtTurnStart,

      // Position analysis at start of the turn. This has length 1 greater
      // than `turns`. Used to implement analysis/auto play.
      analysisAtTurnStart: props.initialPiecesAtTurnStart.map(pieces =>
          new PositionAnalysis(pieces, this.forceUpdate)),

      // Currently loading detailed position analysis.
      //
      // This is used by the AI to play aggressively. While the object here
      // is non-null, the analysis is loading. It is reset to null after
      // playing, and whenever the state changes (e.g. undo pressed while
      // waiting for the analysis to load).
      detailedAnalysis: null,

      // List of turns available for redo. These are populated by undoTurn().
      redoTurns: [],

      // Animations to apply to pieces.
      // Keys are destination field indices, values are lists of objects
      // with two keys: `keyframes` and `options`, which are pased to
      // animate().
      pieceAnimations: {},

      // AI strength.
      strength: 5,

      // AI plays aggressively (by selecting successors with few ties)
      aggressive: false,

      // AI player that plays automatically.
      aiPlayer: -1,
    };

    this.lastPieceAnimations = null;
    this.animationEndTime = 0;
    this.aiTimeoutId = null;
  }

  get undoEnabled() {
    const {moves, turns} = this.state;
    return turns.length > 0 || moves.length > 0;
  }

  get redoEnabled() {
    return this.state.redoTurns.length > 0;
  }

  componentDidUpdate() {
    // Keep track of the end time for animations, so we can schedule an AI move
    // right after the current piece animations end.
    const now = Date.now();
    if (this.state.pieceAnimations != this.lastPieceAnimations) {
      this.lastPieceAnimations = this.state.pieceAnimations;
      if (this.state.pieceAnimations?.duration != null) {
        this.animationEndTime = Math.max(this.animationEndTime,
          now + this.state.pieceAnimations?.duration);
      }
    }
    // Delay before auto-playing.
    const aiDelay = 500;  // milliseconds
    // Schedule an attempt to play as the AI.
    if (this.aiTimeoutId != null) {
      clearTimeout(this.aiTimeoutId);
    }
    this.aiTimeoutId = setTimeout(
      this.maybeAutoplayAiMove,
      Math.max(0, this.animationEndTime - now + aiDelay));
  }

  maybeAutoplayAiMove() {
    const {turns, moves, analysisAtTurnStart, redoTurns, strength, aggressive, aiPlayer} = this.state;

    // Block autoplay when the redo stack is nonempty. This allows the user
    // to undo and redo without the AI clearing the redo stack by making a
    // different move than before.
    if (redoTurns.length > 0) return;

    const {validity, nextPlayer} = validatePieces(this.state.pieces);

    // Check that it's my turn.
    if (nextPlayer !== aiPlayer) return;

    // Check that the position is in a playable state.
    if (validity !== PiecesValidity.STARTED && validity !== PiecesValidity.IN_PROGRESS) return;

    // Don't auto-play if the player has already started moving.
    if (moves.length !== 0) return;

    // Wait until analysis is available.
    const analysis = analysisAtTurnStart[turns.length];
    if (analysis?.result == null) return;

    // Make sure there are successors available.
    const successors = analysis.result.successors;
    if (successors.length === 0) return;

    this.selectAiMove(strength, aggressive, successors);
  }

  selectAiMove(strength, aggressive, successors) {
    if (!aggressive || strength < 2) {
      // Randomly pick one of the best moves to play.
      this.playFullTurn(parseTurn(ai.selectRandomMove(strength, successors)));
    } else if (this.state.detailedAnalysis == null) {
      // Playing aggressively.
      const detailedAnalysis = new Object();

      let callback, tryFetch;  // forward declaration of mutually recursive functions

      callback = (value, error) => {
        // Check if update has been superseded by another state change.
        if (this.state.detailedAnalysis !== detailedAnalysis) return;

        if (error) {
          console.error('Detailed analysis failed!\n', error);  // includes stack trace
          if (confirm('Detailed analysis failed!\n' + error + '\nPress OK to retry.')) {
            tryFetch();
          } else {
            this.setState(() => ({
              detailedAnalysis: null,
              aiPlayer: -1,
            }));
          }
        } else {
          const detailedSuccessors = value.successors;
          this.playFullTurn(parseTurn(ai.selectAggressiveMove(strength, detailedSuccessors)));
          this.setState(() => ({detailedAnalysis: null}));
        }
      }

      tryFetch = () => {
        // Check if update has been superseded by another state change.
        if (this.state.detailedAnalysis !== detailedAnalysis) return;

        fetchPositionAnalysis(this.state.pieces, true)
            .then(
                result => callback(result),
                error => callback(undefined, error));
      };

      this.setState(() => ({detailedAnalysis}), tryFetch);
    }
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
        detailedAnalysis: null,
        pieces: Object.freeze(newPieces),
        moves,
        pieceAnimations,
      };
    });
  }

  handlePush(src, dst) {
    this.setState(({pieces, turns, moves, piecesAtTurnStart, analysisAtTurnStart, redoTurns}) => {
      const push = Object.freeze([src, dst]);
      const turn = [...moves, push];
      const [pieceAnimations, newPieces] = generatePieceAnimations(pieces, push);
      return {
        pieces: Object.freeze(newPieces),
        turns: [...turns, turn],
        moves: [],
        piecesAtTurnStart: [...piecesAtTurnStart, newPieces],
        analysisAtTurnStart: [
            ...analysisAtTurnStart,
            new PositionAnalysis(newPieces, this.forceUpdate)],
        detailedAnalysis: null,
        redoTurns: updateRedoTurns(redoTurns, turn),
        pieceAnimations,
      };
    });
  }

  handleRetryAnalysis() {
    this.setState(({turns, piecesAtTurnStart, analysisAtTurnStart}) => ({
      analysisAtTurnStart: [
        ...analysisAtTurnStart.slice(0, turns.length),
        new PositionAnalysis(piecesAtTurnStart[turns.length], this.forceUpdate)],
      detailedAnalysis: null,
    }));
  }

  // Plays the given turn, overriding any partial moves played so far.
  playFullTurn(turn) {
    this.setState(({turns, piecesAtTurnStart, analysisAtTurnStart, redoTurns}) => {
      const oldPieces = piecesAtTurnStart[turns.length];
      const [pieceAnimations, newPieces] = generatePieceAnimations(oldPieces, ...turn);
      return {
        pieces: newPieces,
        turns: [...turns, turn],
        moves: [],
        piecesAtTurnStart: [...piecesAtTurnStart, newPieces],
        analysisAtTurnStart: [
            ...analysisAtTurnStart,
            new PositionAnalysis(newPieces, this.forceUpdate)],
        detailedAnalysis: null,
        redoTurns: updateRedoTurns(redoTurns, turn),
        pieceAnimations,
      };
    });
  }

  handlePlayAiMove() {
    const {turns, analysisAtTurnStart, strength, aggressive} = this.state;
    const analysis = analysisAtTurnStart[turns.length];
    if (analysis == null || analysis.isLoading()) {
      // This shouldn't be possible, since the button is supposed
      // to be disabled until analysis is loaded.
      alert('Analysis not complete!');
      return;
    }
    const successors = analysis.result.successors;
    if (successors.length === 0) {
      alert('No moves available!');
      return;
    }
    this.selectAiMove(strength, aggressive, successors);
  }

  handleUndo() {
    this.setState(({moves, turns, piecesAtTurnStart, analysisAtTurnStart, redoTurns}) => {
      if (moves.length > 0) {
        // Undo only the moves in the current turn.
        return {
          pieces: piecesAtTurnStart[turns.length],
          moves: [],
          detailedAnalysis: null,
          pieceAnimations: {},
        };
      }
      if (turns.length > 0) {
        // Undo the current turn.
        return {
          pieces: piecesAtTurnStart[turns.length - 1],
          turns: turns.slice(0, turns.length - 1),
          piecesAtTurnStart: piecesAtTurnStart.slice(0, turns.length),
          analysisAtTurnStart: analysisAtTurnStart.slice(0, turns.length),
          detailedAnalysis: null,
          pieceAnimations: {},
          redoTurns: [turns[turns.length - 1], ...redoTurns],
        };
      }
    });
  }

  handleRedo() {
    if (this.state.redoTurns.length > 0) {
      this.playFullTurn(this.state.redoTurns[0]);
    }
  }

  handleExit() {
    if (this.state.turns.length == 0 ||
        confirm('Are you sure? The move history will be lost.')) {
      this.props.onExit(this.state.piecesAtTurnStart[this.state.turns.length]);
    }
  }

  handleKeyDown(ev) {
    if (ev.altKey || ev.ctrlKey || ev.shiftKey || ev.metaKey) return;
    if (ev.key === 'ArrowLeft' && this.undoEnabled) this.handleUndo();
    if (ev.key === 'ArrowRight' && this.redoEnabled) this.handleRedo();
  }

  render() {
    const {pieces, turns, moves, analysisAtTurnStart, piecesAtTurnStart,
        detailedAnalysis, pieceAnimations, strength, aggressive, aiPlayer} = this.state;
    const {validity, error, nextPlayer, winner, index} = validatePieces(pieces);

    const isUnfinished = validity === PiecesValidity.STARTED || validity === PiecesValidity.IN_PROGRESS;
    const statusMessage =
        isUnfinished ? ['Red', 'Blue'][nextPlayer] + ' to move.' :
        validity === PiecesValidity.FINISHED ? ['Red', 'Blue'][winner] + ' won.' :
        error || 'Unknown error!';

    const firstPlayer = this.state.piecesAtTurnStart[0].includes(RED_ANCHOR) ? BLUE_PLAYER : RED_PLAYER;

    const autoplayBlocked = this.redoEnabled && aiPlayer >= 0 && aiPlayer === nextPlayer;

    const analysis = analysisAtTurnStart[turns.length];

    // Note: this is an arrow function so we can use `this` inside to refer
    // to the PlayComponent instance.
    const renderTab = (tab) => (
      // Note: components are made invisible rather than removed so that they
      // don't lose their state when the user switches between tabs.
      <React.Fragment>
        <AI
            visible={tab === 'ai'}
            strength={strength}
            aggressive={aggressive}
            aiPlayer={aiPlayer}
            onChangeStrength={(strength) => this.setState({strength})}
            onChangeAggressive={(aggressive) => this.setState({aggressive})}
            onChangeAiPlayer={(aiPlayer) => this.setState({aiPlayer})}
            autoplayBlocked={autoplayBlocked}
        />
        <History
          visible={tab === 'history'}
          firstPlayer={firstPlayer}
          turns={turns}
          moves={isUnfinished ? moves : undefined}
          analyses={analysisAtTurnStart}
        />
        <Analysis
          visible={tab === 'analysis'}
          validity={validity}
          analysis={analysis}
          onSelectTurn={this.playFullTurn}
          onRetryAnalysisClick={this.handleRetryAnalysis}
        />
      </React.Fragment>
    );

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
          <PlayPanel
              renderTab={renderTab}>
            <div className="buttons">
              <button disabled={!this.undoEnabled} onClick={this.handleUndo}>
                Undo turn
              </button>
              <button disabled={!this.redoEnabled} onClick={this.handleRedo}>
                Redo turn
              </button>
              <button
                  disabled={analysis.result == null || detailedAnalysis != null}
                  onClick={this.handlePlayAiMove}>
                Play AI Move
              </button>
              <button onClick={this.handleExit}>
                Return to setup
              </button>
            </div>
            {detailedAnalysis != null &&
              <div className="fetching-details">
                Fetching detailed analysis for aggressive AI...
              </div>}
          </PlayPanel>
        </div>
        <UpdateUrlHash pieces={piecesAtTurnStart[0]} turns={turns}/>
        <KeyDownListener onKeyDown={this.handleKeyDown}/>
      </React.Fragment>
    );
  }
}

// Parse the history parameter written by UpdateUrlHash.
//
// Returns an object with keys {turns, piecesAtTurnStart} or undefined if the
// string could not be parsed.
function parseHistoryValue(string) {
  const parts = string.split(';');
  let pieces = parsePieces(parts[0]);
  if (pieces == null) return undefined;
  const {validity} = validatePieces(pieces);
  if (validity === PiecesValidity.INVALID) return undefined;
  const turns = [];
  const piecesAtTurnStart = [];
  for (let i = 1; i < parts.length; ++i) {
    const {validity, nextPlayer} = validatePieces(pieces);
    if (validity === PiecesValidity.INVALID) return undefined;
    if (validity === PiecesValidity.FINISHED) break;
    const turn = parseTurn(parts[i]);
    if (turn == null) return undefined;
    const oldPieces = pieces;
    const newPieces = validateTurn(pieces, nextPlayer, turn);
    if (newPieces == null) break;
    turns.push(turn);
    piecesAtTurnStart.push(oldPieces);
    pieces = newPieces;
  }
  piecesAtTurnStart.push(pieces);
  return {turns, piecesAtTurnStart};
}

class RootComponent extends React.Component {
  state = {
    setupComplete: false,
    turns: [],
    piecesAtTurnStart: [INITIAL_PIECES],
  };

  constructor(props) {
    super(props);
    this.handleStart = this.handleStart.bind(this);
    this.handleExit = this.handleExit.bind(this);

    // Parse initial state from the URL hash. This is the reverse of what UpdateUrlHash does.
    if (window.location.hash && window.location.hash[0] === '#') {
      const params = new URLSearchParams(window.location.hash.substring(1));
      const historyParam = params.get('history');
      if (historyParam) {
        const parsedHistory = parseHistoryValue(historyParam);
        this.state.piecesAtTurnStart = parsedHistory.piecesAtTurnStart;
        this.state.turns = parsedHistory.turns;
        this.state.setupComplete = true;
      }
    }
  }

  handleStart(pieces) {
    this.setState({setupComplete: true, piecesAtTurnStart: [pieces], turns: []});
  }

  handleExit(pieces) {
    this.setState({setupComplete: false, piecesAtTurnStart: [pieces], turns: []});
  }

  render() {
    const {setupComplete, piecesAtTurnStart, turns} = this.state;
    let content = !setupComplete ?
        <SetUpComponent
            initialPieces={piecesAtTurnStart[0]}
            onStart={this.handleStart}/> :
        <PlayComponent
            initialTurns={turns}
            initialPiecesAtTurnStart={piecesAtTurnStart}
            onExit={this.handleExit}/>;
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
