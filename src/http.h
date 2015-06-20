/*
 *  Copyright 2015 Masatoshi Teruya All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to 
 *  deal in the Software without restriction, including without limitation the 
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 *
 *  http.h
 *  Created by Masatoshi Teruya on 12/11/23.
 */

#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <stdint.h>


enum {
    HTTP_PHASE_METHOD,
    HTTP_PHASE_URI,
    HTTP_PHASE_VERSION,
    HTTP_PHASE_EOL,
    HTTP_PHASE_HEADER,
    HTTP_PHASE_HKEY,
    HTTP_PHASE_HVAL,
    HTTP_PHASE_DONE
};

typedef struct {
    /* read cursor */
    uintptr_t cur;
    /* token head position */
    uintptr_t head;

    /**
     * protocol 16 bit
     * ver|other
     * ---+-------------
     * XXX|YYYYYYYYYYYYY
     * ---+-------------
     *
     * version 0 bit
     * ---+----------
     * 000| HTTP/0.9
     * 001| HTTP/1.0
     * 010| HTTP/1.1
     * ---+----------
     *
     * method: 4 bit
     * -----+---------
     *  0001| GET
     *  0010| HEAD
     *  0011| POST
     *  0100| PUT
     *  0101| DELETE
     *  0110| OPTIONS
     *  0111| TRACE
     *  1000| CONNECT
     * -----+---------
     */
    uint16_t protocol;
    
    // phase
    uint8_t phase;
    
    // uri
    uint8_t uri;
    uint16_t urilen;
    
    // header
    uint8_t nheader;
    uint8_t maxheader;
} http_t;


/**
 * HTTP version code
 */
#define HTTP_V09    0x0
#define HTTP_V10    0x2000
#define HTTP_V11    0x4000

#define http_req_ver(p) ((p)->protocol & 0xE000)


/**
 * HTTP method code
 */
#define HTTP_MGET       0x1
#define HTTP_MHEAD      0x2
#define HTTP_MPOST      0x3
#define HTTP_MPUT       0x4
#define HTTP_MDELETE    0x5
#define HTTP_MOPTIONS   0x6
#define HTTP_MTRACE     0x7
#define HTTP_MCONNECT   0x8

#define http_req_method(p)  ((p)->protocol & 0x000F)

/**
 * per HTTP header
 *
 * (uintptr_t + uint16_t) * 2
 * uintptr_t key
 * uint16_t klen
 * uintptr_t val
 * uint16_t vlen
 */
#define HTTP_HEADER_SIZE    ((sizeof(uintptr_t)+sizeof(uint16_t))<<1)

/**
 * get the header key-value pair at specified index
 */
int http_getheader_at( http_t *r, uintptr_t *key, uint16_t *klen,
                       uintptr_t *val, uint16_t *vlen, uint8_t at );



/**
 * allocate http_t*
 */
#define http_alloc_size( maxheader ) \
    (sizeof(http_t)+(HTTP_HEADER_SIZE*maxheader))

http_t *http_alloc( uint8_t maxheader );


/**
 * initialize data members
 */
#define http_init(r) do{ \
    *(r) = (http_t){ \
        .cur = 0, \
        .head = 0, \
        .phase = 0, \
        .protocol = 0, \
        .urilen = 0, \
        .uri = 0, \
        .nheader = 0, \
        .maxheader = (r)->maxheader \
    }; \
}while(0)


/**
 * deallocate http_t*
 */
void http_free( http_t *r );


/**
 * return code
 */
/* success */
#define HTTP_SUCCESS    0
/* probably, data structure is corrupted */
#define HTTP_ERROR      -1
/* need more bytes */
#define HTTP_EAGAIN     -2
/* method not implemented */
#define HTTP_EMETHOD    -3
/* invalid uri string */
#define HTTP_EBADURI    -4
/* uri-length too large */
#define HTTP_EURILEN    -5
/* version not support */
#define HTTP_EVERSION   -6
/* invalid line format */
#define HTTP_ELINEFMT   -7
/* invalid header format */
#define HTTP_EHDRFMT    -8
/* too many headers */
#define HTTP_ENHDR      -9
/* header-length too large */
#define HTTP_EHDRLEN    -10

/**
 * parse http 0.9/1.0/1.1 request
 */
int http_req_parse( http_t *r, char *buf, size_t len, uint16_t maxurilen,
                    uint16_t maxhdrlen );


#endif


