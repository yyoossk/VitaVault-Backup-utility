#include "language.h"
#include "types.h"
#include <stdio.h>
#include <string.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>

Translation g_translations[MAX_TRANSLATIONS];
int g_translation_count = 0;
LanguageInfo g_languages[MAX_LANGUAGES];
int g_num_languages = 0;
int g_current_language = 0;

void language_init(void) {
    language_scan();
    language_load(g_current_language);
}

void language_scan(void) {
    g_num_languages = 0;
    const char *base_path = "app0:lang/";
    SceUID dir = sceIoDopen(base_path);
    if (dir < 0) {
        base_path = "ux0:app/VITAVAULT/lang/";
        dir = sceIoDopen(base_path);
    }
    if (dir < 0) return;

    SceIoDirent ent;
    while (sceIoDread(dir, &ent) > 0 && g_num_languages < MAX_LANGUAGES) {
        if (strstr(ent.d_name, ".txt")) {
            char path[PATH_MAX_SIZE];
            snprintf(path, sizeof(path), "%s%s", base_path, ent.d_name);

            char code[8];
            strncpy(code, ent.d_name, sizeof(code)-1);
            char *ext = strstr(code, ".txt");
            if (ext) *ext = '\0';
            strncpy(g_languages[g_num_languages].code, code, sizeof(g_languages[g_num_languages].code)-1);
            strncpy(g_languages[g_num_languages].name, code, sizeof(g_languages[g_num_languages].name)-1);
            SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
            if (fd >= 0) {
                char buf[1024];
                int r = sceIoRead(fd, buf, sizeof(buf)-1);
                if (r > 0) {
                    buf[r] = '\0';
                    char *key = strstr(buf, "language_display_name=");
                    if (key) {
                        char *val = key + strlen("language_display_name=");
                        char *end = strpbrk(val, "\r\n");
                        if (end) *end = '\0';
                        strncpy(g_languages[g_num_languages].name, val, sizeof(g_languages[g_num_languages].name)-1);
                    }
                }
                sceIoClose(fd);
            }
            g_num_languages++;
        }
    }
    sceIoDclose(dir);
}

void language_load(int lang_idx) {
    if (lang_idx < 0 || lang_idx >= g_num_languages) lang_idx = 0;

    char lang_file[128];
    snprintf(lang_file, sizeof(lang_file), "app0:lang/%s.txt", g_languages[lang_idx].code);
    
    g_translation_count = 0;
    
    SceUID fd = sceIoOpen(lang_file, SCE_O_RDONLY, 0);
    if (fd < 0) {
        // Try loading from ux0 if app0 fails
        snprintf(lang_file, sizeof(lang_file), "ux0:app/VITAVAULT/lang/%s.txt", g_languages[lang_idx].code);
        fd = sceIoOpen(lang_file, SCE_O_RDONLY, 0);
    }
    if (fd < 0) return;

    char buffer[16384];
    int total_read = 0;
    while (total_read < sizeof(buffer) - 1) {
        int read = sceIoRead(fd, buffer + total_read, sizeof(buffer) - 1 - total_read);
        if (read <= 0) break;
        total_read += read;
    }
    buffer[total_read] = '\0';
    sceIoClose(fd);
    char *line = strtok(buffer, "\n");
    while (line != NULL && g_translation_count < MAX_TRANSLATIONS) {
        char *cr = strchr(line, '\r');
        if (cr) *cr = '\0';
        if (line[0] == '#' || line[0] == '\0' || strspn(line, " \t") == strlen(line)) {
            line = strtok(NULL, "\n");
            continue;
        }
        char *equals = strchr(line, '=');
        if (!equals) {
            line = strtok(NULL, "\n");
            continue;
        }
        *equals = '\0';
        char *key = line;
        char *value = equals + 1;
        while (*key == ' ' || *key == '\t') key++;
        char *key_end = key + strlen(key) - 1;
        while (key_end > key && (*key_end == ' ' || *key_end == '\t')) {
            *key_end = '\0';
            key_end--;
        }
        while (*value == ' ' || *value == '\t') value++;
        char *value_end = value + strlen(value) - 1;
        while (value_end > value && (*value_end == ' ' || *value_end == '\t')) {
            *value_end = '\0';
            value_end--;
        }
        if (strlen(key) > 0 && strlen(value) > 0) {
            strncpy(g_translations[g_translation_count].key, key, MAX_KEY_LEN - 1);
            g_translations[g_translation_count].key[MAX_KEY_LEN - 1] = '\0';
            strncpy(g_translations[g_translation_count].value, value, MAX_VALUE_LEN - 1);
            g_translations[g_translation_count].value[MAX_VALUE_LEN - 1] = '\0';
            char *v = g_translations[g_translation_count].value;
            for (int j = 0; v[j]; j++) {
                if (v[j] == '\\' && v[j+1] == 'n') {
                    v[j] = '\n';
                    memmove(&v[j+1], &v[j+2], strlen(&v[j+2]) + 1);
                }
            }
            g_translation_count++;
        }
        
        line = strtok(NULL, "\n");
    }
}

const char* tr(const char *key) {
    for (int i = 0; i < g_translation_count; i++) {
        if (strcmp(g_translations[i].key, key) == 0) {
            return g_translations[i].value;
        }
    }
    return key;
}

const char* trf(const char *key, const char *fallback) {
    const char *value = tr(key);
    if (strcmp(value, key) == 0) {
        return fallback;
    }
    return value;
}
