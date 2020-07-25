
#include "streambuffer.h"

#include <stdlib.h>
#include <string.h>

#define CHUNKSIZE (1024*256)


void streaminit( streambuffer_t *buffer )
{
   memset( buffer, 0, sizeof( streambuffer_t ) );
}


void streamadd( streambuffer_t *buffer, const unsigned char *data, int size )
{
   if( buffer->used + size > buffer->malloced )
   {
      buffer->malloced += CHUNKSIZE;
      buffer->start = (unsigned char*)realloc( buffer->start, buffer->malloced );
   }

   memcpy( buffer->start + buffer->used, data, size );
   buffer->used += size;
}


void streamclean( streambuffer_t *buffer )
{
   free( buffer->start );
   streaminit( buffer );
}
