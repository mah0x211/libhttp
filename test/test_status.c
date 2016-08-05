#include "test_http.h"

static void test_status( void )
{
    test_req_t req[] = {
        //
        // line-terminated by CRLF
        //
        // HTTP/1.0
        {
            HTTP_SUCCESS,
            HTTP_CONTINUE | HTTP_V10,
            0,
            "HTTP/1.0 100 OK\r\n\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 200 OK\r\n\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MULTIPLE_CHOICES | HTTP_V10,
            0,
            "HTTP/1.0 300 OK\r\n\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_BAD_REQUEST | HTTP_V10,
            0,
            "HTTP/1.0 400 OK\r\n\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_INTERNAL_SERVER_ERROR | HTTP_V10,
            0,
            "HTTP/1.0 500 OK\r\n\r\n"
        },
        // invalid status code
        {
            HTTP_ESTATUS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 10 OK\r\n\r\n"
        },
        {
            HTTP_ESTATUS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 600 OK\r\n\r\n"
        },
        {
            HTTP_ESTATUS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 700 OK\r\n\r\n"
        },
        {
            HTTP_ESTATUS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 800 OK\r\n\r\n"
        },
        {
            HTTP_ESTATUS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 900 OK\r\n\r\n"
        },


        //
        // line-terminated by LF
        //
        // HTTP/1.0
        {
            HTTP_SUCCESS,
            HTTP_CONTINUE | HTTP_V10,
            0,
            "HTTP/1.0 100 OK\n\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 200 OK\n\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MULTIPLE_CHOICES | HTTP_V10,
            0,
            "HTTP/1.0 300 OK\n\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_BAD_REQUEST | HTTP_V10,
            0,
            "HTTP/1.0 400 OK\n\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_INTERNAL_SERVER_ERROR | HTTP_V10,
            0,
            "HTTP/1.0 500 OK\n\n"
        },
        // invalid status code
        {
            HTTP_ESTATUS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 10 OK\n\n"
        },
        {
            HTTP_ESTATUS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 600 OK\n\n"
        },
        {
            HTTP_ESTATUS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 700 OK\n\n"
        },
        {
            HTTP_ESTATUS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 800 OK\n\n"
        },
        {
            HTTP_ESTATUS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 900 OK\n\n"
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
    test_status();
    return 0;
}

#endif

