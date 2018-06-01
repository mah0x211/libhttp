#include "test_http.h"


static void test_header( void )
{
    test_req_t req[] = {
        //
        // line-terminated by CRLF
        //
        // HTTP/0.9
        {
            HTTP_EBADURI,
            HTTP_MGET | HTTP_V09,
            0,
            "GET /foo/bar/baz\r\n"
            "Host: example.com\r\n"
            "\r\n"
        },
        {
            HTTP_ELINEFMT,
            HTTP_MGET | HTTP_V09,
            0,
            "GET /foo/bar/baz HTTP/0.9\r\n"
            "Host: example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            1,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host: example.com\r\n"
            "\r\n"
        },
        {
            HTTP_EHDRFMT,
            HTTP_MGET | HTTP_V10,
            1,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host: 1.example.com\r\n"
            " 2.example.com\r\n"
            "\t 2.example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            2,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host1: example.com\r\n"
            "Host2: example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            3,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host1: example.com\r\n"
            "Host2: example.com\r\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            2,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host1: \r\n"
            "Host2: example.com\r\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            2,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host1:\r\n"
            "Host2: example.com\r\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            2,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host1:       \r\n"
            "Host2: example.com\r\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            2,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host1: example.com\r\n"
            "Host2: \r\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            2,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host1: example.com\r\n"
            "Host2: example.com\r\n"
            "Host3: \r\n"
            "\r\n"
        },
        // invalid headers
        {
            HTTP_EHDRFMT,
            HTTP_MGET | HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host : example.com\r\n"
            "\r\n"
        },
        {
            HTTP_EHDRFMT,
            HTTP_MGET | HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host: example.com\rinvalid format\n"
            "\r\n"
        },
        {
            HTTP_EHDRFMT,
            HTTP_MGET | HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "invalid header format\r\n"
            "\r\n"
        },
        {
            HTTP_EHDRLEN,
            HTTP_MGET | HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host: this field length will exceeded the limit of maximum header length\r\n"
            "\r\n"
        },
        {
            HTTP_ENHDR,
            HTTP_MGET | HTTP_V10,
            3,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host1: example.com\r\n"
            "Host2: example.com\r\n"
            "Host3: example.com\r\n"
            "Host4: this field will exceeded the limit of  maximum number of header\r\n"
            "\r\n"
        },

        //
        // line-terminated by LF
        //
        // HTTP/0.9
        {
            HTTP_EBADURI,
            HTTP_MGET | HTTP_V09,
            0,
            "GET /foo/bar/baz\n"
            "Host: example.com\n"
            "\n"
        },
        {
            HTTP_ELINEFMT,
            HTTP_MGET | HTTP_V09,
            0,
            "GET /foo/bar/baz HTTP/0.9\n"
            "Host: example.com\n"
            "\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            1,
            "GET /foo/bar/baz HTTP/1.0\n"
            "Host: example.com\n"
            "\n"
        },
        {
            HTTP_EHDRFMT,
            HTTP_MGET | HTTP_V10,
            1,
            "GET /foo/bar/baz HTTP/1.0\n"
            "Host: 1.example.com\n"
            " 2.example.com\n"
            "\t 2.example.com\n"
            "\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            2,
            "GET /foo/bar/baz HTTP/1.0\n"
            "Host1: example.com\n"
            "Host2: example.com\n"
            "\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            3,
            "GET /foo/bar/baz HTTP/1.0\n"
            "Host1: example.com\n"
            "Host2: example.com\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\n"
            "\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            2,
            "GET /foo/bar/baz HTTP/1.0\n"
            "Host1: \n"
            "Host2: example.com\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\n"
            "\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            2,
            "GET /foo/bar/baz HTTP/1.0\n"
            "Host1:\n"
            "Host2: example.com\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\n"
            "\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            2,
            "GET /foo/bar/baz HTTP/1.0\n"
            "Host1:    \n"
            "Host2: example.com\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\n"
            "\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            2,
            "GET /foo/bar/baz HTTP/1.0\n"
            "Host1: example.com\n"
            "Host2: \n"
            "Host3: 1.example.com 2.example.com\t3.example.com\n"
            "\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            2,
            "GET /foo/bar/baz HTTP/1.0\n"
            "Host1: example.com\n"
            "Host2: example.com\n"
            "Host3: \n"
            "\n"
        },
        // invalid headers
        {
            HTTP_EHDRFMT,
            HTTP_MGET | HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\n"
            "Host : example.com\n"
            "\n"
        },
        {
            HTTP_EHDRFMT,
            HTTP_MGET | HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\n"
            "Host: example.com\rinvalid format\n"
            "\n"
        },
        {
            HTTP_EHDRFMT,
            HTTP_MGET | HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\n"
            "invalid header format\n"
            "\n"
        },
        {
            HTTP_EHDRLEN,
            HTTP_MGET | HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\n"
            "Host: this field length will exceeded the limit of maximum header length\n"
            "\n"
        },
        {
            HTTP_ENHDR,
            HTTP_MGET | HTTP_V10,
            3,
            "GET /foo/bar/baz HTTP/1.0\n"
            "Host1: example.com\n"
            "Host2: example.com\n"
            "Host3: example.com\n"
            "Host4: this field will exceeded the limit of  maximum number of header\n"
            "\n"
        },

        // end of request
        { 0, 0, 0, "" }
    };
    test_req_t *ptr = req;
    http_t *r = http_alloc(3);
    int rc;

    while( ptr->protocol )
    {
        rc = http_req_parse( r, ptr->entity, strlen(ptr->entity), INT16_MAX, 63 );
        assert( rc == ptr->rc );
        assert( r->nheader == ptr->nheader );
        if( rc == HTTP_SUCCESS ){
            assert( r->protocol == ptr->protocol );
        }
        ptr++;
        http_init( r );
    }
    http_free( r );
}


static void test_header_res( void )
{
    test_req_t req[] = {
        //
        // line-terminated by CRLF
        //
        // HTTP/0.9
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V09,
            1,
            "HTTP/0.9 200 OK\r\n"
            "Host: example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V10,
            1,
            "HTTP/1.0 200 OK\r\n"
            "Host: example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V11,
            3,
            "HTTP/1.1 200 OK\r\n"
            "Host1: example.com\r\n"
            "Host2: example.com\r\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\r\n"
            "\r\n"
        },

        //
        // line-terminated by LF
        //
        // HTTP/0.9
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V09,
            1,
            "HTTP/0.9 200 OK\n"
            "Host: example.com\n"
            "\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V10,
            1,
            "HTTP/1.0 200 OK\n"
            "Host: example.com\n"
            "\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V11,
            3,
            "HTTP/1.1 200 OK\n"
            "Host1: example.com\n"
            "Host2: example.com\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\n"
            "\n"
        },

        // end of response
        { 0, 0, 0, "" }
    };
    test_req_t *ptr = req;
    http_t *r = http_alloc(3);
    int rc;

    while( ptr->protocol )
    {
        rc = http_res_parse( r, ptr->entity, strlen(ptr->entity), UINT16_MAX );
        assert( rc == ptr->rc );
        assert( r->nheader == ptr->nheader );
        if( rc == HTTP_SUCCESS ){
            assert( r->protocol == ptr->protocol );
        }
        ptr++;
        http_init( r );
    }
    http_free( r );
}


static void test_partial_empty_header( void )
{
    char chunked[] =
            "HTTP/1.1 200 OK\r\n"
            "Host1:  \t  \r";
    char completed[] =
            "HTTP/1.1 200 OK\r\n"
            "Host1:  \t  \r\n"
            "Host2: example.com\r\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\r\n"
            "\r\n";
    http_t *r = http_alloc(3);
    int rc;

    rc = http_res_parse( r, chunked, sizeof(chunked), UINT16_MAX );
    assert( rc == HTTP_EAGAIN );
    rc = http_res_parse( r, completed, sizeof(completed), UINT16_MAX );
    assert( rc == HTTP_SUCCESS );
    assert( r->nheader == 2 );
    assert( r->protocol == (HTTP_OK|HTTP_V11) );

    http_free( r );
}

#ifdef TESTS

int main(void)
{
    test_header();
    test_header_res();
    test_partial_empty_header();
    return 0;
}

#endif


