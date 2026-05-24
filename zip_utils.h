#ifndef ZIP_UTILS_H
#define ZIP_UTILS_H

#include <zip.h>
#include "types.h"

int zip_directory(const char *src_path, const char *zip_path, CopyContext *ctx,
                  const char *entry_name, int current_idx, int total_idx,
                  ProgressCallback cb);

#endif