#include "Types.h"

#include "Random.h"

#include <stdio.h>

uint32_t MurmurOAAT ( const void * blob, int len, uint32_t seed );

//-----------------------------------------------------------------------------

#if defined(_MSC_VER)
#pragma optimize( "", off )
#endif

void blackhole ( uint32_t )
{
}

uint32_t whitehole ( void )
{
  return 0;
}

#if defined(_MSC_VER)
#pragma optimize( "", on ) 
#endif

uint32_t g_verify = 1;

void MixVCode ( const void * blob, int len )
{
	g_verify = MurmurOAAT(blob,len,g_verify);
}

//-----------------------------------------------------------------------------

bool isprime ( uint32_t x )
{
  uint32_t p[] = 
  {
    2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,
    103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,
    199,211,223,227,229,233,239,241,251
  };

  for(size_t i=0; i < sizeof(p)/sizeof(uint32_t); i++)
  { 
    if((x % p[i]) == 0)
    {
      return false;
    }
  } 

  for(int i = 257; i < 65536; i += 2) 
  { 
    if((x % i) == 0)
    {
      return false;
    }
  }

  return true;
}

void GenerateMixingConstants ( void )
{
  Rand r(8350147);

  int count = 0;

  int trials = 0;
  int bitfail = 0;
  int popfail = 0;
  int matchfail = 0;
  int primefail = 0;

  //for(uint32_t x = 1; x; x++)
  while(count < 100)
  {
    //if(x % 100000000 == 0) printf(".");

    trials++;
    uint32_t b = r.rand_u32();
    //uint32_t b = x;

    //----------
    // must have between 14 and 18 set bits

    if(popcount(b) < 16) { b = 0; popfail++; }
    if(popcount(b) > 16) { b = 0; popfail++; }

    if(b == 0) continue;

    //----------
    // must have 3-5 bits set per 8-bit window

    for(int i = 0; i < 32; i++)
    {
      uint32_t c = ROTL32(b,i) & 0xFF;

      if(popcount(c) < 3) { b = 0; bitfail++; break; }
      if(popcount(c) > 5) { b = 0; bitfail++; break; }
    }

    if(b == 0) continue;

    //----------
    // all 8-bit windows must be different

    uint8_t match[256];

    memset(match,0,256);

    for(int i = 0; i < 32; i++)
    {
      uint32_t c = ROTL32(b,i) & 0xFF;
      
      if(match[c]) { b = 0; matchfail++; break; }

      match[c] = 1;
    }

    if(b == 0) continue;

    //----------
    // must be prime

    if(!isprime(b))
    {
      b = 0;
      primefail++;
    }

    if(b == 0) continue;

    //----------

    if(b)
    {
      printf("0x%08x : 0x%08x\n",b,~b);
      count++;
    }
  }

  printf("%d %d %d %d %d %d\n",trials,popfail,bitfail,matchfail,primefail,count);
}

//-----------------------------------------------------------------------------
