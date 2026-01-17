#ifndef HCS_PROJECT_H
#define HCS_PROJECT_H

#include <stdbool.h>

#define HCS_MAX_PROJECT_FILES 256
#define HCS_MAX_PATH_LEN 512
#define HCS_MAX_INCLUDE_DIRS 32
#define HCS_MAX_LINE_LEN 1024

typedef struct {
    char* name;
    char* version;
    char* author;
    char* description;
    char* entry_point;
    char* output;
    char* icon;
    
    char* files[HCS_MAX_PROJECT_FILES];
    int file_count;
    
    char* include_dirs[HCS_MAX_INCLUDE_DIRS];
    int include_dir_count;
    
    char* project_dir;
    
    bool debug;
    bool optimize;
    char* target;
} HcsProject;

HcsProject* project_create(void);
void project_free(HcsProject* proj);

HcsProject* project_load(const char* path);
bool project_save(HcsProject* proj, const char* path);

char* project_get_file_path(HcsProject* proj, const char* relative_path);
bool is_project_file(const char* path);
char* project_resolve_import(HcsProject* proj, const char* import_path, const char* current_file);

#endif