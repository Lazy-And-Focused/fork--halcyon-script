/*
 * HalcyonScript Project System Implementation
 *
 * .halproj file format (simple key=value):
 *
 * name = MyProject
 * version = 1.0.0
 * author = Developer
 * entry = main.hcs
 *
 * [files]
 * main.hcs
 * ui/window.hcs
 * utils/helpers.hcs
 *
 * [include]
 * lib/
 * modules/
 */

#include "project.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEP '\\'
#define PATH_SEP_STR "\\"
#else
#include <unistd.h>
#define PATH_SEP '/'
#define PATH_SEP_STR "/"
#endif

#include "project.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEP '\\'
#define PATH_SEP_STR "\\"
#define IS_ABSOLUTE_PATH(p) ((p)[0] == '/' || ((p)[0] && (p)[1] == ':'))
#else
#include <unistd.h>
#define PATH_SEP '/'
#define PATH_SEP_STR "/"
#define IS_ABSOLUTE_PATH(p) ((p)[0] == '/')
#endif

typedef enum {
    SECTION_MAIN,
    SECTION_FILES,
    SECTION_INCLUDE
} ParserSection;

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
    
    char* last_sep = strrchr(dir, PATH_SEP);
    if (!last_sep) last_sep = strrchr(dir, '/');
    
    if (last_sep) {
        *last_sep = '\0';
    } else {
        free(dir);
        return str_dup(".");
    }
    
    return dir;
}

static char* join_path(const char* dir, const char* file) {
    if (!dir || !file) return NULL;
    
    size_t total_len = strlen(dir) + strlen(file) + 2;
    char* result = malloc(total_len);
    if (!result) return NULL;
    
    snprintf(result, total_len, "%s%c%s", dir, PATH_SEP, file);
    return result;
}

HcsProject *project_create(void)
{
    HcsProject *proj = calloc(1, sizeof(HcsProject));
    if (!proj)
        return NULL;

    proj->name = str_dup("Untitled");
    proj->version = str_dup("1.0.0");
    proj->entry_point = str_dup("main.hcs");
    proj->target = str_dup("windows");

    if (!proj->name || !proj->version || !proj->entry_point || !proj->target)
    {
        project_free(proj);
        return NULL;
    }

    proj->debug = true;
    return proj;
}

void project_free(HcsProject *proj)
{
    if (!proj)
        return;

    free(proj->name);
    free(proj->version);
    free(proj->author);
    free(proj->description);
    free(proj->entry_point);
    free(proj->output);
    free(proj->icon);
    free(proj->project_dir);
    free(proj->target);

    for (int i = 0; i < proj->file_count; i++)
    {
        free(proj->files[i]);
    }

    for (int i = 0; i < proj->include_dir_count; i++)
    {
        free(proj->include_dirs[i]);
    }

    free(proj);
}

typedef enum {
    SECTION_MAIN,
    SECTION_FILES,
    SECTION_INCLUDE
} ParserSection;

static bool parse_key_value(HcsProject* proj, const char* key, char* value) {
    if (!key || !value) return false;
    
    if (strcmp(key, "name") == 0) {
        free(proj->name);
        proj->name = str_dup(value);
        return proj->name != NULL;
    }
    if (strcmp(key, "version") == 0) {
        free(proj->version);
        proj->version = str_dup(value);
        return proj->version != NULL;
    }
    if (strcmp(key, "author") == 0) {
        free(proj->author);
        proj->author = str_dup(value);
        return true;
    }
    if (strcmp(key, "description") == 0) {
        free(proj->description);
        proj->description = str_dup(value);
        return true;
    }
    if (strcmp(key, "entry") == 0) {
        free(proj->entry_point);
        proj->entry_point = str_dup(value);
        return proj->entry_point != NULL;
    }
    if (strcmp(key, "output") == 0) {
        free(proj->output);
        proj->output = str_dup(value);
        return true;
    }
    if (strcmp(key, "icon") == 0) {
        free(proj->icon);
        proj->icon = str_dup(value);
        return true;
    }
    if (strcmp(key, "target") == 0) {
        free(proj->target);
        proj->target = str_dup(value);
        return proj->target != NULL;
    }
    if (strcmp(key, "debug") == 0) {
        proj->debug = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        return true;
    }
    if (strcmp(key, "optimize") == 0) {
        proj->optimize = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
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

static ParserSection parse_section_header(const char* line) {
    if (strncmp(line, "[files]", 7) == 0) return SECTION_FILES;
    if (strncmp(line, "[include]", 9) == 0) return SECTION_INCLUDE;
    if (strncmp(line, "[project]", 9) == 0) return SECTION_MAIN;
    return SECTION_MAIN;
}

static bool is_comment_or_empty(const char* line) {
    return line[0] == '\0' || line[0] == '#' || line[0] == ';';
}

static bool parse_main_section_line(HcsProject* proj, char* line) {
    char* eq = strchr(line, '=');
    if (!eq) return true;
    
    *eq = '\0';
    char* key = trim(line);
    char* value = trim(eq + 1);
    
    // Remove surrounding quotes
    if (value[0] == '"') {
        value++;
        char* end_quote = strchr(value, '"');
        if (end_quote) *end_quote = '\0';
    }
    
    return parse_key_value(proj, key, value);
}

static bool add_project_file(HcsProject* proj, const char* file) {
    if (proj->file_count >= HCS_MAX_PROJECT_FILES) {
        fprintf(stderr, "Warning: Maximum project files (%d) reached\n", 
                HCS_MAX_PROJECT_FILES);
        return false;
    }
    
    proj->files[proj->file_count] = str_dup(file);
    if (!proj->files[proj->file_count]) return false;
    
    proj->file_count++;
    return true;
}

static bool add_include_dir(HcsProject* proj, const char* dir) {
    if (proj->include_dir_count >= HCS_MAX_INCLUDE_DIRS) {
        fprintf(stderr, "Warning: Maximum include directories (%d) reached\n", 
                HCS_MAX_INCLUDE_DIRS);
        return false;
    }
    
    proj->include_dirs[proj->include_dir_count] = str_dup(dir);
    if (!proj->include_dirs[proj->include_dir_count]) return false;
    
    proj->include_dir_count++;
    return true;
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
    
    char line[HCS_MAX_LINE_LEN];
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
    
    // If no files specified, add entry point
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

bool project_save(HcsProject *proj, const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f)
        return false;

    fprintf(f, "# HalcyonScript Project\n\n");
    fprintf(f, "[project]\n");
    fprintf(f, "name = \"%s\"\n", proj->name ? proj->name : "Untitled");
    fprintf(f, "version = \"%s\"\n", proj->version ? proj->version : "1.0.0");
    if (proj->author)
        fprintf(f, "author = \"%s\"\n", proj->author);
    if (proj->description)
        fprintf(f, "description = \"%s\"\n", proj->description);
    fprintf(f, "entry = \"%s\"\n", proj->entry_point ? proj->entry_point : "main.hcs");
    if (proj->output)
        fprintf(f, "output = \"%s\"\n", proj->output);
    if (proj->icon)
        fprintf(f, "icon = \"%s\"\n", proj->icon);
    fprintf(f, "target = \"%s\"\n", proj->target ? proj->target : "windows");
    fprintf(f, "debug = %s\n", proj->debug ? "true" : "false");
    fprintf(f, "optimize = %s\n", proj->optimize ? "true" : "false");

    if (proj->file_count > 0)
    {
        fprintf(f, "\n[files]\n");
        for (int i = 0; i < proj->file_count; i++)
        {
            fprintf(f, "%s\n", proj->files[i]);
        }
    }

    if (proj->include_dir_count > 0)
    {
        fprintf(f, "\n[include]\n");
        for (int i = 0; i < proj->include_dir_count; i++)
        {
            fprintf(f, "%s\n", proj->include_dirs[i]);
        }
    }

    fclose(f);
    return true;
}

char *project_get_file_path(HcsProject *proj, const char *relative_path)
{
    if (!proj || !relative_path)
        return NULL;

    /* If absolute path, return as-is */
    if (relative_path[0] == '/' || (relative_path[1] == ':'))
    {
        return str_dup(relative_path);
    }

    return join_path(proj->project_dir, relative_path);
}

bool is_project_file(const char *path)
{
    if (!path)
        return false;
    const char *ext = strrchr(path, '.');
    return ext && strcmp(ext, ".halproj") == 0;
}

static bool file_exists(const char* path) {
    FILE* f = fopen(path, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

char* project_resolve_import(HcsProject* proj, const char* import_path, 
                             const char* current_file) {
    if (!import_path) return NULL;
    
    // Try relative to current file first
    if (current_file) {
        char* current_dir = get_directory(current_file);
        char* full_path = join_path(current_dir, import_path);
        free(current_dir);
        
        if (full_path && file_exists(full_path)) {
            return full_path;
        }
        free(full_path);
    }
    
    // Try relative to project directory
    if (proj && proj->project_dir) {
        char* full_path = join_path(proj->project_dir, import_path);
        if (full_path && file_exists(full_path)) {
            return full_path;
        }
        free(full_path);
        
        // Try include directories
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
    
    // Try as absolute path
    if (file_exists(import_path)) {
        return str_dup(import_path);
    }
    
    return NULL;
}