
struct streambuffer_s
{
   unsigned char *start;
   int  used;
   int  malloced;
};

typedef struct streambuffer_s streambuffer_t;

void streaminit( streambuffer_t *buffer );
void streamadd( streambuffer_t *buffer, const unsigned char *data, int size );
void streamclean( streambuffer_t *buffer );
