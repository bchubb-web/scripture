#include "buffer.hpp"
#include <cstdlib>
#include <cstring>

Buffer::Buffer() {
    b = NULL;
    len = 0;
}

void Buffer::append(const char *s, int len) {
    char *abNew = (char*) std::realloc(this->b, this->len + len);
    if (abNew == NULL) return;
    memcpy(&abNew[this->len], s, len);
    this->b = abNew;
    this->len += len;
}
void Buffer::free() {
    std::free(this->b);
}
