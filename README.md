# XZ_FIle
An example of using the XZ library to create and read archives (just like in zlib).

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

// Get length uncompressed data from xz file  
int xz_file_get_uncompressed_len ( char * name_file, uint64_t * len );  

char * xz_file_str_error ( lzma_ret ret );  
