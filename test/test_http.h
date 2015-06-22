#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "../src/http.h"

typedef struct {
    int rc;
    uint8_t code;
    uint8_t version;
    uint8_t nheader;
    char entity[2049];
} test_req_t;
