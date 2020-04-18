#pragma once

#include "Types.h"

#include <math.h>
#include <vector>
#include <map>
#include <algorithm>   // for std::sort
#include <string.h>    // for memset
#include <stdio.h>     // for printf

double calcScore ( const int * bins, const int bincount, const int ballcount );

void plot ( double n );

inline double ExpectedCollisions ( double balls, double bins )
{
  return balls - bins + bins * pow(1 - 1/bins,balls);
}

double chooseK ( int b, int k );
double chooseUpToK ( int n, int k );

//-----------------------------------------------------------------------------

inline uint32_t f3mix ( uint32_t k )
{
  k ^= k >> 16;
  k *= 0x85ebca6b;
  k ^= k >> 13;
  k *= 0xc2b2ae35;
  k ^= k >> 16;

  return k;
}

//-----------------------------------------------------------------------------
// Sort the hash list, count the total number of collisions and return
// the first N collisions for further processing

template< typename hashtype >
int FindCollisions ( std::vector<hashtype> & hashes, 
                     HashSet<hashtype> & collisions,
                     int maxCollisions )
{
  int collcount = 0;

  std::sort(hashes.begin(),hashes.end());

  for(size_t i = 1; i < hashes.size(); i++)
  {
    if(hashes[i] == hashes[i-1])
    {
      collcount++;

      if((int)collisions.size() < maxCollisions)
      {
        collisions.insert(hashes[i]);
      }
    }
  }

  return collcount;
}

//-----------------------------------------------------------------------------

template < class keytype, typename hashtype >
int PrintCollisions ( hashfunc<hashtype> hash, std::vector<keytype> & keys )
{
  int collcount = 0;

  typedef std::map<hashtype,keytype> htab;
  htab tab;

  for(size_t i = 1; i < keys.size(); i++)
  {
    keytype & k1 = keys[i];

    hashtype h = hash(&k1,sizeof(keytype),0);

    typename htab::iterator it = tab.find(h);

    if(it != tab.end())
    {
      keytype & k2 = (*it).second;

      printf("A: ");
      printbits(&k1,sizeof(keytype));
      printf("B: ");
      printbits(&k2,sizeof(keytype));
    }
    else
    {
      tab.insert( std::make_pair(h,k1) );
    }
  }

  return collcount;
}

//----------------------------------------------------------------------------
// Measure the distribution "score" for each possible N-bit span up to 20 bits

template< typename hashtype >
double TestDistribution ( std::vector<hashtype> & hashes, bool drawDiagram )
{
  printf("Testing distribution - ");

  if(drawDiagram) printf("\n");

  const int hashbits = sizeof(hashtype) * 8;

  int maxwidth = 20;

  // We need at least 5 keys per bin to reliably test distribution biases
  // down to 1%, so don't bother to test sparser distributions than that

  while(double(hashes.size()) / double(1 << maxwidth) < 5.0)
  {
    maxwidth--;
  }

  std::vector<int> bins;
  bins.resize(1 << maxwidth);

  double worst = 0;
  int worstStart = -1;
  int worstWidth = -1;

  for(int start = 0; start < hashbits; start++)
  {
    int width = maxwidth;
    int bincount = (1 << width);

    memset(&bins[0],0,sizeof(int)*bincount);

    for(size_t j = 0; j < hashes.size(); j++)
    {
      hashtype & hash = hashes[j];

      uint32_t index = window(&hash,sizeof(hash),start,width);

      bins[index]++;
    }

    // Test the distribution, then fold the bins in half,
    // repeat until we're down to 256 bins

    if(drawDiagram) printf("[");

    while(bincount >= 256)
    {
      double n = calcScore(&bins[0],bincount,(int)hashes.size());

      if(drawDiagram) plot(n);

      if(n > worst)
      {
        worst = n;
        worstStart = start;
        worstWidth = width;
      }

      width--;
      bincount /= 2;

      if(width < 8) break;

      for(int i = 0; i < bincount; i++)
      {
        bins[i] += bins[i+bincount];
      }
    }

    if(drawDiagram) printf("]\n");
  }

  double pct = worst * 100.0;

  printf("Worst bias is the %3d-bit window at bit %3d - %5.3f%%",worstWidth,worstStart,pct);
  if(pct >= 1.0) printf(" !!!!! ");
  printf("\n");

  return worst;
}

//----------------------------------------------------------------------------

template < typename hashtype >
bool TestHashList ( std::vector<hashtype> & hashes, std::vector<hashtype> & collisions, bool testDist, bool drawDiagram )
{
  bool result = true;

  {
    size_t count = hashes.size();

    double expected = (double(count) * double(count-1)) / pow(2.0,double(sizeof(hashtype) * 8 + 1));

    printf("Testing collisions   - Expected %8.2f, ",expected);

    double collcount = 0;

    HashSet<hashtype> collisions;

    collcount = FindCollisions(hashes,collisions,1000);

    printf("actual %8.2f (%5.2fx)",collcount, collcount / expected);

    if(sizeof(hashtype) == sizeof(uint32_t))
    {
    // 2x expected collisions = fail

    // #TODO - collision failure cutoff needs to be expressed as a standard deviation instead
    // of a scale factor, otherwise we fail erroneously if there are a small expected number
    // of collisions

    if(double(collcount) / double(expected) > 2.0)
    {
      printf(" !!!!! ");
      result = false;
    }
    }
    else
    {
      // For all hashes larger than 32 bits, _any_ collisions are a failure.
      
      if(collcount > 0)
      {
        printf(" !!!!! ");
        result = false;
      }
    }

    printf("\n");
  }

  //----------

  if(testDist)
  {
    TestDistribution(hashes,drawDiagram);
  }

  return result;
}

//----------

template < typename hashtype >
bool TestHashList ( std::vector<hashtype> & hashes, bool /*testColl*/, bool testDist, bool drawDiagram )
{
  std::vector<hashtype> collisions;

  return TestHashList(hashes,collisions,testDist,drawDiagram);
}

//-----------------------------------------------------------------------------

template < class keytype, typename hashtype >
bool TestKeyList ( hashfunc<hashtype> hash, std::vector<keytype> & keys, bool testColl, bool testDist, bool drawDiagram )
{
  int keycount = (int)keys.size();

  std::vector<hashtype> hashes;

  hashes.resize(keycount);

  printf("Hashing");

  for(int i = 0; i < keycount; i++)
  {
    if(i % (keycount / 10) == 0) printf(".");

    keytype & k = keys[i];

    hash(&k,sizeof(k),0,&hashes[i]);
  }

  printf("\n");

  bool result = TestHashList(hashes,testColl,testDist,drawDiagram);

  printf("\n");

  return result;
}

//-----------------------------------------------------------------------------
// Bytepair test - generate 16-bit indices from all possible non-overlapping
// 8-bit sections of the hash value, check distribution on all of them.

// This is a very good test for catching weak intercorrelations between bits - 
// much harder to pass than the normal distribution test. However, it doesn't
// really model the normal usage of hash functions in hash table lookup, so
// I'm not sure it's that useful (and hash functions that fail this test but
// pass the normal distribution test still work well in practice)

template < typename hashtype >
double TestDistributionBytepairs ( std::vector<hashtype> & hashes, bool drawDiagram )
{
  const int nbytes = sizeof(hashtype);
  const int hashbits = nbytes * 8;
  
  const int nbins = 65536;

  std::vector<int> bins(nbins,0);

  double worst = 0;

  for(int a = 0; a < hashbits; a++)
  {
    if(drawDiagram) if((a % 8 == 0) && (a > 0)) printf("\n");

    if(drawDiagram) printf("[");

    for(int b = 0; b < hashbits; b++)
    {
      if(drawDiagram) if((b % 8 == 0) && (b > 0)) printf(" ");

      bins.clear();
      bins.resize(nbins,0);

      for(size_t i = 0; i < hashes.size(); i++)
      {
        hashtype & hash = hashes[i];

        uint32_t pa = window(&hash,sizeof(hash),a,8);
        uint32_t pb = window(&hash,sizeof(hash),b,8);

        bins[pa | (pb << 8)]++;
      }

      double s = calcScore(bins,bins.size(),hashes.size());

      if(drawDiagram) plot(s);

      if(s > worst)
      {
        worst = s;
      }
    }

    if(drawDiagram) printf("]\n");
  }

  return worst;
}

//-----------------------------------------------------------------------------
// Simplified test - only check 64k distributions, and only on byte boundaries

template < typename hashtype >
void TestDistributionFast ( std::vector<hashtype> & hashes, double & dworst, double & davg )
{
  const int hashbits = sizeof(hashtype) * 8;
  const int nbins = 65536;
  
  std::vector<int> bins(nbins,0);

  dworst = -1.0e90;
  davg = 0;

  for(int start = 0; start < hashbits; start += 8)
  {
    bins.clear();
    bins.resize(nbins,0);

    for(size_t j = 0; j < hashes.size(); j++)
    {
      hashtype & hash = hashes[j];

      uint32_t index = window(&hash,sizeof(hash),start,16);

      bins[index]++;
    }

    double n = calcScore(&bins.front(),(int)bins.size(),(int)hashes.size());
    
    davg += n;

    if(n > dworst) dworst = n;
  }

  davg /= double(hashbits/8);
}

//-----------------------------------------------------------------------------
