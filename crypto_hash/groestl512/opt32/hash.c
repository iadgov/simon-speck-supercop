/* hash.c                         March 2009
 * Implementation of Groestl-512
 * ANSI C code optimised for 32-bit machines
 * Author: Soeren S. Thomsen
 *
 * This code is placed in the public domain
 */

#include "crypto_hash.h"
#include "hash.h"
#include "tables.h"

#define COL(x,y,i,r,s)						\
  y[2*i]    = T[0*256 + (EXT_BYTE(x[2*i], 0)^r)];		\
  y[2*i+1]  = T[4*256 + (EXT_BYTE(x[2*i], 0)^r)];		\
  y[2*i]   ^= T[1*256 + EXT_BYTE(x[(2*i+2)%32], 1)];		\
  y[2*i+1] ^= T[5*256 + EXT_BYTE(x[(2*i+2)%32], 1)];		\
  y[2*i]   ^= T[2*256 + EXT_BYTE(x[(2*i+4)%32], 2)];		\
  y[2*i+1] ^= T[6*256 + EXT_BYTE(x[(2*i+4)%32], 2)];		\
  y[2*i]   ^= T[3*256 + EXT_BYTE(x[(2*i+6)%32], 3)];		\
  y[2*i+1] ^= T[7*256 + EXT_BYTE(x[(2*i+6)%32], 3)];		\
  y[2*i]   ^= T[4*256 + EXT_BYTE(x[(2*i+9)%32], 0)];		\
  y[2*i+1] ^= T[0*256 + EXT_BYTE(x[(2*i+9)%32], 0)];		\
  y[2*i]   ^= T[5*256 + EXT_BYTE(x[(2*i+11)%32], 1)];		\
  y[2*i+1] ^= T[1*256 + EXT_BYTE(x[(2*i+11)%32], 1)];		\
  y[2*i]   ^= T[6*256 + EXT_BYTE(x[(2*i+13)%32], 2)];		\
  y[2*i+1] ^= T[2*256 + EXT_BYTE(x[(2*i+13)%32], 2)];		\
  y[2*i]   ^= T[7*256 + (EXT_BYTE(x[(2*i+23)%32], 3)^s)];	\
  y[2*i+1] ^= T[3*256 + (EXT_BYTE(x[(2*i+23)%32], 3)^s)]


#define RNDP(x, y, r) do {			\
    COL(x,y, 0,r,0);				\
    COL(x,y, 1,0,0);				\
    COL(x,y, 2,0,0);				\
    COL(x,y, 3,0,0);				\
    COL(x,y, 4,0,0);				\
    COL(x,y, 5,0,0);				\
    COL(x,y, 6,0,0);				\
    COL(x,y, 7,0,0);				\
    COL(x,y, 8,0,0);				\
    COL(x,y, 9,0,0);				\
    COL(x,y,10,0,0);				\
    COL(x,y,11,0,0);				\
    COL(x,y,12,0,0);				\
    COL(x,y,13,0,0);				\
    COL(x,y,14,0,0);				\
    COL(x,y,15,0,0);				\
  } while (0)

#define RNDQ(x, y, s) do {			\
    COL(x,y, 0,0,0);				\
    COL(x,y, 1,0,0);				\
    COL(x,y, 2,0,0);				\
    COL(x,y, 3,0,0);				\
    COL(x,y, 4,0,0);				\
    COL(x,y, 5,0,(s^0xff));			\
    COL(x,y, 6,0,0);				\
    COL(x,y, 7,0,0);				\
    COL(x,y, 8,0,0);				\
    COL(x,y, 9,0,0);				\
    COL(x,y,10,0,0);				\
    COL(x,y,11,0,0);				\
    COL(x,y,12,0,0);				\
    COL(x,y,13,0,0);				\
    COL(x,y,14,0,0);				\
    COL(x,y,15,0,0);				\
  } while (0)

/* the compression function */
void F(u32 *h, const u32 *m) {
  u32 x[2*COLS];
  u32 y[2*COLS];
  u32 z[2*COLS];
  int i;

  /* y <- h + m */
  for (i = 0; i < 2*COLS; i++) {
    y[i] = h[i] ^ m[i];
  }
  
  /* z <- Q(m) */
  RNDQ(m,x, 0);
  for (i = 1; i < ROUNDS-1; i += 2) {
    RNDQ(x,z, i);
    RNDQ(z,x, (i+1));
  }
  RNDQ(x,z,13);

  /* y <- P(h+m) */
  for (i = 0; i < ROUNDS; i += 2) {
    RNDP(y,x, i);
    RNDP(x,y, (i+1));
  }
	
  /* h <- h + Q(m) + P(h+m) */
  for (i = 0; i < 2*COLS; i++) {
    h[i] ^= y[i] ^ z[i];
  }
}

/* digest up to len bytes of input (full blocks only) */
void Transform(context *ctx, 
	       const unsigned char *in, 
	       unsigned long long len) {
  /* increment block counter */
  ctx->block_counter += len/SIZE;
  
  /* digest message, one block at a time */
  for (; len >= SIZE; len -= SIZE, in += SIZE) {
    F(ctx->state, (u32*)in);
  }
}

/* given state h, do h <- P(h)+h */
void OutputTransformation(context *ctx) {
  int i;
  u32 y[2*COLS];
  u32 z[2*COLS];

  RNDP(ctx->state,y,0);
  for (i = 1; i < ROUNDS-1; i += 2) {
    RNDP(y,z, i);
    RNDP(z,y, (i+1));
  }
  RNDP(y,z,13);
  for (i = 0; i < 2*COLS; i++) {
    ctx->state[i] ^= z[i];
  }
}

/* initialise context */
int Init(context* ctx) {
  int i;

  /* set initial value */
  for (i = 0; i < 2*COLS-1; i++) ctx->state[i] = 0;
  ctx->state[2*COLS-1] = U32BIG((u32)8*DIGESTSIZE);

  /* set other variables */
  ctx->buf_ptr = 0;
  ctx->block_counter = 0;

  return 0;
}

/* update state with databitlen bits of input */
int Update(context* ctx,
	   const unsigned char* in,
	   unsigned long long len) {
  unsigned long long index = 0;

  /* if the buffer contains data that has not yet been digested, first
     add data to buffer until full */
  if (ctx->buf_ptr) {
    while (ctx->buf_ptr < SIZE && index < len) {
      ctx->buffer[ctx->buf_ptr++] = in[index++];
    }
    if (ctx->buf_ptr < SIZE) return 0;

    /* digest buffer */
    ctx->buf_ptr = 0;
    Transform(ctx, ctx->buffer, SIZE);
  }

  /* digest bulk of message */
  Transform(ctx, in+index, len-index);
  index += ((len-index)/SIZE)*SIZE;

  /* store remaining data in buffer */
  while (index < len) {
    ctx->buffer[ctx->buf_ptr++] = in[index++];
  }

  return 0;
}

/* finalise: process remaining data (including padding), perform
   output transformation, and write hash result to 'output' */
int Final(context* ctx,
	  unsigned char* out) {
  int i, j = 0;
  unsigned char *s = (unsigned char*)ctx->state;

  ctx->buffer[ctx->buf_ptr++] = 0x80;

  /* pad with '0'-bits */
  if (ctx->buf_ptr > SIZE-LENGTHFIELDLEN) {
    /* padding requires two blocks */
    while (ctx->buf_ptr < SIZE) {
      ctx->buffer[ctx->buf_ptr++] = 0;
    }
    /* digest first padding block */
    Transform(ctx, ctx->buffer, SIZE);
    ctx->buf_ptr = 0;
  }
  while (ctx->buf_ptr < SIZE-LENGTHFIELDLEN) {
    ctx->buffer[ctx->buf_ptr++] = 0;
  }

  /* length padding */
  ctx->block_counter++;
  ctx->buf_ptr = SIZE;
  while (ctx->buf_ptr > SIZE-LENGTHFIELDLEN) {
    ctx->buffer[--ctx->buf_ptr] = (unsigned char)ctx->block_counter;
    ctx->block_counter >>= 8;
  }

  /* digest final padding block */
  Transform(ctx, ctx->buffer, SIZE);
  /* perform output transformation */
  OutputTransformation(ctx);

  /* store hash result in output */
  for (i = SIZE-DIGESTSIZE; i < SIZE; i++,j++) {
    out[j] = s[i];
  }

  /* zeroise relevant variables and deallocate memory */
  for (i = 0; i < COLS; i++) {
    ctx->state[i] = 0;
  }
  for (i = 0; i < SIZE; i++) {
    ctx->buffer[i] = 0;
  }

  return 0;
}

/* hash bit sequence */
int crypto_hash(unsigned char *out,
		const unsigned char *in,
		unsigned long long len) {
  int ret;
  context ctx;

  /* initialise */
  if ((ret = Init(&ctx)) < 0)
    return ret;

  /* process message */
  if ((ret = Update(&ctx, in, len)) < 0)
    return ret;

  /* finalise */
  ret = Final(&ctx, out);

  return ret;
}
