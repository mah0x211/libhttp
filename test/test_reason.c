#include "test_http.h"

void test_reason( void )
{
    test_req_t req[] = {
        //
        // line-terminated by CRLF
        //
        // HTTP/1.0
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 200 OK\r\n\r\n"
        },
        // invalid status code
        {
            HTTP_EREASON,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 200 OK\r\t"
        },

        //
        // line-terminated by LF
        //
        // HTTP/1.0
        {
            HTTP_SUCCESS,
            HTTP_OK | HTTP_V10,
            0,
            "HTTP/1.0 200 OK\n\n"
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
    test_reason();
    return 0;
}

#endif

