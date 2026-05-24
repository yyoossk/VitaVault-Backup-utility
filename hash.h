#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint64_t size;
    uint32_t state[4];
    uint8_t buffer[64];
} MD5_CTX;

void md5_init(MD5_CTX *ctx);
void md5_update(MD5_CTX *ctx, const uint8_t *data, size_t len);
void md5_final(uint8_t digest[16], MD5_CTX *ctx);
void md5_to_str(char *out, uint8_t digest[16]);

#endif