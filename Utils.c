#include "Utils.h"


void flushBuffer() {
    char c;
    while((c = getchar()) != '\n' && c != EOF);
}


