//-----------------------------------------------------------------------------
// Differential collision & distribution tests - generate a bunch of random keys,
// see what happens to the hash value when we flip a few bits of the key.

#pragma once

#include "Types.h"
#include "Stats.h"      // for chooseUpToK
#include "KeysetTest.h" // for SparseKeygenRecurse
#include "Random.h"

#include <vector>
#include <algorithm>
#include <stdio.h>

//-----------------------------------------------------------------------------
// Sort through the differentials, ignoring collisions that only occured once 
// (these could be false positives). If we find collisions of 3 or more, the
// differential test fails.

template < class keytype >
bool ProcessDifferentials ( std::vector<keytype> & diffs, int reps, bool dumpCollisions )
{
  std::sort(diffs.begin(), diffs.end());

  int count = 1;
  int ignore = 0;

  bool result = true;

  if(diffs.size())
  {
    keytype kp = diffs[0];

    for(int i = 1; i < (int)diffs.size(); i++)
    {
      if(diffs[i] == kp)
      {
        count++;
        continue;
      }
      else
      {
        if(count > 1)
        {
          result = false;

          double pct = 100 * (double(count) / double(reps));

          if(dumpCollisions)
          {
            printbits((unsigned char*)&kp,sizeof(kp));
            printf(" - %4.2f%%\n", pct );
          }
        }
        else 
        {
          ignore++;
        }

        kp = diffs[i];
        count = 1;
      }
    }

    if(count > 1)
    {
      double pct = 100 * (double(count) / double(reps));

      if(dumpCollisions)
      {
        printbits((unsigned char*)&kp,sizeof(kp));
        printf(" - %4.2f%%\n", pct );
      }
    }
    else 
    {
      ignore++;
    }
  }

  printf("%d total collisions, of which %d single collisions were ignored",(int)diffs.size(),ignore);

  if(result == false)
  {
    printf(" !!!!! ");
  }

  printf("\n");
  printf("\n");

  return result;
}

//-----------------------------------------------------------------------------
// Check all possible keybits-choose-N differentials for collisions, report
// ones that occur significantly more often than expected.

// Random collisions can happen with probability 1 in 2^32 - if we do more than
// 2^32 tests, we'll probably see some spurious random collisions, so don't report
// them.

template < typename keytype, typename hashtype >
void DiffTestRecurse ( pfHash hash, keytype & k1, keytype & k2, hashtype & h1, hashtype & h2, int start, int bitsleft, std::vector<keytype> & diffs )
{
  const int bits = sizeof(keytype)*8;

  for(int i = start; i < bits; i++)
  {
    flipbit(&k2,sizeof(k2),i);
    bitsleft--;

    hash(&k2,sizeof(k2),0,&h2);

    if(h1 == h2)
    {
      diffs.push_back(k1 ^ k2);
    }

    if(bitsleft)
    {
      DiffTestRecurse(hash,k1,k2,h1,h2,i+1,bitsleft,diffs);
    }

    flipbit(&k2,sizeof(k2),i);
    bitsleft++;
  }
}

//----------

template < typename keytype, typename hashtype >
bool DiffTest ( pfHash hash, int diffbits, int reps, bool dumpCollisions )
{
  const int keybits = sizeof(keytype) * 8;
  const int hashbits = sizeof(hashtype) * 8;

  double diffcount = chooseUpToK(keybits,diffbits);
  double testcount = (diffcount * double(reps));
  double expected  = testcount / pow(2.0,double(hashbits));

  Rand r(100);

  std::vector<keytype> diffs;

  keytype k1,k2;
  hashtype h1,h2;

  printf("Testing %0.f up-to-%d-bit differentials in %d-bit keys -> %d bit hashes.\n",diffcount,diffbits,keybits,hashbits);
  printf("%d reps, %0.f total tests, expecting %2.2f random collisions",reps,testcount,expected);

  for(int i = 0; i < reps; i++)
  {
    if(i % (reps/10) == 0) printf(".");

    r.rand_p(&k1,sizeof(keytype));
    k2 = k1;

    hash(&k1,sizeof(k1),0,(uint32_t*)&h1);

    DiffTestRecurse<keytype,hashtype>(hash,k1,k2,h1,h2,0,diffbits,diffs);
  }
  printf("\n");

  bool result = true;

  result &= ProcessDifferentials(diffs,reps,dumpCollisions);

  return result;
}

//-----------------------------------------------------------------------------
// Differential distribution test - for each N-bit input differential, generate
// a large set of differential key pairs, hash them, and test the output 
// differentials using our distribution test code.

// This is a very hard test to pass - even if the hash values are well-distributed,
// the differences between hash values may not be. It's also not entirely relevant
// for testing hash functions, but it's still interesting.

// This test is a _lot_ of work, as it's essentially a full keyset test for
// each of a potentially huge number of input differentials. To speed things
// along, we do only a few distribution tests per keyset instead of the full
// grid.

// #TODO - put diagram drawing back on

template < typename keytype, typename hashtype >
void DiffDistTest ( pfHash hash, const int diffbits, int trials, double & worst, double & avg )
{
  std::vector<keytype>  keys(trials);
  std::vector<hashtype> A(trials),B(trials);

  for(int i = 0; i < trials; i++)
  {
    rand_p(&keys[i],sizeof(keytype));

    hash(&keys[i],sizeof(keytype),0,(uint32_t*)&A[i]);
  }

  //----------

  std::vector<keytype> diffs;

  keytype temp(0);

  SparseKeygenRecurse<keytype>(0,diffbits,true,temp,diffs);

  //----------

  worst = 0;
  avg = 0;

  hashtype h2;

  for(size_t j = 0; j < diffs.size(); j++)
  {
    keytype & d = diffs[j];

    for(int i = 0; i < trials; i++)
    {
      keytype k2 = keys[i] ^ d;

      hash(&k2,sizeof(k2),0,&h2);

      B[i] = A[i] ^ h2;
    }

    double dworst,davg;

    TestDistributionFast(B,dworst,davg);

    avg += davg;
    worst = (dworst > worst) ? dworst : worst;
  }

  avg /= double(diffs.size());
}

//-----------------------------------------------------------------------------
// Simpler differential-distribution test - for all 1-bit differentials,
// generate random key pairs and run full distribution/collision tests on the
// hash differentials

template < typename keytype, typename hashtype >
bool DiffDistTest2 ( pfHash hash  )
{
  Rand r(857374);

  int keybits = sizeof(keytype) * 8;
  const int keycount = 256*256*32;
  keytype k;
  
  std::vector<hashtype> hashes(keycount);
  hashtype h1,h2;

  bool result = true;

  for(int keybit = 0; keybit < keybits; keybit++)
  {
    printf("Testing bit %d\n",keybit);

    for(int i = 0; i < keycount; i++)
    {
      r.rand_p(&k,sizeof(keytype));
      
      hash(&k,sizeof(keytype),0,&h1);
      flipbit(&k,sizeof(keytype),keybit);
      hash(&k,sizeof(keytype),0,&h2);

      hashes[i] = h1 ^ h2;
    }

    result &= TestHashList<hashtype>(hashes,true,true,true);
    printf("\n");
  }

  return result;
}

//----------------------------------------------------------------------------
