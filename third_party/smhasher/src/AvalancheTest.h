//-----------------------------------------------------------------------------
// Flipping a single bit of a key should cause an "avalanche" of changes in
// the hash function's output. Ideally, each output bits should flip 50% of
// the time - if the probability of an output bit flipping is not 50%, that bit
// is "biased". Too much bias means that patterns applied to the input will
// cause "echoes" of the patterns in the output, which in turn can cause the
// hash function to fail to create an even, random distribution of hash values.


#pragma once

#include "Types.h"
#include "Random.h"

#include <vector>
#include <stdio.h>
#include <math.h>

// Avalanche fails if a bit is biased by more than 1%

#define AVALANCHE_FAIL 0.01

double maxBias ( std::vector<int> & counts, int reps );

//-----------------------------------------------------------------------------

template < typename keytype, typename hashtype >
void calcBias ( pfHash hash, std::vector<int> & counts, int reps, Rand & r )
{
  const int keybytes = sizeof(keytype);
  const int hashbytes = sizeof(hashtype);

  const int keybits = keybytes * 8;
  const int hashbits = hashbytes * 8;

  keytype K;
  hashtype A,B;

  for(int irep = 0; irep < reps; irep++)
  {
    if(irep % (reps/10) == 0) printf(".");

    r.rand_p(&K,keybytes);

    hash(&K,keybytes,0,&A);

    int * cursor = &counts[0];

    for(int iBit = 0; iBit < keybits; iBit++)
    {
      flipbit(&K,keybytes,iBit);
      hash(&K,keybytes,0,&B);
      flipbit(&K,keybytes,iBit);

      for(int iOut = 0; iOut < hashbits; iOut++)
      {
        int bitA = getbit(&A,hashbytes,iOut);
        int bitB = getbit(&B,hashbytes,iOut);

        (*cursor++) += (bitA ^ bitB);
      }
    }
  }
}

//-----------------------------------------------------------------------------

template < typename keytype, typename hashtype >
bool AvalancheTest ( pfHash hash, const int reps )
{
  Rand r(48273);
  
  const int keybytes = sizeof(keytype);
  const int hashbytes = sizeof(hashtype);

  const int keybits = keybytes * 8;
  const int hashbits = hashbytes * 8;

  printf("Testing %3d-bit keys -> %3d-bit hashes, %8d reps",keybits,hashbits,reps);

  //----------

  std::vector<int> bins(keybits*hashbits,0);

  calcBias<keytype,hashtype>(hash,bins,reps,r);
  
  //----------

  bool result = true;

  double b = maxBias(bins,reps);

  printf(" worst bias is %f%%",b * 100.0);

  if(b > AVALANCHE_FAIL)
  {
    printf(" !!!!! ");
    result = false;
  }

  printf("\n");

  return result;
}

//----------------------------------------------------------------------------
// Tests the Bit Independence Criteron. Stricter than Avalanche, but slow and
// not really all that useful.

template< typename keytype, typename hashtype >
void BicTest ( pfHash hash, const int keybit, const int reps, double & maxBias, int & maxA, int & maxB, bool verbose )
{
  Rand r(11938);
  
  const int keybytes = sizeof(keytype);
  const int hashbytes = sizeof(hashtype);
  const int hashbits = hashbytes * 8;

  std::vector<int> bins(hashbits*hashbits*4,0);

  keytype key;
  hashtype h1,h2;

  for(int irep = 0; irep < reps; irep++)
  {
    if(verbose)
    {
      if(irep % (reps/10) == 0) printf(".");
    }

    r.rand_p(&key,keybytes);
    hash(&key,keybytes,0,&h1);

    flipbit(key,keybit);
    hash(&key,keybytes,0,&h2);

    hashtype d = h1 ^ h2;

    for(int out1 = 0; out1 < hashbits; out1++)
    for(int out2 = 0; out2 < hashbits; out2++)
    {
      if(out1 == out2) continue;

      uint32_t b = getbit(d,out1) | (getbit(d,out2) << 1);

      bins[(out1 * hashbits + out2) * 4 + b]++;
    }
  }

  if(verbose) printf("\n");

  maxBias = 0;

  for(int out1 = 0; out1 < hashbits; out1++)
  {
    for(int out2 = 0; out2 < hashbits; out2++)
    {
      if(out1 == out2)
      {
        if(verbose) printf("\\");
        continue;
      }

      double bias = 0;

      for(int b = 0; b < 4; b++)
      {
        double b2 = double(bins[(out1 * hashbits + out2) * 4 + b]) / double(reps / 2);
        b2 = fabs(b2 * 2 - 1);

        if(b2 > bias) bias = b2;
      }

      if(bias > maxBias)
      {
        maxBias = bias;
        maxA = out1;
        maxB = out2;
      }

      if(verbose) 
      {
        if     (bias < 0.01) printf(".");
        else if(bias < 0.05) printf("o");
        else if(bias < 0.33) printf("O");
        else                 printf("X");
      }
    }

    if(verbose) printf("\n");
  }
}

//----------

template< typename keytype, typename hashtype >
bool BicTest ( pfHash hash, const int reps )
{
  const int keybytes = sizeof(keytype);
  const int keybits = keybytes * 8;

  double maxBias = 0;
  int maxK = 0;
  int maxA = 0;
  int maxB = 0;

  for(int i = 0; i < keybits; i++)
  {
    if(i % (keybits/10) == 0) printf(".");

    double bias;
    int a,b;
    
    BicTest<keytype,hashtype>(hash,i,reps,bias,a,b,true);

    if(bias > maxBias)
    {
      maxBias = bias;
      maxK = i;
      maxA = a;
      maxB = b;
    }
  }

  printf("Max bias %f - (%3d : %3d,%3d)\n",maxBias,maxK,maxA,maxB);

  // Bit independence is harder to pass than avalanche, so we're a bit more lax here.

  bool result = (maxBias < 0.05);

  return result;
}

//-----------------------------------------------------------------------------
// BIC test variant - store all intermediate data in a table, draw diagram
// afterwards (much faster)

template< typename keytype, typename hashtype >
void BicTest3 ( pfHash hash, const int reps, bool verbose = true )
{
  const int keybytes = sizeof(keytype);
  const int keybits = keybytes * 8;
  const int hashbytes = sizeof(hashtype);
  const int hashbits = hashbytes * 8;
  const int pagesize = hashbits*hashbits*4;

  Rand r(11938);

  double maxBias = 0;
  int maxK = 0;
  int maxA = 0;
  int maxB = 0;

  keytype key;
  hashtype h1,h2;

  std::vector<int> bins(keybits*pagesize,0);

  for(int keybit = 0; keybit < keybits; keybit++)
  {
    if(keybit % (keybits/10) == 0) printf(".");

    int * page = &bins[keybit*pagesize];

    for(int irep = 0; irep < reps; irep++)
    {
      r.rand_p(&key,keybytes);
      hash(&key,keybytes,0,&h1);
      flipbit(key,keybit);
      hash(&key,keybytes,0,&h2);

      hashtype d = h1 ^ h2;

      for(int out1 = 0; out1 < hashbits-1; out1++)
      for(int out2 = out1+1; out2 < hashbits; out2++)
      {
        int * b = &page[(out1*hashbits+out2)*4];

        uint32_t x = getbit(d,out1) | (getbit(d,out2) << 1);

        b[x]++;
      }
    }
  }

  printf("\n");

  for(int out1 = 0; out1 < hashbits-1; out1++)
  {
    for(int out2 = out1+1; out2 < hashbits; out2++)
    {
      if(verbose) printf("(%3d,%3d) - ",out1,out2);

      for(int keybit = 0; keybit < keybits; keybit++)
      {
        int * page = &bins[keybit*pagesize];
        int * bins = &page[(out1*hashbits+out2)*4];

        double bias = 0;

        for(int b = 0; b < 4; b++)
        {
          double b2 = double(bins[b]) / double(reps / 2);
          b2 = fabs(b2 * 2 - 1);

          if(b2 > bias) bias = b2;
        }

        if(bias > maxBias)
        {
          maxBias = bias;
          maxK = keybit;
          maxA = out1;
          maxB = out2;
        }

        if(verbose) 
        {
          if     (bias < 0.01) printf(".");
          else if(bias < 0.05) printf("o");
          else if(bias < 0.33) printf("O");
          else                 printf("X");
        }
      }

      // Finished keybit

      if(verbose) printf("\n");
    }

    if(verbose)
    {
      for(int i = 0; i < keybits+12; i++) printf("-");
      printf("\n");
    }
  }

  printf("Max bias %f - (%3d : %3d,%3d)\n",maxBias,maxK,maxA,maxB);
}


//-----------------------------------------------------------------------------
// BIC test variant - iterate over output bits, then key bits. No temp storage,
// but slooooow

template< typename keytype, typename hashtype >
void BicTest2 ( pfHash hash, const int reps, bool verbose = true )
{
  const int keybytes = sizeof(keytype);
  const int keybits = keybytes * 8;
  const int hashbytes = sizeof(hashtype);
  const int hashbits = hashbytes * 8;

  Rand r(11938);

  double maxBias = 0;
  int maxK = 0;
  int maxA = 0;
  int maxB = 0;

  keytype key;
  hashtype h1,h2;

  for(int out1 = 0; out1 < hashbits-1; out1++)
  for(int out2 = out1+1; out2 < hashbits; out2++)
  {
    if(verbose) printf("(%3d,%3d) - ",out1,out2);

    for(int keybit = 0; keybit < keybits; keybit++)
    {
      int bins[4] = { 0, 0, 0, 0 };

      for(int irep = 0; irep < reps; irep++)
      {
        r.rand_p(&key,keybytes);
        hash(&key,keybytes,0,&h1);
        flipbit(key,keybit);
        hash(&key,keybytes,0,&h2);

        hashtype d = h1 ^ h2;

        uint32_t b = getbit(d,out1) | (getbit(d,out2) << 1);

        bins[b]++;
      }

      double bias = 0;

      for(int b = 0; b < 4; b++)
      {
        double b2 = double(bins[b]) / double(reps / 2);
        b2 = fabs(b2 * 2 - 1);

        if(b2 > bias) bias = b2;
      }

      if(bias > maxBias)
      {
        maxBias = bias;
        maxK = keybit;
        maxA = out1;
        maxB = out2;
      }

      if(verbose) 
      {
        if     (bias < 0.05) printf(".");
        else if(bias < 0.10) printf("o");
        else if(bias < 0.50) printf("O");
        else                 printf("X");
      }
    }

    // Finished keybit

    if(verbose) printf("\n");
  }

  printf("Max bias %f - (%3d : %3d,%3d)\n",maxBias,maxK,maxA,maxB);
}

//-----------------------------------------------------------------------------
