/*
 * HalcyonScript - Token implementation
 */

#include "token.h"
#include "token_names.h"
#include <stdlib.h>
#include <string.h>

HcsToken* token_create(HcsTokenType type, const char* value, int line, int column) {
    HcsToken* token = (HcsToken*)malloc(sizeof(HcsToken));
    if (token == NULL) {
        return NULL;
    }

    token->type = type;
    token->line = line;
    token->column = column;

    if (value != NULL) {
        token->value = strdup(value);
        if (token->value == NULL) {
            free(token);
            return NULL;
        }
    } else {
        token->value = NULL;
    }

    return token;
}

void token_free(HcsToken* token) {
    if (token != NULL) {
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