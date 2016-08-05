#include "test_http.h"

static void test_version( void )
{
    test_req_t req[] = {
        //
        // line-terminated by CRLF
        //
        // HTTP/0.9
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V09,
            0,
            "GET /foo/bar/baz\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V09,
            0,
            "GET /foo/bar/baz HTTP/0.9\r\n\r\n"
        },
        // HTTP/1.0
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\r\n\r\n"
        },
        // HTTP/1.1
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V11,
            0,
            "GET /foo/bar/baz HTTP/1.1\r\n\r\n"
        },


        //
        // line-terminated by LF
        //
        // HTTP/0.9
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V09,
            0,
            "GET /foo/bar/baz\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V09,
            0,
            "GET /foo/bar/baz HTTP/0.9\n\n"
        },
        // HTTP/1.0
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\n\n"
        },
        // HTTP/1.1
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V11,
            0,
            "GET /foo/bar/baz HTTP/1.1\n\n"
        },

        // end of request
        { 0, 0, 0, "" }
    };
    test_req_t *ptr = req;
    http_t *r = http_alloc(0);
    int rc;

    while( ptr->protocol )
    {
        rc = http_req_parse( r, ptr->entity, strlen( ptr->entity ),
                             INT16_MAX, INT16_MAX );
        assert( rc == ptr->rc );
        if( rc == HTTP_SUCCESS ){
            assert( r->protocol == ptr->protocol );
        }
        ptr++;
        http_init( r );
    }
    http_free( r );
}


static void test_version_res( void )
{
    test_req_t req[] = {
        //
        // line-terminated by CRLF
        //
        // HTTP/0.9
        {
            HTTP_SUCCESS,
            0 | HTTP_V09,
            0,
            "/foo/bar/baz\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V09,
            0,
            "HTTP/0.9 200 OK\r\n\r\n"
        },
        // HTTP/1.0
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 200 OK\r\n\r\n"
        },
        // HTTP/1.1
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V11,
            0,
            "HTTP/1.1 200 OK\r\n\r\n"
        },

        //
        // line-terminated by LF
        //
        // HTTP/0.9
        {
            HTTP_SUCCESS,
            0 | HTTP_V09,
            0,
            "/foo/bar/baz\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V09,
            0,
            "HTTP/0.9 200 OK\n\n"
        },
        // HTTP/1.0
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 200 OK\n\n"
        },
        // HTTP/1.1
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V11,
            0,
            "HTTP/1.1 200 OK\n\n"
        },

        // end of request
        { 0, 0, 0, "" }
    };
    test_req_t *ptr = req;
    http_t *r = http_alloc(0);
    int rc;

    while( ptr->protocol )
    {
        rc = http_res_parse( r, ptr->entity, strlen( ptr->entity ), INT16_MAX );
        assert( rc == ptr->rc );
        if( rc == HTTP_SUCCESS ){
            assert( r->protocol == ptr->protocol );
        }
        ptr++;
        http_init( r );
    }
    http_free( r );
}


#ifdef TESTS

int main(int argc, const char * argv[])
{
    test_version();
    test_version_res();
    return 0;
}

#endif

