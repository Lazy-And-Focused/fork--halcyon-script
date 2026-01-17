#include "token.h"
#include "token_names.h"
#include <stdlib.h>
#include <string.h>

HcsToken* token_create(HcsTokenType type, const char* value, int line, int column) {
    HcsToken* token = malloc(sizeof(HcsToken));
    if (!token) return NULL;

    token->type = type;
    token->line = line;
    token->column = column;
    token->value = value ? strdup(value) : NULL;

    if (value && !token->value) {
        free(token);
        return NULL;
    }

    return token;
}

void token_free(HcsToken* token) {
    if (token) {
        free(token->value);
        free(token);
    }
}

const char* token_type_name(HcsTokenType type) {
    if (type >= 0 && type < TOKEN_TYPE_COUNT) {
        return TOKEN_TYPE_NAMES[type];
    }
    return "UNKNOWN";
}