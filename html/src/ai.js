// Implements AI game play with a simulated depth-limited alpha-beta search.

import {parseStatus} from './analysis.js';
import {getRandomElement} from './random.js';

// Strength settings vary from 0 (completely random) to 15 (perfect play).
export const MAX_STRENGTH = 15;

// Converts strength setting to max search depth.
//
// Values from 1 to 8 (inclusive) correspond to search depth 1 through 8,
// while for values 9 through 14 the search depth increases exponentially:
// 10, 12, 16, 24, 40, 72.
export function strengthToMaxDepth(strength) {
  return strength <= 8 ? strength : 8 + (1 << (strength - 8));
}

// Selects candidate moves to chose randomly from.
//
// Conceptually, this considers all wins and losses that are too far away
// as equivalent to ties. Including them means that the AI will occasionally
// make mistakes.
//
// At search depth 0, the AI plays randomly.
// At search depth 1, it takes win-in-1, but doesn't avoid loss-in-1.
// At search depth 2, it also avoids loss-in-1.
// At search depth 3, it also takes win-in-2.
// And so on.
//
// Consequently, the AI can only win against itself if the search depth is odd,
// which only happens at strengths 1, 3, 5, and 7 (strength 8 and above all
// correspond to even search depths).
function selectCandidates(strength, successors) {
  if (strength >= MAX_STRENGTH) {
    // Perfect play: just select from optimal moves.
    return successors[0];
  }

  const maxDepth = strengthToMaxDepth(strength);

  // Check if there is a winning move within our search depth. Since moves
  // are sorted from best to worst, the quickest win is the first element.
  const [bestSign, bestMagnitude] = parseStatus(successors[0].status);
  if (bestSign === 1 && 2*bestMagnitude - 1 <= maxDepth) {
    // Winning moves available.
    return successors[0];
  }

  // Otherwise, select randomly among ties and wins and losses that are outside
  // our search depth.
  const moves = [];
  let details = undefined;
  for (const s of successors) {
    // Check if a losing move is within our search depth. If so, we will avoid
    // it. Since groups are ordered from best to worst, all subsequent moves
    // will be even quicker, so we break out of the loop. The check
    // candidates.length > 0 is necessary to keep some candidates in the
    // case where only losing moves are available.
    const [sign, magnitude] = parseStatus(s.status);
    if (moves.length > 0 && sign === -1 && 2*magnitude <= maxDepth) {
      break;
    }
    moves.push(...s.moves);
    if (s.details) {
      if (details == null) details = [];
      details.push(...s.details);
      if (details.length !== moves.length) {
        console.error('Invalid number of details!', successors);
        alert('Invalid number of details! (See JavaScript console for details)');
        return;
      }
    }
  }
  return {moves, details};
}

// Selects a random move to play, subject to the given play strength.
//
// Play strength is an integer between 0 and MAX_STRENGTH (inclusive).
//
// `successors` is a list of successors grouped by status, as returned by
// PositionAnalysis. If the list is empty, this function returns undefined.
export function selectRandomMove(strength, successors) {
  if (successors.length === 0) return undefined;
  const candidates = selectCandidates(strength, successors);
  return getRandomElement(candidates.moves);
}

// Selects an an aggressive move to play, subject to the given play strength,
// which must be at least 2 (!) and at most MAX_STRENGTH (inclusive).
//
// `successors` is a list of successors grouped by `status` with `details`.
export function selectAggressiveMove(strength, successors) {
  if (successors.length === 0) return undefined;
  const {moves, details} = selectCandidates(strength, successors);

  if (details == null) {
    // This happens when all moves are lost-in-1.
    return getRandomElement(moves);
  }

  const maxDepth = strengthToMaxDepth(strength);
  const bestMoves = [];
  let minValue = Infinity;
  for (let i = 0; i < moves.length; ++i) {
    // Details is a list of comma-separated pairs of successor status and count,
    // ordered from best to worst. Statuses are relative to the opponent; if a
    // turn is L9 for me, then it is W9 for the opponent (or if it's W2 for me,
    // then it's L1 for the opponent).
    //
    // Example string: 'W9*1,W28*1,T*49,L19*1,L12*1,..,L1*1798'
    //
    // The value of the move is the number of optimal moves. But as in
    // selectCandidates(), we evaluate the candidates relative to a simulated
    // search depth.
    let parsedDetails = details[i].split(',').map(d => {
      const [status, count] = d.split('*');
      const [sign, magnitude] = parseStatus(status);
      return [sign, magnitude, Number(count)];
    });
    let value = 0;
    const [bestSign, bestMagnitude, bestCount] = parsedDetails[0];
    if (bestSign === 1 && 2 * bestMagnitude <= maxDepth) {
      // Win-in-x is within search depth. Assume the opponent will play optimally.
      value = bestCount;
    } else {
      // Opponent has no win within the search depth. So consider all wins, ties
      // and losses outside the search depth as ties.
      for (const [sign, magnitude, count] of parsedDetails) {
        if (sign === -1 && 2 * magnitude + 1 <= maxDepth) break;
        value += count;
      }
    }
    if (value < minValue) {
      bestMoves.length = 0;
      minValue = value;
    }
    if (value === minValue) {
      bestMoves.push(moves[i]);
    }
  }
  return getRandomElement(bestMoves);
}
