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
 *  http.c
 *  Created by Masatoshi Teruya on 12/11/23.
 */

/*
    HTTP 0.9:
        1:GET[SP]URL[CRLF]
        [CRLF](end of headers)
        ...(request body)
        
    HTTP 1.0/1.1:
        1:METHOD[SP]URL[SP]HTTP/1.1[CRLF]
        2:header:[LWS]*data[CRLF]
        3:header:[LWS]*data[CRLF]
        n:...[CRLF]
        [CRLF](end of headers)
        ...(request body)
        
        LWS = SP|HT
*/
#include "http.h"
#include "strchr_brk.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>


/**
 *  structure for 64 bit comparison
 */
typedef union {
    char str[8];
    uint64_t bit;
} match64bit_u;

// methods
static match64bit_u M_GET = {
    .str = "GET"
};
static match64bit_u M_HEAD = {
    .str = "HEAD"
};
static match64bit_u M_POST = {
    .str = "POST"
};
static match64bit_u M_PUT = {
    .str = "PUT"
};
static match64bit_u M_DELETE = {
    .str = "DELETE"
};
static match64bit_u M_OPTIONS = {
    .str = "OPTIONS"
};
static match64bit_u M_TRACE = {
    .str = "TRACE"
};
static match64bit_u M_CONNECT = {
    .str = "CONNECT"
};

// versions
static match64bit_u V_09 = {
    .str = "HTTP/0.9"
};
static match64bit_u V_10 = {
    .str = "HTTP/1.0"
};
static match64bit_u V_11 = {
    .str = "HTTP/1.1"
};


/**
 * RFC 3986
 *
 * alpha         = lowalpha | upalpha
 * lowalpha      = "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" |
 *                 "j" | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r" |
 *                 "s" | "t" | "u" | "v" | "w" | "x" | "y" | "z"
 * upalpha       = "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" |
 *                 "J" | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R" |
 *                 "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z"
 * --------------------------------------------------------------------------
 * digit         = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
 * --------------------------------------------------------------------------
 * pct-encoded   = "%" hex hex
 * hex           = digit | "A" | "B" | "C" | "D" | "E" | "F" |
 *                         "a" | "b" | "c" | "d" | "e" | "f"
 * --------------------------------------------------------------------------
 * gen-delims    = ":" | "/" | "?" | "#" | "[" | "]" | "@"
 * --------------------------------------------------------------------------
 * sub-delims    = "!" | "$" | "&" | "'" | "(" | ")" | "*" | "+" | "," | ";"
 *                 "="
 * --------------------------------------------------------------------------
 * reserved      = gen-delims | sub-delims
 * --------------------------------------------------------------------------
 * unreserved    = alpha | digit | "-" | "." | "_" | "~"
 * --------------------------------------------------------------------------
 * pchar         = unreserved | pct-encoded | sub-delim | ":" | "@"
 * --------------------------------------------------------------------------
 * URI           = scheme "://" 
 *                 [ userinfo[ ":" userinfo ] "@" ]
 *                 host
 *                 [ ":" port ]
 *                 path 
 *                 [ "?" query ] 
 *                 [ "#" fragment ]
 * --------------------------------------------------------------------------
 * scheme        = alpha *( alpha / digit / "+" / "-" / "." )
 * --------------------------------------------------------------------------
 * userinfo      = *( unreserved / pct-encoded / sub-delims )
 * --------------------------------------------------------------------------
 * host          = IP-literal / IPv4address / reg-name
 * --------------------------------------------------------------------------
 * port          = *digit
 * --------------------------------------------------------------------------
 * path          = empty / *( "/" pchar )
 * empty         = zero characters
 * --------------------------------------------------------------------------
 * query         = *( pchar / "/" / "?" )
 * --------------------------------------------------------------------------
 * fragment      = query
 * --------------------------------------------------------------------------
 */
static const unsigned char URIC_TBL[256] = {
//  ctrl-code: 0-32
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 
//  SP      "  #
    0, '!', 0, 0, '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', 
//  digit
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 

//            <       > 
    ':', ';', 0, '=', 0, '?', '@', 

//  alpha-upper
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 

//       \       ^       `
    '[', 0, ']', 0, '_', 0, 

//  alpha-lower
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 

//  {  |  }
    0, 0, 0, '~'
};


/* delimiters */
#define CR          '\r'
#define LF          '\n'
#define SP          ' '
#define HT          '\t'
#define COLON       ':'
#define EQ          '='


/**
 * RFC 2616
 * http://tools.ietf.org/html/rfc2616#section-2.2
 *
 * token          = 1*<any CHAR except CTLs or separators>
 * separators     = "(" | ")" | "<" | ">" | "@" | "," | ";" | ":" | "\" | <">
 *                | "/" | "[" | "]" | "?" | "=" | "{" | "}"
 *                | SP  | HT
 */

/**
 * header name token = 1*<alpha | digit | "-" | "_">
 */
static const unsigned char HKEYC_TBL[256] = {
//  0  1  2  3  4  5  6  7  8 HT LF 11 12 CR 14 15 16 17 18 19 20 21 22 23 24
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//  25 26 27 28 29 30 31 
    0, 0, 0, 0, 0, 0, 0, 
//  SP !  "  #  $  %  &  '  (  )  *  +  ,       .  /
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 
//  digit
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
//  :  ;  <  =  >  ?  @
    0, 0, 0, 0, 0, 0, 0, 
// conversion from alpha-upper to alpha-lower
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 
//  [  \  ]  ^       `
    0, 0, 0, 0, '_', 0, 
//  alpha-lower
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 
//  {  |  }  ~
    0, 0, 0, 0
};


static const unsigned char SPHT[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, HT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, SP
};

/**
 * method name characters; GET HEAD POST PUT DELETE OPTIONS TRACE CONNECT
 */
static const unsigned char METHODC_TBL[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'A', 0, 'C', 'D', 'E', 0, 'G', 
    'H', 'I', 0, 0, 'L', 0, 'N', 'O', 'P', 0, 'R', 'S', 'T', 'U'
};

/**
 * version characters; HTTP/X.X
 * X = 0 | 1 | 9
 */
static const unsigned char VERC_TBL[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '.', '/',
    '0', '1', 0, 0, 0, 0, 0, 0, 0, '9', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 'H', 0, 0, 0, 0, 0, 0, 0, 'P', 0, 0, 0, 'T'
};

/**
 * TEXT           = <any OCTET except CTLs,
 *                   but including LWS>
 */
static const unsigned char TEXTC_TBL[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, HT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 
    SP, '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', 
    '/', 
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
    ':', ';', '<', '=', '>', '?', '@', 
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 
    '[', '\\', ']', '^', '_', '`', 
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 
    '{', '|', '}', '~', 0, 
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 
    143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 
    158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 
    173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 
    188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 
    203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 
    218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 
    233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 
    248, 249, 250, 251, 252, 253, 254, 255
};


// method length
#define METHOD_LEN  7
// version length: HTTP/x.x
#define VER_LEN 8

#define HEADER_SIZE     (2 * sizeof(uintptr_t) + sizeof(uint32_t))
#define HKEY_SIZE       (sizeof(uintptr_t) + sizeof(uint16_t))

#define GET_HKEY_PTR(r,n) \
    (&((uint8_t*)r)[sizeof( http_t ) + HEADER_SIZE * n])

#define GET_HVAL_PTR(r,n) \
    (&((uint8_t*)r)[sizeof( http_t ) + HEADER_SIZE * n + HKEY_SIZE])

#define ADD_HKEY(r,k,l) do{ \
    uint8_t *mem = GET_HKEY_PTR(r, r->nheader); \
    *(uintptr_t*)mem = k; \
    *(uint16_t*)&mem[sizeof( uintptr_t )] = (uint16_t)l; \
}while(0)

#define ADD_HVAL(r,v,l) do{ \
    uint8_t *mem = GET_HVAL_PTR(r, r->nheader); \
    *(uintptr_t*)mem = v; \
    *(uint16_t*)&mem[sizeof( uintptr_t )] = (uint16_t)l; \
}while(0)


static int parse_hkey( http_t *r, char *buf, size_t len, uint16_t maxhdrlen );


/**
 * wait the end-of-line(CRLF)
 * HTTP/0.9 does not support the header
 */
static int parse_eol( http_t *r, char *buf )
{
    char *str = buf + r->cur;
    
    switch( *str )
    {
        // need more bytes
        case 0:
            return HTTP_EAGAIN;
        
        // check header-tail
        case CR:
            if( !str[1] ){
                return HTTP_EAGAIN;
            }
            else if( str[1] == LF ){
                // calc and save index
                r->head = r->cur = r->cur + 2;
                r->phase = HTTP_PHASE_DONE;
                return HTTP_SUCCESS;
            }
        
        default:
            return HTTP_ELINEFMT;
    }
}

static int parse_header( http_t *r, char *buf, size_t len, uint16_t maxhdrlen )
{
    char *str = buf + r->cur;
    
    switch( *str )
    {
        // need more bytes
        case 0:
            return HTTP_EAGAIN;
        
        // check header-tail
        case CR:
            if( !str[1] ){
                return HTTP_EAGAIN;
            }
            else if( str[1] == LF ){
                // calc and save index
                r->head = r->cur = r->cur + 2;
                r->phase = HTTP_PHASE_DONE;
                return HTTP_SUCCESS;
            }
            // invalid header format
            return HTTP_EHDRFMT;
        
        default:
            if( r->nheader < r->maxheader ){
                // set next parser hkey
                r->phase = HTTP_PHASE_HKEY;
                return parse_hkey( r, buf, len, maxhdrlen );
            }
            // too many headers
            return HTTP_ENHDR;
    }
}


static int parse_hval( http_t *r, char *buf, size_t len, uint16_t maxhdrlen )
{
    char *delim = strchr_brk( buf + r->cur, len - r->cur, CR, TEXTC_TBL );
    uintptr_t hkey = *(uintptr_t*)GET_HKEY_PTR( r, r->nheader );
    uintptr_t tail = 0;
    uintptr_t vlen = 0;
    
    // EILSEQ: illegal byte sequence == invalid header format
    if( errno ){
        return HTTP_EHDRFMT;
    }
    else if( delim )
    {
        tail = (uintptr_t)delim - (uintptr_t)buf;
        // calc value-length
        vlen = (uint32_t)(tail - r->head);
        
        // LF not found
        if( delim[1] != LF )
        {
            // update cursor if null-terminator
            if( !delim[1] ){
                r->cur = r->head + vlen;
                return HTTP_EAGAIN;
            }
            
            // invalid format
            return HTTP_EHDRFMT;
        }
        // header-length too large
        else if( ( tail - hkey ) > maxhdrlen ){
            return HTTP_EHDRLEN;
        }
        else {
            ADD_HVAL( r, r->head, vlen );
            // skip CRLF
            delim += 2;
            r->nheader++;
            r->cur = (uintptr_t)delim - (uintptr_t)buf;
            r->head = r->cur;
            // set next parser
            r->phase = HTTP_PHASE_HEADER;
            
            return parse_header( r, buf, len, maxhdrlen );
        }
    }
    // header-length too large
    else if( ( len - hkey ) > maxhdrlen ){
        return HTTP_EHDRLEN;
    }
    
    return HTTP_EAGAIN;
}


static int parse_hkey( http_t *r, char *buf, size_t len, uint16_t maxhdrlen )
{
    char *delim = NULL;
    uintptr_t tail = 0;
    uintptr_t klen = 0;
    
RECHECK:
    delim = buf + r->cur;
    
    // check header-tail
    if( delim[0] == CR )
    {
        if( !delim[1] ){
            return HTTP_EAGAIN;
        }
        else if( delim[1] == LF ){
            // calc and save index
            r->head = r->cur = r->cur + 2;
            r->phase = HTTP_PHASE_DONE;
            return HTTP_SUCCESS;
        }
        
        // invalid header format
        return HTTP_EHDRFMT;
    }
    // too many headers
    else if( r->nheader > r->maxheader ){
        return HTTP_ENHDR;
    }
    
    // lookup seperator
    delim = strchr_brkrep( delim, len - r->cur, COLON, HKEYC_TBL );
    // EILSEQ: illegal byte sequence == invalid header format
    if( errno ){
        return HTTP_EHDRFMT;
    }
    // found delimiter
    else if( delim )
    {
        tail = (uintptr_t)delim - (uintptr_t)buf;
        klen = tail - r->head;
        // header-length too large
        if( klen > maxhdrlen ){
            return HTTP_EHDRLEN;
        }
        
        // skip COLON and SPHT
        delim++;
        while( SPHT[(unsigned char)*delim] ){
            delim++;
        }
        
        // empty value
        if( *delim == CR )
        {
            // valid tail format
            if( delim[1] == LF )
            {
                // skip CRLF
                delim += 2;
                // set cursor and head position
                r->head = r->cur = (uintptr_t)delim - (uintptr_t)buf;
                // recheck field name
                if( *delim ){
                    goto RECHECK;
                }
            }
            // invalid header format
            else if( delim[1] ){
                return HTTP_EHDRFMT;
            }
        }
        else {
            // set key-index and hkey-length
            ADD_HKEY( r, r->head, klen );
            // set cursor
            r->cur = (uintptr_t)delim - (uintptr_t)buf;
            r->head = r->cur;
            // set next parser
            r->phase = HTTP_PHASE_HVAL;
            
            return parse_hval( r, buf, len, maxhdrlen );
        }
    }
    // header-length too large
    else if( len - r->head > ( maxhdrlen - 1 ) ){
        return HTTP_EHDRLEN;
    }
        
    // update parse cursor
    r->cur = len;
    
    return HTTP_EAGAIN;
}


static int parse_ver( http_t *r, char *buf, size_t len, uint16_t maxhdrlen )
{
    char *delim = strchr_brk( buf + r->cur, len - r->cur, CR, VERC_TBL );
    
    // EILSEQ: illegal byte sequence == HTTP_BAD_REQUEST
    if( errno ){
        return HTTP_EVERSION;
    }
    // found
    else if( delim && delim[1] )
    {
        // calc index(same as token-length)
        size_t slen = (uintptr_t)delim - (uintptr_t)buf - r->head;
        match64bit_u src = { .bit = 0 };
        
        // unsupported version
        if( delim[1] != LF || slen != VER_LEN ){
            return HTTP_EVERSION;
        }
        
        // check version
        memcpy( src.str, buf + r->head, slen );
        // HTTP/1.1
        if( src.bit == V_11.bit ){
            r->protocol |= HTTP_V11;
        }
        // HTTP/1.0
        else if( src.bit == V_10.bit )
        {
            // illegal request if method is not the GET, HEAD or POST method
            if( r->protocol > HTTP_MPOST ){
                return HTTP_EMETHOD;
            }
            r->protocol |= HTTP_V10;
        }
        // not HTTP/0.9
        else if( src.bit == V_09.bit )
        {
            // illegal request if method is not the GET method
            if( r->protocol != HTTP_MGET ){
                return HTTP_EMETHOD;
            }
            r->protocol |= HTTP_V09;
            // skip CRLF
            r->head = r->cur = r->head + slen + 2;
            // set next phase
            r->phase = HTTP_PHASE_EOL;
            
            return parse_eol( r, buf );
        }
        // unsupported version
        else {
            return HTTP_EVERSION;
        }
                
        // skip CRLF
        r->head = r->cur = r->head + slen + 2;
        // set next phase
        r->phase = HTTP_PHASE_HEADER;
        
        return parse_header( r, buf, len, maxhdrlen );
    }
    // invalid version format
    else if( ( len - r->head ) > VER_LEN ){
        return HTTP_EVERSION;
    }
    
    // update parse cursor
    r->cur = len;
    
    return HTTP_EAGAIN;
}


static int parse_uri( http_t *r, char *buf, size_t len, uint16_t maxurilen,
                      uint16_t maxhdrlen )
{
    char *delim = strchr_brk( buf + r->cur, len - r->cur, SP, URIC_TBL );
    
    // EILSEQ: illegal byte sequence == HTTP_BAD_REQUEST
    if( errno )
    {
        // probably, HTTP/0.9 request
        if( delim[0] == CR && delim[1] == LF && !delim[2] )
        {
            // reset errno
            errno = 0;
            // HTTP/0.9 supports a GET method only
            if( r->protocol != HTTP_MGET ){
                return HTTP_EMETHOD;
            }
            
            // set next phase
            r->phase = HTTP_PHASE_DONE;
            goto CHECK_URI;
        }
        
        // invalid uri string
        return HTTP_EBADURI;
    }
    // found
    else if( delim )
    {
        // set next phase
        r->phase = HTTP_PHASE_VERSION;

CHECK_URI:
        // calc uri-length
        r->uri = (uint8_t)r->head;
        r->urilen = (uint16_t)((uintptr_t)delim - (uintptr_t)buf - r->head);
        // request-uri too long
        if( r->urilen > maxurilen ){
            return HTTP_EURILEN;
        }
        // HTTP/0.9 request
        else if( r->phase == HTTP_PHASE_DONE ){
            return HTTP_SUCCESS;
        }
        
        // update cursor and token head position
        r->head = r->cur = r->head + r->urilen + 1;
        
        return parse_ver( r, buf, len, maxhdrlen );
    }
    // request-uri too long
    else if( len - r->head > maxurilen ){
        return HTTP_EURILEN;
    }
    
    // update parse cursor
    r->cur = len;
    
    return HTTP_EAGAIN;
}



static int parse_method( http_t *r, char *buf, size_t len, uint16_t maxurilen,
                         uint16_t maxhdrlen )
{
    char *delim = strchr_brk( buf + r->cur, len, SP, METHODC_TBL );
    
    // EILSEQ: illegal byte sequence == method not implemented
    if( errno ){
        return HTTP_EMETHOD;
    }
    // found
    else if( delim )
    {
        size_t slen = (uintptr_t)delim - (uintptr_t)buf;
        match64bit_u src = { .bit = 0 };
        
        if( slen > METHOD_LEN ){
            return HTTP_EMETHOD;
        }
        
        memcpy( src.str, buf, slen );
        // check method
        if( src.bit == M_GET.bit ){
            r->protocol = HTTP_MGET;
        }
        else if( src.bit == M_POST.bit ){
            r->protocol = HTTP_MPOST;
        }
        else if( src.bit == M_PUT.bit ){
            r->protocol = HTTP_MPUT;
        }
        else if( src.bit == M_DELETE.bit ){
            r->protocol = HTTP_MDELETE;
        }
        else if( src.bit == M_HEAD.bit ){
            r->protocol = HTTP_MHEAD;
        }
        else if( src.bit == M_OPTIONS.bit ){
            r->protocol = HTTP_MOPTIONS;
        }
        else if( src.bit == M_TRACE.bit ){
            r->protocol = HTTP_MTRACE;
        }
        else if( src.bit == M_CONNECT.bit ){
            r->protocol = HTTP_MCONNECT;
        }
        // method not implemented
        else {
            return HTTP_EMETHOD;
        }

        // update parse cursor, token-head and url head
        r->head = r->cur = (uintptr_t)delim - (uintptr_t)buf + 1;
        // set next phase
        r->phase = HTTP_PHASE_URI;
        
        return parse_uri( r, buf, len, maxurilen, maxhdrlen );
    }
    // method not implemented
    else if( len > METHOD_LEN ){
        return HTTP_EMETHOD;
    }
    
    // update cursor
    r->cur = len;
    
    return HTTP_EAGAIN;
}


int http_req_parse( http_t *r, char *buf, size_t len, uint16_t maxurilen,
                    uint16_t maxhdrlen )
{
    switch( r->phase )
    {
        case HTTP_PHASE_DONE:
            return HTTP_SUCCESS;
        
        case HTTP_PHASE_METHOD:
            return parse_method( r, buf, len, maxurilen, maxhdrlen );
        
        case HTTP_PHASE_URI:
            return parse_uri( r, buf, len, maxurilen, maxhdrlen );
        
        case HTTP_PHASE_VERSION:
            return parse_ver( r, buf, len, maxhdrlen );

        case HTTP_PHASE_EOL:
            return parse_eol( r, buf );
            
        case HTTP_PHASE_HEADER:
            return parse_header( r, buf, len, maxhdrlen );
        
        case HTTP_PHASE_HKEY:
            return parse_hkey( r, buf, len, maxhdrlen );
        
        case HTTP_PHASE_HVAL:
            return parse_hval( r, buf, len, maxhdrlen );
    }
    
    return HTTP_ERROR;
}


http_t *http_alloc( uint8_t maxheader )
{
    http_t *r = (http_t*)calloc( 1, http_alloc_size( maxheader ) );
    
    if( r ){
        r->maxheader = maxheader;
    }
    
    return r;
}


void http_reset( http_t *r )
{
    *r = (http_t){
        .cur = 0,
        .head = 0,
        .phase = 0,
        .protocol = 0,
        .urilen = 0,
        .uri = 0,
        .nheader = 0,
        .maxheader = r->maxheader
    };
}


void http_free( http_t *r )
{
    free( (void*)r );
}


int http_getheader_at( http_t *r, uintptr_t *key, uint16_t *klen, 
                       uintptr_t *val, uint16_t *vlen, uint8_t at )
{
    if( at < r->nheader ){
        uint8_t *mem = GET_HKEY_PTR( r, at );
    
        *key = *(uintptr_t*)mem;
        *klen = *(uint16_t*)&mem[sizeof( uintptr_t )];
        
        mem += HKEY_SIZE;
        *val = *(uintptr_t*)mem;
        *vlen = *(uint16_t*)&mem[sizeof( uintptr_t )];
        
        return 0;
    }
    
    return -1;
}


