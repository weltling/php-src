#ifdef _WIN32
# include "config.w32.h"
#else
# include "php_config.h"
#endif

#ifdef HAVE_GD_PNG
/* needs to be first */
# include <png.h>
#endif

#ifdef HAVE_GD_JPG
# include <jpeglib.h>
#endif

#include "php.h"
#include "gd.h"
#include "gd_compat.h"

#if GD_MAJOR_VERSION > 2 || GD_MAJOR_VERSION == 2 && GD_MINOR_VERSION >= 2

#ifdef HAVE_GD_JPG
int gdJpegGetVersionInt()
{
	return JPEG_LIB_VERSION;
}

const char * gdJpegGetVersionString()
{
	switch(JPEG_LIB_VERSION) {
		case 62:
			return "6b";
			break;

		case 70:
			return "7 compatible";
			break;

		case 80:
			return "8 compatible";
			break;

		case 90:
			return "9 compatible";
			break;

		default:
			return "unknown";
	}
}
#endif

#ifdef HAVE_GD_PNG
const char * gdPngGetVersionString()
{
	return PNG_LIBPNG_VER_STRING;
}
#endif

#endif /* GD-2.2+*/

#if GD_MAJOR_VERSION < 2 || GD_MAJOR_VERSION == 2 && GD_MINOR_VERSION < 2
int overflow2(int a, int b)
{

	if(a <= 0 || b <= 0) {
		php_error_docref(NULL, E_WARNING, "gd warning: one parameter to a memory allocation multiplication is negative or zero, failing operation gracefully\n");
		return 1;
	}
	if(a > INT_MAX / b) {
		php_error_docref(NULL, E_WARNING, "gd warning: product of memory allocation multiplication would exceed INT_MAX, failing operation gracefully\n");
		return 1;
	}
	return 0;
}
#endif

