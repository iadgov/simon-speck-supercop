/*
 * crypto_smult/try.c version 20080913
 * D. J. Bernstein
 * Public domain.
 */

#include <stdlib.h>
#include "crypto_smult.h"

extern unsigned char *alignedcalloc(unsigned long long);

const char *primitiveimplementation = crypto_smult_IMPLEMENTATION;

#define mlen crypto_smult_SCALARBYTES
#define nlen crypto_smult_SCALARBYTES
#define plen crypto_smult_BYTES
#define qlen crypto_smult_BYTES
#define rlen crypto_smult_BYTES

static unsigned char *m;
static unsigned char *n;
static unsigned char *p;
static unsigned char *q;
static unsigned char *r;

static unsigned char *m2;
static unsigned char *n2;
static unsigned char *p2;
static unsigned char *q2;
static unsigned char *r2;

void allocate(void)
{
  m = alignedcalloc(mlen);
  n = alignedcalloc(nlen);
  p = alignedcalloc(plen);
  q = alignedcalloc(qlen);
  r = alignedcalloc(rlen);
  m2 = alignedcalloc(mlen + crypto_smult_BYTES);
  n2 = alignedcalloc(nlen + crypto_smult_BYTES);
  p2 = alignedcalloc(plen + crypto_smult_BYTES);
  q2 = alignedcalloc(qlen + crypto_smult_BYTES);
  r2 = alignedcalloc(rlen + crypto_smult_BYTES);
}

void predoit(void)
{
}

void doit(void)
{
  crypto_smult(q,n,p);
  crypto_smult_base(r,n);
}

char checksum[crypto_smult_BYTES * 2 + 1];

const char *checksum_compute(void)
{
  long long i;
  long long j;
  long long tests;

  for (i = 0;i < mlen;++i) m[i] = i;
  for (i = 0;i < nlen;++i) n[i] = i + 1;
  for (i = 0;i < plen;++i) p[i] = i + 2;
  for (i = 0;i < qlen;++i) q[i] = i + 3;
  for (i = 0;i < rlen;++i) r[i] = i + 4;

  for (i = -16;i < 0;++i) p[i] = random();
  for (i = -16;i < 0;++i) n[i] = random();
  for (i = plen;i < plen + 16;++i) p[i] = random();
  for (i = nlen;i < nlen + 16;++i) n[i] = random();
  for (i = -16;i < plen + 16;++i) p2[i] = p[i];
  for (i = -16;i < nlen + 16;++i) n2[i] = n[i];

  if (crypto_smult_base(p,n) != 0) return "crypto_smult_base returns nonzero";

  for (i = -16;i < nlen + 16;++i) if (n2[i] != n[i]) return "crypto_smult_base overwrites input";
  for (i = -16;i < 0;++i) if (p2[i] != p[i]) return "crypto_smult_base writes before output";
  for (i = plen;i < plen + 16;++i) if (p2[i] != p[i]) return "crypto_smult_base writes after output";

  for (tests = 0;tests < 100;++tests) {
    for (i = -16;i < 0;++i) q[i] = random();
    for (i = -16;i < 0;++i) p[i] = random();
    for (i = -16;i < 0;++i) m[i] = random();
    for (i = qlen;i < qlen + 16;++i) q[i] = random();
    for (i = plen;i < plen + 16;++i) p[i] = random();
    for (i = mlen;i < mlen + 16;++i) m[i] = random();
    for (i = -16;i < qlen + 16;++i) q2[i] = q[i];
    for (i = -16;i < plen + 16;++i) p2[i] = p[i];
    for (i = -16;i < mlen + 16;++i) m2[i] = m[i];

    if (crypto_smult(q,m,p) != 0) return "crypto_smult returns nonzero";

    for (i = -16;i < mlen + 16;++i) if (m2[i] != m[i]) return "crypto_smult overwrites n input";
    for (i = -16;i < plen + 16;++i) if (p2[i] != p[i]) return "crypto_smult overwrites p input";
    for (i = -16;i < 0;++i) if (q2[i] != q[i]) return "crypto_smult writes before output";
    for (i = qlen;i < qlen + 16;++i) if (q2[i] != q[i]) return "crypto_smult writes after output";

    if (crypto_smult(m2,m2,p) != 0) return "crypto_smult returns nonzero";
    for (i = 0;i < qlen;++i) if (q[i] != m2[i]) return "crypto_smult does not handle n overlap";
    for (i = 0;i < qlen;++i) m2[i] = m[i];

    if (crypto_smult(p2,m2,p2) != 0) return "crypto_smult returns nonzero";
    for (i = 0;i < qlen;++i) if (q[i] != p2[i]) return "crypto_smult does not handle p overlap";

    if (crypto_smult(r,n,q) != 0) return "crypto_smult returns nonzero";
    if (crypto_smult(q,n,p) != 0) return "crypto_smult returns nonzero";
    if (crypto_smult(p,m,q) != 0) return "crypto_smult returns nonzero";
    for (j = 0;j < plen;++j) if (p[j] != r[j]) return "crypto_smult not associative";
    for (j = 0;j < mlen;++j) m[j] ^= q[j % qlen];
    for (j = 0;j < nlen;++j) n[j] ^= p[j % plen];
  }

  for (i = 0;i < crypto_smult_BYTES;++i) {
    checksum[2 * i] = "0123456789abcdef"[15 & (p[i] >> 4)];
    checksum[2 * i + 1] = "0123456789abcdef"[15 & p[i]];
  }
  checksum[2 * i] = 0;
  return 0;
}