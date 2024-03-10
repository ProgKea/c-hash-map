#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

typedef uint8_t U8;
typedef uint32_t U32;
typedef uint64_t U64;

U64 djb2(U8 *bytes, size_t bytes_count)
{
    U64 result = 5381;

    for (size_t i = 0; i < bytes_count; ++i) {
        result = ((result << 5) + result) + bytes[i];
    }

    return result;
}

#define HashTableInitSize 256

typedef struct Bucket Bucket;
struct Bucket {
    float key;
    int value;
    bool taken;
};

Bucket bucket_make(float key, int value) 
{
    return (Bucket) {
        .key = key,
        .value = value,
        .taken = true,
    };
}

typedef struct HashTable HashTable;
struct HashTable {
    Bucket *buckets;
    size_t count;
    size_t capacity;
};

size_t ht_index(float key, size_t capacity);
void ht_extend(HashTable *ht);
void ht_insert(HashTable *ht, float key, int value);
bool ht_get(HashTable *ht, float key, int *value);

size_t ht_index(float key, size_t capacity)
{
    // TODO: Abuse the fact that our capacity is a power of two
    return djb2((U8*) &key, sizeof(key)) % capacity;
}

void ht_extend(HashTable *ht)
{
    if (ht->capacity == 0) {
        ht->capacity = HashTableInitSize;
        ht->buckets = calloc(ht->capacity, sizeof(*ht->buckets));
    } else {
        HashTable new_hash_table = {
            .buckets = calloc(ht->capacity * 2, sizeof(*ht->buckets)),
            .capacity = ht->capacity * 2,
            .count = ht->count,
        };

        for (size_t i = 0; i < ht->capacity; ++i) {
            Bucket it = ht->buckets[i];
            if (it.taken) {
                ht_insert(&new_hash_table, it.key, it.value);
            }
        }

        free(ht->buckets);
        *ht = new_hash_table;
    }
}

void ht_insert(HashTable *ht, float key, int value)
{
    if (ht->count >= ht->capacity) {
        ht_extend(ht);
    }

    U64 index = ht_index(key, ht->capacity);
    for (size_t i = 0; ht->buckets[index].taken && i < ht->capacity; ++i) {
        index = (index + i) % ht->capacity;
    }
    assert(!ht->buckets[index].taken);

    ht->buckets[index] = bucket_make(key, value);
    ht->count++;
}

bool ht_get(HashTable *ht, float key, int *value)
{
    size_t index = ht_index(key, ht->capacity);
    if (!ht->buckets[index].taken) {
        return false;
    }

    for (size_t i = 0; ht->buckets[index].key != key && i < ht->capacity; ++i) {
        index = (index + i) % ht->capacity;
    }
    if (ht->buckets[index].key != key) {
        return false;
    }

    if (value) {
        *value = ht->buckets[index].value;
    }

    return true;
}

#define ArrayCount(a) (sizeof(a) / sizeof(a[0]))

int main(void)
{
    HashTable ht = {0};

    float keys[] = { 
        3.2, 4.2, 5.2, 6.2, 7.2, 
        8.2, 9.2, 10.2, 11.2, 12.2, 
        13.2, 14.2, 15.2, 16.2, 17.2,
        18.2, 19.2, 20.2, 21.2, 22.2,
        23.2, 24.2, 25.2, 26.2, 27.2,
        28.2, 29.2, 30.2, 31.2, 32.2,
        33.2, 34.2, 
    };
    int values[] = { 
        3, 4, 5, 6, 7,
        8, 9, 10, 11, 12,
        13, 14, 15, 16, 17,
        18, 19, 20, 21, 22,
        23, 24, 25, 26, 27,
        28, 29, 30, 31, 32,
        33, 34, 
    };
    static_assert(ArrayCount(keys) == ArrayCount(values), "Please make sure the two arrays are of equal length.");

    for (size_t i = 0; i < ArrayCount(keys); ++i) {
        ht_insert(&ht, keys[i], values[i]);
    }

    for (size_t i = 0; i < ArrayCount(keys); ++i) {
        int value;
        if (ht_get(&ht, keys[i], &value)) {
            if (value != values[i]) {
                fprintf(stderr, "ERROR: Expected %2d but got %2d\n", values[i], value);
            } else {
                printf("%f => %d\n", keys[i], value);
            }
        } else {
            fprintf(stderr, "ERROR: Could not get value for key: %2f\n", keys[i]);
        }
    }

    float not_present_key = 64.2;
    int value;
    if (ht_get(&ht, not_present_key, &value)) {
        printf("%f => %d\n", not_present_key, value);
    } else {
        fprintf(stderr, "ERROR: Could not get value for key: %f\n", not_present_key);
    }

    return 0;
}
