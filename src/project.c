#include "project.h"
#include "project_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
/* Windows compatible string comparison */
#define strcasecmp _stricmp
#else
#include <unistd.h>
#endif

static const char* TRUE_VALUE_STRINGS[] = TRUE_VALUES;
static const int NUM_TRUE_VALUES = TRUE_VALUES_COUNT;

typedef enum {
    SECTION_MAIN,
    SECTION_FILES,
    SECTION_INCLUDE
} ParserSection;

static bool string_to_bool(const char* str) {
    if (!str) return false;
    
    for (int i = 0; i < NUM_TRUE_VALUES; i++) {
        if (strcasecmp(str, TRUE_VALUE_STRINGS[i]) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_comment_char(char c) {
    for (int i = 0; i < COMMENT_CHARS_COUNT; i++) {
        if (c == COMMENT_CHARS[i]) {
            return true;
        }
    }
    return false;
}

static char* str_dup(const char* s) {
    if (!s) return NULL;
    
    char* d = malloc(strlen(s) + 1);
    if (!d) return NULL;
    
    strcpy(d, s);
    return d;
}

static char* trim(char* str) {
    if (!str) return NULL;
    
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    *(end + 1) = '\0';
    return str;
}

static char* get_directory(const char* path) {
    if (!path) return str_dup(".");
    
    char* dir = str_dup(path);
    if (!dir) return str_dup(".");
    
    char* last_sep = strrchr(dir, PATH_SEPARATOR);
    if (!last_sep) last_sep = strrchr(dir, '/');
    
    if (last_sep) {
        *last_sep = '\0';
        if (dir[0] == '\0') {
            free(dir);
            return str_dup(".");
        }
        return dir;
    }
    
    free(dir);
    return str_dup(".");
}

static char* join_path(const char* dir, const char* file) {
    if (!dir || !file) return NULL;
    
    size_t total_len = strlen(dir) + strlen(file) + 2;
    char* result = malloc(total_len);
    if (!result) return NULL;
    
    snprintf(result, total_len, "%s%c%s", dir, PATH_SEPARATOR, file);
    return result;
}

static bool file_exists(const char* path) {
    FILE* f = fopen(path, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

HcsProject* project_create(void) {
    HcsProject* proj = calloc(1, sizeof(HcsProject));
    if (!proj) return NULL;
    
    proj->name = str_dup(DEFAULT_PROJECT_NAME);
    proj->version = str_dup(DEFAULT_VERSION);
    proj->entry_point = str_dup(DEFAULT_ENTRY_POINT);
    proj->target = str_dup(DEFAULT_TARGET);
    
    if (!proj->name || !proj->version || !proj->entry_point || !proj->target) {
        project_free(proj);
        return NULL;
    }
    
    proj->debug = true;
    return proj;
}

static bool is_comment_or_empty(const char* line) {
    if (!line || line[0] == '\0') return true;
    return is_comment_char(line[0]);
}

static ParserSection parse_section_header(const char* line) {
    if (strcmp(line, SECTION_FILES_HEADER) == 0) return SECTION_FILES;
    if (strcmp(line, SECTION_INCLUDE_HEADER) == 0) return SECTION_INCLUDE;
    if (strcmp(line, SECTION_PROJECT) == 0) return SECTION_MAIN;
    return SECTION_MAIN;
}

static bool parse_key_value(HcsProject* proj, const char* key, char* value) {
    if (!key || !value) return false;
    
    if (strcmp(key, KEY_NAME) == 0) {
        free(proj->name);
        proj->name = str_dup(value);
        return proj->name != NULL;
    }
    if (strcmp(key, KEY_VERSION) == 0) {
        free(proj->version);
        proj->version = str_dup(value);
        return proj->version != NULL;
    }
    if (strcmp(key, KEY_AUTHOR) == 0) {
        free(proj->author);
        proj->author = str_dup(value);
        return true;
    }
    if (strcmp(key, KEY_DESCRIPTION) == 0) {
        free(proj->description);
        proj->description = str_dup(value);
        return true;
    }
    if (strcmp(key, KEY_ENTRY) == 0) {
        free(proj->entry_point);
        proj->entry_point = str_dup(value);
        return proj->entry_point != NULL;
    }
    if (strcmp(key, KEY_OUTPUT) == 0) {
        free(proj->output);
        proj->output = str_dup(value);
        return true;
    }
    if (strcmp(key, KEY_ICON) == 0) {
        free(proj->icon);
        proj->icon = str_dup(value);
        return true;
    }
    if (strcmp(key, KEY_TARGET) == 0) {
        free(proj->target);
        proj->target = str_dup(value);
        return proj->target != NULL;
    }
    if (strcmp(key, KEY_DEBUG) == 0) {
        proj->debug = string_to_bool(value);
        return true;
    }
    if (strcmp(key, KEY_OPTIMIZE) == 0) {
        proj->optimize = string_to_bool(value);
        return true;
    }
    
    return true;
}

static void remove_quotes(char* str) {
    if (!str || str[0] != '"') return;
    
    size_t len = strlen(str);
    if (len > 1 && str[len - 1] == '"') {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

static bool parse_main_section_line(HcsProject* proj, char* line) {
    char* eq = strchr(line, '=');
    if (!eq) return true;
    
    *eq = '\0';
    char* key = trim(line);
    char* value = trim(eq + 1);
    
    remove_quotes(value);
    return parse_key_value(proj, key, value);
}

static bool add_project_file(HcsProject* proj, const char* file) {
    if (proj->file_count >= MAX_PROJECT_FILES) {
        fprintf(stderr, "Warning: Maximum project files (%d) reached\n", 
                MAX_PROJECT_FILES);
        return false;
    }
    
    proj->files[proj->file_count] = str_dup(file);
    if (!proj->files[proj->file_count]) return false;
    
    proj->file_count++;
    return true;
}

static bool add_include_dir(HcsProject* proj, const char* dir) {
    if (proj->include_dir_count >= MAX_INCLUDE_DIRS) {
        fprintf(stderr, "Warning: Maximum include directories (%d) reached\n", 
                MAX_INCLUDE_DIRS);
        return false;
    }
    
    proj->include_dirs[proj->include_dir_count] = str_dup(dir);
    if (!proj->include_dirs[proj->include_dir_count]) return false;
    
    proj->include_dir_count++;
    return true;
}

void project_free(HcsProject* proj) {
    if (!proj) return;
    
    free(proj->name);
    free(proj->version);
    free(proj->author);
    free(proj->description);
    free(proj->entry_point);
    free(proj->output);
    free(proj->icon);
    free(proj->project_dir);
    free(proj->target);
    
    for (int i = 0; i < proj->file_count; i++) {
        free(proj->files[i]);
    }
    
    for (int i = 0; i < proj->include_dir_count; i++) {
        free(proj->include_dirs[i]);
    }
    
    free(proj);
}

HcsProject* project_load(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open project file: %s\n", path);
        return NULL;
    }
    
    HcsProject* proj = project_create();
    if (!proj) {
        fclose(f);
        return NULL;
    }
    
    free(proj->project_dir);
    proj->project_dir = get_directory(path);
    
    char line[MAX_LINE_LENGTH];
    ParserSection section = SECTION_MAIN;
    bool error = false;
    
    while (fgets(line, sizeof(line), f) && !error) {
        char* trimmed = trim(line);
        
        if (is_comment_or_empty(trimmed)) continue;
        
        if (trimmed[0] == '[') {
            section = parse_section_header(trimmed);
            continue;
        }
        
        switch (section) {
            case SECTION_MAIN:
                error = !parse_main_section_line(proj, trimmed);
                break;
                
            case SECTION_FILES:
                if (!add_project_file(proj, trimmed)) {
                    error = true;
                }
                break;
                
            case SECTION_INCLUDE:
                if (!add_include_dir(proj, trimmed)) {
                    error = true;
                }
                break;
        }
    }
    
    fclose(f);
    
    if (error) {
        project_free(proj);
        return NULL;
    }
    
    if (proj->file_count == 0 && proj->entry_point) {
        if (!add_project_file(proj, proj->entry_point)) {
            project_free(proj);
            return NULL;
        }
    }
    
    printf("Loaded project: %s v%s\n", proj->name, proj->version);
    printf("  Entry: %s\n", proj->entry_point);
    printf("  Files: %d\n", proj->file_count);
    
    return proj;
}

bool is_project_file(const char* path) {
    if (!path) return false;
    
    const char* ext = strrchr(path, '.');
    return ext && strcmp(ext, HCS_PROJECT_EXTENSION) == 0;
}

bool project_save(HcsProject* proj, const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return false;
    
    fprintf(f, "# HalcyonScript Project\n\n");
    fprintf(f, SECTION_PROJECT "\n");
    fprintf(f, KEY_NAME " = \"%s\"\n", 
            proj->name ? proj->name : DEFAULT_PROJECT_NAME);
    fprintf(f, KEY_VERSION " = \"%s\"\n", 
            proj->version ? proj->version : DEFAULT_VERSION);
    
    if (proj->author) {
        fprintf(f, KEY_AUTHOR " = \"%s\"\n", proj->author);
    }
    if (proj->description) {
        fprintf(f, KEY_DESCRIPTION " = \"%s\"\n", proj->description);
    }
    
    fprintf(f, KEY_ENTRY " = \"%s\"\n", 
            proj->entry_point ? proj->entry_point : DEFAULT_ENTRY_POINT);
    
    if (proj->output) {
        fprintf(f, KEY_OUTPUT " = \"%s\"\n", proj->output);
    }
    if (proj->icon) {
        fprintf(f, KEY_ICON " = \"%s\"\n", proj->icon);
    }
    
    fprintf(f, KEY_TARGET " = \"%s\"\n", 
            proj->target ? proj->target : DEFAULT_TARGET);
    
    fprintf(f, KEY_DEBUG " = %s\n", 
            proj->debug ? DEFAULT_TRUE_VALUE : DEFAULT_FALSE_VALUE);
    fprintf(f, KEY_OPTIMIZE " = %s\n", 
            proj->optimize ? DEFAULT_TRUE_VALUE : DEFAULT_FALSE_VALUE);
    
    if (proj->file_count > 0) {
        fprintf(f, "\n" SECTION_FILES_HEADER "\n");
        for (int i = 0; i < proj->file_count; i++) {
            fprintf(f, "%s\n", proj->files[i]);
        }
    }
    
    if (proj->include_dir_count > 0) {
        fprintf(f, "\n" SECTION_INCLUDE_HEADER "\n");
        for (int i = 0; i < proj->include_dir_count; i++) {
            fprintf(f, "%s\n", proj->include_dirs[i]);
        }
    }
    
    fclose(f);
    return true;
}

char* project_get_file_path(HcsProject* proj, const char* relative_path) {
    if (!proj || !relative_path) return NULL;
    
    if (IS_ABSOLUTE_PATH(relative_path)) {
        return str_dup(relative_path);
    }
    
    return join_path(proj->project_dir, relative_path);
}

char* project_resolve_import(HcsProject* proj, const char* import_path, 
                             const char* current_file) {
    if (!import_path) return NULL;
    
    if (current_file) {
        char* current_dir = get_directory(current_file);
        char* full_path = join_path(current_dir, import_path);
        free(current_dir);
        
        if (full_path && file_exists(full_path)) {
            return full_path;
        }
        free(full_path);
    }
    
    if (proj && proj->project_dir) {
        char* full_path = join_path(proj->project_dir, import_path);
        if (full_path && file_exists(full_path)) {
            return full_path;
        }
        free(full_path);
        
        for (int i = 0; i < proj->include_dir_count; i++) {
            char* inc_dir = join_path(proj->project_dir, proj->include_dirs[i]);
            full_path = join_path(inc_dir, import_path);
            free(inc_dir);
            
            if (full_path && file_exists(full_path)) {
                return full_path;
            }
            free(full_path);
        }
    }
    
    if (file_exists(import_path)) {
        return str_dup(import_path);
    }
    
    return NULL;
}