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
    return successors[0].moves;
  }

  const maxDepth = strengthToMaxDepth(strength);

  // Check if there is a winning move within our search depth. Since moves
  // are sorted from best to worst, the quickest win is the first element.
  const [bestSign, bestMagnitude] = parseStatus(successors[0].status);
  if (bestSign === 1 && 2*bestMagnitude - 1 <= maxDepth) {
    // Winning moves available.
    return successors[0].moves;
  }

  // Otherwise, select randomly among ties and wins and losses that are outside
  // our search depth.
  const candidates = [];
  for (const s of successors) {
    // Check if a losing move is within our search depth. If so, we will avoid
    // it. Since groups are ordered from best to worst, all subsequent moves
    // will be even quicker, so we break out of the loop. The check
    // candidates.length > 0 is necessary to keep some candidates in the
    // case where only losing moves are available.
    const [sign, magnitude] = parseStatus(s.status);
    if (candidates.length > 0 && sign === -1 && 2*magnitude <= maxDepth) {
      break;
    }
    candidates.push(...s.moves);
  }
  return candidates;
}

// Selects a random move to play, subject to the given play strength.
//
// Play strength is an integer between 0 and MAX_STRENGTH (inclusive).
//
// `successors` is a list of successors grouped by status, as returned by
// PositionAnalysis. If the list is empty, this function returns undefined.
export function selectMove(strength, successors) {
  if (successors.length === 0) return undefined;
  const candidates = selectCandidates(strength, successors);
  return getRandomElement(candidates);
}
