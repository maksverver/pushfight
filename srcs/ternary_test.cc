#include "macros.h"
#include "ternary.h"

#ifdef NDEBUG
#error "Can't compile test with -DNDEBUG!"
#endif
#include <assert.h>

int main() {
  size_t offset = 0;
  REP(a, 3) REP(b, 3) REP(c, 3) REP(d, 3) REP(e, 3) {
    offset += 31337;
    uint8_t byte = 0;
    byte = EncodeTernary(byte, offset + 0, a);
    byte = EncodeTernary(byte, offset + 1, b);
    byte = EncodeTernary(byte, offset + 2, c);
    byte = EncodeTernary(byte, offset + 3, d);
    byte = EncodeTernary(byte, offset + 4, e);
    assert(byte < 243);

    assert(DecodeTernary(byte, offset + 0) == a);
    assert(DecodeTernary(byte, offset + 1) == b);
    assert(DecodeTernary(byte, offset + 2) == c);
    assert(DecodeTernary(byte, offset + 3) == d);
    assert(DecodeTernary(byte, offset + 4) == e);

    REP(i, 5) REP(value, 3) {
      uint8_t new_byte = EncodeTernary(byte, offset + i, value);
      assert(new_byte < 243);
      assert(DecodeTernary(new_byte, offset + 0) == (i == 0 ? value : a));
      assert(DecodeTernary(new_byte, offset + 1) == (i == 1 ? value : b));
      assert(DecodeTernary(new_byte, offset + 2) == (i == 2 ? value : c));
      assert(DecodeTernary(new_byte, offset + 3) == (i == 3 ? value : d));
      assert(DecodeTernary(new_byte, offset + 4) == (i == 4 ? value : e));
    }
  }
}
