/*
 *  Copyright 2013 Masatoshi Teruya. All rights reserved.
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
 *  strchr_brk.h
 *  Created by Masatoshi Teruya on 13/07/015.
 *
 */

#ifndef STRCHR_BRK_H
#define STRCHR_BRK_H

#include <stddef.h>
#include <errno.h>

static inline char *strchr_brk( char *s, size_t len, char c, 
                                const unsigned char accept256[] )
{
    unsigned char *p = (unsigned char*)s;
    size_t i = 0;
    
    errno = 0;
    for(; i < len; i++ )
    {
        if( p[i] == (unsigned char)c ){
            return (void*)(p + i);
        }
        // illegal character
        else if( !accept256[p[i]] ){
            errno = EILSEQ;
            return (void*)(p + i);
        }
    }
    
    return NULL;
}


static inline char *strchr_brkrep( char *s, size_t len, char c, 
                                   const unsigned char accept256[] )
{
    unsigned char *p = (unsigned char*)s;
    size_t i = 0;
    
    errno = 0;
    for(; i < len; i++ )
    {
        if( p[i] == (unsigned char)c ){
            return (void*)(p + i);
        }
        // illegal character
        else if( !accept256[p[i]] ){
            errno = EILSEQ;
            return (void*)(p + i);
        }
        p[i] = accept256[p[i]];
    }
    
    return NULL;
}


#endif

