/* croOS math.c - Math library
 * Integer and floating-point math functions for userspace. */

#include "kernel/types.h"

/* Integer math */
int abs(int x) { return x < 0 ? -x : x; }

int min(int a, int b) { return a < b ? a : b; }
int max(int a, int b) { return a > b ? a : b; }

int clamp(int x, int lo, int hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

/* Integer power */
int ipow(int base, int exp) {
    int result = 1;
    while (exp > 0) {
        if (exp & 1) result *= base;
        base *= base;
        exp >>= 1;
    }
    return result;
}

/* Integer square root (Newton's method) */
int isqrt(int n) {
    if (n <= 0) return 0;
    int x = n;
    int y = (x + 1) / 2;
    while (y < x) { x = y; y = (x + n / x) / 2; }
    return x;
}

/* Greatest common divisor */
int gcd(int a, int b) {
    a = abs(a); b = abs(b);
    while (b) { int t = b; b = a % b; a = t; }
    return a;
}

/* Least common multiple */
int lcm(int a, int b) {
    return abs(a) / gcd(a, b) * abs(b);
}

/* Simple hash (FNV-1a) */
uint32_t fnv1a(const void *data, uint32_t len) {
    const uint8_t *p = (const uint8_t*)data;
    uint32_t hash = 2166136261;
    for (uint32_t i = 0; i < len; i++) {
        hash ^= p[i];
        hash *= 16777619;
    }
    return hash;
}

/* CRC32 */
static uint32_t crc32_table[256];
static int crc32_inited = 0;

static void crc32_init(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++)
            c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        crc32_table[i] = c;
    }
    crc32_inited = 1;
}

uint32_t crc32(const void *data, uint32_t len) {
    if (!crc32_inited) crc32_init();
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *p = (const uint8_t*)data;
    for (uint32_t i = 0; i < len; i++)
        crc = crc32_table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}

/* Simple pseudo-random number generator (xorshift32) */
static uint32_t rng_state = 12345;

void srand(uint32_t seed) { rng_state = seed; }

uint32_t rand(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

int rand_range(int min_val, int max_val) {
    return min_val + (int)(rand() % (uint32_t)(max_val - min_val + 1));
}

/* Matrix operations (4x4) */
void mat4_identity(float *m) {
    for (int i = 0; i < 16; i++) m[i] = 0;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void mat4_multiply(const float *a, const float *b, float *result) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++)
                result[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
        }
    }
}
