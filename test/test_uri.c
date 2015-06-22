#include "test_http.h"


static void test_uri( void )
{
    test_req_t req[] = {
        // invalid uri length
        {
            HTTP_EURILEN,
            HTTP_MGET,
            HTTP_V11,
            0,
            "GET /foo/bar/baz/qux/quux\r\n"
        },
        // invalid uri character
        {
            HTTP_EBADURI,
            HTTP_MGET,
            HTTP_V11,
            0,
            "GET /invalid/\"uri\r\n"
        },
        {
            HTTP_EBADURI,
            HTTP_MGET,
            HTTP_V11,
            0,
            "GET /invalid/#uri\r\n"
        },
        {
            HTTP_EBADURI,
            HTTP_MGET,
            HTTP_V11,
            0,
            "GET /invalid/<uri\r\n"
        },
        {
            HTTP_EBADURI,
            HTTP_MGET,
            HTTP_V11,
            0,
            "GET /invalid/>uri\r\n"
        },
        {
            HTTP_EBADURI,
            HTTP_MGET,
            HTTP_V11,
            0,
            "GET /invalid/\\uri\r\n"
        },
        {
            HTTP_EBADURI,
            HTTP_MGET,
            HTTP_V11,
            0,
            "GET /invalid/^uri\r\n"
        },
        {
            HTTP_EBADURI,
            HTTP_MGET,
            HTTP_V11,
            0,
            "GET /invalid/`uri\r\n"
        },
        {
            HTTP_EBADURI,
            HTTP_MGET,
            HTTP_V11,
            0,
            "GET /invalid/{uri\r\n"
        },
        {
            HTTP_EBADURI,
            HTTP_MGET,
            HTTP_V11,
            0,
            "GET /invalid/|uri\r\n"
        },
        {
            HTTP_EBADURI,
            HTTP_MGET,
            HTTP_V11,
            0,
            "GET /invalid/}uri\r\n"
        },
        
        // end of request
        { 0, 0, 0, 0, "" }
    };
    test_req_t *ptr = req;
    http_t *r = http_alloc(0);
    int rc;
    
    while( ptr->code )
    {
        rc = http_req_parse( r, ptr->entity, strlen(ptr->entity), 15, INT16_MAX );
        assert( rc == ptr->rc );
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
    test_uri();
    return 0;
}

#endif


