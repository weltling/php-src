#ifndef GD_COMPAT_H
#define GD_COMPAT_H 1

#if !defined(HAVE_GD_BUNDLED) || !(GD_MAJOR_VERSION < 2 || GD_MAJOR_VERSION == 2 && GD_MINOR_VERSION < 2)
/* from gd_compat.c */
const char * gdPngGetVersionString();
const char * gdJpegGetVersionString();
int gdJpegGetVersionInt();
#endif

/* from gd_compat.c of libgd/gd_security.c */
int overflow2(int a, int b);

#endif /* GD_COMPAT_H */
