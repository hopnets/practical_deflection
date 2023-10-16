#include <stdlib.h>

long apply_function_to_seq(long seq, long ret_count) {
    for (int i = 0; i < ret_count; i++) {
        // todo: here we should define function
        seq -= 10;
    }
    return std::max(0, seq);
}
