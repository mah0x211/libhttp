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
     * 000001| 100 Continue
     * 000010| 101 Switching Protocols
     * 000011| 102 Processing
     * ------+-------------------------------------
     * 103-199 unassigned
     *
     * ------+-------------------------------------
     * 000100| 200 OK
     * 000101| 201 Created
     * 000110| 202 Accepted
     * 000111| 203 Non-Authoriative Information
     * 001000| 204 No Content
     * 001001| 205 Reset Content
     * 001010| 206 Partial Content
     * 001011| 207 Multi-Status
     * 001100| 208 Already Reported
     * ------+-------------------------------------
     * 209-225 unassigned
     * ------+-------------------------------------
     * 001101| 226 IM Used
     * ------+-------------------------------------
     * 227-299 unassigned
     *
     * ------+-------------------------------------
     * 001110| 300 Multiple Choices
     * 001111| 301 Moved Permanently
     * 010001| 302 Found
     * 010010| 303 See Other
     * 010011| 304 Not Modified
     * 010100| 305 Use Proxy
     * ------+-------------------------------------
     * 306 unused
     * ------+-------------------------------------
     * 010101| 307 Temporary Redirect
     * 010110| 308 Permanent Redirect
     * ------+-------------------------------------
     * 309-399 unassigned
     *
     * ------+-------------------------------------
     * 010111| 400 Bad Request
     * 011001| 401 Unauthorized
     * 011010| 402 Payment Required
     * 011011| 403 Forbidden
     * 011100| 404 Not Found
     * 011101| 405 Method Not Allowed
     * 011110| 406 Not Acceptable
     * 011111| 407 Proxy Authentication Required
     * 100000| 408 Request Timeout
     * 100001| 409 Conflict
     * 100010| 410 Gone
     * 100011| 411 Length Required
     * 100100| 412 Precondition Failed
     * 100101| 413 Payload Too Large
     * 100110| 414 URI Too Large
     * 100111| 415 Unsupported Media Type
     * 101000| 416 Range Not Satisfiable
     * 101001| 417 Expectation Failed
     * ------+-------------------------------------
     * 418-420 unassigned
     * ------+-------------------------------------
     * 101010| 421 Misdirected Request
     * 101011| 422 Unprocessable Entity
     * 101100| 423 Locked
     * 101101| 424 Failed Dependency
     * ------+-------------------------------------
     * 425 unassigned
     * ------+-------------------------------------
     * 101110| 426 Upgrade Required
     * ------+-------------------------------------
     * 427 unassigned
     * ------+-------------------------------------
     * 101111| 428 Precondition Required
     * 110000| 429 Too Many Requests
     * ------+-------------------------------------
     * 430 unassigned
     * ------+-------------------------------------
     * 110001| 431 Request Header Fields Too Large
     * ------+-------------------------------------
     * 432-499 unassigned
     *
     * ------+-------------------------------------
     * 110010| 500 Internal Server Error
     * 110011| 501 Not Implemented
     * 110100| 502 Bad Gateway
     * 110101| 503 Service Unavailable
     * 110110| 504 Gateway Timeout
     * 110111| 505 HTTP Version Not Supported
     * 111000| 506 Variant Also Negotiates
     * 111001| 507 Insufficient Storage
     * 111010| 508 Loop Detected
     * ------+-------------------------------------
     * 509 unassigned
     * ------+-------------------------------------
     * 111011| 510 Not Extended
     * 111100| 511 Network Authentication Required
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


