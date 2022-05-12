#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#include "xz_file.h"

#ifdef _MSC_VER
#define Lseek64(x,y,z) _lseeki64((x),(y),(z))
#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif
#pragma warning(disable:4996)
#else
#define Lseek64(x,y,z) lseeki64((x),(y),(z))
#endif


char * xz_file_str_error ( lzma_ret ret )
{

  switch (ret) {
  case LZMA_OK:
    return "Operation completed successfully";
  case LZMA_STREAM_END :
    return "End of stream was reached";
  case LZMA_NO_CHECK:
    return "Input stream has no integrity check";
  case LZMA_UNSUPPORTED_CHECK:
    return "Specified integrity check is not supported";
  case LZMA_GET_CHECK:
    return "Integrity check type is now available";
  case LZMA_MEM_ERROR:
    return "Memory allocation failed";
  case LZMA_MEMLIMIT_ERROR:
    return "File format not recognized";
  case LZMA_FORMAT_ERROR:
    return "Memory usage limit was reached";
  case LZMA_OPTIONS_ERROR:
    return "Specified preset is not supported";
  case LZMA_DATA_ERROR:
    return "File size limits exceeded";
  case LZMA_BUF_ERROR:
    return "No progress is possible";
  case LZMA_PROG_ERROR:
    return "Programming error";
  }
  
  return "Unknown error, possibly a bug";
}

void xz_init_stream ( XZ_file_st * fxz )
{
  if ( fxz==0 )
    return;
  fxz->strm->next_in = NULL;
  fxz->strm->avail_in = 0;
  fxz->strm->next_out = fxz->buf_temp;
  fxz->strm->avail_out = fxz->size_buf_temp;
}

// Create and init struct XZ_file_st
XZ_file_st * xz_file_create ( unsigned int size_buf )
{
  XZ_file_st * fxz;

  fxz=(XZ_file_st *)malloc(sizeof(XZ_file_st));
  if ( fxz==NULL )
    return NULL;
  memset(fxz,0,sizeof(XZ_file_st));

  fxz->buf_temp=(char *)malloc(size_buf);
  if ( fxz->buf_temp==NULL )
    goto _M_Err;
  memset(fxz->buf_temp,0,size_buf);

  fxz->strm=(lzma_stream*)malloc(sizeof(lzma_stream));
  if ( fxz->strm==NULL )
    goto _M_Err;
  memset(fxz->strm,0,sizeof(lzma_stream));

  fxz->size_buf_temp=size_buf;
  fxz->xz_file=-1;

  return fxz;

_M_Err:
  if ( fxz->strm!=0 )
    free(fxz->strm);
  if ( fxz->buf_temp )
    free(fxz->buf_temp);
  if ( fxz )
    free(fxz);
  return NULL;
}

// Delete and null struct XZ_file_st
void xz_file_delete ( XZ_file_st ** fxz )
{
  if ( *fxz==0 )
    return;
  if ( (*fxz)->strm!=0 )
    free((*fxz)->strm);
  if ( (*fxz)->buf_temp )
    free((*fxz)->buf_temp);
  if ( (*fxz) )
    free((*fxz));
  *fxz=NULL;
}

// Open file for xz, return 0 - OK, -1 - Error
int xz_file_open ( XZ_file_st * fxz, char * name_file, XZ_file_mode_open_en mode_open, int compression_levels )
{
  lzma_ret ret;

  if ( name_file==NULL || name_file[0]==0 || fxz==NULL )
    return 0;

  fxz->xz_file=-1;
  fxz->count_byte_decompress=0;
  fxz->count_byte_read=0;
  fxz->count_byte_write=0;
  fxz->len_file_read=0;
  fxz->mode_open=XZ_FILE_MODE_NO_OPEN;
  if ( fxz->name_file!=0 )
    free(fxz->name_file);
  fxz->name_file = strdup(name_file);
  fxz->len_uncompress=0;
  fxz->ret=LZMA_OK;

  switch ( mode_open ) {

  case XZ_FILE_MODE_CREARE_WRITE:
    ret = lzma_easy_encoder(fxz->strm, compression_levels, LZMA_CHECK_CRC64);
    if ( ret != LZMA_OK)
      return -1;
    fxz->xz_file=open(name_file,O_WRONLY|O_BINARY|O_APPEND|O_CREAT|O_TRUNC,S_IWRITE);
    break;

  case XZ_FILE_MODE_APPEND_WRITE:
    ret = lzma_easy_encoder(fxz->strm, compression_levels, LZMA_CHECK_CRC64);
    if ( ret != LZMA_OK)
      return -1;
    fxz->xz_file=open(name_file,O_WRONLY|O_BINARY|O_APPEND,S_IWRITE);
    if ( fxz->xz_file!=-1 )
      Lseek64(fxz->xz_file,0,SEEK_END);
    break;

  case XZ_FILE_MODE_READ:
    ret = lzma_stream_decoder(fxz->strm, UINT64_MAX, LZMA_CONCATENATED);
    if (ret != LZMA_OK)
      return -1;
    fxz->xz_file=open(name_file,O_RDONLY|O_BINARY,S_IREAD);
    if ( fxz->xz_file!=-1 ) {
      fxz->len_file_read=Lseek64(fxz->xz_file,0,SEEK_END);
      Lseek64(fxz->xz_file,0,SEEK_SET);
    }
    fxz->count_byte_read=0;
    fxz->count_byte_decompress=0;
    break;

  default:
    return -1;

  }

  if ( fxz->xz_file==-1 ) {
    lzma_end(fxz->strm);
    return -1;
  } else {
    xz_init_stream(fxz);
    fxz->mode_open=mode_open;
    return 0;
  }
}

void xz_file_close ( XZ_file_st * fxz )
{
  if ( fxz==0 || fxz->mode_open==0 )
    return;
  if ( fxz->name_file!=0 )
    free(fxz->name_file);
  fxz->name_file=0;
  if ( fxz->buf_seek!=0 )
    free(fxz->buf_seek);
  fxz->buf_seek=0;
  if ( fxz->xz_file!=-1 )
    close(fxz->xz_file);
  fxz->xz_file=-1;
  if ( fxz->strm!=0 )
    lzma_end(fxz->strm);
}

int xz_file_write ( XZ_file_st * fxz, char * buf_write, int len_write )
{
  if ( fxz==NULL || buf_write==NULL || (fxz->mode_open!=XZ_FILE_MODE_CREARE_WRITE && fxz->mode_open!=XZ_FILE_MODE_APPEND_WRITE) || fxz->xz_file==-1 )
    return -1;

  lzma_action action = LZMA_RUN;
  lzma_ret ret;
  size_t write_size;

  while ( 1 ) {
    
    if ( fxz->strm->avail_in==0 ) {
      fxz->strm->next_in = buf_write;
      fxz->strm->avail_in = len_write;
    }

    ret = lzma_code(fxz->strm, action);

    if ( fxz->strm->avail_out == 0 || ret == LZMA_STREAM_END ) {
      write_size = fxz->size_buf_temp - fxz->strm->avail_out;
      if ( write(fxz->xz_file, fxz->buf_temp, write_size )!= write_size )
        return -1;
      fxz->strm->next_out = fxz->buf_temp;
      fxz->strm->avail_out = fxz->size_buf_temp;
      fxz->count_byte_write+=write_size;
      //printf("xz_write_end_file: write_size=%d\n",fxz->count_byte_write);
    }

    if ( ret != LZMA_OK ) {
      if ( ret == LZMA_STREAM_END ) {
        xz_init_stream(fxz);
        return 0;
      }
      fxz->ret=ret;
      return -1;
    }

    if ( fxz->strm->avail_in==0 )
      return 0;

  } //  while 1

}

int xz_file_write_finish ( XZ_file_st * fxz )
{
  if ( fxz==NULL || (fxz->mode_open!=XZ_FILE_MODE_CREARE_WRITE && fxz->mode_open!=XZ_FILE_MODE_APPEND_WRITE) || fxz->xz_file==-1 )
    return -1;

  lzma_action action = LZMA_RUN;
  lzma_ret ret;
  size_t write_size;

  while ( 1 ) {
    
    if ( fxz->strm->avail_in==0 )
      action = LZMA_FINISH;

    ret = lzma_code(fxz->strm, action);

    if ( fxz->strm->avail_out == 0 || ret == LZMA_STREAM_END ) {
      write_size = fxz->size_buf_temp - fxz->strm->avail_out;
      if ( write_size )
      if ( write(fxz->xz_file, fxz->buf_temp, write_size )!= write_size )
        return -1;
      fxz->strm->next_out = fxz->buf_temp;
      fxz->strm->avail_out = fxz->size_buf_temp;
      fxz->count_byte_write+=write_size;
      //printf("xz_write_end_file: write_size=%d\n",fxz->count_byte_write);
    }

    if ( ret != LZMA_OK ) {
      if ( ret == LZMA_STREAM_END ) {
        xz_init_stream(fxz);
        return 0;
      }
      fxz->ret=ret;
      return -1;
    }

  } //  while 1

}


int xz_file_read ( XZ_file_st * fxz, char * buf_read, int size_buf )
{
  if ( fxz==NULL || buf_read==NULL || fxz->mode_open!=XZ_FILE_MODE_READ || fxz->xz_file==-1 )
    return -1;

  lzma_action action = LZMA_RUN;
  lzma_ret ret;
  int rc, size_decompress;

  if ( fxz->ret == LZMA_STREAM_END )
    return 0;

  if ( Lseek64(fxz->xz_file,0,SEEK_CUR)==0 ) {
    fxz->strm->next_in = NULL;
    fxz->strm->avail_in = 0;
  }

  fxz->strm->next_out = buf_read;
  fxz->strm->avail_out = size_buf;

  if ( fxz->len_file_read == Lseek64(fxz->xz_file,0,SEEK_CUR) )
    action = LZMA_FINISH;

  while (1) {

    if ( fxz->strm->avail_in == 0 && fxz->len_file_read != Lseek64(fxz->xz_file,0,SEEK_CUR) ) {

      rc = read(fxz->xz_file, fxz->buf_temp, fxz->size_buf_temp);
      fxz->strm->next_in = fxz->buf_temp;
      fxz->strm->avail_in = rc;

      if ( rc==-1 ) 
        return -1;
      
      //printf("xz_file_read: count_byte_read=%d\n",fxz->count_byte_read);

      fxz->count_byte_read += fxz->strm->avail_in;

      if ( fxz->len_file_read == Lseek64(fxz->xz_file,0,SEEK_CUR) )
        action = LZMA_FINISH;
    }

    fxz->ret = ret = lzma_code(fxz->strm, action);

    if ( fxz->strm->avail_out == 0 || ret == LZMA_STREAM_END ) {

      size_decompress = size_buf - fxz->strm->avail_out;
      fxz->count_byte_decompress += size_decompress;
      //printf("xz_file_read: count_byte_decompress=%d\n",size_decompress);

      return size_decompress;
    }

    if ( ret != LZMA_OK ) {
      if ( ret == LZMA_STREAM_END )
        return 0;
      return -1;
    }

  } //  while (1)

}

// seek through reading (works in any direction)
__int64 xz_file_seek ( XZ_file_st * fxz, __int64 offset, int origin )
{
  
  if ( fxz==NULL || fxz->mode_open!=XZ_FILE_MODE_READ || fxz->xz_file==-1 )
    return -1;

  int rc, size_read;

  if ( fxz->len_uncompress==0 ) {
    rc=xz_file_get_uncompressed_len(fxz->name_file,&fxz->len_uncompress);
    if ( rc==-1 || fxz->len_uncompress==0 )
      return -1;
  }
  if ( fxz->buf_seek==0 ) {
    fxz->buf_seek=malloc(fxz->size_buf_temp);
    if ( fxz->buf_seek==0 )
      return -1;
  }

  uint64_t new_offset, len_seek;

  switch ( origin ) {
  case SEEK_SET:
    new_offset = offset;
    break;
  case SEEK_END:
    new_offset = fxz->len_uncompress + offset;
    break;
  case SEEK_CUR:
    new_offset = fxz->count_byte_decompress + offset;
    break;
  default:
    return -1;
  }

   if ( new_offset>fxz->len_uncompress )
     new_offset=fxz->len_uncompress;
   if ( new_offset<0 )
     new_offset=0;

  if ( new_offset==fxz->count_byte_decompress )
    return (__int64)fxz->count_byte_decompress;

  if ( new_offset<fxz->count_byte_decompress ) {
    if ( fxz->strm!=0 )
      lzma_end(fxz->strm);
    lzma_ret ret;
    ret = lzma_stream_decoder(fxz->strm, UINT64_MAX, LZMA_CONCATENATED);
    if (ret != LZMA_OK)
      return -1;
    xz_init_stream(fxz);
    Lseek64(fxz->xz_file,0,SEEK_SET);
    fxz->count_byte_read=0;
    fxz->count_byte_decompress=0;
    fxz->ret=LZMA_OK;
    if ( new_offset==0 )
      return 0;
  }

  len_seek = new_offset - fxz->count_byte_decompress;

  while ( 1 ) {
    if ( len_seek>fxz->size_buf_temp )
      size_read = fxz->size_buf_temp;
    else
      size_read = (int)len_seek;
    rc = xz_file_read(fxz,fxz->buf_seek,size_read);
    if ( rc==-1 )
      return -1;
    if ( rc==0 ) {
      if ( fxz->count_byte_decompress!=new_offset )
        return -1;
      return (__int64)fxz->count_byte_decompress;
    }
    if ( len_seek<rc )
      return -1;
    len_seek -= rc;
    if ( len_seek==0 )
      return (__int64)fxz->count_byte_decompress;
  }

}

#pragma pack(1)

/// Information about a .xz file
typedef struct {
  /// Combined Index of all Streams in the file
  lzma_index *idx;

  /// Total amount of Stream Padding
  uint64_t stream_padding;

  /// Highest memory usage so far
  uint64_t memusage_max;

  /// True if all Blocks so far have Compressed Size and
  /// Uncompressed Size fields
  char all_have_sizes;

  /// Oldest XZ Utils version that will decompress the file
  uint32_t min_version;

} xz_file_info;

typedef struct index_tree_node_s index_tree_node;
struct index_tree_node_s {
  /// Uncompressed start offset of this Stream (relative to the
  /// beginning of the file) or Block (relative to the beginning
  /// of the Stream)
  lzma_vli uncompressed_base;

  /// Compressed start offset of this Stream or Block
  lzma_vli compressed_base;

  index_tree_node *parent;
  index_tree_node *left;
  index_tree_node *right;
};


/// \brief      AVL tree to hold index_stream or index_group structures
typedef struct {
  /// Root node
  index_tree_node *root;

  /// Leftmost node. Since the tree will be filled sequentially,
  /// this won't change after the first node has been added to
  /// the tree.
  index_tree_node *leftmost;

  /// The rightmost node in the tree. Since the tree is filled
  /// sequentially, this is always the node where to add the new data.
  index_tree_node *rightmost;

  /// Number of nodes in the tree
  uint32_t count;

} index_tree;

struct lzma_index_s {
  /// AVL-tree containing the Stream(s). Often there is just one
  /// Stream, but using a tree keeps lookups fast even when there
  /// are many concatenated Streams.
  index_tree streams;

  /// Uncompressed size of all the Blocks in the Stream(s)
  lzma_vli uncompressed_size;

  /// Total size of all the Blocks in the Stream(s)
  lzma_vli total_size;

  /// Total number of Records in all Streams in this lzma_index
  lzma_vli record_count;

  /// Size of the List of Records field if all the Streams in this
  /// lzma_index were packed into a single Stream (makes it simpler to
  /// take many .xz files and combine them into a single Stream).
  ///
  /// This value together with record_count is needed to calculate
  /// Backward Size that is stored into Stream Footer.
  lzma_vli index_list_size;

  /// How many Records to allocate at once in lzma_index_append().
  /// This defaults to INDEX_GROUP_SIZE but can be overridden with
  /// lzma_index_prealloc().
  size_t prealloc;

  /// Bitmask indicating what integrity check types have been used
  /// as set by lzma_index_stream_flags(). The bit of the last Stream
  /// is not included here, since it is possible to change it by
  /// calling lzma_index_stream_flags() again.
  uint32_t checks;
};

#define IO_BUFFER_SIZE 8192

typedef union {
  uint8_t u8[IO_BUFFER_SIZE];
  uint32_t u32[IO_BUFFER_SIZE / sizeof(uint32_t)];
  uint64_t u64[IO_BUFFER_SIZE / sizeof(uint64_t)];
} io_buf;

enum operation_mode {
  MODE_COMPRESS,
  MODE_DECOMPRESS,
  MODE_TEST,
  MODE_LIST,
};

#define my_min(x, y) ((x) < (y) ? (x) : (y))

#define XZ_FILE_INFO_INIT { NULL, 0, 0, 1, 50000002 }
#pragma pack(1)


int io_pread(int src_fd, io_buf *buf, size_t size, long long pos)
{
  // Using lseek() and read() is more portable than pread() and
  // for us it is as good as real pread().
  if (Lseek64(src_fd, pos, SEEK_SET) != pos) {
    return -1;
  }

  const size_t amount = read(src_fd, buf, size);
  if (amount == SIZE_MAX)
    return -1;

  if (amount != size) {
    return -1;
  }

  return 0;
}

int xz_get_info_file ( char * name_file, xz_file_info * xfi )
{
  if ( name_file==0 || name_file[0]==0 || xfi==0 )
    return -1;

  int h_xz=open ( name_file, O_RDONLY|O_BINARY );
  if ( h_xz==-1 )
    return -1;

  long long len_file=Lseek64(h_xz,0,SEEK_END);
  if ( len_file < 2 * LZMA_STREAM_HEADER_SIZE ) {
    close(h_xz);
    return -1;
  }

  io_buf buf;
  lzma_stream_flags header_flags;
  lzma_stream_flags footer_flags;
  lzma_ret ret;

  // lzma_stream for the Index decoder
  lzma_stream strm = LZMA_STREAM_INIT;

  // All Indexes decoded so far
  lzma_index *combined_index = NULL;

  // The Index currently being decoded
  lzma_index *this_index = NULL;

  // Current position in the file. We parse the file backwards so
  // initialize it to point to the end of the file.
  long long pos = len_file;

  // Each loop iteration decodes one Index.
  do {
    // Check that there is enough data left to contain at least
    // the Stream Header and Stream Footer. This check cannot
    // fail in the first pass of this loop.
    if (pos < 2 * LZMA_STREAM_HEADER_SIZE) {
      goto error;
    }

    pos -= LZMA_STREAM_HEADER_SIZE;
    lzma_vli stream_padding = 0;

    // Locate the Stream Footer. There may be Stream Padding which
    // we must skip when reading backwards.
    while (1) {
      if (pos < LZMA_STREAM_HEADER_SIZE) {
        goto error;
      }

      if (io_pread(h_xz, &buf,
          LZMA_STREAM_HEADER_SIZE, pos))
        goto error;

      // Stream Padding is always a multiple of four bytes.
      int i = 2;
      if (buf.u32[i] != 0)
        break;

      int j=0;
      while ( 1 ) { // åñëè âñòðåòèëèñü îäíè íóëè â ôàéëå
        if ( j>=LZMA_STREAM_HEADER_SIZE/4 )
          goto error;
        if ( buf.u32[j]!=0 )
          break;
        j++;
      }

      // To avoid calling io_pread() for every four bytes
      // of Stream Padding, take advantage that we read
      // 12 bytes (LZMA_STREAM_HEADER_SIZE) already and
      // check them too before calling io_pread() again.
      do {
        stream_padding += 4;
        pos -= 4;
        --i;
      } while (i >= 0 && buf.u32[i] == 0);
    }

    // Decode the Stream Footer.
    ret = lzma_stream_footer_decode(&footer_flags, buf.u8);
    if (ret != LZMA_OK) {
      goto error;
    }

    // Check that the Stream Footer doesn't specify something
    // that we don't support. This can only happen if the xz
    // version is older than liblzma and liblzma supports
    // something new.
    //
    // It is enough to check Stream Footer. Stream Header must
    // match when it is compared against Stream Footer with
    // lzma_stream_flags_compare().
    if (footer_flags.version != 0) {
      goto error;
    }

    // Check that the size of the Index field looks sane.
    lzma_vli index_size = footer_flags.backward_size;
    if ((lzma_vli)(pos) < index_size + LZMA_STREAM_HEADER_SIZE) {
      goto error;
    }

    // Set pos to the beginning of the Index.
    pos -= index_size;

    // See how much memory we can use for decoding this Index.
    uint64_t memlimit = UINT64_MAX;
    uint64_t memused = 0;
    if (combined_index != NULL) {
      memused = lzma_index_memused(combined_index);
      if (memused > memlimit)
        goto error;

      memlimit -= memused;
    }

    // Decode the Index.
    ret = lzma_index_decoder(&strm, &this_index, memlimit);
    if (ret != LZMA_OK) {
      goto error;
    }

    do {
      // Don't give the decoder more input than the
      // Index size.
      strm.avail_in = (size_t)my_min(IO_BUFFER_SIZE, index_size);
      if (io_pread(h_xz, &buf, strm.avail_in, pos))
        goto error;

      pos += strm.avail_in;
      index_size -= strm.avail_in;

      strm.next_in = buf.u8;
      ret = lzma_code(&strm, LZMA_RUN);

    } while (ret == LZMA_OK);

    // If the decoding seems to be successful, check also that
    // the Index decoder consumed as much input as indicated
    // by the Backward Size field.
    if (ret == LZMA_STREAM_END)
      if (index_size != 0 || strm.avail_in != 0)
        ret = LZMA_DATA_ERROR;

    if (ret != LZMA_STREAM_END) {
      // LZMA_BUFFER_ERROR means that the Index decoder
      // would have liked more input than what the Index
      // size should be according to Stream Footer.
      // The message for LZMA_DATA_ERROR makes more
      // sense in that case.
      if (ret == LZMA_BUF_ERROR)
        ret = LZMA_DATA_ERROR;

      // If the error was too low memory usage limit,
      // show also how much memory would have been needed.
      if (ret == LZMA_MEMLIMIT_ERROR) {
        uint64_t needed = lzma_memusage(&strm);
        if (UINT64_MAX - needed < memused)
          needed = UINT64_MAX;
        else
          needed += memused;

      }

      goto error;
    }

    // Decode the Stream Header and check that its Stream Flags
    // match the Stream Footer.
    pos -= footer_flags.backward_size + LZMA_STREAM_HEADER_SIZE;
    if ((lzma_vli)(pos) < lzma_index_total_size(this_index)) {
      goto error;
    }

    pos -= lzma_index_total_size(this_index);
    if (io_pread(h_xz, &buf, LZMA_STREAM_HEADER_SIZE, pos))
      goto error;

    ret = lzma_stream_header_decode(&header_flags, buf.u8);
    if (ret != LZMA_OK) {
      goto error;
    }

    ret = lzma_stream_flags_compare(&header_flags, &footer_flags);
    if (ret != LZMA_OK) {
      goto error;
    }

    // Store the decoded Stream Flags into this_index. This is
    // needed so that we can print which Check is used in each
    // Stream.
    ret = lzma_index_stream_flags(this_index, &footer_flags);
    if (ret != LZMA_OK)
      goto error;

    // Store also the size of the Stream Padding field. It is
    // needed to show the offsets of the Streams correctly.
    ret = lzma_index_stream_padding(this_index, stream_padding);
    if (ret != LZMA_OK)
      goto error;

    if (combined_index != NULL) {
      // Append the earlier decoded Indexes
      // after this_index.
      ret = lzma_index_cat(
          this_index, combined_index, NULL);
      if (ret != LZMA_OK) {
        goto error;
      }
    }

    combined_index = this_index;
    this_index = NULL;

    xfi->stream_padding += stream_padding;

  } while (pos > 0);

  lzma_end(&strm);

  // All OK. Make combined_index available to the caller.
  xfi->idx = combined_index;
  close(h_xz);
  return 0;

error:
  // Something went wrong, free the allocated memory.
  lzma_end(&strm);
  lzma_index_end(combined_index, NULL);
  lzma_index_end(this_index, NULL);
  close(h_xz);
  return -1;

}

int xz_file_get_uncompressed_len ( char * name_file, uint64_t * len )
{
  xz_file_info xfi = XZ_FILE_INFO_INIT;

  int rc=xz_get_info_file(name_file,&xfi);
  if ( rc==-1 ) {
    *len=0;
    return rc;
  }
  *len=xfi.idx->uncompressed_size;
  return 0;
}
