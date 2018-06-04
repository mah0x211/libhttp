/**
 *  example.c
 *  Copyright 2015 Masatoshi Teruya All rights reserved.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include "http.h"


static void keys( http_t *r, char *buf )
{
    uint8_t i;
    uintptr_t key, val;
    uint16_t klen, vlen;
    char k[1024*8];
    char v[1024*8];

    for( i = 0; i < r->nheader; i++ ){
        http_getheader_at( r, &key, &klen, &val, &vlen, i );
        memcpy( k, buf + key, klen );
        k[klen] = 0;
        memcpy( v, buf + val, vlen );
        v[vlen] = 0;

        printf("%d:\t[%s][%s]\n", i, k, v );
    }
}

static void parse_request( void )
{
    char req[] =
        "GET /mah0x211/libhttp HTTP/1.1\r\n"
        "Host: github.com\r\n"
        "Connection: keep-alive\r\n"
        "Keep-Alive: 115\r\n"
        "Cache-Control: max-age=0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
        "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/43.0.2357.130 Safari/537.36\r\n"
        "Accept-Encoding: gzip, deflate, sdch\r\n"
        "Accept-Language: ja,en-US;q=0.8,en;q=0.6\r\n"
        "Accept-Charset: Shift_JIS,utf-8;q=0.7,*;q=0.7\r\n"
        "Cookie: __utma=xxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.x; _octo=XXX.X.XXXXXXXX.XXXXXXXXXX; logged_in=XX; _ga=XXX.X.XXXXXXXXX.XXXXXXXXXX\r\n"
        "\r\n";
    size_t len = sizeof( req ) + 1;
    uint16_t maxurilen = UINT16_MAX;
    uint16_t maxhdrlen = UINT16_MAX;
    uint8_t nheader = 20;
    http_t *r = http_alloc( nheader );
    int rc = http_parse_request( r, req, len, maxurilen, maxhdrlen );

    assert( rc == HTTP_SUCCESS );
    assert( http_method(r) == HTTP_MGET );
    assert( http_version(r) == HTTP_V11 );
    assert( r->nheader == 10 );
    keys( r, req );

    http_free( r );
}


static void parse_response( void )
{
    char res[] =
        "HTTP/1.1 200 OK\r\n"
        "Server: GitHub.com\r\n"
        "Date: Sat, 27 Jun 2015 04:10:06 GMT\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Status: 200 OK\r\n"
        "Content-Security-Policy: default-src *; script-src assets-cdn.github.com collector-cdn.github.com; object-src assets-cdn.github.com; style-src 'self' 'unsafe-inline' 'unsafe-eval' assets-cdn.github.com; img-src 'self' data: assets-cdn.github.com identicons.github.com www.google-analytics.com collector.githubapp.com *.githubusercontent.com *.gravatar.com *.wp.com; media-src 'none'; frame-src 'self' render.githubusercontent.com gist.github.com www.youtube.com player.vimeo.com checkout.paypal.com; font-src assets-cdn.github.com; connect-src 'self' live.github.com wss://live.github.com uploads.github.com status.github.com api.github.com www.google-analytics.com github-cloud.s3.amazonaws.com\r\n"
        "Cache-Control: no-cache\r\n"
        "Vary: X-PJAX\r\n"
        "X-UA-Compatible: IE=Edge,chrome=1\r\n"
        "Set-Cookie: logged_in=no; domain=.github.com; path=/; expires=Wed, 27 Jun 2035 04:10:06 -0000; secure; HttpOnly\r\n"
        "X-Request-Id: 8ea8d359ded1d2d30094d38b2a4e73d3\r\n"
        "X-Runtime: 0.037659\r\n"
        "X-GitHub-Request-Id: 999DDDE5:0A3A:102882A:558E221D\r\n"
        "Strict-Transport-Security: max-age=31536000; includeSubdomains; preload\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "X-XSS-Protection: 1; mode=block\r\n"
        "X-Frame-Options: deny\r\n"
        "Vary: Accept-Encoding\r\n"
        "X-Served-By: 1868c9f28a71d80b2987f48dbd1824a0\r\n"
        "\r\n";
    size_t len = sizeof( res );
    uint16_t maxhdrlen = UINT16_MAX;
    http_t *r = http_alloc(20);
    int rc = http_parse_response( r, res, len, maxhdrlen );
    assert( rc == HTTP_SUCCESS );
    assert( http_version(r) == HTTP_V11 );
    assert( http_status(r) == HTTP_OK );
    assert( r->nheader == 19 );
    keys( r, res );

    http_free( r );
}


int main( int argc, const char *argv[] )
{
    printf("parse_request:\n");
    parse_request();
    printf("parse_response:\n");
    parse_response();

    return 0;
}
