#include "Stats.h"

//-----------------------------------------------------------------------------

double chooseK ( int n, int k )
{
  if(k > (n - k)) k = n - k;

  double c = 1;

  for(int i = 0; i < k; i++)
  {
    double t = double(n-i) / double(i+1);

    c *= t;
  }

    return c;
}

double chooseUpToK ( int n, int k )
{
  double c = 0;

  for(int i = 1; i <= k; i++)
  {
    c += chooseK(n,i);
  }

  return c;
}

//-----------------------------------------------------------------------------
// Distribution "score"
// TODO - big writeup of what this score means

// Basically, we're computing a constant that says "The test distribution is as
// uniform, RMS-wise, as a random distribution restricted to (1-X)*100 percent of
// the bins. This makes for a nice uniform way to rate a distribution that isn't
// dependent on the number of bins or the number of keys

// (as long as # keys > # bins * 3 or so, otherwise random fluctuations show up
// as distribution weaknesses)

double calcScore ( const int * bins, const int bincount, const int keycount )
{
  double n = bincount;
  double k = keycount;

  // compute rms value

  double r = 0;

  for(int i = 0; i < bincount; i++)
  {
    double b = bins[i];

    r += b*b;
  }

  r = sqrt(r / n);

  // compute fill factor

  double f = (k*k - 1) / (n*r*r - k);

  // rescale to (0,1) with 0 = good, 1 = bad

  return 1 - (f / n);
}


//----------------------------------------------------------------------------

void plot ( double n )
{
  double n2 = n * 1;

  if(n2 < 0) n2 = 0;

  n2 *= 100;

  if(n2 > 64) n2 = 64;

  int n3 = (int)n2;

  if(n3 == 0)
    printf(".");
  else
  {
    char x = '0' + char(n3);

    if(x > '9') x = 'X';

    printf("%c",x);
  }
}

//-----------------------------------------------------------------------------
