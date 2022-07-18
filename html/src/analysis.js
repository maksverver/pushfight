import { validatePieces, PiecesValidity, formatPieces } from './board.js';

// For development: make the RPC to analyze positions intentionally slow and unreliable.
const unreliableAnalysis = false;

const LOOKUP_URL = window.location.hostname === 'localhost' ? 'http://localhost:8003/' : 'lookup/';

// Fetches the position analysis using LOOKUP_URL. Don't call this directly;
// use PositionAnalysis instead.
async function fetchPositionAnalysis(pieces) {
  async function fetchImpl(pieces) {
    const url = LOOKUP_URL + '?perm=' + encodeURIComponent(formatPieces(pieces, true));
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

  if (!unreliableAnalysis) {
    return fetchImpl(pieces);
  }

  return new Promise((resolve, reject) => {
    setTimeout(() => {
      if (Math.random() < 0.25) {
        reject(new Error('Fake error'));
      }
      resolve(fetchImpl(pieces));
    }, Math.random() * 3000);
  });
}

// Returns an object that represents the analysis of a game position.
//
// It can be in one of four states:
//
//   - Skipped (position was invalid, or game is over)
//   - Loading (RPC started, but not yet completed)
//   - Failed (error != null)
//   - Succeeded (result != null)
//
export class PositionAnalysis {
  // Set to true if analysis was skipped because this position cannot be
  // analyzed (either the game is over, or the position is invalid).
  skipped = false

  // If loading failed, this is an Error object describing the reason.
  error = undefined;

  // If loading succeeded, this contains the result as an object with two
  // keys: "status" and "successors".
  //
  // "status" is the position status as a string (e.g. 'T' or 'L1').
  //
  // "successors" is a list of successors grouped by result status. Example:
  // [{'status': 'T', moves: [['a1-b1]]}, {'status': 'L1', moves: [...]}, ...]
  // Elements are ordered by decreasing status values (best first).
  result = undefined;

  // Note: callback is called only when the object is updated (i.e., it is
  // never called if analysis is skipped).
  constructor(pieces, callback) {
    const {validity} = validatePieces(pieces);

    if (validity === PiecesValidity.INVALID ||
        validity === PiecesValidity.FINISHED) {
      this.skipped = true;
      return;
    }

    fetchPositionAnalysis(pieces)
      .then(result => {
        this.result = result;
        if (callback) callback();
      })
      .catch(error => {
        this.error = error;
        if (callback) callback();
      });
  }

  isLoading() {
    return !this.skipped && this.error == null && this.result == null;
  }

  isLoaded() {
    return !this.isLoading();
  }
}
