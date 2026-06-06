#ifndef GAME_BACKUP_H
#define GAME_BACKUP_H

#include "types.h"

int scan_games(void);
int backup_game(int game_index, const char *backup_root, ProgressCallback cb);

#endif
