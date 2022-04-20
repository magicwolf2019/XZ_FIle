#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <time.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <conio.h>

#include "xz_file.h"

#ifdef _MSC_VER
#define Lseek64(x,y,z) _lseeki64((x),(y),(z))
#pragma warning(disable:4996)
#else
#define Lseek64(x,y,z) lseeki64((x),(y),(z))
#endif


uint64_t Filelength ( int h )
{
  uint64_t len_file, off;

  if ( h==-1 )
    return 0;
  off=Lseek64(h,0,SEEK_CUR);
  len_file=Lseek64(h,0,SEEK_END);
  Lseek64(h,off,SEEK_SET);
  return len_file;
}


void print_progress ( uint64_t offset, uint64_t file_len, char * str_end )
{
  double progress=(double)offset;
  progress/=(double)file_len;
  progress*=100.;
  printf("%06.2f%%%s",progress,str_end);
}

#define SIZE_BUF 1024*1024

void test_hst ( void )
{
  XZ_file_st * fxz;
  int rc;
  char buf[512];

  for ( rc=0; rc<512; rc++ )
    buf[rc]='0'+rc%10;

  fxz=xz_file_create(SIZE_BUF);
  rc=xz_file_open(fxz,"i:\\xz\\test.hst.xz",XZ_FILE_MODE_CREARE_WRITE,0);
  rc=xz_file_write(fxz,buf,2);
  rc=xz_file_write(fxz,buf+2,2);
  rc=xz_file_write(fxz,buf+4,96);
  xz_file_write_finish(fxz);
  xz_file_close(fxz);
}

void main ( int argc, char *argv[] )
{
  size_t len_read;
  int i=0, rc, flag_cmp=0;
  uint64_t len, len_file_read, count_read=0, len_file_compress, len_decompress;
  char name_file_compress[_MAX_PATH]="test.xz", name_file_decompress[_MAX_PATH]="test_xz";
  XZ_file_st * fxz;
  int h_read, h_write;
  time_t t_start, t_last;
  char * buf_read, * buf_decompress;

  //test_hst();
  //return;

  if ( argc<2 ) {
    printf("Specify the file name to compress and (dir decompress)!\n");
    return;
  }

  fxz=xz_file_create(SIZE_BUF);
  if ( fxz==0 ) {
    printf("Error - create XZ File!\n");
    return;
  }
  buf_read=(char*)malloc(SIZE_BUF);
  if ( buf_read==0 ) {
    printf("Error - BUF Read!\n");
    return;
  }
  buf_decompress=(char*)malloc(SIZE_BUF);
  if ( buf_decompress==0 ) {
    printf("Error - BUF decompress!\n");
    return;
  }

  if ( argc>=3 ) {
    sprintf(name_file_compress,"%s\\test.xz",argv[2]);
    sprintf(name_file_decompress,"%s\\test_xz",argv[2]);
  }

  //goto M_Read;
  //goto M_Test;

  printf("Start compress %s -> %s\n",argv[1],name_file_compress);
  rc=xz_file_open(fxz,name_file_compress,XZ_FILE_MODE_CREARE_WRITE,0);
  if ( rc==-1 ) {
    printf("Error - ");
    if ( fxz->ret==LZMA_OK )
      printf("LZMA %s\n",xz_file_str_error(fxz->ret));
    else
      printf("open file.");
    return;
  }

  h_read=open(argv[1],O_RDONLY|O_BINARY,S_IREAD);
  if ( h_read==-1 ) {
    printf("Error open file - %s\n",argv[1]);
    return;
  }
  len_file_read=Filelength(h_read);
  
  // COMPRESS
  t_last=t_start=time(0);
  while ( 1 ) {
    len_read=read(h_read,buf_read,SIZE_BUF);
    if ( len_read==0 || len_read==-1 )
      break;
    rc=xz_file_write(fxz,buf_read,len_read);
    if ( rc==-1 ) {
      if ( fxz->ret!=LZMA_OK )
        printf("Error - LZMA %s\n",xz_file_str_error(fxz->ret));
      else
        printf("Error - write file\n");
      return;
    }
    if ( t_last!=time(0) ) {
      t_last=time(0);
      print_progress(Lseek64(h_read,0,SEEK_CUR),len_file_read,"; ");
    }
  }
  print_progress(Lseek64(h_read,0,SEEK_CUR),len_file_read,"\n");
  xz_file_write_finish(fxz);
  len_file_compress=Filelength(fxz->xz_file);
  xz_file_close(fxz);
  close(h_read);
  printf("End compress: time=%d, sec, sec ratio=", time(0)-t_start);
  print_progress(len_file_compress,len_file_read,"\n");

  //M_Read:
  xz_file_get_uncompressed_len(name_file_compress,&len);
  printf("\nUncompressed len=%I64d, len file read=%I64d - ",len,len_file_read);
  if ( len!=len_file_read ) {
    printf("Error.\n");
    return;
  } else
    printf("OK.\n");

  printf("\nStart decompress %s -> %s\n",name_file_compress,name_file_decompress);
  rc=xz_file_open(fxz,name_file_compress,XZ_FILE_MODE_READ,0);
  if ( rc==-1 ) {
    printf("Error: ");
    if ( fxz->ret==LZMA_OK )
      printf("LZMA - %s\n",xz_file_str_error(fxz->ret));
    else
      printf("open file.");
    return;
  }

  h_write=open(name_file_decompress,O_WRONLY|O_BINARY|O_APPEND|O_CREAT|O_TRUNC,S_IWRITE);
  if ( h_write==-1 ) {
    printf("Error - open file %s\n",name_file_decompress);
    return;
  }

  // DECOMPRESS
  t_last=t_start=time(0);
  while ( 1 ) {
    len_read=xz_file_read(fxz,buf_read,SIZE_BUF);
    if ( len_read==0 || len_read==-1 )
      break;
    write(h_write,buf_read,len_read);
    if ( t_last!=time(0) ) {
      t_last=time(0);
      print_progress(Lseek64(h_write,0,SEEK_CUR),len_file_read,"; ");
    }
  }
  print_progress(Lseek64(h_write,0,SEEK_CUR),len_file_read,"\n");
  xz_file_close(fxz);
  close(h_write);
  printf("End decompress: time=%d, sec\n", time(0)-t_start);

  printf("count byte decompress=%I64d, len file read=%I64d - ",fxz->count_byte_decompress,len_file_read);
  if ( fxz->count_byte_decompress!=len_file_read ) {
    printf("Error.\n");
    return;
  } else
    printf("OK.\n");
  
  //M_Test:
  printf("\nTest comparison file source %s and file decompress %s\n",argv[1],name_file_decompress);
  h_read=open(argv[1],O_RDONLY|O_BINARY,S_IREAD);
  if ( h_read==-1 ) {
    printf("Error - open file %s\n",argv[1]);
    return;
  }
  len_file_read=Filelength(h_read);
  h_write=open(name_file_decompress,O_RDONLY|O_BINARY,S_IREAD);
  if ( h_write==-1 ) {
    printf("Error - open file %s\n",name_file_decompress);
    return;
  }

  // TEST BINARY
  t_last=t_start=time(0);
  while ( 1 ) {
    len_read=read(h_read,buf_read,SIZE_BUF);
    if ( len_read==-1 )
      break;
    len_decompress=read(h_write,buf_decompress,SIZE_BUF);
    if ( len_decompress==0 || len_decompress==-1 )
      break;
    if ( len_read!=len_decompress )
      break;
    if ( memcmp(buf_read,buf_decompress,len_read)!=0 ) {
      flag_cmp=-1;
      printf("Error - file comparison\n");
      break;
    }
    if ( t_last!=time(0) ) {
      t_last=time(0);
      print_progress(Lseek64(h_read,0,SEEK_CUR),len_file_read,"; ");
    }
  }
  if ( len_read==-1 || len_decompress==-1 || len_read!=len_decompress || flag_cmp==-1 ) {
      if ( flag_cmp==-1 )
        printf("Error - file comparison\n");
      else
        printf("Error - read test files\n");
  } else {
    print_progress(Lseek64(h_read,0,SEEK_CUR),len_file_read,"\n");
    printf("Test OK\n");
  }
  close(h_read);
  close(h_write);


  printf("\nTest seek file compress %s and file source %s\n",name_file_compress,argv[1]);

  h_read=open(argv[1],O_RDONLY|O_BINARY,S_IREAD);
  if ( h_read==-1 ) {
    printf("Error - open file %s\n",argv[1]);
    return;
  }
  len_file_read=Filelength(h_read);

  rc=xz_file_open(fxz,name_file_compress,XZ_FILE_MODE_READ,0);
  if ( rc==-1 ) {
    printf("Error: ");
    if ( fxz->ret==LZMA_OK )
      printf("LZMA - %s\n",xz_file_str_error(fxz->ret));
    else
      printf("open file.");
    return;
  }

  srand ( (int)time(NULL) );
  t_last=t_start=time(0);
  flag_cmp=0;

  __int64 offset;
  int origin;
  __int64 rc_read, rc_xz;

  printf("count tests - ");
  for ( int i=0; i<100; i++ ) {

    offset=rand();
    origin=i%3;

    if ( i==0 ) {
      offset=len_file_read;
      origin=SEEK_SET;
    }
    if ( i==1 ) {
      offset=0;
      origin=SEEK_SET;
    }
    rc_read=Lseek64(h_read,offset,origin);
    if ( rc_read==-1 )
      break;
    rc_xz=xz_file_seek(fxz,offset,origin);
    if ( rc_xz==-1 )
      break;

    len_read=read(h_read,buf_read,SIZE_BUF);
    if ( len_read==-1 )
      break;
    len_decompress=xz_file_read(fxz,buf_decompress,SIZE_BUF);
    if ( len_decompress==-1 )
      break;
    if ( len_read!=len_decompress )
      break;
    if ( len_read==0 )
      continue;

    if ( memcmp(buf_read,buf_decompress,len_read)!=0 ) {
      flag_cmp=-1;
      printf("Error - file comparison\n");
      break;
    }

    if ( t_last!=time(0) ) {
      t_last=time(0);
      printf("%d, ",i);
    }

  }

  if ( rc_xz==-1 || len_read==-1 || len_decompress==-1 || len_read!=len_decompress || flag_cmp==-1 ) {
    if ( flag_cmp==-1 )
      printf("Error - file comparison\n");
    else
      printf("Error - seek test\n");
  } else {
    printf("Test OK\n");
  }

  close(h_read);
  xz_file_close(fxz);

  xz_file_delete(&fxz);

  free(buf_read);
  free(buf_decompress);

  _getch();
}
