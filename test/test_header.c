#include "test_http.h"


static void test_header( void )
{
    test_req_t req[] = {
        // HTTP/0.9
        {
            HTTP_EBADURI,
            HTTP_MGET,
            HTTP_V09,
            0,
            "GET /foo/bar/baz\r\n"
            "Host: example.com\r\n"
            "\r\n"
        },
        {
            HTTP_ELINEFMT,
            HTTP_MGET,
            HTTP_V09,
            0,
            "GET /foo/bar/baz HTTP/0.9\r\n"
            "Host: example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET,
            HTTP_V10,
            1,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host: example.com\r\n"
            "\r\n"
        },
        {
            HTTP_EHDRFMT,
            HTTP_MGET,
            HTTP_V10,
            1,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host: 1.example.com\r\n"
            " 2.example.com\r\n"
            "\t 2.example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET,
            HTTP_V10,
            2,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host1: example.com\r\n"
            "Host2: example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET,
            HTTP_V10,
            3,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host1: example.com\r\n"
            "Host2: example.com\r\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\r\n"
            "\r\n"
        },
        // invalid headers
        {
            HTTP_EHDRFMT,
            HTTP_MGET,
            HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host : example.com\r\n"
            "\r\n"
        },
        {
            HTTP_EHDRFMT,
            HTTP_MGET,
            HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host: example.com\rinvalid format\n"
            "\r\n"
        },
        {
            HTTP_EHDRFMT,
            HTTP_MGET,
            HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "invalid header format\r\n"
            "\r\n"
        },
        {
            HTTP_EHDRLEN,
            HTTP_MGET,
            HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host: this field length will exceeded the limit of maximum header length\r\n"
            "\r\n"
        },
        {
            HTTP_ENHDR,
            HTTP_MGET,
            HTTP_V10,
            3,
            "GET /foo/bar/baz HTTP/1.0\r\n"
            "Host1: example.com\r\n"
            "Host2: example.com\r\n"
            "Host3: example.com\r\n"
            "Host4: this field will exceeded the limit of  maximum number of header\r\n"
            "\r\n"
        },
        // end of request
        { 0, 0, 0, 0, "" }
    };
    test_req_t *ptr = req;
    http_t *r = http_alloc(3);
    int rc;
    
    while( ptr->code )
    {
        rc = http_req_parse( r, ptr->entity, strlen(ptr->entity), INT16_MAX, 63 );
        assert( rc == ptr->rc );
        assert( r->nheader == ptr->nheader );
        if( rc == HTTP_SUCCESS ){
            assert( r->version == ptr->version );
            assert( r->code == ptr->code );
        }
        ptr++;
        http_init( r );
    }
    http_free( r );
}


static void test_header_res( void )
{
    test_req_t req[] = {
        // HTTP/0.9
        {
            HTTP_SUCCESS,
            HTTP_OK,
            HTTP_V09,
            1,
            "HTTP/0.9 200 OK\r\n"
            "Host: example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_OK,
            HTTP_V10,
            1,
            "HTTP/1.0 200 OK\r\n"
            "Host: example.com\r\n"
            "\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_OK,
            HTTP_V11,
            3,
            "HTTP/1.1 200 OK\r\n"
            "Host1: example.com\r\n"
            "Host2: example.com\r\n"
            "Host3: 1.example.com 2.example.com\t3.example.com\r\n"
            "\r\n"
        },
        // end of request
        { 0, 0, 0, 0, "" }
    };
    test_req_t *ptr = req;
    http_t *r = http_alloc(3);
    int rc;
    
    while( ptr->code )
    {
        rc = http_res_parse( r, ptr->entity, strlen(ptr->entity), UINT16_MAX );
        assert( rc == ptr->rc );
        assert( r->nheader == ptr->nheader );
        if( rc == HTTP_SUCCESS ){
            assert( r->version == ptr->version );
            assert( r->code == ptr->code );
        }
        ptr++;
        http_init( r );
    }
    http_free( r );
}

#ifdef TESTS

int main(int argc, const char * argv[])
{
    test_header();
    test_header_res();
    return 0;
}

#endif


