#ifndef QTC_H
#define QTC_H

extern int compress( struct image *input, struct image *refimage, struct qti *output, int maxerror, int minsize, int maxdepth, int lazyness );
extern int decompress( struct qti *input, struct image *refimage, struct image *output );
extern int decompress_ccode( struct qti *input, struct image *output, int refimage );

#endif
