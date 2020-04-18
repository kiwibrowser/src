#pragma once

#include "Platform.h"

#include <vector>

//-----------------------------------------------------------------------------

void     printbits   ( const void * blob, int len );
void     printhex32  ( const void * blob, int len );
void     printbytes  ( const void * blob, int len );
void     printbytes2 ( const void * blob, int len );

uint32_t popcount    ( uint32_t v );
uint32_t parity      ( uint32_t v );

uint32_t getbit      ( const void * blob, int len, uint32_t bit );
uint32_t getbit_wrap ( const void * blob, int len, uint32_t bit );

void     setbit      ( void * blob, int len, uint32_t bit );
void     setbit      ( void * blob, int len, uint32_t bit, uint32_t val );

void     clearbit    ( void * blob, int len, uint32_t bit );

void     flipbit     ( void * blob, int len, uint32_t bit );

int      countbits   ( uint32_t v );
int      countbits   ( std::vector<uint32_t> & v );

int      countbits   ( const void * blob, int len );

void     invert      ( std::vector<uint32_t> & v );

//----------

template< typename T >
inline uint32_t getbit ( T & blob, uint32_t bit )
{
  return getbit(&blob,sizeof(blob),bit);
}

template<> inline uint32_t getbit ( uint32_t & blob, uint32_t bit ) { return (blob >> (bit & 31)) & 1; }
template<> inline uint32_t getbit ( uint64_t & blob, uint32_t bit ) { return (blob >> (bit & 63)) & 1; }

//----------

template< typename T >
inline void setbit ( T & blob, uint32_t bit )
{
  return setbit(&blob,sizeof(blob),bit);
}

template<> inline void setbit ( uint32_t & blob, uint32_t bit ) { blob |= uint32_t(1) << (bit & 31); }
template<> inline void setbit ( uint64_t & blob, uint32_t bit ) { blob |= uint64_t(1) << (bit & 63); }

//----------

template< typename T >
inline void flipbit ( T & blob, uint32_t bit )
{
  flipbit(&blob,sizeof(blob),bit);
}

template<> inline void flipbit ( uint32_t & blob, uint32_t bit ) { bit &= 31; blob ^= (uint32_t(1) << bit); }
template<> inline void flipbit ( uint64_t & blob, uint32_t bit ) { bit &= 63; blob ^= (uint64_t(1) << bit); }

//-----------------------------------------------------------------------------
// Left and right shift of blobs. The shift(N) versions work on chunks of N
// bits at a time (faster)

void lshift1  ( void * blob, int len, int c );
void lshift8  ( void * blob, int len, int c );
void lshift32 ( void * blob, int len, int c );

void rshift1  ( void * blob, int len, int c );
void rshift8  ( void * blob, int len, int c );
void rshift32 ( void * blob, int len, int c );

inline void lshift ( void * blob, int len, int c )
{
  if((len & 3) == 0)
  {
    lshift32(blob,len,c);
  }
  else
  {
    lshift8(blob,len,c);
  }
}

inline void rshift ( void * blob, int len, int c )
{
  if((len & 3) == 0)
  {
    rshift32(blob,len,c);
  }
  else
  {
    rshift8(blob,len,c);
  }
}

template < typename T >
inline void lshift ( T & blob, int c )
{
  if((sizeof(T) & 3) == 0)
  {
    lshift32(&blob,sizeof(T),c);
  }
  else
  {
    lshift8(&blob,sizeof(T),c);
  }
}

template < typename T >
inline void rshift ( T & blob, int c )
{
  if((sizeof(T) & 3) == 0)
  {
    lshift32(&blob,sizeof(T),c);
  }
  else
  {
    lshift8(&blob,sizeof(T),c);
  }
}

template<> inline void lshift ( uint32_t & blob, int c ) { blob <<= c; }
template<> inline void lshift ( uint64_t & blob, int c ) { blob <<= c; }
template<> inline void rshift ( uint32_t & blob, int c ) { blob >>= c; }
template<> inline void rshift ( uint64_t & blob, int c ) { blob >>= c; }

//-----------------------------------------------------------------------------
// Left and right rotate of blobs. The rot(N) versions work on chunks of N
// bits at a time (faster)

void lrot1    ( void * blob, int len, int c );
void lrot8    ( void * blob, int len, int c );
void lrot32   ( void * blob, int len, int c );

void rrot1    ( void * blob, int len, int c );
void rrot8    ( void * blob, int len, int c );
void rrot32   ( void * blob, int len, int c );

inline void lrot ( void * blob, int len, int c )
{
  if((len & 3) == 0)
  {
    return lrot32(blob,len,c);
  }
  else
  {
    return lrot8(blob,len,c);
  }
}

inline void rrot ( void * blob, int len, int c )
{
  if((len & 3) == 0)
  {
    return rrot32(blob,len,c);
  }
  else
  {
    return rrot8(blob,len,c);
  }
}

template < typename T >
inline void lrot ( T & blob, int c )
{
  if((sizeof(T) & 3) == 0)
  {
    return lrot32(&blob,sizeof(T),c);
  }
  else
  {
    return lrot8(&blob,sizeof(T),c);
  }
}

template < typename T >
inline void rrot ( T & blob, int c )
{
  if((sizeof(T) & 3) == 0)
  {
    return rrot32(&blob,sizeof(T),c);
  }
  else
  {
    return rrot8(&blob,sizeof(T),c);
  }
}

template<> inline void lrot ( uint32_t & blob, int c ) { blob = ROTL32(blob,c); }
template<> inline void lrot ( uint64_t & blob, int c ) { blob = ROTL64(blob,c); }
template<> inline void rrot ( uint32_t & blob, int c ) { blob = ROTR32(blob,c); }
template<> inline void rrot ( uint64_t & blob, int c ) { blob = ROTR64(blob,c); }

//-----------------------------------------------------------------------------
// Bit-windowing functions - select some N-bit subset of the input blob

uint32_t window1  ( void * blob, int len, int start, int count );
uint32_t window8  ( void * blob, int len, int start, int count );
uint32_t window32 ( void * blob, int len, int start, int count );

inline uint32_t window ( void * blob, int len, int start, int count )
{
  if(len & 3)
  {
    return window8(blob,len,start,count);
  }
  else
  {
    return window32(blob,len,start,count);
  }
}

template < typename T >
inline uint32_t window ( T & blob, int start, int count )
{
  if((sizeof(T) & 3) == 0)
  {
    return window32(&blob,sizeof(T),start,count);
  }
  else
  {
    return window8(&blob,sizeof(T),start,count);
  }
}

template<> 
inline uint32_t window ( uint32_t & blob, int start, int count )
{
  return ROTR32(blob,start) & ((1<<count)-1);
}

template<> 
inline uint32_t window ( uint64_t & blob, int start, int count )
{
  return (uint32_t)ROTR64(blob,start) & ((1<<count)-1);
}

//-----------------------------------------------------------------------------
