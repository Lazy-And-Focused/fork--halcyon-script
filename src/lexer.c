/*
 * HalcyonScript - Lexer implementation
 */

#include "lexer.h"
#include <string.h>
#include <ctype.h>

/* Configuration constants */
#define MAX_IDENTIFIER_LENGTH    256
#define MAX_NUMBER_LENGTH        50
#define ESCAPE_CHARACTER         '\\'
#define NULL_CHARACTER           '\0'
#define NEWLINE_CHARACTER        '\n'
#define TAB_CHARACTER            '\t'
#define CARRIAGE_RETURN_CHARACTER '\r'
#define SPACE_CHARACTER          ' '
#define UNDERSCORE_CHARACTER     '_'
#define DOT_CHARACTER            '.'
#define SINGLE_QUOTE_CHARACTER   '\''
#define DOUBLE_QUOTE_CHARACTER   '"'
#define HASH_COMMENT_CHARACTER   '#'
#define ASTERISK_CHARACTER       '*'
#define SLASH_CHARACTER          '/'
#define DOUBLE_SLASH_SEQUENCE    "//"
#define MULTILINE_COMMENT_START  "/*"
#define MULTILINE_COMMENT_END    "*/"

/* Token representation constants */
#define NEWLINE_REPRESENTATION   "\\n"
#define END_OF_FILE_REPRESENTATION ""
#define UNKNOWN_CHAR_BUFFER_SIZE 2

/* Operator length constants */
#define SINGLE_CHAR_OPERATOR_LENGTH 1
#define DOUBLE_CHAR_OPERATOR_LENGTH 2
#define OPERATOR_NOT_FOUND_LENGTH 0

/* Macros for table definitions */
#define OPERATOR(operator, type) {operator, HCS_TOK_##type, sizeof(operator) - 1}
#define HCS_KEYWORD(keyword, type) {#keyword, HCS_TOK_##type}

/* Keyword lookup table */
typedef struct {
    const char* keyword;
    HcsTokenType type;
} KeywordEntry;

static KeywordEntry keywords[] = {
    /* UI Controls */
    HCS_KEYWORD(button, BUTTON),
    HCS_KEYWORD(canvas, CANVAS),
    HCS_KEYWORD(checkbox, CHECKBOX),
    HCS_KEYWORD(combobox, DROPDOWN),
    HCS_KEYWORD(create, CREATE),
    HCS_KEYWORD(dialog, DIALOG),
    HCS_KEYWORD(dropdown, DROPDOWN),
    HCS_KEYWORD(grid, TABLE),
    HCS_KEYWORD(image, IMAGE),
    HCS_KEYWORD(input, INPUT),
    HCS_KEYWORD(label, LABEL),
    HCS_KEYWORD(listbox, LISTBOX),
    HCS_KEYWORD(menu, MENU),
    HCS_KEYWORD(menuitem, MENUITEM),
    HCS_KEYWORD(panel, PANEL),
    HCS_KEYWORD(progress, PROGRESS),
    HCS_KEYWORD(progressbar, PROGRESS),
    HCS_KEYWORD(slider, SLIDER),
    HCS_KEYWORD(splitter, SPLITTER),
    HCS_KEYWORD(statusbar, STATUSBAR),
    HCS_KEYWORD(tab, TAB),
    HCS_KEYWORD(tabcontrol, TABS),
    HCS_KEYWORD(table, TABLE),
    HCS_KEYWORD(tabs, TABS),
    HCS_KEYWORD(textarea, TEXTAREA),
    HCS_KEYWORD(toolbar, TOOLBAR),
    HCS_KEYWORD(tooltip, TOOLTIP),
    HCS_KEYWORD(trackbar, SLIDER),
    HCS_KEYWORD(tree, TREEVIEW),
    HCS_KEYWORD(treeview, TREEVIEW),
    HCS_KEYWORD(window, WINDOW),

    /* Events */
    HCS_KEYWORD(blur, BLUR),
    HCS_KEYWORD(change, CHANGED),
    HCS_KEYWORD(changed, CHANGED),
    HCS_KEYWORD(checked, CHECKED),
    HCS_KEYWORD(click, CLICKED),
    HCS_KEYWORD(clicked, CLICKED),
    HCS_KEYWORD(closed, CLOSED),
    HCS_KEYWORD(dblclick, DOUBLECLICK),
    HCS_KEYWORD(doubleclick, DOUBLECLICK),
    HCS_KEYWORD(drag, DRAG),
    HCS_KEYWORD(drop, DROP),
    HCS_KEYWORD(focus, FOCUS),
    HCS_KEYWORD(hover, HOVER),
    HCS_KEYWORD(keydown, KEYDOWN),
    HCS_KEYWORD(keypress, KEYPRESS),
    HCS_KEYWORD(keyup, KEYUP),
    HCS_KEYWORD(mousedown, MOUSEDOWN),
    HCS_KEYWORD(mousemove, MOUSEMOVE),
    HCS_KEYWORD(mouseup, MOUSEUP),
    HCS_KEYWORD(resize, RESIZED),
    HCS_KEYWORD(resized, RESIZED),
    HCS_KEYWORD(rightclick, RIGHTCLICK),
    HCS_KEYWORD(scroll, SCROLL),
    HCS_KEYWORD(selected, SELECTED),
    HCS_KEYWORD(started, STARTED),
    HCS_KEYWORD(tick, TICK),
    HCS_KEYWORD(timer, TIMER),
    HCS_KEYWORD(when, WHEN),

    /* Control flow */
    HCS_KEYWORD(break, BREAK),
    HCS_KEYWORD(case, CASE),
    HCS_KEYWORD(class, CLASS),
    HCS_KEYWORD(continue, CONTINUE),
    HCS_KEYWORD(default, DEFAULT),
    HCS_KEYWORD(elif, ELSEIF),
    HCS_KEYWORD(else, ELSE),
    HCS_KEYWORD(elseif, ELSEIF),
    HCS_KEYWORD(export, EXPORT),
    HCS_KEYWORD(extends, EXTENDS),
    HCS_KEYWORD(for, FOR),
    HCS_KEYWORD(from, FROM),
    HCS_KEYWORD(func, FUNC),
    HCS_KEYWORD(function, FUNC),
    HCS_KEYWORD(if, IF),
    HCS_KEYWORD(import, IMPORT),
    HCS_KEYWORD(in, IN),
    HCS_KEYWORD(new, NEW),
    HCS_KEYWORD(return, RETURN),
    HCS_KEYWORD(step, STEP),
    HCS_KEYWORD(switch, SWITCH),
    HCS_KEYWORD(this, THIS),
    HCS_KEYWORD(to, TO),
    HCS_KEYWORD(while, WHILE),

    /* Variables */
    HCS_KEYWORD(and, AND),
    HCS_KEYWORD(as, AS),
    HCS_KEYWORD(const, CONST),
    HCS_KEYWORD(false, FALSE),
    HCS_KEYWORD(global, GLOBAL),
    HCS_KEYWORD(is, IS),
    HCS_KEYWORD(let, LET),
    HCS_KEYWORD(none, NULL),
    HCS_KEYWORD(not, NOT),
    HCS_KEYWORD(null, NULL),
    HCS_KEYWORD(or, OR),
    HCS_KEYWORD(true, TRUE),
    HCS_KEYWORD(var, VAR),

    {NULL, HCS_TOK_UNKNOWN}
};

typedef struct {
    const char* operator;
    HcsTokenType type;
    size_t length;
} OperatorEntry;

static OperatorEntry operators[] = {
    OPERATOR("++", INCREMENT),
    OPERATOR("--", DECREMENT),
    OPERATOR("+=", PLUS_ASSIGN),
    OPERATOR("-=", MINUS_ASSIGN),
    OPERATOR("->", ARROW),
    OPERATOR("**", POWER),
    OPERATOR("*=", MUL_ASSIGN),
    OPERATOR("/=", DIV_ASSIGN),
    OPERATOR("==", EQUAL),
    OPERATOR("=>", ARROW),
    OPERATOR("!=", NOT_EQUAL),
    OPERATOR(">=", GREATER_EQ),
    OPERATOR("<=", LESS_EQ),
    OPERATOR("&&", AND),
    OPERATOR("||", OR),

    OPERATOR("+", PLUS),
    OPERATOR("-", MINUS),
    OPERATOR("*", MULTIPLY),
    OPERATOR("/", DIVIDE),
    OPERATOR("%", MODULO),
    OPERATOR("=", ASSIGN),
    OPERATOR("!", NOT),
    OPERATOR(">", GREATER),
    OPERATOR("<", LESS),
    OPERATOR("&", UNKNOWN),
    OPERATOR("|", UNKNOWN),
    OPERATOR("(", LPAREN),
    OPERATOR(")", RPAREN),
    OPERATOR("{", LBRACE),
    OPERATOR("}", RBRACE),
    OPERATOR("[", LBRACKET),
    OPERATOR("]", RBRACKET),
    OPERATOR(",", COMMA),
    OPERATOR(".", DOT),
    OPERATOR(":", COLON),
    OPERATOR(";", SEMICOLON),
    OPERATOR("?", QUESTION),

    {NULL, HCS_TOK_UNKNOWN, OPERATOR_NOT_FOUND_LENGTH}
};

static char lexer_get_current(HcsLexer* lexer) {
    return lexer->position < lexer->length ? lexer->source[lexer->position] : NULL_CHARACTER;
}

static char lexer_get_next(HcsLexer* lexer) {
    return lexer->position + 1 < lexer->length ? lexer->source[lexer->position + 1] : NULL_CHARACTER;
}

static void lexer_advance(HcsLexer* lexer) {
    lexer->position++;
    lexer->column++;
}

static void lexer_advance_many(HcsLexer* lexer, int count) {
    for (int i = 0; i < count; i++) {
        lexer->position++;
        lexer->column++;
    }
}

static int insensitive_compare(const char* str1, const char* str2) {
    while (*str1 && *str2) {
        int c1 = tolower((unsigned char)*str1);
        int c2 = tolower((unsigned char)*str2);

        if (c1 != c2) {
            return c1 - c2;
        }
        
        str1++; str2++;
    }

    return tolower((unsigned char)*str1) - tolower((unsigned char)*str2);
}

static HcsTokenType lookup_keyword(const char* word) {
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        KeywordEntry keyword = keywords[i];
        bool isEquals = insensitive_compare(word, keyword.keyword) == 0;
        if (!isEquals) {
            continue;
        }

        return keyword.type;
    }

    return HCS_TOK_IDENTIFIER;
}

HcsLexer* lexer_create(const char* source) {
    HcsLexer* lexer = (HcsLexer*)malloc(sizeof(HcsLexer));
    if (!lexer) {
        return NULL;
    }
    
    lexer->source = source;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->length = strlen(source);
    return lexer;
}

void lexer_free(HcsLexer* lexer) {
    free(lexer);
}

static void skip_whitespace_and_comments(HcsLexer* lexer) {
    while (lexer->position < lexer->length) {
        char current = lexer_get_current(lexer);
        char next = lexer_get_next(lexer);
        
        bool isWhitespace = (current == SPACE_CHARACTER || 
                             current == TAB_CHARACTER || 
                             current == CARRIAGE_RETURN_CHARACTER);
        if (isWhitespace) {
            lexer_advance(lexer);
            continue;
        }
        
        bool isSingleLineComment = ((current == SLASH_CHARACTER && next == SLASH_CHARACTER) || 
                                    current == HASH_COMMENT_CHARACTER);
        if (isSingleLineComment) {
            while (lexer->position < lexer->length && lexer_get_current(lexer) != NEWLINE_CHARACTER) {
                lexer_advance(lexer);
            }
            continue;
        }
        
        bool isMultiLineComment = (current == SLASH_CHARACTER && next == ASTERISK_CHARACTER);
        if (isMultiLineComment) {
            lexer_advance_many(lexer, DOUBLE_CHAR_OPERATOR_LENGTH);
            
            while (lexer->position < lexer->length) {
                bool isCommentEnd = (lexer_get_current(lexer) == ASTERISK_CHARACTER &&  
                                     lexer_get_next(lexer) == SLASH_CHARACTER);
                
                if (isCommentEnd) {
                    lexer_advance_many(lexer, DOUBLE_CHAR_OPERATOR_LENGTH);
                    break;
                }
                
                if (lexer_get_current(lexer) == NEWLINE_CHARACTER) {
                    lexer->line++;
                    lexer->column = 0;
                }
                
                lexer_advance(lexer);
            }
            continue;
        }
        
        break;
    }
}

static char* read_sequence(HcsLexer* lexer, int (*is_valid)(char), int max_length) {
    int start = lexer->position;
    int chars_read = 0;
    
    while (lexer->position < lexer->length && 
           is_valid(lexer_get_current(lexer)) &&
           chars_read < max_length) {
        lexer_advance(lexer);
        chars_read++;
    }
    
    int length = lexer->position - start;
    char* sequence = (char*)malloc(length + 1);
    if (!sequence) return NULL;
    
    strncpy(sequence, lexer->source + start, length);
    sequence[length] = NULL_CHARACTER;
    
    return sequence;
}

static int is_identifier_char_check(char c) {
    return isalnum((unsigned char)c) || c == UNDERSCORE_CHARACTER;
}

static int is_number_char_check(char c) {
    return isdigit((unsigned char)c) || c == DOT_CHARACTER;
}

static HcsToken* read_identifier(HcsLexer* lexer) {
    int start_column = lexer->column;
    char* value = read_sequence(lexer, is_identifier_char_check, MAX_IDENTIFIER_LENGTH);
    if (!value) return NULL;
    
    HcsTokenType type = lookup_keyword(value);
    HcsToken* token = token_create(type, value, lexer->line, start_column);
    free(value);
    
    return token;
}

static HcsToken* read_number(HcsLexer* lexer) {
    int start_column = lexer->column;
    char* value = read_sequence(lexer, is_number_char_check, MAX_NUMBER_LENGTH);
    if (!value) return NULL;
    
    HcsToken* token = token_create(HCS_TOK_NUMBER, value, lexer->line, start_column);
    free(value);
    
    return token;
}

typedef struct {
    char code;
    char value;
} EscapeSequence;

static EscapeSequence escape_table[] = {
    {'n', '\n'},
    {'t', '\t'},
    {'r', '\r'},
    {'\\', '\\'},
    {'"', '"'},
    {'\'', '\''},
    {'0', '\0'},
    {'b', '\b'},
    {'f', '\f'},
    {'v', '\v'},
    {'a', '\a'},
    {0, 0}
};

static char resolve_escape_sequence(char code) {
    for (int i = 0; escape_table[i].code != 0; i++) {
        if (escape_table[i].code == code) {
            return escape_table[i].value;
        }
    }
    return code;
}

static HcsToken* read_string(HcsLexer* lexer, char quote) {
    int start_column = lexer->column;
    lexer_advance(lexer);
    
    char buffer[MAX_STRING_LEN];
    int length = 0;
    
    while (lexer->position < lexer->length && 
           lexer_get_current(lexer) != quote &&
           length < MAX_STRING_LEN - 1) {
        
        char current = lexer_get_current(lexer);
        if (current == ESCAPE_CHARACTER && lexer->position + 1 < lexer->length) {
            lexer_advance(lexer);
            buffer[length++] = resolve_escape_sequence(lexer_get_current(lexer));
        } else {
            buffer[length++] = current;
        }
        
        lexer_advance(lexer);
    }
    
    buffer[length] = NULL_CHARACTER;
    
    if (lexer->position < lexer->length) {
        lexer_advance(lexer);
    }
    
    return token_create(HCS_TOK_STRING, buffer, lexer->line, start_column);
}

static HcsToken* read_operator(HcsLexer* lexer) {
    int start_column = lexer->column;
    char current = lexer_get_current(lexer);
    char next = lexer_get_next(lexer);
    
    for (int index = 0; operators[index].operator != NULL; index++) {
        OperatorEntry operator = operators[index];

        bool isDoubleOperator = (operator.length == DOUBLE_CHAR_OPERATOR_LENGTH);
        bool isOperatorsEquals = (current == operator.operator[0] && next == operator.operator[1]);
        if (isDoubleOperator && isOperatorsEquals) {
            lexer_advance_many(lexer, DOUBLE_CHAR_OPERATOR_LENGTH);
            return token_create(operator.type, operator.operator, lexer->line, start_column);
        }

        bool isSingleOperator = (operator.length == SINGLE_CHAR_OPERATOR_LENGTH);
        bool isOperatorEquals = (current == operator.operator[0]);
        if (isSingleOperator && isOperatorEquals) {
            lexer_advance(lexer);
            return token_create(operator.type, operator.operator, lexer->line, start_column);
        }
    }

    char buffer[UNKNOWN_CHAR_BUFFER_SIZE] = { current, NULL_CHARACTER };
    lexer_advance(lexer);
    return token_create(HCS_TOK_UNKNOWN, buffer, lexer->line, start_column);
}

typedef HcsToken* (*TokenReader)(HcsLexer*);
typedef struct {
    bool (*predicate)(char);
    TokenReader reader;
} TokenHandler;

static bool is_newline_char(char c) {
    return c == NEWLINE_CHARACTER;
}

static bool is_identifier_start_char(char c) {
    return isalpha((unsigned char)c) || c == UNDERSCORE_CHARACTER;
}

static bool is_number_start_char(char c) {
    return isdigit((unsigned char)c);
}

static bool is_string_start_char(char c) {
    return c == DOUBLE_QUOTE_CHARACTER || c == SINGLE_QUOTE_CHARACTER;
}

static HcsToken* read_newline_token(HcsLexer* lexer) {
    HcsToken* token = token_create(HCS_TOK_NEWLINE, NEWLINE_REPRESENTATION, lexer->line, lexer->column);
    lexer_advance(lexer);
    lexer->line++;
    lexer->column = 1;
    return token;
}

static HcsToken* read_string_token(HcsLexer* lexer) {
    char quote = lexer_get_current(lexer);
    return read_string(lexer, quote);
}

static TokenHandler token_handlers[] = {
    {is_newline_char,        read_newline_token},
    {is_identifier_start_char, read_identifier},
    {is_number_start_char,   read_number},
    {is_string_start_char,   read_string_token},
    {NULL,                   read_operator}
};

HcsToken** lexer_tokenize(HcsLexer* lexer, int* token_count) {
    HcsToken** tokens = (HcsToken**)malloc(sizeof(HcsToken*) * MAX_TOKENS);
    int count = 0;
    
    while (lexer->position < lexer->length && count < MAX_TOKENS - 1) {
        skip_whitespace_and_comments(lexer);
        if (lexer->position >= lexer->length) {
            break;
        }
        
        char current = lexer_get_current(lexer);
        HcsToken* token = NULL;
        
        for (size_t i = 0; i < sizeof(token_handlers) / sizeof(token_handlers[0]); i++) {
            TokenHandler handler = token_handlers[i];
            
            if (handler.predicate == NULL || handler.predicate(current)) {
                token = handler.reader(lexer);
                break;
            }
        }
        
        if (token) {
            tokens[count++] = token;
        }
    }
    
    tokens[count++] = token_create(HCS_TOK_EOF, END_OF_FILE_REPRESENTATION, lexer->line, lexer->column);
    *token_count = count;
    return tokens;
}

void lexer_free_tokens(HcsToken** tokens, int count) {
    for (int i = 0; i < count; i++) {
        token_free(tokens[i]);
    }
    free(tokens);
}