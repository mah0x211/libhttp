#include "test_http.h"

static void test_method( void )
{
    test_req_t req[] = {
        // HTTP/0.9
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V09,
            0,
            "GET /foo/bar/baz\r\n"
        },
        // HTTP/0.9 invalid methods
        {
            HTTP_EMETHOD,
            HTTP_MHEAD | HTTP_V09,
            0,
            "HEAD /foo/bar/baz\r\n"
        },
        {
            HTTP_EMETHOD,
            HTTP_MPOST | HTTP_V09,
            0,
            "POST /foo/bar/baz\r\n"
        },
        {
            HTTP_EMETHOD,
            HTTP_MPUT | HTTP_V09,
            0,
            "PUT /foo/bar/baz\r\n"
        },
        {
            HTTP_EMETHOD,
            HTTP_MDELETE | HTTP_V09,
            0,
            "DELETE /foo/bar/baz\r\n"
        },
        {
            HTTP_EMETHOD,
            HTTP_MOPTIONS | HTTP_V09,
            0,
            "OPTIONS /foo/bar/baz\r\n"
        },
        {
            HTTP_EMETHOD,
            HTTP_MTRACE | HTTP_V09,
            0,
            "TRACE /foo/bar/baz\r\n"
        },
        {
            HTTP_EMETHOD,
            HTTP_MCONNECT | HTTP_V09,
            0,
            "CONNECT /foo/bar/baz\r\n"
        },

        // HTTP/1.0
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V10,
            0,
            "GET /foo/bar/baz HTTP/1.0\r\n\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MHEAD | HTTP_V10,
            0,
            "HEAD /foo/bar/baz HTTP/1.0\r\n\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MPOST | HTTP_V10,
            0,
            "POST /foo/bar/baz HTTP/1.0\r\n\r\n"
        },
        // HTTP/1.0 invalid methods
        {
            HTTP_EMETHOD,
            HTTP_MPUT | HTTP_V10,
            0,
            "PUT /foo/bar/baz HTTP/1.0\r\n\r\n"
        },
        {
            HTTP_EMETHOD,
            HTTP_MDELETE | HTTP_V10,
            0,
            "DELETE /foo/bar/baz HTTP/1.0\r\n\r\n"
        },
        {
            HTTP_EMETHOD,
            HTTP_MOPTIONS | HTTP_V10,
            0,
            "OPTIONS /foo/bar/baz HTTP/1.0\r\n\r\n"
        },
        {
            HTTP_EMETHOD,
            HTTP_MTRACE | HTTP_V10,
            0,
            "TRACE /foo/bar/baz HTTP/1.0\r\n\r\n"
        },
        {
            HTTP_EMETHOD,
            HTTP_MCONNECT | HTTP_V10,
            0,
            "CONNECT /foo/bar/baz HTTP/1.0\r\n\r\n"
        },
        
        // HTTP/1.1
        {
            HTTP_SUCCESS,
            HTTP_MGET | HTTP_V11,
            0,
            "GET /foo/bar/baz HTTP/1.1\r\n\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MHEAD | HTTP_V11,
            0,
            "HEAD /foo/bar/baz HTTP/1.1\r\n\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MPOST | HTTP_V11,
            0,
            "POST /foo/bar/baz HTTP/1.1\r\n\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MPUT | HTTP_V11,
            0,
            "PUT /foo/bar/baz HTTP/1.1\r\n\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MDELETE | HTTP_V11,
            0,
            "DELETE /foo/bar/baz HTTP/1.1\r\n\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MOPTIONS | HTTP_V11,
            0,
            "OPTIONS /foo/bar/baz HTTP/1.1\r\n\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MTRACE | HTTP_V11,
            0,
            "TRACE /foo/bar/baz HTTP/1.1\r\n\r\n"
        },
        {
            HTTP_SUCCESS,
            HTTP_MCONNECT | HTTP_V11,
            0,
            "CONNECT /foo/bar/baz HTTP/1.1\r\n\r\n"
        },

        // unknown method
        {
            HTTP_EMETHOD,
            HTTP_MGET | HTTP_V11,
            0,
            "HELO /foo/bar/baz HTTP/1.1\r\n\r\n"
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
    test_method();
    return 0;
}

#endif

