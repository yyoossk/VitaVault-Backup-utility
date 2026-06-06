#ifndef LANGUAGE_H
#define LANGUAGE_H

#define MAX_TRANSLATIONS 200
#define MAX_KEY_LEN 64
#define MAX_VALUE_LEN 256
#define MAX_LANGUAGES 10

typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
} Translation;

typedef struct {
    char code[8];
    char name[32];
} LanguageInfo;

extern Translation g_translations[MAX_TRANSLATIONS];
extern int g_translation_count;
extern LanguageInfo g_languages[MAX_LANGUAGES];
extern int g_num_languages;
extern int g_current_language;

void language_init(void);
void language_scan(void);
void language_load(int lang_idx);
const char* tr(const char *key);
const char* trf(const char *key, const char *fallback);

#endif
