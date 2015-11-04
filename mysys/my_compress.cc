/* Copyright (c) 2000, 2015, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/* Written by Sinisa Milivojevic <sinisa@mysql.com> */

/**
  @file mysys/my_compress.cc
*/

#include <my_global.h>
#include <mysys_priv.h>
#include <my_sys.h>
#include <m_string.h>
#include <zlib.h>
#include "mysql/service_mysql_alloc.h"

#include <algorithm>

/*
   This replaces the packet with a compressed packet

   SYNOPSIS
     my_compress()
     packet	Data to compress. This is is replaced with the compressed data.
     len	Length of data to compress at 'packet'
     complen	out: 0 if packet was not compressed

   RETURN
     1   error. 'len' is not changed'
     0   ok.  In this case 'len' contains the size of the compressed packet
*/

my_bool my_compress(uchar *packet, size_t *len, size_t *complen)
{
  DBUG_ENTER("my_compress");
  if (*len < MIN_COMPRESS_LENGTH)
  {
    *complen=0;
    DBUG_PRINT("note",("Packet too short: Not compressed"));
  }
  else
  {
    uchar *compbuf=my_compress_alloc(packet,len,complen);
    if (!compbuf)
      DBUG_RETURN(*complen ? 0 : 1);
    memcpy(packet,compbuf,*len);
    my_free(compbuf);
  }
  DBUG_RETURN(0);
}


uchar *my_compress_alloc(const uchar *packet, size_t *len, size_t *complen)
{
  uchar *compbuf;
  uLongf tmp_complen;
  int res;
  *complen=  *len * 120 / 100 + 12;

  if (!(compbuf= (uchar *) my_malloc(key_memory_my_compress_alloc,
                                     *complen, MYF(MY_WME))))
    return 0;					/* Not enough memory */

  tmp_complen= (uint) *complen;
  res= compress((Bytef*) compbuf, &tmp_complen, (Bytef*) packet, (uLong) *len);
  *complen=    tmp_complen;

  if (res != Z_OK)
  {
    my_free(compbuf);
    return 0;
  }

  if (*complen >= *len)
  {
    *complen= 0;
    my_free(compbuf);
    DBUG_PRINT("note",("Packet got longer on compression; Not compressed"));
    return 0;
  }
  /* Store length of compressed packet in *len */
  std::swap(*len, *complen);
  return compbuf;
}


/*
  Uncompress packet

   SYNOPSIS
     my_uncompress()
     packet	Compressed data. This is is replaced with the orignal data.
     len	Length of compressed data
     complen	Length of the packet buffer (must be enough for the original
	        data)

   RETURN
     1   error
     0   ok.  In this case 'complen' contains the updated size of the
              real data.
*/

my_bool my_uncompress(uchar *packet, size_t len, size_t *complen)
{
  uLongf tmp_complen;
  DBUG_ENTER("my_uncompress");

  if (*complen)					/* If compressed */
  {
    uchar *compbuf= (uchar *) my_malloc(key_memory_my_compress_alloc,
                                        *complen,MYF(MY_WME));
    int error;
    if (!compbuf)
      DBUG_RETURN(1);				/* Not enough memory */

    tmp_complen= (uint) *complen;
    error= uncompress((Bytef*) compbuf, &tmp_complen, (Bytef*) packet,
                      (uLong) len);
    *complen= tmp_complen;
    if (error != Z_OK)
    {						/* Probably wrong packet */
      DBUG_PRINT("error",("Can't uncompress packet, error: %d",error));
      my_free(compbuf);
      DBUG_RETURN(1);
    }
    memcpy(packet, compbuf, *complen);
    my_free(compbuf);
  }
  else
    *complen= len;
  DBUG_RETURN(0);
}


/**
  Compress meta data.

  @note Not implemented yet. Will probably be needed by NDB.

  @param       meta_data                   Meta data to be compressed.
  @param       meta_data_length            Length of meta data.
  @param [out] compressed_meta_data        Compressed meta data.
  @param [out] compressed_meta_data_length Length of compressed meta data.
*/
/* purecov: begin deadcode */
my_bool compress_serialized_meta_data(
          uchar *meta_data __attribute__((unused)),
          size_t meta_data_length __attribute__((unused)),
          uchar **compressed_meta_data __attribute__((unused)),
          size_t *compressed_meta_data_length __attribute__((unused)))
{
  // TODO: This function is currently not implemented
  // TODO: Add new P_S key for allocation
  DBUG_ASSERT(0);
  return 0;
}


/**
  Uncompress meta data.

  @note Not implemented yet. Will probably be needed by NDB.

  @param       compressed_meta_data        Compressed meta data.
  @param       compressed_meta_data_length Length of compressed meta data.
  @param [out] meta_data                   Meta data to be compressed.
  @param [out] meta_data_length            Length of meta data.
*/
my_bool uncompress_serialized_meta_data(
          uchar *compressed_meta_data __attribute__((unused)),
          size_t compressed_meta_data_length __attribute__((unused)),
          uchar **meta_data __attribute__((unused)),
          size_t *meta_data_length __attribute__((unused)))
{
  // TODO: This function is currently not implemented
  DBUG_ASSERT(0);
  return 0;
}
/* purecov: end */
