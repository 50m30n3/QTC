#ifndef QTC_H
#define QTC_H

extern int qtc_compress( struct image *input, struct image *refimage, struct qti *output, int minsize, int maxdepth, int lazyness, int bgra, int colordiff );
extern int qtc_decompress( struct qti *input, struct image *refimage, struct image *output );
extern int qtc_decompress_ccode( struct qti *input, struct image *output, int refimage );

extern int qtc_compress_color_diff( struct image *input, struct image *refimage, struct qti *output, int minsize, int maxdepth, int lazyness );
extern int qtc_decompress_color_diff( struct qti *input, struct image *refimage, struct image *output );

#endif
