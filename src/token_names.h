/*
 * HalcyonScript - Token type names
 *
 * This file contains the string representations of token types.
 * Keep in sync with HcsTokenType enum in halcyon.h
 */

#ifndef TOKEN_NAMES_H
#define TOKEN_NAMES_H

#include "../include/halcyon.h"

/*
 * Token type names organized by category for better maintainability.
 * IMPORTANT: This array must match the HcsTokenType enum order exactly.
 */
static const char *TOKEN_TYPE_NAMES[] = {
    /* Widgets */
    "CREATE", "WINDOW", "BUTTON", "LABEL", "INPUT", "LISTBOX",
    "CHECKBOX", "IMAGE", "PANEL", "TEXTAREA", "DROPDOWN", "SLIDER",
    "PROGRESS", "TAB", "TABS", "MENU", "MENUITEM", "TOOLBAR",
    "STATUSBAR", "TREEVIEW", "TABLE", "CANVAS", "SPLITTER",
    "SCROLLBAR", "TOOLTIP", "DIALOG",

    /* Events */
    "WHEN", "CLICKED", "CHANGED", "STARTED", "CHECKED", "CLOSED",
    "RESIZED", "KEYDOWN", "KEYUP", "KEYPRESS", "MOUSEMOVE",
    "MOUSEDOWN", "MOUSEUP", "DOUBLECLICK", "RIGHTCLICK", "FOCUS",
    "BLUR", "SCROLL", "DRAG", "DROP", "TIMER", "TICK",
    "SELECTED", "HOVER",

    /* Actions */
    "SHOW", "HIDE", "CLOSE", "OPEN", "MINIMIZE", "MAXIMIZE",
    "RESTORE", "SET", "GET", "ADD", "REMOVE", "CLEAR", "INSERT",
    "UPDATE", "ENABLE", "DISABLE", "FOCUS_ACTION", "SELECT",
    "DESELECT", "PLAY", "PAUSE", "STOP", "RESUME", "SEEK",
    "LOAD", "START", "INTERVAL", "TIMEOUT",

    /* Control flow */
    "IF", "ELSE", "ELSEIF", "WHILE", "FOR", "FROM", "TO",
    "STEP", "BREAK", "CONTINUE", "FUNC", "RETURN", "IMPORT",
    "EXPORT", "CLASS", "NEW", "THIS", "EXTENDS", "SWITCH",
    "CASE", "DEFAULT", "IN",

    /* Declarations and literals */
    "VAR", "CONST", "LET", "GLOBAL", "TRUE", "FALSE", "NULL",
    "AND", "OR", "NOT", "IS", "AS",

    /* Built-in functions */
    "PRINT", "ALERT", "CONFIRM", "PROMPT", "LOG", "DEBUG",
    "READ", "WRITE", "APPEND", "DELETE", "EXISTS", "COPY",
    "MOVE", "MKDIR", "LISTDIR", "HTTP", "FETCH", "REQUEST",
    "RESPONSE", "WAIT", "ASYNC", "AWAIT", "PARALLEL",
    "TRY", "CATCH", "THROW", "FINALLY",
    "JSON", "PARSE", "STRINGIFY", "ENCODE", "DECODE",
    "REGEX", "MATCH", "TEST", "SEARCH",
    "RUN", "EXEC", "SHELL", "EXIT", "ENV", "CLIPBOARD",
    "NOTIFY", "BEEP",

    /* Value types */
    "NUMBER", "STRING", "IDENTIFIER",

    /* Operators */
    "PLUS", "MINUS", "MULTIPLY", "DIVIDE", "MODULO", "POWER",
    "EQUAL", "NOT_EQUAL", "GREATER", "LESS", "GREATER_EQ", "LESS_EQ",
    "ASSIGN", "PLUS_ASSIGN", "MINUS_ASSIGN", "MUL_ASSIGN", "DIV_ASSIGN",
    "INCREMENT", "DECREMENT",

    /* Punctuation */
    "LPAREN", "RPAREN", "LBRACE", "RBRACE", "LBRACKET", "RBRACKET",
    "COMMA", "DOT", "COLON", "SEMICOLON", "ARROW", "QUESTION",

    /* Special */
    "NEWLINE", "EOF", "UNKNOWN"};

/* Total number of token types */
#define TOKEN_TYPE_COUNT (sizeof(TOKEN_TYPE_NAMES) / sizeof(TOKEN_TYPE_NAMES[0]))

#endif