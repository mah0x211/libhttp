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
//   !   "   #    $    %    &    '    (  )   *    +   ,   -    .   /
    '!', 0, '#', '$', '%', '&', '\'', 0, 0, '*', '+', 0, '-', '.', 0,
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
 * 3.1.2.  Status Line
 * https://tools.ietf.org/html/rfc7230#section-3.1.2
 *
 * reason-phrase  = *( HTAB / SP / VCHAR / obs-text )
 *
 * VCHAR          = %x21-7E
 * obs-text       = %x80-FF
 *
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
// 1 = field-content
// 2 = LF or CR
// 0 = invalid
static const unsigned char VCHAR[256] = {
//                             HT LF       CR
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0,
//  SP !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  0  1  2  3  4  5  6  7  8  9
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  :  ;  <  =  >  ?  @
    1, 1, 1, 1, 1, 1, 1,
//  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  Z  [  \  ]  ^  _  `
    1, 1, 1, 1, 1, 1, 1,
//  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  z  {  |  }  ~
    1, 1, 1, 1, 1
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

#define GET_HKEY_PTR(h,n) \
    (&((uint8_t*)(h))[sizeof( http_t ) + HEADER_SIZE * n])

#define GET_HVAL_PTR(h,n) \
    (&((uint8_t*)(h))[sizeof( http_t ) + HEADER_SIZE * n + HKEY_SIZE])

#define ADD_HKEY(h,k,l) do{ \
    uint8_t *mem = GET_HKEY_PTR(h, (h)->nheader); \
    *(uintptr_t*)mem = k; \
    *(uint16_t*)&mem[sizeof( uintptr_t )] = (uint16_t)l; \
}while(0)

#define ADD_HVAL(h,v,l) do{ \
    uint8_t *mem = GET_HVAL_PTR(h, (h)->nheader); \
    *(uintptr_t*)mem = v; \
    *(uint16_t*)&mem[sizeof( uintptr_t )] = (uint16_t)(l); \
}while(0)


/**
 * prototypes
 */
static int parse_hkey( http_t *h, char *buf, size_t len, uint16_t maxhdrlen );


/**
 * wait the end-of-line(CRLF)
 * HTTP/0.9 does not support the header
 */
static int parse_eol( http_t *h, char *buf )
{
    char *str = buf + h->cur;

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
                // skip CR
                h->cur++;
        case LF:
                // skip LF
                h->cur++;
                // calc and save index
                h->head = h->cur;
                h->phase = HTTP_PHASE_DONE;
                return HTTP_SUCCESS;
            }

        default:
            return HTTP_ELINEFMT;
    }
}


static int parse_header( http_t *h, char *buf, size_t len, uint16_t maxhdrlen )
{
    char *str = buf + h->cur;

    switch( *str )
    {
        // need more bytes
        case 0:
            return HTTP_EAGAIN;

        // check header-tail
        case LF:
            // calc and save index
            h->head = h->cur = h->cur + 1;
            h->phase = HTTP_PHASE_DONE;
            return HTTP_SUCCESS;

        // check header-tail
        case CR:
            if( !str[1] ){
                return HTTP_EAGAIN;
            }
            else if( str[1] == LF ){
                // calc and save index
                h->head = h->cur = h->cur + 2;
                h->phase = HTTP_PHASE_DONE;
                return HTTP_SUCCESS;
            }
            // invalid header format
            return HTTP_EHDRFMT;

        default:
            if( h->nheader < h->maxheader ){
                // set next parser hkey
                h->phase = HTTP_PHASE_HKEY;
                return parse_hkey( h, buf, len, maxhdrlen );
            }
            // too many headers
            return HTTP_ENHDR;
    }
}


static int parse_hval( http_t *h, char *buf, size_t len, uint16_t maxhdrlen )
{
    unsigned char *delim = (unsigned char*)buf;
    uintptr_t hkey = *(uintptr_t*)GET_HKEY_PTR( h, h->nheader );
    size_t cur = h->cur;
    size_t tail = 0;
    unsigned char c = 0;

    for(; cur < len; cur++ )
    {
        c = delim[cur];
        switch( VCHAR[c] )
        {
            case 1:
                continue;

            // LF or CR
            case 2:
                tail = cur;
                // found LF
                if( c == LF ){
                    cur++;
                }
                else if( delim[cur + 1] == LF ){
                    cur += 2;
                }
                // null-terminator
                else if( !delim[cur + 1] ){
                    goto CHECK_AGAIN;
                }
                else {
                    return HTTP_EHDRFMT;
                }

                // found LF
                // remove OWS
                while( SPHT[delim[tail-1]] ){
                    tail--;
                }
                // check length
                if( ( tail - hkey ) > maxhdrlen ){
                    return HTTP_EHDRLEN;
                }

                // calc value-length
                ADD_HVAL( h, h->head, tail - h->head );
                h->nheader++;
                // skip CRLF
                h->head = h->cur = cur;
                // set next parser
                h->phase = HTTP_PHASE_HEADER;

                return parse_header( h, buf, len, maxhdrlen );

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
    h->cur = cur;

    return HTTP_EAGAIN;
}


static int parse_hkey( http_t *h, char *buf, size_t len, uint16_t maxhdrlen )
{
    unsigned char *delim = (unsigned char*)buf;
    size_t cur = h->cur;
    uintptr_t klen = h->head;
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
                            h->cur = cur;
                            return HTTP_EAGAIN;
                        }
                        else if( delim[cur+1] == LF ){
                            h->head = h->cur = cur + 2;
                            h->phase = HTTP_PHASE_HEADER;
                            return parse_header( h, buf, len, maxhdrlen );
                        }
                        return HTTP_EHDRFMT;
                    }
                    else if( delim[cur] == LF ){
                        h->head = h->cur = cur + 1;
                        h->phase = HTTP_PHASE_HEADER;
                        return parse_header( h, buf, len, maxhdrlen );
                    }
                    break;
                }

                // set key-index and hkey-length
                ADD_HKEY( h, h->head, klen );
                // set cursor
                h->head = h->cur = cur;
                // set next parser
                h->phase = HTTP_PHASE_HVAL;

                if( cur >= len ){
                    return HTTP_EAGAIN;
                }

                return parse_hval( h, buf, len, maxhdrlen );
        }
        delim[cur] = c;
        cur++;
        goto RECHECK;
    }
    // header-length too large
    else if( ( len - h->head ) > maxhdrlen ){
        return HTTP_EHDRLEN;
    }

    // update parse cursor
    h->cur = len;

    return HTTP_EAGAIN;
}


static int parse_ver( http_t *h, char *buf, size_t len, uint16_t maxhdrlen )
{
    char *delim = memchr( buf + h->cur, LF, len - h->cur );

    if( delim )
    {
        // calc index(same as token-length)
        size_t slen = (uintptr_t)delim - (uintptr_t)buf - h->head;
        match64bit_u src = { .bit = 0 };

        // calc index(same as token-length)
        if( *(delim - 1) == CR ){
            slen = (uintptr_t)delim - (uintptr_t)buf - h->head - 1;
        }
        else {
            slen = (uintptr_t)delim - (uintptr_t)buf - h->head;
        }

        // unsupported version
        if( slen != VER_LEN ){
            return HTTP_EVERSION;
        }

        // check version
        src.bit = *((uint64_t*)(buf + h->head));
        // HTTP/1.1
        if( src.bit == V_11.bit ){
            h->protocol |= HTTP_V11;
        }
        // HTTP/1.0
        else if( src.bit == V_10.bit )
        {
            // illegal request if method is not the GET, HEAD or POST method
            if( h->protocol > HTTP_MPOST ){
                return HTTP_EMETHOD;
            }
            h->protocol |= HTTP_V10;
        }
        // HTTP/0.9
        else if( src.bit == V_09.bit )
        {
            // illegal request if method is not the GET method
            if( h->protocol != HTTP_MGET ){
                return HTTP_EMETHOD;
            }
            // skip CRLF
            h->head = h->cur = (uintptr_t)delim - (uintptr_t)buf + 1;
            // set next phase
            h->phase = HTTP_PHASE_EOL;

            return parse_eol( h, buf );
        }
        // unsupported version
        else {
            return HTTP_EVERSION;
        }

        // skip LF
        h->head = h->cur = (uintptr_t)delim - (uintptr_t)buf + 1;
        // set next phase
        h->phase = HTTP_PHASE_HEADER;

        return parse_header( h, buf, len, maxhdrlen );
    }
    // invalid version format
    else if( ( len - h->head ) > VER_LEN ){
        return HTTP_EVERSION;
    }
    // update parse cursor
    else {
        h->cur = len;
    }

    return HTTP_EAGAIN;
}


static int parse_uri( http_t *h, char *buf, size_t len, uint16_t maxurilen,
                      uint16_t maxhdrlen )
{
    char *delim = strchr_brk( buf + h->cur, len - h->cur, SP, URIC_TBL );

    // EILSEQ: illegal byte sequence == HTTP_BAD_REQUEST
    if( errno )
    {
        // probably, HTTP/0.9 request
        if( ( delim[0] == LF && !delim[1] ) ||
            ( delim[0] == CR && delim[1] == LF && !delim[2] ) )
        {
            // reset errno
            errno = 0;
            // HTTP/0.9 supports a GET method only
            if( h->protocol != HTTP_MGET ){
                return HTTP_EMETHOD;
            }

            // set next phase
            h->phase = HTTP_PHASE_DONE;
            goto CHECK_URI;
        }

        // invalid uri string
        return HTTP_EBADURI;
    }
    // found
    else if( delim )
    {
        // set next phase
        h->phase = HTTP_PHASE_VERSION;

CHECK_URI:
        // calc uri-length
        h->msg = (uint8_t)h->head;
        h->msglen = (uint16_t)((uintptr_t)delim - (uintptr_t)buf - h->head);
        // request-uri too long
        if( h->msglen > maxurilen ){
            return HTTP_EURILEN;
        }
        // HTTP/0.9 request
        else if( h->phase == HTTP_PHASE_DONE ){
            return HTTP_SUCCESS;
        }

        // update cursor and token head position
        h->head = h->cur = h->head + h->msglen + 1;

        return parse_ver( h, buf, len, maxhdrlen );
    }
    // request-uri too long
    else if( len - h->head > maxurilen ){
        return HTTP_EURILEN;
    }

    // update parse cursor
    h->cur = len;

    return HTTP_EAGAIN;
}


static int parse_method( http_t *h, char *buf, size_t len, uint16_t maxurilen,
                         uint16_t maxhdrlen )
{
    char *delim = memchr( buf + h->cur, SP, len );

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
            h->protocol = HTTP_MGET;
        }
        else if( src.bit == M_POST.bit ){
            h->protocol = HTTP_MPOST;
        }
        else if( src.bit == M_PUT.bit ){
            h->protocol = HTTP_MPUT;
        }
        else if( src.bit == M_DELETE.bit ){
            h->protocol = HTTP_MDELETE;
        }
        else if( src.bit == M_HEAD.bit ){
            h->protocol = HTTP_MHEAD;
        }
        else if( src.bit == M_OPTIONS.bit ){
            h->protocol = HTTP_MOPTIONS;
        }
        else if( src.bit == M_TRACE.bit ){
            h->protocol = HTTP_MTRACE;
        }
        else if( src.bit == M_CONNECT.bit ){
            h->protocol = HTTP_MCONNECT;
        }
        // method not implemented
        else {
            return HTTP_EMETHOD;
        }

        // update parse cursor, token-head and url head
        h->head = h->cur = (uintptr_t)delim - (uintptr_t)buf + 1;
        // set next phase
        h->phase = HTTP_PHASE_URI;

        return parse_uri( h, buf, len, maxurilen, maxhdrlen );
    }
    // method not implemented
    else if( len > METHOD_LEN ){
        return HTTP_EMETHOD;
    }

    // update cursor
    h->cur = len;

    return HTTP_EAGAIN;
}


int http_req_parse( http_t *h, char *buf, size_t len, uint16_t maxurilen,
                    uint16_t maxhdrlen )
{
    switch( h->phase )
    {
        case HTTP_PHASE_METHOD:
            return parse_method( h, buf, len, maxurilen, maxhdrlen );

        case HTTP_PHASE_URI:
            return parse_uri( h, buf, len, maxurilen, maxhdrlen );

        case HTTP_PHASE_VERSION:
            return parse_ver( h, buf, len, maxhdrlen );

        case HTTP_PHASE_EOL:
            return parse_eol( h, buf );

        case HTTP_PHASE_HEADER:
            return parse_header( h, buf, len, maxhdrlen );

        case HTTP_PHASE_HKEY:
            return parse_hkey( h, buf, len, maxhdrlen );

        case HTTP_PHASE_HVAL:
            return parse_hval( h, buf, len, maxhdrlen );

        case HTTP_PHASE_DONE:
            return HTTP_SUCCESS;
    }

    return HTTP_ERROR;
}


static int parse_reason( http_t *h, char *buf, size_t len, uint16_t maxhdrlen )
{
    unsigned char *delim = (unsigned char*)buf;
    size_t cur = h->cur;
    unsigned char c = 0;
    size_t tail = 0;

    for(; cur < len; cur++ )
    {
        c = delim[cur];
        switch( VCHAR[c] )
        {
            case 1:
                continue;

            // LF or CR
            case 2:
                tail = cur;
                // found LF
                if( c == LF ){
                    cur++;
                }
                else if( delim[cur+1] == LF ){
                    cur += 2;
                }
                // null-terminator
                else if( !delim[cur+1] ){
                    goto CHECK_AGAIN;
                }
                else {
                    return HTTP_EREASON;
                }

                // phrase-length too large
                if( ( cur - h->head ) > UINT16_MAX ){
                    return HTTP_EREASON;
                }

                // calc phrase-length
                h->msg = (uint8_t)h->head;
                h->msglen = (uint16_t)(cur - h->head);
                // skip CRLF
                h->head = h->cur = cur;
                // set next parser
                h->phase = HTTP_PHASE_HEADER;

                return parse_header( h, buf, len, maxhdrlen );

            // invalid
            default:
                return HTTP_EREASON;
        }
    }

CHECK_AGAIN:
    // phrase-length too large
    if( ( len - h->head ) > UINT16_MAX ){
        return HTTP_EREASON;
    }
    h->cur = cur;

    return HTTP_EAGAIN;
}


static int parse_status( http_t *h, char *buf, size_t len, uint16_t maxhdrlen )
{
    char *delim = memchr( buf + h->cur, SP, len - h->cur );

    if( delim )
    {
        unsigned char *head = (unsigned char*)(buf + h->head);
        size_t slen = (uintptr_t)delim - (uintptr_t)head;

        // invalid status code
        if( slen != STATUS_LEN ||
            head[0] < '1' || head[0] > '5' ||
            head[1] < '0' || head[1] > '9' ||
            head[2] < '0' || head[2] > '9' ){
            return HTTP_ESTATUS;
        }

        // set status
        h->protocol |= ( head[0] - 0x30 ) * 100 +
                       ( head[1] - 0x30 ) * 10 +
                       ( head[2] - 0x30 );
        // update parse cursor, token-head and url head
        h->head = h->cur = h->head + slen + 1;
        // set next phase
        h->phase = HTTP_PHASE_REASON;

        return parse_reason( h, buf, len, maxhdrlen );
    }
    // method not implemented
    else if( ( len - h->cur ) > STATUS_LEN ){
        return HTTP_ESTATUS;
    }

    // update cursor
    h->cur = len;

    return HTTP_EAGAIN;
}


static int parse_ver_res( http_t *h, char *buf, size_t len, uint16_t maxhdrlen )
{
    char *delim = memchr( buf + h->cur, SP, len - h->cur );

    if( delim )
    {
        // calc index(same as token-length)
        size_t slen = (uintptr_t)delim - (uintptr_t)buf;

        if( slen == VER_LEN )
        {
            match64bit_u src = {
                .bit = *((uint64_t*)(buf + h->head))
            };

            // check version
            // HTTP/1.1
            if( src.bit == V_11.bit ){
                h->protocol = HTTP_V11;
            }
            // HTTP/1.0
            else if( src.bit == V_10.bit ){
                h->protocol = HTTP_V10;
            }
            // HTTP/0.9
            else if( src.bit == V_09.bit ){
                h->protocol = HTTP_V09;
            }
            // unsupported version
            else {
                return HTTP_EVERSION;
            }

            // skip SP
            h->head = h->cur = VER_LEN + 1;
            // set next phase
            h->phase = HTTP_PHASE_STATUS;

            return parse_status( h, buf, len, maxhdrlen );
        }

        // HTTP/0.9 simple-response
        h->protocol = HTTP_V09;
        h->cur = 0;

        return HTTP_SUCCESS;
    }
    // HTTP/0.9 simple-response
    else if( len > VER_LEN ){
        h->protocol = HTTP_V09;
        h->cur = 0;
        return HTTP_SUCCESS;
    }

    // update parse cursor
    h->cur = len;

    return HTTP_EAGAIN;
}


int http_res_parse( http_t *h, char *buf, size_t len, uint16_t maxhdrlen )
{
    switch( h->phase )
    {
        case HTTP_PHASE_VERSION_RES:
            return parse_ver_res( h, buf, len, maxhdrlen );

        case HTTP_PHASE_STATUS:
            return parse_status( h, buf, len, maxhdrlen );

        case HTTP_PHASE_REASON:
            return parse_reason( h, buf, len, maxhdrlen );

        case HTTP_PHASE_EOL:
            return parse_eol( h, buf );

        case HTTP_PHASE_HEADER:
            return parse_header( h, buf, len, maxhdrlen );

        case HTTP_PHASE_HKEY:
            return parse_hkey( h, buf, len, maxhdrlen );

        case HTTP_PHASE_HVAL:
            return parse_hval( h, buf, len, maxhdrlen );

        case HTTP_PHASE_DONE:
            return HTTP_SUCCESS;
    }

    return HTTP_ERROR;
}


http_t *http_alloc( uint8_t maxheader )
{
    http_t *h = (http_t*)calloc( 1, http_alloc_size( maxheader ) );

    if( h ){
        h->maxheader = maxheader;
    }

    return h;
}


void http_free( http_t *h )
{
    free( (void*)h );
}


int http_getheader_at( http_t *h, uintptr_t *key, uint16_t *klen,
                       uintptr_t *val, uint16_t *vlen, uint8_t at )
{
    if( at < h->nheader ){
        uint8_t *mem = GET_HKEY_PTR( h, at );

        *key = *(uintptr_t*)mem;
        *klen = *(uint16_t*)&mem[sizeof( uintptr_t )];

        mem += HKEY_SIZE;
        *val = *(uintptr_t*)mem;
        *vlen = *(uint16_t*)&mem[sizeof( uintptr_t )];

        return 0;
    }

    return -1;
}


