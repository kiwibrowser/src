#include "KeysetTest.h"

#include "Platform.h"
#include "Random.h"

#include <map>
#include <set>

//-----------------------------------------------------------------------------
// This should hopefully be a thorough and uambiguous test of whether a hash
// is correctly implemented on a given platform

bool VerificationTest ( pfHash hash, const int hashbits, uint32_t expected, bool verbose )
{
  const int hashbytes = hashbits / 8;

  uint8_t * key    = new uint8_t[256];
  uint8_t * hashes = new uint8_t[hashbytes * 256];
  uint8_t * final  = new uint8_t[hashbytes];

  memset(key,0,256);
  memset(hashes,0,hashbytes*256);
  memset(final,0,hashbytes);

  // Hash keys of the form {0}, {0,1}, {0,1,2}... up to N=255,using 256-N as
  // the seed

  for(int i = 0; i < 256; i++)
  {
    key[i] = (uint8_t)i;

    hash(key,i,256-i,&hashes[i*hashbytes]);
  }

  // Then hash the result array

  hash(hashes,hashbytes*256,0,final);

  // The first four bytes of that hash, interpreted as a little-endian integer, is our
  // verification value

  uint32_t verification = (final[0] << 0) | (final[1] << 8) | (final[2] << 16) | (final[3] << 24);

  delete [] key;
  delete [] hashes;
  delete [] final;

  //----------

  if(expected != verification)
  {
    if(verbose) printf("Verification value 0x%08X : Failed! (Expected 0x%08x)\n",verification,expected);
    return false;
  }
  else
  {
    if(verbose) printf("Verification value 0x%08X : Passed!\n",verification);
    return true;
  }
}

//----------------------------------------------------------------------------
// Basic sanity checks -

// A hash function should not be reading outside the bounds of the key.

// Flipping a bit of a key should, with overwhelmingly high probability,
// result in a different hash.

// Hashing the same key twice should always produce the same result.

// The memory alignment of the key should not affect the hash result.

bool SanityTest ( pfHash hash, const int hashbits )
{
  printf("Running sanity check 1");
  
  Rand r(883741);

  bool result = true;

  const int hashbytes = hashbits/8;
  const int reps = 10;
  const int keymax = 256;
  const int pad = 16;
  const int buflen = keymax + pad*3;
  
  uint8_t * buffer1 = new uint8_t[buflen];
  uint8_t * buffer2 = new uint8_t[buflen];

  uint8_t * hash1 = new uint8_t[hashbytes];
  uint8_t * hash2 = new uint8_t[hashbytes];

  //----------
  
  for(int irep = 0; irep < reps; irep++)
  {
    if(irep % (reps/10) == 0) printf(".");

    for(int len = 4; len <= keymax; len++)
    {
      for(int offset = pad; offset < pad*2; offset++)
      {
        uint8_t * key1 = &buffer1[pad];
        uint8_t * key2 = &buffer2[pad+offset];

        r.rand_p(buffer1,buflen);
        r.rand_p(buffer2,buflen);

        memcpy(key2,key1,len);

        hash(key1,len,0,hash1);

        for(int bit = 0; bit < (len * 8); bit++)
        {
          // Flip a bit, hash the key -> we should get a different result.

          flipbit(key2,len,bit);
          hash(key2,len,0,hash2);

          if(memcmp(hash1,hash2,hashbytes) == 0)
          {
            result = false;
          }

          // Flip it back, hash again -> we should get the original result.

          flipbit(key2,len,bit);
          hash(key2,len,0,hash2);

          if(memcmp(hash1,hash2,hashbytes) != 0)
          {
            result = false;
          }
        }
      }
    }
  }

  if(result == false)
  {
    printf("*********FAIL*********\n");
  }
  else
  {
    printf("PASS\n");
  }

  delete [] buffer1;
  delete [] buffer2;

  delete [] hash1;
  delete [] hash2;

  return result;
}

//----------------------------------------------------------------------------
// Appending zero bytes to a key should always cause it to produce a different
// hash value

void AppendedZeroesTest ( pfHash hash, const int hashbits )
{
  printf("Running sanity check 2");
  
  Rand r(173994);

  const int hashbytes = hashbits/8;

  for(int rep = 0; rep < 100; rep++)
  {
    if(rep % 10 == 0) printf(".");

    unsigned char key[256];

    memset(key,0,sizeof(key));

    r.rand_p(key,32);

    uint32_t h1[16];
    uint32_t h2[16];

    memset(h1,0,hashbytes);
    memset(h2,0,hashbytes);

    for(int i = 0; i < 32; i++)
    {
      hash(key,32+i,0,h1);

      if(memcmp(h1,h2,hashbytes) == 0)
      {
        printf("\n*********FAIL*********\n");
        return;
      }

      memcpy(h2,h1,hashbytes);
    }
  }

  printf("PASS\n");
}

//-----------------------------------------------------------------------------
// Generate all keys of up to N bytes containing two non-zero bytes

void TwoBytesKeygen ( int maxlen, KeyCallback & c )
{
  //----------
  // Compute # of keys

  int keycount = 0;

  for(int i = 2; i <= maxlen; i++) keycount += (int)chooseK(i,2);

  keycount *= 255*255;

  for(int i = 2; i <= maxlen; i++) keycount += i*255;

  printf("Keyset 'TwoBytes' - up-to-%d-byte keys, %d total keys\n",maxlen, keycount);

  c.reserve(keycount);

  //----------
  // Add all keys with one non-zero byte

  uint8_t key[256];

  memset(key,0,256);

  for(int keylen = 2; keylen <= maxlen; keylen++)
  for(int byteA = 0; byteA < keylen; byteA++)
  {
    for(int valA = 1; valA <= 255; valA++)
    {
      key[byteA] = (uint8_t)valA;

      c(key,keylen);
    }

    key[byteA] = 0;
  }

  //----------
  // Add all keys with two non-zero bytes

  for(int keylen = 2; keylen <= maxlen; keylen++)
  for(int byteA = 0; byteA < keylen-1; byteA++)
  for(int byteB = byteA+1; byteB < keylen; byteB++)
  {
    for(int valA = 1; valA <= 255; valA++)
    {
      key[byteA] = (uint8_t)valA;

      for(int valB = 1; valB <= 255; valB++)
      {
        key[byteB] = (uint8_t)valB;
        c(key,keylen);
      }

      key[byteB] = 0;
    }

    key[byteA] = 0;
  }
}

//-----------------------------------------------------------------------------

template< typename hashtype >
void DumpCollisionMap ( CollisionMap<hashtype,ByteVec> & cmap )
{
  typedef CollisionMap<hashtype,ByteVec> cmap_t;

  for(typename cmap_t::iterator it = cmap.begin(); it != cmap.end(); ++it)
  {
    const hashtype & hash = (*it).first;

    printf("Hash - ");
    printbytes(&hash,sizeof(hashtype));
    printf("\n");

    std::vector<ByteVec> & keys = (*it).second;

    for(int i = 0; i < (int)keys.size(); i++)
    {
      ByteVec & key = keys[i];

      printf("Key  - ");
      printbytes(&key[0],(int)key.size());
      printf("\n");
    }
    printf("\n");
  }

}

// test code

void ReportCollisions ( pfHash hash )
{
  printf("Hashing keyset\n");

  std::vector<uint128_t> hashes;

  HashCallback<uint128_t> c(hash,hashes);

  TwoBytesKeygen(20,c);

  printf("%d hashes\n",(int)hashes.size());

  printf("Finding collisions\n");

  HashSet<uint128_t> collisions;

  FindCollisions(hashes,collisions,1000);

  printf("%d collisions\n",(int)collisions.size());

  printf("Mapping collisions\n");

  CollisionMap<uint128_t,ByteVec> cmap;

  CollisionCallback<uint128_t> c2(hash,collisions,cmap);

  TwoBytesKeygen(20,c2);

  printf("Dumping collisions\n");

  DumpCollisionMap(cmap);
}
