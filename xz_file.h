#ifndef _XZ_FILE_H_
#define _XZ_FILE_H_

#include <lzma.h>

typedef enum {
  XZ_FILE_MODE_NO_OPEN=0,
  XZ_FILE_MODE_CREARE_WRITE=1,
  XZ_FILE_MODE_APPEND_WRITE=2,
  XZ_FILE_MODE_READ=3,
  XZ_FILE_MODE_MAX=4,
} XZ_file_mode_open_en;

typedef struct {
  XZ_file_mode_open_en mode_open;
  lzma_stream * strm;
  int xz_file;
  char * buf_temp;
  unsigned int size_buf_temp;
  char * name_file;
  char * buf_seek;

  lzma_ret ret;
  uint64_t count_byte_write;
  uint64_t len_file_read;
  uint64_t count_byte_read;
  uint64_t count_byte_decompress;
  uint64_t len_uncompress;
} XZ_file_st;

#ifdef __cplusplus
extern "C" {
#endif

// Create and init struct XZ_file_st
XZ_file_st * xz_file_create ( unsigned int size_buf );
// Delete and null struct XZ_file_st
void xz_file_delete ( XZ_file_st ** fxz );

// Open file for xz, return 0 - OK, -1 - Error
int xz_file_open ( XZ_file_st * fxz, char * name_file, XZ_file_mode_open_en mode_open, int compression_levels );
void xz_file_close ( XZ_file_st * fxz );

// Write buf into file
int xz_file_write ( XZ_file_st * fxz, char * buf_write, int len_write );
// Write last buf into file
int xz_file_write_finish ( XZ_file_st * fxz );
// Read buf from file
int xz_file_read ( XZ_file_st * fxz, char * buf_read, int size_buf );
// seek through reading (works in any direction)
__int64 xz_file_seek ( XZ_file_st * fxz, __int64 offset, int origin );

// Get length uncompressed data from xz file
int xz_file_get_uncompressed_len ( char * name_file, uint64_t * len );

char * xz_file_str_error ( lzma_ret ret );

#ifdef __cplusplus
}
#endif

#endif
