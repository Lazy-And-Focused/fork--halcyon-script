#ifndef TOKEN_H
#define TOKEN_H

#include "../include/halcyon.h"

typedef struct {
    HcsTokenType type;
    char* value;
    int line;
    int column;
} HcsToken;

HcsToken* token_create(HcsTokenType type, const char* value, int line, int column);
void token_free(HcsToken* token);
const char* token_type_name(HcsTokenType type);

#endif