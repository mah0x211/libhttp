#include "test_http.h"

static void test_version( void )
{
    test_req_t req[] = {
        // HTTP/0.9
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V09,
            0,
            "GET /foo/bar/baz\r\n"
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
        // end of request
        { 0, 0, 0, NULL }
    };
    test_req_t *ptr = req;
    http_t *r = http_alloc(0);
    int rc;
    
    while( ptr->entity )
    {
        rc = http_req_parse( r, ptr->entity, strlen( ptr->entity ), 
                             INT16_MAX, INT16_MAX );
        assert( rc == ptr->rc );
        if( rc == HTTP_SUCCESS ){
            assert( r->protocol == ptr->request );
        }
        ptr++;
        http_reset( r );
    }
    http_free( r );
}

#ifdef TESTS

int main(int argc, const char * argv[])
{
    test_version();
    return 0;
}

#endif

