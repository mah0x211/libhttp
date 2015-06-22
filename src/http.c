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


/**
 * RFC 7230
 * 3.2.  Header Fields
 * https://tools.ietf.org/html/rfc7230#section-3.2
 *
 * OWS            = *( SP / HTAB )
 *                   ; optional whitespace
 * RWS            = 1*( SP / HTAB )
 *                  ; required whitespace
 * BWS            = OWS
 *                  ; "bad" whitespace
 *
 * header-field   = field-name ":" OWS field-value OWS
 *
 * field-name     = token
 *
 * 3.2.6.  Field Value Components
 * https://tools.ietf.org/html/rfc7230#section-3.2.6
 *
 * token          = 1*tchar
 * tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
 *                / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
 *                / DIGIT / ALPHA
 *                ; any VCHAR, except delimiters
 *
 * VCHAR          = %x21-7E
 * delimiters     = "(),/:;<=>?@[\]{}
 * 
 */
static const unsigned char HKEYC_TBL[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
//   !    "    #    $    %    &    '    (  )   *    +   ,   -    .   /
    '!', '"', '#', '$', '%', '&', '\'', 0, 0, '*', '+', 0, '-', '.', 0,
//   0    1    2    3    4    5    6    7    8    9
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
//  :  ;  <  =  >  ?  @
    2, 0, 0, 0, 0, 0, 0,
//   A    B    C    D    E    F    G    H    I    J    K    L    M    N    O
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
//   P   Q     R    S    T    U    V    W    X    Y    Z
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
//  [  \  ]   ^    _    `
    0, 0, 0, '^', '_', '`',
//   a    b    c    d    e    f    g    h    i    j    k    l    m    n    o
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
//   p    q    r    s    t    u    v    w    x    y    z
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
//  {   |   }   ~
    0, '|', 0, '~'
};


/**
 * RFC 7230
 * 3.2.  Header Fields
 * https://tools.ietf.org/html/rfc7230#section-3.2
 *
 * field-value    = *( field-content / obs-fold )
 * field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]
 * field-vchar    = VCHAR / obs-text
 *
 * VCHAR          = %x21-7E
 * obs-text       = %x80-FF
 * obs-fold       = CRLF 1*( SP / HTAB )
 *                  ; obsolete line folding
 *                  ; see https://tools.ietf.org/html/rfc7230#section-3.2.4
 */
// 0 = field-content
// 1 = CR
// 3 = invalid
static const unsigned char HVALC_TBL[256] = {
//                             HT          CR
    3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
    3, 3, 3, 3, 3, 3, 3,
//  SP !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//  0  1  2  3  4  5  6  7  8  9
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
//  :  ;  <  =  >  ?  @ 
    0, 0, 0, 0, 0, 0, 0, 
//  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
//  Z  [  \  ]  ^  _  ` 
    0, 0, 0, 0, 0, 0, 0,
//  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//  z  {  |  }  ~ 
    0, 0, 0, 0, 0,
    3
};


/**
 * RFC 7230
 * 3.1.2.  Status Line
 * https://tools.ietf.org/html/rfc7230#section-3.1.2
 *
 * reason-phrase  = *( HTAB / SP / VCHAR / obs-text )
 *
 * VCHAR          = %x21-7E
 * obs-text       = %x80-FF
 */
// 0 = HTAB / SP / VCHAR / obs-text
// 3 = invalid
static const unsigned char PHRASE_TBL[256] = {
//                             HT          CR
    3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
    3, 3, 3, 3, 3, 3, 3,
//  SP !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//  0  1  2  3  4  5  6  7  8  9
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
//  :  ;  <  =  >  ?  @ 
    0, 0, 0, 0, 0, 0, 0, 
//  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
//  Z  [  \  ]  ^  _  ` 
    0, 0, 0, 0, 0, 0, 0,
//  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//  z  {  |  }  ~ 
    0, 0, 0, 0, 0,
    3
};


static const unsigned char SPHT[256] = {
//                             HT
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
//                          SP
    0, 0, 0, 0, 0, 0, 0, 0, 1
};


// method length
#define METHOD_LEN  7
// version length: HTTP/x.x
#define VER_LEN     8
// status length
#define STATUS_LEN  3

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
    *(uint16_t*)&mem[sizeof( uintptr_t )] = (uint16_t)(l); \
}while(0)


/**
 * prototypes
 */
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
    unsigned char *delim = (unsigned char*)buf;
    uintptr_t hkey = *(uintptr_t*)GET_HKEY_PTR( r, r->nheader );
    size_t cur = r->cur;
    
    for(; cur < len; cur++ )
    {
        switch( HVALC_TBL[delim[cur]] )
        {
            // field-content
            case 0:
                continue;
            
            // CR
            case 1:
                // found LF
                if( delim[cur+1] == LF )
                {
                    size_t tail = cur;
                    
                    // remove OWS
                    while( SPHT[delim[tail-1]] ){
                        tail--;
                    }
                    // check length
                    if( ( tail - hkey ) > maxhdrlen ){
                        return HTTP_EHDRLEN;
                    }
                    
                    // calc value-length
                    ADD_HVAL( r, r->head, tail - r->head );
                    r->nheader++;
                    // skip CRLF
                    r->head = r->cur = cur + 2;
                    // set next parser
                    r->phase = HTTP_PHASE_HEADER;
                    
                    return parse_header( r, buf, len, maxhdrlen );
                }
                // null-terminator
                else if( !delim[cur+1] ){
                    goto CHECK_AGAIN;
                }
            
            // invalid
            default:
                return HTTP_EHDRFMT;
        }
    }

CHECK_AGAIN:
    // header-length too large
    if( ( len - hkey ) > maxhdrlen ){
        return HTTP_EHDRLEN;
    }
    r->cur = cur;
    
    return HTTP_EAGAIN;
}


static int parse_hkey( http_t *r, char *buf, size_t len, uint16_t maxhdrlen )
{
    unsigned char *delim = (unsigned char*)buf;
    size_t cur = r->cur;
    uintptr_t klen = r->head;
    unsigned char c = 0;
    
RECHECK:
    if( cur < len )
    {
        c = HKEYC_TBL[delim[cur]];
        switch( c )
        {
            // invalid
            case 0:
                return HTTP_EHDRFMT;
            
            // COLON
            case 2:
                // check length
                klen = cur - klen;
                if( klen > maxhdrlen ){
                    return HTTP_EHDRLEN;
                }
                
                // remove OWS
                while( ++cur < len )
                {
                    // skip SPHT
                    if( SPHT[delim[cur]] ){
                        continue;
                    }
                    // empty field
                    else if( delim[cur] == CR )
                    {
                        if( !delim[cur+1] ){
                            r->cur = cur;
                            r->phase = HTTP_PHASE_HEADER;
                            return EAGAIN;
                        }
                        else if( delim[cur+1] == LF ){
                            r->head = klen = cur += 2;
                            goto RECHECK;
                        }
                    }
                    break;
                }
                
                // set key-index and hkey-length
                ADD_HKEY( r, r->head, klen );
                // set cursor
                r->head = r->cur = cur;
                // set next parser
                r->phase = HTTP_PHASE_HVAL;
                
                if( cur >= len ){
                    return HTTP_EAGAIN;
                }
                
                return parse_hval( r, buf, len, maxhdrlen );
        }
        delim[cur] = c;
        cur++;
        goto RECHECK;
    }
    // header-length too large
    else if( ( len - r->head ) > maxhdrlen ){
        return HTTP_EHDRLEN;
    }
    
    // update parse cursor
    r->cur = len;
    
    return HTTP_EAGAIN;
}


static int parse_ver( http_t *r, char *buf, size_t len, uint16_t maxhdrlen )
{
    char *delim = memchr( buf + r->cur, CR, len - r->cur );
    
    if( delim )
    {
        if( delim[1] == LF )
        {
            // calc index(same as token-length)
            size_t slen = (uintptr_t)delim - (uintptr_t)buf - r->head;
            match64bit_u src = { .bit = 0 };
            
            // unsupported version
            if( delim[1] != LF || slen != VER_LEN ){
                return HTTP_EVERSION;
            }
            
            // check version
            src.bit = *((uint64_t*)(buf + r->head));
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
            // HTTP/0.9
            else if( src.bit == V_09.bit )
            {
                // illegal request if method is not the GET method
                if( r->protocol != HTTP_MGET ){
                    return HTTP_EMETHOD;
                }
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
        // invalid line format
        else if( delim[1] ){
            return HTTP_ELINEFMT;
        }
        
        // update cursor
        r->cur = (uintptr_t)delim - (uintptr_t)buf;
    }
    // invalid version format
    else if( ( len - r->head ) > VER_LEN ){
        return HTTP_EVERSION;
    }
    // update parse cursor
    else {
        r->cur = len;
    }
    
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
        r->msg = (uint8_t)r->head;
        r->msglen = (uint16_t)((uintptr_t)delim - (uintptr_t)buf - r->head);
        // request-uri too long
        if( r->msglen > maxurilen ){
            return HTTP_EURILEN;
        }
        // HTTP/0.9 request
        else if( r->phase == HTTP_PHASE_DONE ){
            return HTTP_SUCCESS;
        }
        
        // update cursor and token head position
        r->head = r->cur = r->head + r->msglen + 1;
        
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
    char *delim = memchr( buf + r->cur, SP, len );
    
    if( delim )
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
        
        case HTTP_PHASE_DONE:
            return HTTP_SUCCESS;
    }
    
    return HTTP_ERROR;
}


static int parse_reason( http_t *r, char *buf, size_t len, uint16_t maxhdrlen )
{
    unsigned char *delim = (unsigned char*)buf;
    size_t cur = r->cur;
    
    for(; cur < len; cur++ )
    {
        switch( PHRASE_TBL[delim[cur]] )
        {
            // reason-phrase
            case 0:
                continue;
            
            // CR
            case 1:
                // found LF
                if( delim[cur+1] == LF )
                {
                    // check length
                    if( ( cur - r->head ) > UINT16_MAX ){
                        return HTTP_ELINEFMT;
                    }
                    
                    // calc reason-length
                    r->msg = (uint8_t)r->head;
                    r->msglen = (uint16_t)(cur - r->head);
                    // skip CRLF
                    r->head = r->cur = cur + 2;
                    // set next parser
                    r->phase = HTTP_PHASE_HEADER;
                    
                    return parse_header( r, buf, len, maxhdrlen );
                }
                // null-terminator
                else if( !delim[cur+1] ){
                    goto CHECK_AGAIN;
                }
            
            // invalid
            default:
                return HTTP_EHDRFMT;
        }
    }

CHECK_AGAIN:
    // header-length too large
    if( ( len - r->head ) > UINT16_MAX ){
        return HTTP_EHDRLEN;
    }
    r->cur = cur;
    
    return HTTP_EAGAIN;
}


static int parse_status( http_t *r, char *buf, size_t len, uint16_t maxhdrlen )
{
    char *delim = memchr( buf + r->cur, SP, len - r->cur );
    
    if( delim )
    {
        unsigned char *head = (unsigned char*)(buf + r->head);
        size_t slen = (uintptr_t)delim - (uintptr_t)head;
        
        // invalid status code
        if( slen != STATUS_LEN ||
            head[0] < '1' || head[0] > '5' ||
            head[1] < '0' || head[1] > '9' ||
            head[2] < '0' || head[2] > '9' ){
            return HTTP_ESTATUS;
        }
        
        // set status
        r->protocol |= ( head[0] - 0x30 ) * 100 +
                       ( head[1] - 0x30 ) * 10 +
                       ( head[2] - 0x30 );
        // update parse cursor, token-head and url head
        r->head = r->cur = r->head + slen + 1;
        // set next phase
        r->phase = HTTP_PHASE_REASON;
        
        return parse_reason( r, buf, len, maxhdrlen );
    }
    // method not implemented
    else if( ( len - r->cur ) > STATUS_LEN ){
        return HTTP_ESTATUS;
    }
    
    // update cursor
    r->cur = len;
    
    return HTTP_EAGAIN;
}


static int parse_ver_res( http_t *r, char *buf, size_t len, uint16_t maxhdrlen )
{
    char *delim = memchr( buf + r->cur, SP, len - r->cur );
    
    if( delim )
    {
        // calc index(same as token-length)
        size_t slen = (uintptr_t)delim - (uintptr_t)buf;
        
        if( slen == VER_LEN )
        {
            match64bit_u src = {
                .bit = *((uint64_t*)(buf + r->head))
            };
            
            // check version
            // HTTP/1.1
            if( src.bit == V_11.bit ){
                r->protocol = HTTP_V11;
            }
            // HTTP/1.0
            else if( src.bit == V_10.bit ){
                r->protocol = HTTP_V10;
            }
            // HTTP/0.9
            else if( src.bit == V_09.bit ){
                r->protocol = HTTP_V09;
            }
            // unsupported version
            else {
                return HTTP_EVERSION;
            }
            
            // skip SP
            r->head = r->cur = VER_LEN + 1;
            // set next phase
            r->phase = HTTP_PHASE_STATUS;
            
            return parse_status( r, buf, len, maxhdrlen );
        }
        
        // HTTP/0.9 simple-response
        r->protocol = HTTP_V09;
        r->cur = 0;
        
        return HTTP_SUCCESS;
    }
    // HTTP/0.9 simple-response
    else if( len > VER_LEN ){
        r->protocol = HTTP_V09;
        r->cur = 0;
        return HTTP_SUCCESS;
    }
    
    // update parse cursor
    r->cur = len;
    
    return HTTP_EAGAIN;
}


int http_res_parse( http_t *r, char *buf, size_t len, uint16_t maxhdrlen )
{
    switch( r->phase )
    {
        case HTTP_PHASE_VERSION_RES:
            return parse_ver_res( r, buf, len, maxhdrlen );
        
        case HTTP_PHASE_STATUS:
            return parse_status( r, buf, len, maxhdrlen );
        
        case HTTP_PHASE_REASON:
            return parse_reason( r, buf, len, maxhdrlen );

        case HTTP_PHASE_EOL:
            return parse_eol( r, buf );
        
        case HTTP_PHASE_HEADER:
            return parse_header( r, buf, len, maxhdrlen );
        
        case HTTP_PHASE_HKEY:
            return parse_hkey( r, buf, len, maxhdrlen );
        
        case HTTP_PHASE_HVAL:
            return parse_hval( r, buf, len, maxhdrlen );
        
        case HTTP_PHASE_DONE:
            return HTTP_SUCCESS;
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


