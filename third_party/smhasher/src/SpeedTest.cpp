#include "SpeedTest.h"

#include "Random.h"

#include <stdio.h>   // for printf
#include <memory.h>  // for memset
#include <math.h>    // for sqrt
#include <algorithm> // for sort

//-----------------------------------------------------------------------------
// We view our timing values as a series of random variables V that has been
// contaminated with occasional outliers due to cache misses, thread
// preemption, etcetera. To filter out the outliers, we search for the largest
// subset of V such that all its values are within three standard deviations
// of the mean.

double CalcMean ( std::vector<double> & v )
{
  double mean = 0;
  
  for(int i = 0; i < (int)v.size(); i++)
  {
    mean += v[i];
  }
  
  mean /= double(v.size());
  
  return mean;
}

double CalcMean ( std::vector<double> & v, int a, int b )
{
  double mean = 0;
  
  for(int i = a; i <= b; i++)
  {
    mean += v[i];
  }
  
  mean /= (b-a+1);
  
  return mean;
}

double CalcStdv ( std::vector<double> & v, int a, int b )
{
  double mean = CalcMean(v,a,b);

  double stdv = 0;
  
  for(int i = a; i <= b; i++)
  {
    double x = v[i] - mean;
    
    stdv += x*x;
  }
  
  stdv = sqrt(stdv / (b-a+1));
  
  return stdv;
}

// Return true if the largest value in v[0,len) is more than three
// standard deviations from the mean

bool ContainsOutlier ( std::vector<double> & v, size_t len )
{
  double mean = 0;
  
  for(size_t i = 0; i < len; i++)
  {
    mean += v[i];
  }
  
  mean /= double(len);
  
  double stdv = 0;
  
  for(size_t i = 0; i < len; i++)
  {
    double x = v[i] - mean;
    stdv += x*x;
  }
  
  stdv = sqrt(stdv / double(len));

  double cutoff = mean + stdv*3;
  
  return v[len-1] > cutoff;  
}

// Do a binary search to find the largest subset of v that does not contain
// outliers.

void FilterOutliers ( std::vector<double> & v )
{
  std::sort(v.begin(),v.end());
  
  size_t len = 0;
  
  for(size_t x = 0x40000000; x; x = x >> 1 )
  {
    if((len | x) >= v.size()) continue;
    
    if(!ContainsOutlier(v,len | x))
    {
      len |= x;
    }
  }
  
  v.resize(len);
}

// Iteratively tighten the set to find a subset that does not contain
// outliers. I'm not positive this works correctly in all cases.

void FilterOutliers2 ( std::vector<double> & v )
{
  std::sort(v.begin(),v.end());
  
  int a = 0;
  int b = (int)(v.size() - 1);
  
  for(int i = 0; i < 10; i++)
  {
    //printf("%d %d\n",a,b);
  
    double mean = CalcMean(v,a,b);
    double stdv = CalcStdv(v,a,b);
    
    double cutA = mean - stdv*3;  
    double cutB = mean + stdv*3;
    
    while((a < b) && (v[a] < cutA)) a++;
    while((b > a) && (v[b] > cutB)) b--;
  }
  
  std::vector<double> v2;
  
  v2.insert(v2.begin(),v.begin()+a,v.begin()+b+1);
  
  v.swap(v2);
}

//-----------------------------------------------------------------------------
// We really want the rdtsc() calls to bracket the function call as tightly
// as possible, but that's hard to do portably. We'll try and get as close as
// possible by marking the function as NEVER_INLINE (to keep the optimizer from
// moving it) and marking the timing variables as "volatile register".

NEVER_INLINE int64_t timehash ( pfHash hash, const void * key, int len, int seed )
{
  volatile register int64_t begin,end;
  
  uint32_t temp[16];
  
  begin = rdtsc();
  
  hash(key,len,seed,temp);
  
  end = rdtsc();
  
  return end-begin;
}

//-----------------------------------------------------------------------------

double SpeedTest ( pfHash hash, uint32_t seed, const int trials, const int blocksize, const int align )
{
  Rand r(seed);
  
  uint8_t * buf = new uint8_t[blocksize + 512];

  uint64_t t1 = reinterpret_cast<uint64_t>(buf);
  
  t1 = (t1 + 255) & BIG_CONSTANT(0xFFFFFFFFFFFFFF00);
  t1 += align;
  
  uint8_t * block = reinterpret_cast<uint8_t*>(t1);

  r.rand_p(block,blocksize);

  //----------

  std::vector<double> times;
  times.reserve(trials);

  for(int itrial = 0; itrial < trials; itrial++)
  {
    r.rand_p(block,blocksize);
    
    double t = (double)timehash(hash,block,blocksize,itrial);
    
    if(t > 0) times.push_back(t);
  }

  //----------
  
  std::sort(times.begin(),times.end());
  
  FilterOutliers(times);
  
  delete [] buf;
  
  return CalcMean(times);
}

//-----------------------------------------------------------------------------
// 256k blocks seem to give the best results.

void BulkSpeedTest ( pfHash hash, uint32_t seed )
{
  const int trials = 2999;
  const int blocksize = 256 * 1024;

  printf("Bulk speed test - %d-byte keys\n",blocksize);

  for(int align = 0; align < 8; align++)
  {
    double cycles = SpeedTest(hash,seed,trials,blocksize,align);
    
    double bestbpc = double(blocksize)/cycles;
    
    double bestbps = (bestbpc * 3000000000.0 / 1048576.0);
    printf("Alignment %2d - %6.3f bytes/cycle - %7.2f MiB/sec @ 3 ghz\n",align,bestbpc,bestbps);
  }
}

//-----------------------------------------------------------------------------

void TinySpeedTest ( pfHash hash, int hashsize, int keysize, uint32_t seed, bool verbose, double & /*outCycles*/ )
{
  const int trials = 999999;

  if(verbose) printf("Small key speed test - %4d-byte keys - ",keysize);
  
  double cycles = SpeedTest(hash,seed,trials,keysize,0);
  
  printf("%8.2f cycles/hash\n",cycles);  
}

//-----------------------------------------------------------------------------
