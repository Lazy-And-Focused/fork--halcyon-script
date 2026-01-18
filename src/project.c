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

// =====================================================================
// СЕКЦИЯ: Внутренние утилиты (будут перенесены в utils.c позже)
// =====================================================================

static const char* TRUE_VALUE_STRINGS[] = TRUE_VALUES;
static const int NUM_TRUE_VALUES = TRUE_VALUES_COUNT;

static bool hcs_string_to_bool(const char* str) {
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

static char* hcs_strdup(const char* s) {
    if (!s) return NULL;
    
    char* d = malloc(strlen(s) + 1);
    if (!d) return NULL;
    
    strcpy(d, s);
    return d;
}

static char* hcs_trim(char* str) {
    if (!str) return NULL;
    
    // Пропускаем начальные пробелы
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    
    // Убираем конечные пробелы
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    return str;
}

static void hcs_remove_quotes(char* str) {
    if (!str || str[0] != '"') return;
    
    size_t len = strlen(str);
    if (len > 1 && str[len - 1] == '"') {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

static char* hcs_join_path(const char* dir, const char* file) {
    if (!dir || !file) return NULL;
    
    size_t dir_len = strlen(dir);
    size_t file_len = strlen(file);
    size_t total_len = dir_len + file_len + 2;
    
    char* result = malloc(total_len);
    if (!result) return NULL;
    
    snprintf(result, total_len, "%s%c%s", dir, PATH_SEPARATOR, file);
    return result;
}

static bool hcs_file_exists(const char* path) {
    if (!path) return false;
    
    FILE* f = fopen(path, "r");
    if (!f) return false;
    
    fclose(f);
    return true;
}

static char* hcs_get_directory(const char* path) {
    if (!path) return hcs_strdup(".");
    
    char* dir_copy = hcs_strdup(path);
    if (!dir_copy) return hcs_strdup(".");
    
    char* last_sep = strrchr(dir_copy, PATH_SEPARATOR);
    if (!last_sep) {
        char alternate_sep = (PATH_SEPARATOR == '/') ? '\\' : '/';
        last_sep = strrchr(dir_copy, alternate_sep);
    }
    
    if (last_sep) {
        *last_sep = '\0';
        if (dir_copy[0] == '\0') {
            free(dir_copy);
            return hcs_strdup(".");
        }
        return dir_copy;
    }
    
    free(dir_copy);
    return hcs_strdup(".");
}

// =====================================================================
// СЕКЦИЯ: Внутренние типы и константы
// =====================================================================

typedef enum {
    SECTION_MAIN,
    SECTION_FILES,
    SECTION_INCLUDE
} ParserSection;

// =====================================================================
// СЕКЦИЯ: Вспомогательные функции для парсинга
// =====================================================================

static bool is_comment_or_empty(const char* line) {
    if (!line || line[0] == '\0') return true;
    return is_comment_char(line[0]);
}

static ParserSection parse_section_header(const char* line) {
    if (!line) return SECTION_MAIN;
    
    if (strcmp(line, SECTION_FILES_HEADER) == 0) return SECTION_FILES;
    if (strcmp(line, SECTION_INCLUDE_HEADER) == 0) return SECTION_INCLUDE;
    if (strcmp(line, SECTION_PROJECT) == 0) return SECTION_MAIN;
    
    return SECTION_MAIN;
}

static bool hcs_update_string_field(char** field, const char* value, bool required) {
    if (!field) return false;
    
    free(*field);
    *field = hcs_strdup(value);
    
    if (required && *field == NULL) {
        return false;
    }
    
    return true;
}

static bool parse_key_value(HcsProject* proj, const char* key, const char* value) {
    if (!proj || !key || !value) return false;
    
    typedef struct {
        const char* key;
        char** field;
        bool required;
    } FieldMapping;
    
    FieldMapping mappings[] = {
        {KEY_NAME, &proj->name, true},
        {KEY_VERSION, &proj->version, true},
        {KEY_AUTHOR, &proj->author, false},
        {KEY_DESCRIPTION, &proj->description, false},
        {KEY_ENTRY, &proj->entry_point, true},
        {KEY_OUTPUT, &proj->output, false},
        {KEY_ICON, &proj->icon, false},
        {KEY_TARGET, &proj->target, true},
    };
    
    for (size_t i = 0; i < sizeof(mappings) / sizeof(mappings[0]); i++) {
        if (strcmp(key, mappings[i].key) == 0) {
            return hcs_update_string_field(mappings[i].field, value, mappings[i].required);
        }
    }
    
    if (strcmp(key, KEY_DEBUG) == 0) {
        proj->debug = hcs_string_to_bool(value);
        return true;
    }
    if (strcmp(key, KEY_OPTIMIZE) == 0) {
        proj->optimize = hcs_string_to_bool(value);
        return true;
    }
    
    // Игнорируем неизвестные ключи
    return true;
}

static bool parse_main_section_line(HcsProject* proj, char* line) {
    if (!proj || !line) return false;
    
    char* eq = strchr(line, '=');
    if (!eq) return true;
    
    *eq = '\0';
    
    char* key = hcs_trim(line);
    char* value = hcs_trim(eq + 1);
    
    if (!key || !value) return false;
    
    hcs_remove_quotes(value);
    return parse_key_value(proj, key, value);
}

// =====================================================================
// СЕКЦИЯ: Функции для работы с коллекциями (файлы, директории)
// =====================================================================

static bool add_project_file(HcsProject* proj, const char* file) {
    if (!proj || !file) return false;
    
    if (proj->file_count >= MAX_PROJECT_FILES) {
        fprintf(stderr, "Warning: Maximum project files (%d) reached\n", 
                MAX_PROJECT_FILES);
        return false;
    }
    
    proj->files[proj->file_count] = hcs_strdup(file);
    if (!proj->files[proj->file_count]) return false;
    
    proj->file_count++;
    return true;
}

static bool add_include_dir(HcsProject* proj, const char* dir) {
    if (!proj || !dir) return false;
    
    if (proj->include_dir_count >= MAX_INCLUDE_DIRS) {
        fprintf(stderr, "Warning: Maximum include directories (%d) reached\n", 
                MAX_INCLUDE_DIRS);
        return false;
    }
    
    proj->include_dirs[proj->include_dir_count] = hcs_strdup(dir);
    if (!proj->include_dirs[proj->include_dir_count]) return false;
    
    proj->include_dir_count++;
    return true;
}

// =====================================================================
// СЕКЦИЯ: Публичный API
// =====================================================================

HcsProject* project_create(void) {
    HcsProject* proj = calloc(1, sizeof(HcsProject));
    if (!proj) return NULL;
    
    proj->name = hcs_strdup(DEFAULT_PROJECT_NAME);
    proj->version = hcs_strdup(DEFAULT_VERSION);
    proj->entry_point = hcs_strdup(DEFAULT_ENTRY_POINT);
    proj->target = hcs_strdup(DEFAULT_TARGET);
    
    if (!proj->name || !proj->version || !proj->entry_point || !proj->target) {
        project_free(proj);
        return NULL;
    }
    
    proj->debug = true;
    proj->optimize = false;
    
    return proj;
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
    if (!path) {
        fprintf(stderr, "Error: Null path provided\n");
        return NULL;
    }
    
    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open project file: %s\n", path);
        return NULL;
    }
    
    HcsProject* proj = project_create();
    if (!proj) {
        fclose(f);
        fprintf(stderr, "Error: Failed to create project structure\n");
        return NULL;
    }
    
    free(proj->project_dir);
    proj->project_dir = hcs_get_directory(path);
    
    char line[MAX_LINE_LENGTH];
    ParserSection current_section = SECTION_MAIN;
    bool has_error = false;
    
    while (fgets(line, sizeof(line), f) && !has_error) {
        line[strcspn(line, "\n")] = '\0';
        
        char* trimmed = hcs_trim(line);
        
        if (is_comment_or_empty(trimmed)) {
            continue;
        }
        
        // Обработка заголовков секций
        if (trimmed[0] == '[') {
            current_section = parse_section_header(trimmed);
            continue;
        }
        
        switch (current_section) {
            case SECTION_MAIN:
                has_error = !parse_main_section_line(proj, trimmed);
                break;
                
            case SECTION_FILES:
                has_error = !add_project_file(proj, trimmed);
                break;
                
            case SECTION_INCLUDE:
                has_error = !add_include_dir(proj, trimmed);
                break;
        }
    }
    
    fclose(f);
    
    if (has_error) {
        fprintf(stderr, "Error: Failed to parse project file\n");
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
    if (!proj || !path) return false;
    
    FILE* f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "Error: Cannot create project file: %s\n", path);
        return false;
    }
    
    fprintf(f, "# HalcyonScript Project\n");
    fprintf(f, "# Generated by HalcyonScript\n\n");
    
    fprintf(f, SECTION_PROJECT "\n");
    fprintf(f, KEY_NAME " = \"%s\"\n", 
            proj->name ? proj->name : DEFAULT_PROJECT_NAME);
    fprintf(f, KEY_VERSION " = \"%s\"\n", 
            proj->version ? proj->version : DEFAULT_VERSION);
    
    // Опциональные поля
    if (proj->author && proj->author[0] != '\0') {
        fprintf(f, KEY_AUTHOR " = \"%s\"\n", proj->author);
    }
    if (proj->description && proj->description[0] != '\0') {
        fprintf(f, KEY_DESCRIPTION " = \"%s\"\n", proj->description);
    }
    
    // Обязательные поля с fallback значениями
    fprintf(f, KEY_ENTRY " = \"%s\"\n", 
            proj->entry_point ? proj->entry_point : DEFAULT_ENTRY_POINT);
    
    // Остальные опциональные поля
    if (proj->output && proj->output[0] != '\0') {
        fprintf(f, KEY_OUTPUT " = \"%s\"\n", proj->output);
    }
    if (proj->icon && proj->icon[0] != '\0') {
        fprintf(f, KEY_ICON " = \"%s\"\n", proj->icon);
    }
    
    fprintf(f, KEY_TARGET " = \"%s\"\n", 
            proj->target ? proj->target : DEFAULT_TARGET);
    
    // Булевые поля
    fprintf(f, KEY_DEBUG " = %s\n", 
            proj->debug ? DEFAULT_TRUE_VALUE : DEFAULT_FALSE_VALUE);
    fprintf(f, KEY_OPTIMIZE " = %s\n", 
            proj->optimize ? DEFAULT_TRUE_VALUE : DEFAULT_FALSE_VALUE);
    
    if (proj->file_count > 0) {
        fprintf(f, "\n" SECTION_FILES_HEADER "\n");
        for (int i = 0; i < proj->file_count; i++) {
            if (proj->files[i]) {
                fprintf(f, "%s\n", proj->files[i]);
            }
        }
    }
    
    if (proj->include_dir_count > 0) {
        fprintf(f, "\n" SECTION_INCLUDE_HEADER "\n");
        for (int i = 0; i < proj->include_dir_count; i++) {
            if (proj->include_dirs[i]) {
                fprintf(f, "%s\n", proj->include_dirs[i]);
            }
        }
    }
    
    fclose(f);
    return true;
}

char* project_get_file_path(HcsProject* proj, const char* relative_path) {
    if (!proj || !relative_path) return NULL;
    
    if (IS_ABSOLUTE_PATH(relative_path)) {
        return hcs_strdup(relative_path);
    }
    
    return hcs_join_path(proj->project_dir, relative_path);
}

char* project_resolve_import(HcsProject* proj, const char* import_path, 
                             const char* current_file) {
    if (!import_path) return NULL;
    
    // 1. Пробуем относительно текущего файла
    if (current_file) {
        char* current_dir = hcs_get_directory(current_file);
        char* full_path = hcs_join_path(current_dir, import_path);
        free(current_dir);
        
        if (full_path && hcs_file_exists(full_path)) {
            return full_path;
        }
        free(full_path);
    }
    
    // 2. Пробуем относительно директории проекта
    if (proj && proj->project_dir) {
        char* full_path = hcs_join_path(proj->project_dir, import_path);
        if (full_path && hcs_file_exists(full_path)) {
            return full_path;
        }
        free(full_path);
        
        // 3. Пробуем в include директориях
        for (int i = 0; i < proj->include_dir_count; i++) {
            if (!proj->include_dirs[i]) continue;
            
            char* inc_dir = hcs_join_path(proj->project_dir, proj->include_dirs[i]);
            full_path = hcs_join_path(inc_dir, import_path);
            free(inc_dir);
            
            if (full_path && hcs_file_exists(full_path)) {
                return full_path;
            }
            free(full_path);
        }
    }
    
    // 4. Пробуем как абсолютный путь
    if (hcs_file_exists(import_path)) {
        return hcs_strdup(import_path);
    }
    
    return NULL;
}