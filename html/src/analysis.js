// Implements the logic for fetching position analysis results from the server.
//
// See lookup-min-http-server.py for a description of the server API.

import { validatePieces, PiecesValidity, formatPieces } from './board.js';

// For development: make the RPC to analyze positions intentionally slow and unreliable.
const unreliableAnalysis = false;

// Fetches the position analysis using LOOKUP_URL. Don't call this directly;
// use PositionAnalysis instead.
async function fetchPositionAnalysis(pieces) {
  async function fetchImpl(pieces) {
    const url = 'lookup/perms/' + encodeURIComponent(formatPieces(pieces, true));
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

// Parses a status string into a sign and magnitude.
//
// Examples:
//
//   parseStatus('W7') => [ 1, 7]
//   parseStatus('L0') => [-1, 0]
//   parseStatus('T')  => [ 0, 0]
//
export function parseStatus(string) {
  if (string[0] === 'W') return [+1, parseInt(string.substring(1), 10)];
  if (string[0] === 'L') return [-1, parseInt(string.substring(1), 10)];
  if (string === 'T') return [0, 0];
  return undefined;
}
