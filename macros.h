#ifndef MACROS_H_INCLUDED
#define MACROS_H_INCLUDED

#define FOR(i, a, b)  for (int i = int(a), i##_end = int(b); i < i##_end; ++i)
#define FORE(i, a, b) for (int i = int(a), i##_end = int(b); i <= i##_end; ++i)
#define FORD(i, a, b) for (int i = int(b - 1), i##_start = int(a); i >= i##_start; --i)
#define REP(i, n) FOR(i, 0, n)
#define REPE(i, n) FORE(i, 0, n)
#define REPD(i, n) FORD(i, 0, n)

#endif // ndef MACROS_H_INCLUDED
