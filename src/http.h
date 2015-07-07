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
    HTTP_PHASE_METHOD = 0,
    HTTP_PHASE_VERSION_RES = 0,
    HTTP_PHASE_URI,
    HTTP_PHASE_VERSION,
    HTTP_PHASE_STATUS,
    HTTP_PHASE_REASON,
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
    /* parse phase */
    uint8_t phase;
    /* uri or message */
    uint8_t msg;
    uint16_t msglen;
    /* http version 0.9/1.0/1.1 */
    /* method or status */
    uint16_t protocol;
    /* header */
    uint8_t nheader;
    uint8_t maxheader;
} http_t;


/**
 * current cursor
 */
#define http_cursor(h)  ((h)->cur)


/**
 * HTTP version code
 */
enum {
    HTTP_V09 = 0,
    HTTP_V10 = 0x1000,
    HTTP_V11 = 0x2000
};

#define http_version(p) ((p)->protocol & 0xF000)


/**
 * HTTP method code
 * http://www.iana.org/assignments/http-methods/http-methods.xhtml
 */
enum {
    HTTP_MGET = 1,
    HTTP_MHEAD,
    HTTP_MPOST,
    HTTP_MPUT,
    HTTP_MDELETE,
    HTTP_MOPTIONS,
    HTTP_MTRACE,
    HTTP_MCONNECT
};

#define http_method(p)  ((p)->protocol & 0xFFF)


/**
 * HTTP status code
 * http://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml
 */
enum {
    // 100-102
    HTTP_CONTINUE = 100,
    HTTP_SWITCHING_PROTOCOLS,
    HTTP_PROCESSING,
    
    // 200-208
    HTTP_OK = 200,
    HTTP_CREATED,
    HTTP_ACCEPTED,
    HTTP_NON_AUTHORIATIVE_INFORMATION,
    HTTP_NO_CONTENT,
    HTTP_RESET_CONTENT,
    HTTP_PARTIAL_CONTENT,
    HTTP_MULTI_STATUS,
    HTTP_ALREADY_REPORTED,
    // 226
    HTTP_IM_USED = 226,
    
    // 300-305
    HTTP_MULTIPLE_CHOICES = 300,
    HTTP_MOVED_PERMANENTLY,
    HTTP_FOUND,
    HTTP_SEE_OTHER,
    HTTP_NOT_MODIFIED,
    HTTP_USE_PROXY,
    // 307-308
    HTTP_TEMPORARY_REDIRECT = 307,
    HTTP_PERMANENT_REDIRECT,
    
    // 400-417
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED,
    HTTP_PAYMENT_REQUIRED,
    HTTP_FORBIDDEN,
    HTTP_NOT_FOUND,
    HTTP_METHOD_NOT_ALLOWED,
    HTTP_NOT_ACCEPTABLE,
    HTTP_PROXY_AUTHENTICATION_REQUIRED,
    HTTP_REQUEST_TIMEOUT,
    HTTP_CONFLICT,
    HTTP_GONE,
    HTTP_LENGTH_REQUIRED,
    HTTP_PRECONDITION_FAILED,
    HTTP_PAYLOAD_TOO_LARGE,
    HTTP_URI_TOO_LARGE,
    HTTP_UNSUPPORTED_MEDIA_TYPE,
    HTTP_RANGE_NOT_SATISFIABLE,
    HTTP_EXPECTATION_FAILED,
    // 421-424
    HTTP_MISDIRECTED_REQUEST = 421,
    HTTP_UNPROCESSABLE_ENTITY,
    HTTP_LOCKED,
    HTTP_FAILED_DEPENDENCY,
    // 426
    HTTP_UPGRADE_REQUIRED = 426,
    // 428-429
    HTTP_PRECONDITION_REQUIRED = 428,
    HTTP_TOO_MANY_REQUESTS,
    // 431
    HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    
    // 500-508
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED,
    HTTP_BAD_GATEWAY,
    HTTP_SERVICE_UNAVAILABLE,
    HTTP_GATEWAY_TIMEOUT,
    HTTP_VERSION_NOT_SUPPORTED,
    HTTP_VARIANT_ALSO_NEGOTIATES,
    HTTP_INSUFFICIENT_STORAGE,
    HTTP_LOOP_DETECTED,
    // 510-511
    HTTP_NOT_EXTENDED = 510,
    HTTP_NETWORK_AUTHENTICATION_REQUIRED
};

#define http_status(p)  ((p)->protocol & 0xFFF)


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
        .msg = 0, \
        .msglen = 0, \
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
/* invalid status code */
#define HTTP_ESTATUS    -11


/**
 * parsing the http 0.9/1.0/1.1 request
 */
int http_req_parse( http_t *r, char *buf, size_t len, uint16_t maxurilen,
                    uint16_t maxhdrlen );


/**
 * parsing the http 0.9/1.0/1.1 response
 */
int http_res_parse( http_t *r, char *buf, size_t len, uint16_t maxhdrlen );

#endif


