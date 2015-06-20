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
     * protocol 8 bit
     *
     * RESPONSE   REQUEST
     * ---+------+---+-------
     * ver|status|ver|method
     * ---+------+----------
     *  WW|XXXXXX| YY|ZZZZZZ
     * ---+------+---+-------
     *
     * REQUEST
     * ---+------
     * ver|method
     * ---+------
     *  YY|ZZZZZZ
     * ---+------
     *
     * version X 2 bit
     * --+----------
     * 00| HTTP/0.9
     * 01| HTTP/1.0
     * 10| HTTP/1.1
     * --+----------
     *
     * method Y 6 bit
     * ------+---------
     * 000001| GET
     * 000010| HEAD
     * 000011| POST
     * 000100| PUT
     * 000101| DELETE
     * 000110| OPTIONS
     * 000111| TRACE
     * 001000| CONNECT
     * ------+---------
     *
     * RESPONSE 8bit
     * ---+------
     * ver|method
     * ---+------
     *  WW|XXXXXX
     * ---+------
     *
     * version W 2 bit
     * --+----------
     * 00| HTTP/0.9
     * 01| HTTP/1.0
     * 10| HTTP/1.1
     * --+----------
     *
     * Hypertext Transfer Protocol (HTTP) Status Code Registry
     * http://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml
     *
     * status X 6 bit
     * ------+-------------------------------------
     * 000000| 100 Continue
     * 000001| 101 Switching Protocols
     * 000010| 102 Processing
     * ------+-------------------------------------
     * 103-199 unassigned
     *
     * ------+-------------------------------------
     * 000011| 200 OK
     * 000100| 201 Created
     * 000101| 202 Accepted
     * 000110| 203 Non-Authoriative Information
     * 000111| 204 No Content
     * 001000| 205 Reset Content
     * 001001| 206 Partial Content
     * 001010| 207 Multi-Status
     * 001011| 208 Already Reported
     * ------+-------------------------------------
     * 209-225 unassigned
     * ------+-------------------------------------
     * 001100| 226 IM Used
     * ------+-------------------------------------
     * 227-299 unassigned
     *
     * ------+-------------------------------------
     * 001101| 300 Multiple Choices
     * 001110| 301 Moved Permanently
     * 001111| 302 Found
     * 010000| 303 See Other
     * 010001| 304 Not Modified
     * 010010| 305 Use Proxy
     * ------+-------------------------------------
     * 306 unused
     * ------+-------------------------------------
     * 010011| 307 Temporary Redirect
     * 010100| 308 Permanent Redirect
     * ------+-------------------------------------
     * 309-399 unassigned
     *
     * ------+-------------------------------------
     * 010101| 400 Bad Request
     * 010110| 401 Unauthorized
     * 010111| 402 Payment Required
     * 011000| 403 Forbidden
     * 011001| 404 Not Found
     * 011010| 405 Method Not Allowed
     * 011011| 406 Not Acceptable
     * 011100| 407 Proxy Authentication Required
     * 011101| 408 Request Timeout
     * 011110| 409 Conflict
     * 011111| 410 Gone
     * 100000| 411 Length Required
     * 100001| 412 Precondition Failed
     * 100010| 413 Payload Too Large
     * 100011| 414 URI Too Large
     * 100100| 415 Unsupported Media Type
     * 100101| 416 Range Not Satisfiable
     * 100110| 417 Expectation Failed
     * ------+-------------------------------------
     * 418-420 unassigned
     * ------+-------------------------------------
     * 100111| 421 Misdirected Request
     * 101000| 422 Unprocessable Entity
     * 101001| 423 Locked
     * 101010| 424 Failed Dependency
     * ------+-------------------------------------
     * 425 unassigned
     * ------+-------------------------------------
     * 101011| 426 Upgrade Required
     * ------+-------------------------------------
     * 427 unassigned
     * ------+-------------------------------------
     * 101100| 428 Precondition Required
     * 101101| 429 Too Many Requests
     * ------+-------------------------------------
     * 430 unassigned
     * ------+-------------------------------------
     * 101110| 431 Request Header Fields Too Large
     * ------+-------------------------------------
     * 432-499 unassigned
     *
     * ------+-------------------------------------
     * 101111| 500 Internal Server Error
     * 110000| 501 Not Implemented
     * 110001| 502 Bad Gateway
     * 110010| 503 Service Unavailable
     * 110011| 504 Gateway Timeout
     * 110100| 505 HTTP Version Not Supported
     * 110101| 506 Variant Also Negotiates
     * 110110| 507 Insufficient Storage
     * 110111| 508 Loop Detected
     * ------+-------------------------------------
     * 509 unassigned
     * ------+-------------------------------------
     * 111000| 510 Not Extended
     * 111001| 511 Network Authentication Required
     * ------+-------------------------------------
     * 512-599 unassigned
     *
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
#define HTTP_V10    0x40
#define HTTP_V11    0x80

#define http_req_ver(p) ((p)->protocol & 0xC0)


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

#define http_req_method(p)  ((p)->protocol & 0x3F)

/**
 * HTTP status code
 */
enum {
    HTTP_CONTINUE
};


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


