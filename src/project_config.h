#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#define HCS_PROJECT_EXTENSION     ".halproj"
#define HCS_SCRIPT_EXTENSION      ".hcs"

#define SECTION_PROJECT           "[project]"
#define SECTION_FILES_HEADER      "[files]"
#define SECTION_INCLUDE_HEADER    "[include]"

#define KEY_NAME                  "name"
#define KEY_VERSION               "version"
#define KEY_AUTHOR                "author"
#define KEY_DESCRIPTION           "description"
#define KEY_ENTRY                 "entry"
#define KEY_OUTPUT                "output"
#define KEY_ICON                  "icon"
#define KEY_TARGET                "target"
#define KEY_DEBUG                 "debug"
#define KEY_OPTIMIZE              "optimize"

#define DEFAULT_PROJECT_NAME      "Untitled"
#define DEFAULT_VERSION           "1.0.0"
#define DEFAULT_ENTRY_POINT       "main.hcs"
#define DEFAULT_TARGET            "windows"
#define DEFAULT_TRUE_VALUE        "true"
#define DEFAULT_FALSE_VALUE       "false"

#define TRUE_VALUES               {"true", "1", "yes", "on"}
#define TRUE_VALUES_COUNT         4

#define COMMENT_CHARS             "#;"
#define COMMENT_CHARS_COUNT       2

#ifdef _WIN32
#define PATH_SEPARATOR           '\\'
#define PATH_SEPARATOR_STRING    "\\"
#define IS_ABSOLUTE_PATH(p)      ((p) && ((p)[0] == '/' || ((p)[0] != '\0' && (p)[1] == ':')))
#else
#define PATH_SEPARATOR           '/'
#define PATH_SEPARATOR_STRING    "/"
#define IS_ABSOLUTE_PATH(p)      ((p) && (p)[0] == '/')
#endif

#define MAX_PROJECT_FILES         256
#define MAX_INCLUDE_DIRS          32
#define MAX_LINE_LENGTH           1024
#define MAX_PATH_LENGTH           512

#endif