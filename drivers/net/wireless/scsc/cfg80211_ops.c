/*
 * Cryptographic API.
 *
 * RIPEMD-256 - RACE Integrity Primitives Evaluation Message Digest.
 *
 * Based on the reference implementation by Antoon Bosselaers, ESAT-COSIC
 *
 * Copyright (c) 2008 Adrian-Ken Rueegsegger <ken@codelabs.ch>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */
#include <crypto/internal/hash.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <asm/byteorder.h>

#include "ripemd.h"

struct rmd256_ctx {
	u64 byte_count;
	u32 state[8];
	__le32 buffer[16];
};

#define K1  RMD_K1
#define K2  RMD_K2
#define K3  RMD_K3
#define K4  RMD_K4
#define KK1 RMD_K6
#define KK2 RMD_K7
#define KK3 RMD_K8
#define KK4 RMD_K1

#define F1(x, y, z) (x ^ y ^ z)		/* XOR */
#define F2(x, y, z) (z ^ (x & (y ^ z)))	/* x ? y : z */
#define F3(x, y, z) ((x | ~y) ^ z)
#define F4(x, y, z) (y ^ (z & (x ^ y)))	/* z ? x : y */

#define ROUND(a, b, c, d, f, k, x, s)  { \
	(a) += f((b), (c), (d)) + le32_to_cpup(&(x)) + (k); \
	(a) = rol32((a), (s)); \
}

static void rmd256_transform(u32 *state, const __le32 *in)
{
	u32 aa, bb, cc, dd, aaa, bbb, ccc, ddd, tmp;

	/* Initialize left lane */
	aa = state[0];
	bb = state[1];
	cc = state[2];
	dd = state[3];

	/* Initialize right lane */
	aaa = state[4];
	bbb = state[5];
	ccc = state[6];
	ddd = state[7];

	/* round 1: left lane */
	ROUND(aa, bb, cc, dd, F1, K1, in[0],  11);
	ROUND(dd, aa, bb, cc, F1, K1, in[1],  14);
	ROUND(cc, dd, aa, bb, F1, K1, in[2],  15);
	ROUND(bb, cc, dd, aa, F1, K1, in[3],  12);
	ROUND(aa, bb, cc, dd, F1, K1, in[4],   5);
	ROUND(dd, aa, bb, cc, F1, K1, in[5],   8);
	ROUND(cc, dd, aa, bb, F1, K1, in[6],   7);
	ROUND(bb, cc, dd, aa, F1, K1, in[7],   9);
	ROUND(aa, bb, cc, dd, F1, K1, in[8],  11);
	ROUND(dd, aa, bb, cc, F1, K1, in[9],  13);
	ROUND(cc, dd, aa, bb, F1, K1, in[10], 14);
	ROUND(bb, cc, dd, aa, F1, K1, in[11], 15);
	ROUND(aa, bb, cc, dd, F1, K1, in[12],  6);
	ROUND(dd, aa, bb, cc, F1, K1, in[13],  7);
	ROUND(cc, dd, aa, bb, F1, K1, in[14],  9);
	ROUND(bb, cc, dd, aa, F1, K1, in[15],  8);

	/* round 1: right lane */
	ROUND(aaa, bbb, ccc, ddd, F4, KK1, in[5],   8);
	ROUND(ddd, aaa, bbb, ccc, F4, KK1, in[14],  9);
	ROUND(ccc, ddd, aaa, bbb, F4, KK1, in[7],   9);
	ROUND(bbb, ccc, ddd, aaa, F4, KK1, in[0],  11);
	ROUND(aaa, bbb, ccc, ddd, F4, KK1, in[9],  13);
	ROUND(ddd, aaa, bbb, ccc, F4, KK1, in[2],  15);
	ROUND(ccc, ddd, aaa, bbb, F4, KK1, in[11], 15);
	ROUND(bbb, ccc, ddd, aaa, F4, KK1, in[4],   5);
	ROUND(aaa, bbb, ccc, ddd, F4, KK1, in[13],  7);
	ROUND(ddd, aaa, bbb, ccc, F4, KK1, in[6],   7);
	ROUND(ccc, ddd, aaa, bbb, F4, KK1, in[15],  8);
	ROUND(bbb, ccc, ddd, aaa, F4, KK1, in[8],  11);
	ROUND(aaa, bbb, ccc, ddd, F4, KK1, in[1],  14);
	ROUND(ddd, aaa, bbb, ccc, F4, KK1, in[10], 14);
	ROUND(ccc, ddd, aaa, bbb, F4, KK1, in[3],  12);
	ROUND(bbb, ccc, ddd, aaa, F4, KK1, in[12],  6);

	/* Swap contents of "a" registers */
	tmp = aa; aa = aaa; aaa = tmp;

	/* round 2: left lane */
	ROUND(aa, bb, cc, dd, F2, K2, in[7],   7);
	ROUND(dd, aa, bb, cc, F2, K2, in[4],   6);
	ROUND(cc, dd, aa, bb, F2, K2, in[13],  8);
	ROUND(bb, cc, dd, aa, F2, K2, in[1],  13);
	ROUND(aa, bb, cc, dd, F2, K2, in[10], 11);
	ROUND(dd, aa, bb, cc, F2, K2, in[6],   9);
	ROUND(cc, dd, aa, bb, F2, K2, in[15],  7);
	ROUND(bb, cc, dd, aa, F2, K2, in[3],  15);
	ROUND(aa, bb, cc, dd, F2, K2, in[12],  7);
	ROUND(dd, aa, bb, cc, F2, K2, in[0],  12);
	ROUND(cc, dd, aa, bb, F2, K2, in[9],  15);
	ROUND(bb, cc, dd, aa, F2, K2, in[5],   9);
	ROUND(aa, bb, cc, dd, F2, K2, in[2],  11);
	ROUND(dd, aa, bb, cc, F2, K2, in[14],  7);
	ROUND(cc, dd, aa, bb, F2, K2, in[11], 13);
	ROUND(bb, cc, dd, aa, F2, K2, in[8],  12);

	/* round 2: right lane */
	ROUND(aaa, bbb, ccc, ddd, F3, KK2, in[6],   9);
	ROUND(ddd, aaa, bbb, ccc, F3, KK2, in[11], 13);
	ROUND(ccc, ddd, aaa, bbb, F3, KK2, in[3],  15);
	ROUND(bbb, ccc, ddd, aaa, F3, KK2, in[7],   7);
	ROUND(aaa, bbb, ccc, ddd, F3, KK2, in[0],  12);
	ROUND(ddd, aaa, bbb, ccc, F3, KK2, in[13],  8);
	ROUND(ccc, ddd, aaa, bbb, F3, KK2, in[5],   9);
	ROUND(bbb, ccc, ddd, aaa, F3, KK2, in[10], 11);
	ROUND(aaa, bbb, ccc, ddd, F3, KK2, in[14],  7);
	ROUND(ddd, aaa, bbb, ccc, F3, KK2, in[15],  7);
	ROUND(ccc, ddd, aaa, bbb, F3, KK2, in[8],  12);
	ROUND(bbb, ccc, ddd, aaa, F3, KK2, in[12],  7);
	ROUND(aaa, bbb, ccc, ddd, F3, KK2, in[4],   6);
	ROUND(ddd, aaa, bbb, ccc, F3, KK2, in[9],  15);
	ROUND(ccc, ddd, aaa, bbb, F3, KK2, in[1],  13);
	ROUND(bbb, ccc, ddd, aaa, F3, KK2, in[2],  11);

	/* Swap contents of "b" registers */
	tmp = bb; bb = bbb; bbb = tmp;

	/* round 3: left lane */
	ROUND(aa, bb, cc, dd, F3, K3, in[3],  11);
	ROUND(dd, aa, bb, cc, F3, K3, in[10], 13);
	ROUND(cc, dd, aa, bb, F3, K3, in[14],  6);
	ROUND(bb, cc, dd, aa, F3, K3, in[4],   7);
	ROUND(aa, bb, cc, dd, F3, K3, in[9],  14);
	ROUND(dd, aa, bb, cc, F3, K3, in[15],  9);
	ROUND(cc, dd, aa, bb, F3, K3, in[8],  13);
	ROUND(bb, cc, dd, aa, F3, K3, in[1],  15);
	ROUND(aa, bb, cc, dd, F3, K3, in[2],  14);
	ROUND(dd, aa, bb, cc, F3, K3, in[7],   8);
	ROUND(cc, dd, aa, bb, F3, K3, in[0],  13);
	ROUND(bb, cc, dd, aa, F3, K3, in[6],   6);
	ROUND(aa, bb, cc, dd, F3, K3, in[13],  5);
	ROUND(dd, aa, bb, cc, F3, K3, in[11], 12);
	ROUND(cc, dd, aa, bb, F3, K3, in[5],   7);
	ROUND(bb, cc, dd, aa, F3, K3, in[12],  5);

	/* round 3: right lane */
	ROUND(aaa, bbb, ccc, ddd, F2, KK3, in[15],  9);
	ROUND(ddd, aaa, bbb, ccc, F2, KK3, in[5],   7);
	ROUND(ccc, ddd, aaa, bbb, F2, KK3, in[1],  15);
	ROUND(bbb, ccc, ddd, aaa, F2, KK3, in[3],  11);
	ROUND(aaa, bbb, ccc, ddd, F2, KK3, in[7],   8);
	ROUND(ddd, aaa, bbb, ccc, F2, KK3, in[14],  6);
	ROUND(ccc, ddd, aaa, bbb, F2, KK3, in[6],   6);
	ROUND(bbb, ccc, ddd, aaa, F2, KK3, in[9],  14);
	ROUND(aaa, bbb, ccc, ddd, F2, KK3, in[11], 12);
	ROUND(ddd, aaa, bbb, ccc, F2, KK3, in[8],  13);
	ROUND(ccc, ddd, aaa, bbb, F2, KK3, in[12],  5);
	ROUND(bbb, ccc, ddd, aaa, F2, KK3, in[2],  14);
	ROUND(aaa, bbb, ccc, ddd, F2, KK3, in[10], 13);
	ROUND(ddd, aaa, bbb, ccc, F2, KK3, in[0],  13);
	ROUND(ccc, ddd, aaa, bbb, F2, KK3, in[4],   7);
	ROUND(bbb, ccc, ddd, aaa, F2, KK3, in[13],  5);

	/* Swap contents of "c" registers */
	tmp = cc; cc = ccc; ccc = tmp;

	/* round 4: left lane */
	ROUND(aa, bb, cc, dd, F4, K4, in[1],  11);
	ROUND(dd, aa, bb, cc, F4, K4, in[9],  12);
	ROUND(cc, dd, aa, bb, F4, K4, in[11], 14);
	ROUND(bb, cc, dd, aa, F4, K4, in[10], 15);
	ROUND(aa, bb, cc, dd, F4, K4, in[0],  14);
	ROUND(dd, aa, bb, cc, F4, K4, in[8],  15);
	ROUND(cc, dd, aa, bb, F4, K4, in[12],  9);
	ROUND(bb, cc, dd, aa, F4, K4, in[4],   8);
	ROUND(aa, bb, cc, dd, F4, K4, in[13],  9);
	ROUND(dd, aa, bb, cc, F4, K4, in[3],  14);
	ROUND(cc, dd, aa, bb, F4, K4, in[7],   5);
	ROUND(bb, cc, dd, aa, F4, K4, in[15],  6);
	ROUND(aa, bb, cc, dd, F4, K4, in[14],  8);
	ROUND(dd, aa, bb, cc, F4, K4, in[5],   6);
	ROUND(cc, dd, aa, bb, F4, K4, in[6],   5);
	ROUND(bb, cc, dd, aa, F4, K4, in[2],  12);

	/* round 4: right lane */
	ROUND(aaa, bbb, ccc, ddd, F1, KK4, in[8],  15);
	ROUND(ddd, aaa, bbb, ccc, F1, KK4, in[6],   5);
	ROUND(ccc, ddd, aaa, bbb, F1, KK4, in[4],   8);
	ROUND(bbb, ccc, ddd, aaa, F1, KK4, in[1],  11);
	ROUND(aaa, bbb, ccc, ddd, F1, KK4, in[3],  14);
	ROUND(ddd, aaa, bbb, ccc, F1, KK4, in[11], 14);
	ROUND(ccc, ddd, aaa, bbb, F1, KK4, in[15],  6);
	ROUND(bbb, ccc, ddd, aaa, F1, KK4, in[0],  14);
	ROUND(aaa, bbb, ccc, ddd, F1, KK4, in[5],   6);
	ROUND(ddd, aaa, bbb, ccc, F1, KK4, in[12],  9);
	ROUND(ccc, ddd, aaa, bbb, F1, KK4, in[2],  12);
	ROUND(bbb, ccc, ddd, aaa, F1, KK4, in[13],  9);
	ROUND(aaa, bbb, ccc, ddd, F1, KK4, in[9],  12);
	ROUND(ddd, aaa, bbb, ccc, F1, KK4, in[7],   5);
	ROUND(ccc, ddd, aaa, bbb, F1, KK4, in[10], 15);
	ROUND(bbb, ccc, ddd, aaa, F1, KK4, in[14],  8);

	/* Swap contents of "d" registers */
	tmp = dd; dd = ddd; ddd = tmp;

	/* combine results */
	state[0] += aa;
	state[1] += bb;
	state[2] += cc;
	state[3] += dd;
	state[4] += aaa;
	state[5] += bbb;
	state[6] += ccc;
	state[7] += ddd;

	return;
}

static int rmd256_init(struct shash_desc *desc)
{
	struct rmd256_ctx *rctx = shash_desc_ctx(desc);

	rctx->byte_count = 0;

	rctx->state[0] = RMD_H0;
	rctx->state[1] = RMD_H1;
	rctx->state[2] = RMD_H2;
	rctx->state[3] = RMD_H3;
	rctx->state[4] = RMD_H5;
	rctx->state[5] = RMD_H6;
	rctx->state[6] = RMD_H7;
	rctx->state[7] = RMD_H8;

	memset(rctx->buffer, 0, sizeof(rctx->buffer));

	return 0;
}

static int rmd256_update(struct shash_desc *desc, const u8 *data,
			 unsigned int len)
{
	struct rmd256_ctx *rctx = shash_desc_ctx(desc);
	const u32 avail = sizeof(rctx->buffer) - (rctx->byte_count & 0x3f);

	rctx->byte_count += len;

	/* Enough space in buffer? If so copy and we're done */
	if (avail > len) {
		memcpy((char *)rctx->buffer + (sizeof(rctx->buffer) - avail),
		       data, len);
		goto out;
	}

	memcpy((char *)rctx->buffer + (sizeof(rctx->buffer) - avail),
	       data, avail);

	rmd256_transform(rctx->state, rctx->buffer);
	data += avail;
	len -= avail;

	while (len >= sizeof(rctx->buffer)) {
		memcpy(rctx->buffer, data, sizeof(rctx->buffer));
		rmd256_transform(rctx->state, rctx->buffer);
		data += sizeof(rctx->buffer);
		len -= sizeof(rctx->buffer);
	}

	memcpy(rctx->buffer, data, len);

out:
	return 0;
}

/* Add padding and return the message digest. */
static int rmd256_final(struct shash_desc *desc, u8 *out)
{
	struct rmd256_ctx *rctx = shash_desc_ctx(desc);
	u32 i, index, padlen;
	__le64 bits;
	__le32 *dst = (__le32 *)out;
	static const u8 padding[64] = { 0x80, };

	bits = cpu_to_le64(rctx->byte_count << 3);

	/* Pad out to 56 mod 64 */
	index = rctx->byte_count & 0x3f;
	padlen = (index < 56) ? (56 - index) : ((64+56) - index);
	rmd256_update(desc, padding, padlen);

	/* Append length */
	rmd256_update(desc, (const u8 *)&bits, sizeof(bits));

	/* Store state in digest */
	for (i = 0; i < 8; i++)
		dst[i] = cpu_to_le32p(&rctx->state[i]);

	/* Wipe context */
	memset(rctx, 0, sizeof(*rctx));

	return 0;
}

static struct shash_alg alg = {
	.digestsize	=	RMD256_DIGEST_SIZE,
	.init		=	rmd256_init,
	.update		=	rmd256_update,
	.final		=	rmd256_final,
	.descsize	=	sizeof(struct rmd256_ctx),
	.base		=	{
		.cra_name	 =	"rmd256",
		.cra_flags	 =	CRYPTO_ALG_TYPE_SHASH,
		.cra_blocksize	 =	RMD256_BLOCK_SIZE,
		.cra_module	 =	THIS_MODULE,
	}
};

static int __init rmd256_mod_init(void)
{
	return crypto_register_shash(&alg);
}

static void __exit rmd256_mod_fini(void)
{
	crypto_unregister_shash(&alg);
}

module_init(rmd256_mod_init);
module_exit(rmd256_mod_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adrian-Ken Rueegsegger <ken@codelabs.ch>");
MODULE_DESCRIPTION("RIPEMD-256 Message Digest");
MODULE_ALIAS_CRYPTO("rmd256");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * Cryptographic API.
 *
 * RIPEMD-320 - RACE Integrity Primitives Evaluation Message Digest.
 *
 * Based on the reference implementation by Antoon Bosselaers, ESAT-COSIC
 *
 * Copyright (c) 2008 Adrian-Ken Rueegsegger <ken@codelabs.ch>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */
#include <crypto/internal/hash.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <asm/byteorder.h>

#include "ripemd.h"

struct rmd320_ctx {
	u64 byte_count;
	u32 state[10];
	__le32 buffer[16];
};

#define K1  RMD_K1
#define K2  RMD_K2
#define K3  RMD_K3
#define K4  RMD_K4
#define K5  RMD_K5
#define KK1 RMD_K6
#define KK2 RMD_K7
#define KK3 RMD_K8
#define KK4 RMD_K9
#define KK5 RMD_K1

#define F1(x, y, z) (x ^ y ^ z)		/* XOR */
#define F2(x, y, z) (z ^ (x & (y ^ z)))	/* x ? y : z */
#define F3(x, y, z) ((x | ~y) ^ z)
#define F4(x, y, z) (y ^ (z & (x ^ y)))	/* z ? x : y */
#define F5(x, y, z) (x ^ (y | ~z))

#define ROUND(a, b, c, d, e, f, k, x, s)  { \
	(a) += f((b), (c), (d)) + le32_to_cpup(&(x)) + (k); \
	(a) = rol32((a), (s)) + (e); \
	(c) = rol32((c), 10); \
}

static void rmd320_transform(u32 *state, const __le32 *in)
{
	u32 aa, bb, cc, dd, ee, aaa, bbb, ccc, ddd, eee, tmp;

	/* Initialize left lane */
	aa = state[0];
	bb = state[1];
	cc = state[2];
	dd = state[3];
	ee = state[4];

	/* Initialize right lane */
	aaa = state[5];
	bbb = state[6];
	ccc = state[7];
	ddd = state[8];
	eee = state[9];

	/* round 1: left lane */
	ROUND(aa, bb, cc, dd, ee, F1, K1, in[0],  11);
	ROUND(ee, aa, bb, cc, dd, F1, K1, in[1],  14);
	ROUND(dd, ee, aa, bb, cc, F1, K1, in[2],  15);
	ROUND(cc, dd, ee, aa, bb, F1, K1, in[3],  12);
	ROUND(bb, cc, dd, ee, aa, F1, K1, in[4],   5);
	ROUND(aa, bb, cc, dd, ee, F1, K1, in[5],   8);
	ROUND(ee, aa, bb, cc, dd, F1, K1, in[6],   7);
	ROUND(dd, ee, aa, bb, cc, F1, K1, in[7],   9);
	ROUND(cc, dd, ee, aa, bb, F1, K1, in[8],  11);
	ROUND(bb, cc, dd, ee, aa, F1, K1, in[9],  13);
	ROUND(aa, bb, cc, dd, ee, F1, K1, in[10], 14);
	ROUND(ee, aa, bb, cc, dd, F1, K1, in[11], 15);
	ROUND(dd, ee, aa, bb, cc, F1, K1, in[12],  6);
	ROUND(cc, dd, ee, aa, bb, F1, K1, in[13],  7);
	ROUND(bb, cc, dd, ee, aa, F1, K1, in[14],  9);
	ROUND(aa, bb, cc, dd, ee, F1, K1, in[15],  8);

	/* round 1: right lane */
	ROUND(aaa, bbb, ccc, ddd, eee, F5, KK1, in[5],   8);
	ROUND(eee, aaa, bbb, ccc, ddd, F5, KK1, in[14],  9);
	ROUND(ddd, eee, aaa, bbb, ccc, F5, KK1, in[7],   9);
	ROUND(ccc, ddd, eee, aaa, bbb, F5, KK1, in[0],  11);
	ROUND(bbb, ccc, ddd, eee, aaa, F5, KK1, in[9],  13);
	ROUND(aaa, bbb, ccc, ddd, eee, F5, KK1, in[2],  15);
	ROUND(eee, aaa, bbb, ccc, ddd, F5, KK1, in[11], 15);
	ROUND(ddd, eee, aaa, bbb, ccc, F5, KK1, in[4],   5);
	ROUND(ccc, ddd, eee, aaa, bbb, F5, KK1, in[13],  7);
	ROUND(bbb, ccc, ddd, eee, aaa, F5, KK1, in[6],   7);
	ROUND(aaa, bbb, ccc, ddd, eee, F5, KK1, in[15],  8);
	ROUND(eee, aaa, bbb, ccc, ddd, F5, KK1, in[8],  11);
	ROUND(ddd, eee, aaa, bbb, ccc, F5, KK1, in[1],  14);
	ROUND(ccc, ddd, eee, aaa, bbb, F5, KK1, in[10], 14);
	ROUND(bbb, ccc, ddd, eee, aaa, F5, KK1, in[3],  12);
	ROUND(aaa, bbb, ccc, ddd, eee, F5, KK1, in[12],  6);

	/* Swap contents of "a" registers */
	tmp = aa; aa = aaa; aaa = tmp;

	/* round 2: left lane" */
	ROUND(ee, aa, bb, cc, dd, F2, K2, in[7],   7);
	ROUND(dd, ee, aa, bb, cc, F2, K2, in[4],   6);
	ROUND(cc, dd, ee, aa, bb, F2, K2, in[13],  8);
	ROUND(bb, cc, dd, ee, aa, F2, K2, in[1],  13);
	ROUND(aa, bb, cc, dd, ee, F2, K2, in[10], 11);
	ROUND(ee, aa, bb, cc, dd, F2, K2, in[6],   9);
	ROUND(dd, ee, aa, bb, cc, F2, K2, in[15],  7);
	ROUND(cc, dd, ee, aa, bb, F2, K2, in[3],  15);
	ROUND(bb, cc, dd, ee, aa, F2, K2, in[12],  7);
	ROUND(aa, bb, cc, dd, ee, F2, K2, in[0],  12);
	ROUND(ee, aa, bb, cc, dd, F2, K2, in[9],  15);
	ROUND(dd, ee, aa, bb, cc, F2, K2, in[5],   9);
	ROUND(cc, dd, ee, aa, bb, F2, K2, in[2],  11);
	ROUND(bb, cc, dd, ee, aa, F2, K2, in[14],  7);
	ROUND(aa, bb, cc, dd, ee, F2, K2, in[11], 13);
	ROUND(ee, aa, bb, cc, dd, F2, K2, in[8],  12);

	/* round 2: right lane */
	ROUND(eee, aaa, bbb, ccc, ddd, F4, KK2, in[6],   9);
	ROUND(ddd, eee, aaa, bbb, ccc, F4, KK2, in[11], 13);
	ROUND(ccc, ddd, eee, aaa, bbb, F4, KK2, in[3],  15);
	ROUND(bbb, ccc, ddd, eee, aaa, F4, KK2, in[7],   7);
	ROUND(aaa, bbb, ccc, ddd, eee, F4, KK2, in[0],  12);
	ROUND(eee, aaa, bbb, ccc, ddd, F4, KK2, in[13],  8);
	ROUND(ddd, eee, aaa, bbb, ccc, F4, KK2, in[5],   9);
	ROUND(ccc, ddd, eee, aaa, bbb, F4, KK2, in[10], 11);
	ROUND(bbb, ccc, ddd, eee, aaa, F4, KK2, in[14],  7);
	ROUND(aaa, bbb, ccc, ddd, eee, F4, KK2, in[15],  7);
	ROUND(eee, aaa, bbb, ccc, ddd, F4, KK2, in[8],  12);
	ROUND(ddd, eee, aaa, bbb, ccc, F4, KK2, in[12],  7);
	ROUND(ccc, ddd, eee, aaa, bbb, F4, KK2, in[4],   6);
	ROUND(bbb, ccc, ddd, eee, aaa, F4, KK2, in[9],  15);
	ROUND(aaa, bbb, ccc, ddd, eee, F4, KK2, in[1],  13);
	ROUND(eee, aaa, bbb, ccc, ddd, F4, KK2, in[2],  11);

	/* Swap contents of "b" registers */
	tmp = bb; bb = bbb; bbb = tmp;

	/* round 3: left lane" */
	ROUND(dd, ee, aa, bb, cc, F3, K3, in[3],  11);
	ROUND(cc, dd, ee, aa, bb, F3, K3, in[10], 13);
	ROUND(bb, cc, dd, ee, aa, F3, K3, in[14],  6);
	ROUND(aa, bb, cc, dd, ee, F3, K3, in[4],   7);
	ROUND(ee, aa, bb, cc, dd, F3, K3, in[9],  14);
	ROUND(dd, ee, aa, bb, cc, F3, K3, in[15],  9);
	ROUND(cc, dd, ee, aa, bb, F3, K3, in[8],  13);
	ROUND(bb, cc, dd, ee, aa, F3, K3, in[1],  15);
	ROUND(aa, bb, cc, dd, ee, F3, K3, in[2],  14);
	ROUND(ee, aa, bb, cc, dd, F3, K3, in[7],   8);
	ROUND(dd, ee, aa, bb, cc, F3, K3, in[0],  13);
	ROUND(cc, dd, ee, aa, bb, F3, K3, in[6],   6);
	ROUND(bb, cc, dd, ee, aa, F3, K3, in[13],  5);
	ROUND(aa, bb, cc, dd, ee, F3, K3, in[11], 12);
	ROUND(ee, aa, bb, cc, dd, F3, K3, in[5],   7);
	ROUND(dd, ee, aa, bb, cc, F3, K3, in[12],  5);

	/* round 3: right lane */
	ROUND(ddd, eee, aaa, bbb, ccc, F3, KK3, in[15],  9);
	ROUND(ccc, ddd, eee, aaa, bbb, F3, KK3, in[5],   7);
	ROUND(bbb, ccc, ddd, eee, aaa, F3, KK3, in[1],  15);
	ROUND(aaa, bbb, ccc, ddd, eee, F3, KK3, in[3],  11);
	ROUND(eee, aaa, bbb, ccc, ddd, F3, KK3, in[7],   8);
	ROUND(ddd, eee, aaa, bbb, ccc, F3, KK3, in[14],  6);
	ROUND(ccc, ddd, eee, aaa, bbb, F3, KK3, in[6],   6);
	ROUND(bbb, ccc, ddd, eee, aaa, F3, KK3, in[9],  14);
	ROUND(aaa, bbb, ccc, ddd, eee, F3, KK3, in[11], 12);
	ROUND(eee, aaa, bbb, ccc, ddd, F3, KK3, in[8],  13);
	ROUND(ddd, eee, aaa, bbb, ccc, F3, KK3, in[12],  5);
	ROUND(ccc, ddd, eee, aaa, bbb, F3, KK3, in[2],  14);
	ROUND(bbb, ccc, ddd, eee, aaa, F3, KK3, in[10], 13);
	ROUND(aaa, bbb, ccc, ddd, eee, F3, KK3, in[0],  13);
	ROUND(eee, aaa, bbb, ccc, ddd, F3, KK3, in[4],   7);
	ROUND(ddd, eee, aaa, bbb, ccc, F3, KK3, in[13],  5);

	/* Swap contents of "c" registers */
	tmp = cc; cc = ccc; ccc = tmp;

	/* round 4: left lane" */
	ROUND(cc, dd, ee, aa, bb, F4, K4, in[1],  11);
	ROUND(bb, cc, dd, ee, aa, F4, K4, in[9],  12);
	ROUND(aa, bb, cc, dd, ee, F4, K4, in[11], 14);
	ROUND(ee, aa, bb, cc, dd, F4, K4, in[10], 15);
	ROUND(dd, ee, aa, bb, cc, F4, K4, in[0],  14);
	ROUND(cc, dd, ee, aa, bb, F4, K4, in[8],  15);
	ROUND(bb, cc, dd, ee, aa, F4, K4, in[12],  9);
	ROUND(aa, bb, cc, dd, ee, F4, K4, in[4],   8);
	ROUND(ee, aa, bb, cc, dd, F4, K4, in[13],  9);
	ROUND(dd, ee, aa, bb, cc, F4, K4, in[3],  14);
	ROUND(cc, dd, ee, aa, bb, F4, K4, in[7],   5);
	ROUND(bb, cc, dd, ee, aa, F4, K4, in[15],  6);
	ROUND(aa, bb, cc, dd, ee, F4, K4, in[14],  8);
	ROUND(ee, aa, bb, cc, dd, F4, K4, in[5],   6);
	ROUND(dd, ee, aa, bb, cc, F4, K4, in[6],   5);
	ROUND(cc, dd, ee, aa, bb, F4, K4, in[2],  12);

	/* round 4: right lane */
	ROUND(ccc, ddd, eee, aaa, bbb, F2, KK4, in[8],  15);
	ROUND(bbb, ccc, ddd, eee, aaa, F2, KK4, in[6],   5);
	ROUND(aaa, bbb, ccc, ddd, eee, F2, KK4, in[4],   8);
	ROUND(eee, aaa, bbb, ccc, ddd, F2, KK4, in[1],  11);
	ROUND(ddd, eee, aaa, bbb, ccc, F2, KK4, in[3],  14);
	ROUND(ccc, ddd, eee, aaa, bbb, F2, KK4, in[11], 14);
	ROUND(bbb, ccc, ddd, eee, aaa, F2, KK4, in[15],  6);
	ROUND(aaa, bbb, ccc, ddd, eee, F2, KK4, in[0],  14);
	ROUND(eee, aaa, bbb, ccc, ddd, F2, KK4, in[5],   6);
	ROUND(ddd, eee, aaa, bbb, ccc, F2, KK4, in[12],  9);
	ROUND(ccc, ddd, eee, aaa, bbb, F2, KK4, in[2],  12);
	ROUND(bbb, ccc, ddd, eee, aaa, F2, KK4, in[13],  9);
	ROUND(aaa, bbb, ccc, ddd, eee, F2, KK4, in[9],  12);
	ROUND(eee, aaa, bbb, ccc, ddd, F2, KK4, in[7],   5);
	ROUND(ddd, eee, aaa, bbb, ccc, F2, KK4, in[10], 15);
	ROUND(ccc, ddd, eee, aaa, bbb, F2, KK4, in[14],  8);

	/* Swap contents of "d" registers */
	tmp = dd; dd = ddd; ddd = tmp;

	/* round 5: left lane" */
	ROUND(bb, cc, dd, ee, aa, F5, K5, in[4],   9);
	ROUND(aa, bb, cc, dd, ee, F5, K5, in[0],  15);
	ROUND(ee, aa, bb, cc, dd, F5, K5, in[5],   5);
	ROUND(dd, ee, aa, bb, cc, F5, K5, in[9],  11);
	ROUND(cc, dd, ee, aa, bb, F5, K5, in[7],   6);
	ROUND(bb, cc, dd, ee, aa, F5, K5, in[12],  8);
	ROUND(aa, bb, cc, dd, ee, F5, K5, in[2],  13);
	ROUND(ee, aa, bb, cc, dd, F5, K5, in[10], 12);
	ROUND(dd, ee, aa, bb, cc, F5, K5, in[14],  5);
	ROUND(cc, dd, ee, aa, bb, F5, K5, in[1],  12);
	ROUND(bb, cc, dd, ee, aa, F5, K5, in[3],  13);
	ROUND(aa, bb, cc, dd, ee, F5, K5, in[8],  14);
	ROUND(ee, aa, bb, cc, dd, F5, K5, in[11], 11);
	ROUND(dd, ee, aa, bb, cc, F5, K5, in[6],   8);
	ROUND(cc, dd, ee, aa, bb, F5, K5, in[15],  5);
	ROUND(bb, cc, dd, ee, aa, F5, K5, in[13],  6);

	/* round 5: right lane */
	ROUND(bbb, ccc, ddd, eee, aaa, F1, KK5, in[12],  8);
	ROUND(aaa, bbb, ccc, ddd, eee, F1, KK5, in[15],  5);
	ROUND(eee, aaa, bbb, ccc, ddd, F1, KK5, in[10], 12);
	ROUND(ddd, eee, aaa, bbb, ccc, F1, KK5, in[4],   9);
	ROUND(ccc, ddd, eee, aaa, bbb, F1, KK5, in[1],  12);
	ROUND(bbb, ccc, ddd, eee, aaa, F1, KK5, in[5],   5);
	ROUND(aaa, bbb, ccc, ddd, eee, F1, KK5, in[8],  14);
	ROUND(eee, aaa, bbb, ccc, ddd, F1, KK5, in[7],   6);
	ROUND(ddd, eee, aaa, bbb, ccc, F1, KK5, in[6],   8);
	ROUND(ccc, ddd, eee, aaa, bbb, F1, KK5, in[2],  13);
	ROUND(bbb, ccc, ddd, eee, aaa, F1, KK5, in[13],  6);
	ROUND(aaa, bbb, ccc, ddd, eee, F1, KK5, in[14],  5);
	ROUND(eee, aaa, bbb, ccc, ddd, F1, KK5, in[0],  15);
	ROUND(ddd, eee, aaa, bbb, ccc, F1, KK5, in[3],  13);
	ROUND(ccc, ddd, eee, aaa, bbb, F1, KK5, in[9],  11);
	ROUND(bbb, ccc, ddd, eee, aaa, F1, KK5, in[11], 11);

	/* Swap contents of "e" registers */
	tmp = ee; ee = eee; eee = tmp;

	/* combine results */
	state[0] += aa;
	state[1] += bb;
	state[2] += cc;
	state[3] += dd;
	state[4] += ee;
	state[5] += aaa;
	state[6] += bbb;
	state[7] += ccc;
	state[8] += ddd;
	state[9] += eee;

	return;
}

static int rmd320_init(struct shash_desc *desc)
{
	struct rmd320_ctx *rctx = shash_desc_ctx(desc);

	rctx->byte_count = 0;

	rctx->state[0] = RMD_H0;
	rctx->state[1] = RMD_H1;
	rctx->state[2] = RMD_H2;
	rctx->state[3] = RMD_H3;
	rctx->state[4] = RMD_H4;
	rctx->state[5] = RMD_H5;
	rctx->state[6] = RMD_H6;
	rctx->state[7] = RMD_H7;
	rctx->state[8] = RMD_H8;
	rctx->state[9] = RMD_H9;

	memset(rctx->buffer, 0, sizeof(rctx->buffer));

	return 0;
}

static int rmd320_update(struct shash_desc *desc, const u8 *data,
			 unsigned int len)
{
	struct rmd320_ctx *rctx = shash_desc_ctx(desc);
	const u32 avail = sizeof(rctx->buffer) - (rctx->byte_count & 0x3f);

	rctx->byte_count += len;

	/* Enough space in buffer? If so copy and we're done */
	if (avail > len) {
		memcpy((char *)rctx->buffer + (sizeof(rctx->buffer) - avail),
		       data, len);
		goto out;
	}

	memcpy((char *)rctx->buffer + (sizeof(rctx->buffer) - avail),
	       data, avail);

	rmd320_transform(rctx->state, rctx->buffer);
	data += avail;
	len -= avail;

	while (len >= sizeof(rctx->buffer)) {
		memcpy(rctx->buffer, data, sizeof(rctx->buffer));
		rmd320_transform(rctx->state, rctx->buffer);
		data += sizeof(rctx->buffer);
		len -= sizeof(rctx->buffer);
	}

	memcpy(rctx->buffer, data, len);

out:
	return 0;
}

/* Add padding and return the message digest. */
static int rmd320_final(struct shash_desc *desc, u8 *out)
{
	struct rmd320_ctx *rctx = shash_desc_ctx(desc);
	u32 i, index, padlen;
	__le64 bits;
	__le32 *dst = (__le32 *)out;
	static const u8 padding[64] = { 0x80, };

	bits = cpu_to_le64(rctx->byte_count << 3);

	/* Pad out to 56 mod 64 */
	index = rctx->byte_count & 0x3f;
	padlen = (index < 56) ? (56 - index) : ((64+56) - index);
	rmd320_update(desc, padding, padlen);

	/* Append length */
	rmd320_update(desc, (const u8 *)&bits, sizeof(bits));

	/* Store state in digest */
	for (i = 0; i < 10; i++)
		dst[i] = cpu_to_le32p(&rctx->state[i]);

	/* Wipe context */
	memset(rctx, 0, sizeof(*rctx));

	return 0;
}

static struct shash_alg alg = {
	.digestsize	=	RMD320_DIGEST_SIZE,
	.init		=	rmd320_init,
	.update		=	rmd320_update,
	.final		=	rmd320_final,
	.descsize	=	sizeof(struct rmd320_ctx),
	.base		=	{
		.cra_name	 =	"rmd320",
		.cra_flags	 =	CRYPTO_ALG_TYPE_SHASH,
		.cra_blocksize	 =	RMD320_BLOCK_SIZE,
		.cra_module	 =	THIS_MODULE,
	}
};

static int __init rmd320_mod_init(void)
{
	return crypto_register_shash(&alg);
}

static void __exit rmd320_mod_fini(void)
{
	crypto_unregister_shash(&alg);
}

module_init(rmd320_mod_init);
module_exit(rmd320_mod_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adrian-Ken Rueegsegger <ken@codelabs.ch>");
MODULE_DESCRIPTION("RIPEMD-320 Message Digest");
MODULE_ALIAS_CRYPTO("rmd320");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * Cryptographic API.
 *
 * RNG operations.
 *
 * Copyright (c) 2008 Neil Horman <nhorman@tuxdriver.com>
 * Copyright (c) 2015 Herbert Xu <herbert@gondor.apana.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#include <linux/atomic.h>
#include <crypto/internal/rng.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/random.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/cryptouser.h>
#include <net/netlink.h>

#include "internal.h"

static DEFINE_MUTEX(crypto_default_rng_lock);
struct crypto_rng *crypto_default_rng;
EXPORT_SYMBOL_GPL(crypto_default_rng);
static int crypto_default_rng_refcnt;

static inline struct crypto_rng *__crypto_rng_cast(struct crypto_tfm *tfm)
{
	return container_of(tfm, struct crypto_rng, base);
}

int crypto_rng_reset(struct crypto_rng *tfm, const u8 *seed, unsigned int slen)
{
	u8 *buf = NULL;
	int err;

	if (!seed && slen) {
		buf = kmalloc(slen, GFP_KERNEL);
		if (!buf)
			return -ENOMEM;

		get_random_bytes(buf, slen);
		seed = buf;
	}

	err = crypto_rng_alg(tfm)->seed(tfm, seed, slen);

	kzfree(buf);
	return err;
}
EXPORT_SYMBOL_GPL(crypto_rng_reset);

static int crypto_rng_init_tfm(struct crypto_tfm *tfm)
{
	return 0;
}

static unsigned int seedsize(struct crypto_alg *alg)
{
	struct rng_alg *ralg = container_of(alg, struct rng_alg, base);

	return ralg->seedsize;
}

#ifdef CONFIG_NET
static int crypto_rng_report(struct sk_buff *skb, struct crypto_alg *alg)
{
	struct crypto_report_rng rrng;

	strncpy(rrng.type, "rng", sizeof(rrng.type));

	rrng.seedsize = seedsize(alg);

	if (nla_put(skb, CRYPTOCFGA_REPORT_RNG,
		    sizeof(struct crypto_report_rng), &rrng))
		goto nla_put_failure;
	return 0;

nla_put_failure:
	return -EMSGSIZE;
}
#else
static int crypto_rng_report(struct sk_buff *skb, struct crypto_alg *alg)
{
	return -ENOSYS;
}
#endif

static void crypto_rng_show(struct seq_file *m, struct crypto_alg *alg)
	__attribute__ ((unused));
static void crypto_rng_show(struct seq_file *m, struct crypto_alg *alg)
{
	seq_printf(m, "type         : rng\n");
	seq_printf(m, "seedsize     : %u\n", seedsize(alg));
}

static const struct crypto_type crypto_rng_type = {
	.extsize = crypto_alg_extsize,
	.init_tfm = crypto_rng_init_tfm,
#ifdef CONFIG_PROC_FS
	.show = crypto_rng_show,
#endif
	.report = crypto_rng_report,
	.maskclear = ~CRYPTO_ALG_TYPE_MASK,
	.maskset = CRYPTO_ALG_TYPE_MASK,
	.type = CRYPTO_ALG_TYPE_RNG,
	.tfmsize = offsetof(struct crypto_rng, base),
};

struct crypto_rng *crypto_alloc_rng(const char *alg_name, u32 type, u32 mask)
{
	return crypto_alloc_tfm(alg_name, &crypto_rng_type, type, mask);
}
EXPORT_SYMBOL_GPL(crypto_alloc_rng);

int crypto_get_default_rng(void)
{
	struct crypto_rng *rng;
	int err;

	mutex_lock(&crypto_default_rng_lock);
	if (!crypto_default_rng) {
		rng = crypto_alloc_rng("stdrng", 0, 0);
		err = PTR_ERR(rng);
		if (IS_ERR(rng))
			goto unlock;

		err = crypto_rng_reset(rng, NULL, crypto_rng_seedsize(rng));
		if (err) {
			crypto_free_rng(rng);
			goto unlock;
		}

		crypto_default_rng = rng;
	}

	crypto_default_rng_refcnt++;
	err = 0;

unlock:
	mutex_unlock(&crypto_default_rng_lock);

	return err;
}
EXPORT_SYMBOL_GPL(crypto_get_default_rng);

void crypto_put_default_rng(void)
{
	mutex_lock(&crypto_default_rng_lock);
	crypto_default_rng_refcnt--;
	mutex_unlock(&crypto_default_rng_lock);
}
EXPORT_SYMBOL_GPL(crypto_put_default_rng);

#if defined(CONFIG_CRYPTO_RNG) || defined(CONFIG_CRYPTO_RNG_MODULE)
int crypto_del_default_rng(void)
{
	int err = -EBUSY;

	mutex_lock(&crypto_default_rng_lock);
	if (crypto_default_rng_refcnt)
		goto out;

	crypto_free_rng(crypto_default_rng);
	crypto_default_rng = NULL;

	err = 0;

out:
	mutex_unlock(&crypto_default_rng_lock);

	return err;
}
EXPORT_SYMBOL_GPL(crypto_del_default_rng);
#endif

int crypto_register_rng(struct rng_alg *alg)
{
	struct crypto_alg *base = &alg->base;

	if (alg->seedsize > PAGE_SIZE / 8)
		return -EINVAL;

	base->cra_type = &crypto_rng_type;
	base->cra_flags &= ~CRYPTO_ALG_TYPE_MASK;
	base->cra_flags |= CRYPTO_ALG_TYPE_RNG;

	return crypto_register_alg(base);
}
EXPORT_SYMBOL_GPL(crypto_register_rng);

void crypto_unregister_rng(struct rng_alg *alg)
{
	crypto_unregister_alg(&alg->base);
}
EXPORT_SYMBOL_GPL(crypto_unregister_rng);

int crypto_register_rngs(struct rng_alg *algs, int count)
{
	int i, ret;

	for (i = 0; i < count; i++) {
		ret = crypto_register_rng(algs + i);
		if (ret)
			goto err;
	}

	return 0;

err:
	for (--i; i >= 0; --i)
		crypto_unregister_rng(algs + i);

	return ret;
}
EXPORT_SYMBOL_GPL(crypto_register_rngs);

void crypto_unregister_rngs(struct rng_alg *algs, int count)
{
	int i;

	for (i = count - 1; i >= 0; --i)
		crypto_unregister_rng(algs + i);
}
EXPORT_SYMBOL_GPL(crypto_unregister_rngs);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Random Number Generator");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /* RSA asymmetric public-key algorithm [RFC3447]
 *
 * Copyright (c) 2015, Intel Corporation
 * Authors: Tadeusz Struk <tadeusz.struk@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

#include <linux/module.h>
#include <crypto/internal/rsa.h>
#include <crypto/internal/akcipher.h>
#include <crypto/akcipher.h>

/*
 * RSAEP function [RFC3447 sec 5.1.1]
 * c = m^e mod n;
 */
static int _rsa_enc(const struct rsa_key *key, MPI c, MPI m)
{
	/* (1) Validate 0 <= m < n */
	if (mpi_cmp_ui(m, 0) < 0 || mpi_cmp(m, key->n) >= 0)
		return -EINVAL;

	/* (2) c = m^e mod n */
	return mpi_powm(c, m, key->e, key->n);
}

/*
 * RSADP function [RFC3447 sec 5.1.2]
 * m = c^d mod n;
 */
static int _rsa_dec(const struct rsa_key *key, MPI m, MPI c)
{
	/* (1) Validate 0 <= c < n */
	if (mpi_cmp_ui(c, 0) < 0 || mpi_cmp(c, key->n) >= 0)
		return -EINVAL;

	/* (2) m = c^d mod n */
	return mpi_powm(m, c, key->d, key->n);
}

/*
 * RSASP1 function [RFC3447 sec 5.2.1]
 * s = m^d mod n
 */
static int _rsa_sign(const struct rsa_key *key, MPI s, MPI m)
{
	/* (1) Validate 0 <= m < n */
	if (mpi_cmp_ui(m, 0) < 0 || mpi_cmp(m, key->n) >= 0)
		return -EINVAL;

	/* (2) s = m^d mod n */
	return mpi_powm(s, m, key->d, key->n);
}

/*
 * RSAVP1 function [RFC3447 sec 5.2.2]
 * m = s^e mod n;
 */
static int _rsa_verify(const struct rsa_key *key, MPI m, MPI s)
{
	/* (1) Validate 0 <= s < n */
	if (mpi_cmp_ui(s, 0) < 0 || mpi_cmp(s, key->n) >= 0)
		return -EINVAL;

	/* (2) m = s^e mod n */
	return mpi_powm(m, s, key->e, key->n);
}

static inline struct rsa_key *rsa_get_key(struct crypto_akcipher *tfm)
{
	return akcipher_tfm_ctx(tfm);
}

static int rsa_enc(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	const struct rsa_key *pkey = rsa_get_key(tfm);
	MPI m, c = mpi_alloc(0);
	int ret = 0;
	int sign;

	if (!c)
		return -ENOMEM;

	if (unlikely(!pkey->n || !pkey->e)) {
		ret = -EINVAL;
		goto err_free_c;
	}

	if (req->dst_len < mpi_get_size(pkey->n)) {
		req->dst_len = mpi_get_size(pkey->n);
		ret = -EOVERFLOW;
		goto err_free_c;
	}

	ret = -ENOMEM;
	m = mpi_read_raw_from_sgl(req->src, req->src_len);
	if (!m)
		goto err_free_c;

	ret = _rsa_enc(pkey, c, m);
	if (ret)
		goto err_free_m;

	ret = mpi_write_to_sgl(c, req->dst, &req->dst_len, &sign);
	if (ret)
		goto err_free_m;

	if (sign < 0)
		ret = -EBADMSG;

err_free_m:
	mpi_free(m);
err_free_c:
	mpi_free(c);
	return ret;
}

static int rsa_dec(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	const struct rsa_key *pkey = rsa_get_key(tfm);
	MPI c, m = mpi_alloc(0);
	int ret = 0;
	int sign;

	if (!m)
		return -ENOMEM;

	if (unlikely(!pkey->n || !pkey->d)) {
		ret = -EINVAL;
		goto err_free_m;
	}

	if (req->dst_len < mpi_get_size(pkey->n)) {
		req->dst_len = mpi_get_size(pkey->n);
		ret = -EOVERFLOW;
		goto err_free_m;
	}

	ret = -ENOMEM;
	c = mpi_read_raw_from_sgl(req->src, req->src_len);
	if (!c)
		goto err_free_m;

	ret = _rsa_dec(pkey, m, c);
	if (ret)
		goto err_free_c;

	ret = mpi_write_to_sgl(m, req->dst, &req->dst_len, &sign);
	if (ret)
		goto err_free_c;

	if (sign < 0)
		ret = -EBADMSG;
err_free_c:
	mpi_free(c);
err_free_m:
	mpi_free(m);
	return ret;
}

static int rsa_sign(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	const struct rsa_key *pkey = rsa_get_key(tfm);
	MPI m, s = mpi_alloc(0);
	int ret = 0;
	int sign;

	if (!s)
		return -ENOMEM;

	if (unlikely(!pkey->n || !pkey->d)) {
		ret = -EINVAL;
		goto err_free_s;
	}

	if (req->dst_len < mpi_get_size(pkey->n)) {
		req->dst_len = mpi_get_size(pkey->n);
		ret = -EOVERFLOW;
		goto err_free_s;
	}

	ret = -ENOMEM;
	m = mpi_read_raw_from_sgl(req->src, req->src_len);
	if (!m)
		goto err_free_s;

	ret = _rsa_sign(pkey, s, m);
	if (ret)
		goto err_free_m;

	ret = mpi_write_to_sgl(s, req->dst, &req->dst_len, &sign);
	if (ret)
		goto err_free_m;

	if (sign < 0)
		ret = -EBADMSG;

err_free_m:
	mpi_free(m);
err_free_s:
	mpi_free(s);
	return ret;
}

static int rsa_verify(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	const struct rsa_key *pkey = rsa_get_key(tfm);
	MPI s, m = mpi_alloc(0);
	int ret = 0;
	int sign;

	if (!m)
		return -ENOMEM;

	if (unlikely(!pkey->n || !pkey->e)) {
		ret = -EINVAL;
		goto err_free_m;
	}

	if (req->dst_len < mpi_get_size(pkey->n)) {
		req->dst_len = mpi_get_size(pkey->n);
		ret = -EOVERFLOW;
		goto err_free_m;
	}

	ret = -ENOMEM;
	s = mpi_read_raw_from_sgl(req->src, req->src_len);
	if (!s) {
		ret = -ENOMEM;
		goto err_free_m;
	}

	ret = _rsa_verify(pkey, m, s);
	if (ret)
		goto err_free_s;

	ret = mpi_write_to_sgl(m, req->dst, &req->dst_len, &sign);
	if (ret)
		goto err_free_s;

	if (sign < 0)
		ret = -EBADMSG;

err_free_s:
	mpi_free(s);
err_free_m:
	mpi_free(m);
	return ret;
}

static int rsa_check_key_length(unsigned int len)
{
	switch (len) {
	case 512:
	case 1024:
	case 1536:
	case 2048:
	case 3072:
	case 4096:
		return 0;
	}

	return -EINVAL;
}

static int rsa_set_pub_key(struct crypto_akcipher *tfm, const void *key,
			   unsigned int keylen)
{
	struct rsa_key *pkey = akcipher_tfm_ctx(tfm);
	int ret;

	ret = rsa_parse_pub_key(pkey, key, keylen);
	if (ret)
		return ret;

	if (rsa_check_key_length(mpi_get_size(pkey->n) << 3)) {
		rsa_free_key(pkey);
		ret = -EINVAL;
	}
	return ret;
}

static int rsa_set_priv_key(struct crypto_akcipher *tfm, const void *key,
			    unsigned int keylen)
{
	struct rsa_key *pkey = akcipher_tfm_ctx(tfm);
	int ret;

	ret = rsa_parse_priv_key(pkey, key, keylen);
	if (ret)
		return ret;

	if (rsa_check_key_length(mpi_get_size(pkey->n) << 3)) {
		rsa_free_key(pkey);
		ret = -EINVAL;
	}
	return ret;
}

static int rsa_max_size(struct crypto_akcipher *tfm)
{
	struct rsa_key *pkey = akcipher_tfm_ctx(tfm);

	return pkey->n ? mpi_get_size(pkey->n) : -EINVAL;
}

static void rsa_exit_tfm(struct crypto_akcipher *tfm)
{
	struct rsa_key *pkey = akcipher_tfm_ctx(tfm);

	rsa_free_key(pkey);
}

static struct akcipher_alg rsa = {
	.encrypt = rsa_enc,
	.decrypt = rsa_dec,
	.sign = rsa_sign,
	.verify = rsa_verify,
	.set_priv_key = rsa_set_priv_key,
	.set_pub_key = rsa_set_pub_key,
	.max_size = rsa_max_size,
	.exit = rsa_exit_tfm,
	.base = {
		.cra_name = "rsa",
		.cra_driver_name = "rsa-generic",
		.cra_priority = 100,
		.cra_module = THIS_MODULE,
		.cra_ctxsize = sizeof(struct rsa_key),
	},
};

static int rsa_init(void)
{
	return crypto_register_akcipher(&rsa);
}

static void rsa_exit(void)
{
	crypto_unregister_akcipher(&rsa);
}

module_init(rsa_init);
module_exit(rsa_exit);
MODULE_ALIAS_CRYPTO("rsa");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RSA generic algorithm");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /*
 * RSA key extract helper
 *
 * Copyright (c) 2015, Intel Corporation
 * Authors: Tadeusz Struk <tadeusz.struk@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/err.h>
#include <linux/fips.h>
#include <crypto/internal/rsa.h>
#include "rsapubkey-asn1.h"
#include "rsaprivkey-asn1.h"

int rsa_get_n(void *context, size_t hdrlen, unsigned char tag,
	      const void *value, size_t vlen)
{
	struct rsa_key *key = context;

	key->n = mpi_read_raw_data(value, vlen);

	if (!key->n)
		return -ENOMEM;

	/* In FIPS mode only allow key size 2K & 3K */
	if (fips_enabled && (mpi_get_size(key->n) != 256 &&
			     mpi_get_size(key->n) != 384)) {
		pr_err("RSA: key size not allowed in FIPS mode\n");
		mpi_free(key->n);
		key->n = NULL;
		return -EINVAL;
	}
	return 0;
}

int rsa_get_e(void *context, size_t hdrlen, unsigned char tag,
	      const void *value, size_t vlen)
{
	struct rsa_key *key = context;

	key->e = mpi_read_raw_data(value, vlen);

	if (!key->e)
		return -ENOMEM;

	return 0;
}

int rsa_get_d(void *context, size_t hdrlen, unsigned char tag,
	      const void *value, size_t vlen)
{
	struct rsa_key *key = context;

	key->d = mpi_read_raw_data(value, vlen);

	if (!key->d)
		return -ENOMEM;

	/* In FIPS mode only allow key size 2K & 3K */
	if (fips_enabled && (mpi_get_size(key->d) != 256 &&
			     mpi_get_size(key->d) != 384)) {
		pr_err("RSA: key size not allowed in FIPS mode\n");
		mpi_free(key->d);
		key->d = NULL;
		return -EINVAL;
	}
	return 0;
}

static void free_mpis(struct rsa_key *key)
{
	mpi_free(key->n);
	mpi_free(key->e);
	mpi_free(key->d);
	key->n = NULL;
	key->e = NULL;
	key->d = NULL;
}

/**
 * rsa_free_key() - frees rsa key allocated by rsa_parse_key()
 *
 * @rsa_key:	struct rsa_key key representation
 */
void rsa_free_key(struct rsa_key *key)
{
	free_mpis(key);
}
EXPORT_SYMBOL_GPL(rsa_free_key);

/**
 * rsa_parse_pub_key() - extracts an rsa public key from BER encoded buffer
 *			 and stores it in the provided struct rsa_key
 *
 * @rsa_key:	struct rsa_key key representation
 * @key:	key in BER format
 * @key_len:	length of key
 *
 * Return:	0 on success or error code in case of error
 */
int rsa_parse_pub_key(struct rsa_key *rsa_key, const void *key,
		      unsigned int key_len)
{
	int ret;

	free_mpis(rsa_key);
	ret = asn1_ber_decoder(&rsapubkey_decoder, rsa_key, key, key_len);
	if (ret < 0)
		goto error;

	return 0;
error:
	free_mpis(rsa_key);
	return ret;
}
EXPORT_SYMBOL_GPL(rsa_parse_pub_key);

/**
 * rsa_parse_pub_key() - extracts an rsa private key from BER encoded buffer
 *			 and stores it in the provided struct rsa_key
 *
 * @rsa_key:	struct rsa_key key representation
 * @key:	key in BER format
 * @key_len:	length of key
 *
 * Return:	0 on success or error code in case of error
 */
int rsa_parse_priv_key(struct rsa_key *rsa_key, const void *key,
		       unsigned int key_len)
{
	int ret;

	free_mpis(rsa_key);
	ret = asn1_ber_decoder(&rsaprivkey_decoder, rsa_key, key, key_len);
	if (ret < 0)
		goto error;

	return 0;
error:
	free_mpis(rsa_key);
	return ret;
}
EXPORT_SYMBOL_GPL(rsa_parse_priv_key);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          RsaPrivKey ::= SEQUENCE {
	version		INTEGER,
	n		INTEGER ({ rsa_get_n }),
	e		INTEGER ({ rsa_get_e }),
	d		INTEGER ({ rsa_get_d }),
	prime1		INTEGER,
	prime2		INTEGER,
	exponent1	INTEGER,
	exponent2	INTEGER,
	coefficient	INTEGER
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         RsaPubKey ::= SEQUENCE {
	n INTEGER ({ rsa_get_n }),
	e INTEGER ({ rsa_get_e })
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 * Salsa20: Salsa20 stream cipher algorithm
 *
 * Copyright (c) 2007 Tan Swee Heng <thesweeheng@gmail.com>
 *
 * Derived from:
 * - salsa20.c: Public domain C code by Daniel J. Bernstein <djb@cr.yp.to>
 *
 * Salsa20 is a stream cipher candidate in eSTREAM, the ECRYPT Stream
 * Cipher Project. It is designed by Daniel J. Bernstein <djb@cr.yp.to>.
 * More information about eSTREAM and Salsa20 can be found here:
 *   http://www.ecrypt.eu.org/stream/
 *   http://cr.yp.to/snuffle.html
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/crypto.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <crypto/algapi.h>
#include <asm/byteorder.h>

#define SALSA20_IV_SIZE        8U
#define SALSA20_MIN_KEY_SIZE  16U
#define SALSA20_MAX_KEY_SIZE  32U

/*
 * Start of code taken from D. J. Bernstein's reference implementation.
 * With some modifications and optimizations made to suit our needs.
 */

/*
salsa20-ref.c version 20051118
D. J. Bernstein
Public domain.
*/

#define U32TO8_LITTLE(p, v) \
	{ (p)[0] = (v >>  0) & 0xff; (p)[1] = (v >>  8) & 0xff; \
	  (p)[2] = (v >> 16) & 0xff; (p)[3] = (v >> 24) & 0xff; }
#define U8TO32_LITTLE(p)   \
	(((u32)((p)[0])      ) | ((u32)((p)[1]) <<  8) | \
	 ((u32)((p)[2]) << 16) | ((u32)((p)[3]) << 24)   )

struct salsa20_ctx
{
	u32 input[16];
};

static void salsa20_wordtobyte(u8 output[64], const u32 input[16])
{
	u32 x[16];
	int i;

	memcpy(x, input, sizeof(x));
	for (i = 20; i > 0; i -= 2) {
		x[ 4] ^= rol32((x[ 0] + x[12]),  7);
		x[ 8] ^= rol32((x[ 4] + x[ 0]),  9);
		x[12] ^= rol32((x[ 8] + x[ 4]), 13);
		x[ 0] ^= rol32((x[12] + x[ 8]), 18);
		x[ 9] ^= rol32((x[ 5] + x[ 1]),  7);
		x[13] ^= rol32((x[ 9] + x[ 5]),  9);
		x[ 1] ^= rol32((x[13] + x[ 9]), 13);
		x[ 5] ^= rol32((x[ 1] + x[13]), 18);
		x[14] ^= rol32((x[10] + x[ 6]),  7);
		x[ 2] ^= rol32((x[14] + x[10]),  9);
		x[ 6] ^= rol32((x[ 2] + x[14]), 13);
		x[10] ^= rol32((x[ 6] + x[ 2]), 18);
		x[ 3] ^= rol32((x[15] + x[11]),  7);
		x[ 7] ^= rol32((x[ 3] + x[15]),  9);
		x[11] ^= rol32((x[ 7] + x[ 3]), 13);
		x[15] ^= rol32((x[11] + x[ 7]), 18);
		x[ 1] ^= rol32((x[ 0] + x[ 3]),  7);
		x[ 2] ^= rol32((x[ 1] + x[ 0]),  9);
		x[ 3] ^= rol32((x[ 2] + x[ 1]), 13);
		x[ 0] ^= rol32((x[ 3] + x[ 2]), 18);
		x[ 6] ^= rol32((x[ 5] + x[ 4]),  7);
		x[ 7] ^= rol32((x[ 6] + x[ 5]),  9);
		x[ 4] ^= rol32((x[ 7] + x[ 6]), 13);
		x[ 5] ^= rol32((x[ 4] + x[ 7]), 18);
		x[11] ^= rol32((x[10] + x[ 9]),  7);
		x[ 8] ^= rol32((x[11] + x[10]),  9);
		x[ 9] ^= rol32((x[ 8] + x[11]), 13);
		x[10] ^= rol32((x[ 9] + x[ 8]), 18);
		x[12] ^= rol32((x[15] + x[14]),  7);
		x[13] ^= rol32((x[12] + x[15]),  9);
		x[14] ^= rol32((x[13] + x[12]), 13);
		x[15] ^= rol32((x[14] + x[13]), 18);
	}
	for (i = 0; i < 16; ++i)
		x[i] += input[i];
	for (i = 0; i < 16; ++i)
		U32TO8_LITTLE(output + 4 * i,x[i]);
}

static const char sigma[16] = "expand 32-byte k";
static const char tau[16] = "expand 16-byte k";

static void salsa20_keysetup(struct salsa20_ctx *ctx, const u8 *k, u32 kbytes)
{
	const char *constants;

	ctx->input[1] = U8TO32_LITTLE(k + 0);
	ctx->input[2] = U8TO32_LITTLE(k + 4);
	ctx->input[3] = U8TO32_LITTLE(k + 8);
	ctx->input[4] = U8TO32_LITTLE(k + 12);
	if (kbytes == 32) { /* recommended */
		k += 16;
		constants = sigma;
	} else { /* kbytes == 16 */
		constants = tau;
	}
	ctx->input[11] = U8TO32_LITTLE(k + 0);
	ctx->input[12] = U8TO32_LITTLE(k + 4);
	ctx->input[13] = U8TO32_LITTLE(k + 8);
	ctx->input[14] = U8TO32_LITTLE(k + 12);
	ctx->input[0] = U8TO32_LITTLE(constants + 0);
	ctx->input[5] = U8TO32_LITTLE(constants + 4);
	ctx->input[10] = U8TO32_LITTLE(constants + 8);
	ctx->input[15] = U8TO32_LITTLE(constants + 12);
}

static void salsa20_ivsetup(struct salsa20_ctx *ctx, const u8 *iv)
{
	ctx->input[6] = U8TO32_LITTLE(iv + 0);
	ctx->input[7] = U8TO32_LITTLE(iv + 4);
	ctx->input[8] = 0;
	ctx->input[9] = 0;
}

static void salsa20_encrypt_bytes(struct salsa20_ctx *ctx, u8 *dst,
				  const u8 *src, unsigned int bytes)
{
	u8 buf[64];

	if (dst != src)
		memcpy(dst, src, bytes);

	while (bytes) {
		salsa20_wordtobyte(buf, ctx->input);

		ctx->input[8]++;
		if (!ctx->input[8])
			ctx->input[9]++;

		if (bytes <= 64) {
			crypto_xor(dst, buf, bytes);
			return;
		}

		crypto_xor(dst, buf, 64);
		bytes -= 64;
		dst += 64;
	}
}

/*
 * End of code taken from D. J. Bernstein's reference implementation.
 */

static int setkey(struct crypto_tfm *tfm, const u8 *key,
		  unsigned int keysize)
{
	struct salsa20_ctx *ctx = crypto_tfm_ctx(tfm);
	salsa20_keysetup(ctx, key, keysize);
	return 0;
}

static int encrypt(struct blkcipher_desc *desc,
		   struct scatterlist *dst, struct scatterlist *src,
		   unsigned int nbytes)
{
	struct blkcipher_walk walk;
	struct crypto_blkcipher *tfm = desc->tfm;
	struct salsa20_ctx *ctx = crypto_blkcipher_ctx(tfm);
	int err;

	blkcipher_walk_init(&walk, dst, src, nbytes);
	err = blkcipher_walk_virt_block(desc, &walk, 64);

	salsa20_ivsetup(ctx, desc->info);

	while (walk.nbytes >= 64) {
		salsa20_encrypt_bytes(ctx, walk.dst.virt.addr,
				      walk.src.virt.addr,
				      walk.nbytes - (walk.nbytes % 64));
		err = blkcipher_walk_done(desc, &walk, walk.nbytes % 64);
	}

	if (walk.nbytes) {
		salsa20_encrypt_bytes(ctx, walk.dst.virt.addr,
				      walk.src.virt.addr, walk.nbytes);
		err = blkcipher_walk_done(desc, &walk, 0);
	}

	return err;
}

static struct crypto_alg alg = {
	.cra_name           =   "salsa20",
	.cra_driver_name    =   "salsa20-generic",
	.cra_priority       =   100,
	.cra_flags          =   CRYPTO_ALG_TYPE_BLKCIPHER,
	.cra_type           =   &crypto_blkcipher_type,
	.cra_blocksize      =   1,
	.cra_ctxsize        =   sizeof(struct salsa20_ctx),
	.cra_alignmask      =	3,
	.cra_module         =   THIS_MODULE,
	.cra_u              =   {
		.blkcipher = {
			.setkey         =   setkey,
			.encrypt        =   encrypt,
			.decrypt        =   encrypt,
			.min_keysize    =   SALSA20_MIN_KEY_SIZE,
			.max_keysize    =   SALSA20_MAX_KEY_SIZE,
			.ivsize         =   SALSA20_IV_SIZE,
		}
	}
};

static int __init salsa20_generic_mod_init(void)
{
	return crypto_register_alg(&alg);
}

static void __exit salsa20_generic_mod_fini(void)
{
	crypto_unregister_alg(&alg);
}

module_init(salsa20_generic_mod_init);
module_exit(salsa20_generic_mod_fini);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION ("Salsa20 stream cipher algorithm");
MODULE_ALIAS_CRYPTO("salsa20");
MODULE_ALIAS_CRYPTO("salsa20-generic");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /*
 * Cryptographic API.
 *
 * Cipher operations.
 *
 * Copyright (c) 2002 James Morris <jmorris@intercode.com.au>
 *               2002 Adam J. Richter <adam@yggdrasil.com>
 *               2004 Jean-Luc Cooke <jlcooke@certainkey.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#include <crypto/scatterwalk.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/scatterlist.h>

static inline void memcpy_dir(void *buf, void *sgdata, size_t nbytes, int out)
{
	void *src = out ? buf : sgdata;
	void *dst = out ? sgdata : buf;

	memcpy(dst, src, nbytes);
}

void scatterwalk_start(struct scatter_walk *walk, struct scatterlist *sg)
{
	walk->sg = sg;

	BUG_ON(!sg->length);

	walk->offset = sg->offset;
}
EXPORT_SYMBOL_GPL(scatterwalk_start);

void *scatterwalk_map(struct scatter_walk *walk)
{
	return kmap_atomic(scatterwalk_page(walk)) +
	       offset_in_page(walk->offset);
}
EXPORT_SYMBOL_GPL(scatterwalk_map);

static void scatterwalk_pagedone(struct scatter_walk *walk, int out,
				 unsigned int more)
{
	if (out) {
		struct page *page;

		page = sg_page(walk->sg) + ((walk->offset - 1) >> PAGE_SHIFT);
		/* Test ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE first as
		 * PageSlab cannot be optimised away per se due to
		 * use of volatile pointer.
		 */
		if (ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE && !PageSlab(page))
			flush_dcache_page(page);
	}

	if (more) {
		walk->offset += PAGE_SIZE - 1;
		walk->offset &= PAGE_MASK;
		if (walk->offset >= walk->sg->offset + walk->sg->length)
			scatterwalk_start(walk, sg_next(walk->sg));
	}
}

void scatterwalk_done(struct scatter_walk *walk, int out, int more)
{
	if (!more || walk->offset >= walk->sg->offset + walk->sg->length ||
	    !(walk->offset & (PAGE_SIZE - 1)))
		scatterwalk_pagedone(walk, out, more);
}
EXPORT_SYMBOL_GPL(scatterwalk_done);

void scatterwalk_copychunks(void *buf, struct scatter_walk *walk,
			    size_t nbytes, int out)
{
	for (;;) {
		unsigned int len_this_page = scatterwalk_pagelen(walk);
		u8 *vaddr;

		if (len_this_page > nbytes)
			len_this_page = nbytes;

		vaddr = scatterwalk_map(walk);
		memcpy_dir(buf, vaddr, len_this_page, out);
		scatterwalk_unmap(vaddr);

		scatterwalk_advance(walk, len_this_page);

		if (nbytes == len_this_page)
			break;

		buf += len_this_page;
		nbytes -= len_this_page;

		scatterwalk_pagedone(walk, out, 1);
	}
}
EXPORT_SYMBOL_GPL(scatterwalk_copychunks);

void scatterwalk_map_and_copy(void *buf, struct scatterlist *sg,
			      unsigned int start, unsigned int nbytes, int out)
{
	struct scatter_walk walk;
	struct scatterlist tmp[2];

	if (!nbytes)
		return;

	sg = scatterwalk_ffwd(tmp, sg, start);

	if (sg_page(sg) == virt_to_page(buf) &&
	    sg->offset == offset_in_page(buf))
		return;

	scatterwalk_start(&walk, sg);
	scatterwalk_copychunks(buf, &walk, nbytes, out);
	scatterwalk_done(&walk, out, 0);
}
EXPORT_SYMBOL_GPL(scatterwalk_map_and_copy);

int scatterwalk_bytes_sglen(struct scatterlist *sg, int num_bytes)
{
	int offset = 0, n = 0;

	/* num_bytes is too small */
	if (num_bytes < sg->length)
		return -1;

	do {
		offset += sg->length;
		n++;
		sg = sg_next(sg);

		/* num_bytes is too large */
		if (unlikely(!sg && (num_bytes < offset)))
			return -1;
	} while (sg && (num_bytes > offset));

	return n;
}
EXPORT_SYMBOL_GPL(scatterwalk_bytes_sglen);

struct scatterlist *scatterwalk_ffwd(struct scatterlist dst[2],
				     struct scatterlist *src,
				     unsigned int len)
{
	for (;;) {
		if (!len)
			return src;

		if (src->length > len)
			break;

		len -= src->length;
		src = sg_next(src);
	}

	sg_init_table(dst, 2);
	sg_set_page(dst, sg_page(src), src->length - len, src->offset + len);
	scatterwalk_crypto_chain(dst, sg_next(src), 0, 2);

	return dst;
}
EXPORT_SYMBOL_GPL(scatterwalk_ffwd);
                                                   /*
 * Cryptographic API.
 *
 * SEED Cipher Algorithm.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Documentation of SEED can be found in RFC 4269.
 * Copyright (C) 2007 Korea Information Security Agency (KISA).
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/crypto.h>
#include <asm/byteorder.h>

#define SEED_NUM_KCONSTANTS	16
#define SEED_KEY_SIZE		16
#define SEED_BLOCK_SIZE		16
#define SEED_KEYSCHED_LEN	32

/*
 * #define byte(x, nr) ((unsigned char)((x) >> (nr*8)))
 */
static inline u8
byte(const u32 x, const unsigned n)
{
	return x >> (n << 3);
}

struct seed_ctx {
	u32 keysched[SEED_KEYSCHED_LEN];
};

static const u32 SS0[256] = {
	0x2989a1a8, 0x05858184, 0x16c6d2d4, 0x13c3d3d0,
	0x14445054, 0x1d0d111c, 0x2c8ca0ac, 0x25052124,
	0x1d4d515c, 0x03434340, 0x18081018, 0x1e0e121c,
	0x11415150, 0x3cccf0fc, 0x0acac2c8, 0x23436360,
	0x28082028, 0x04444044, 0x20002020, 0x1d8d919c,
	0x20c0e0e0, 0x22c2e2e0, 0x08c8c0c8, 0x17071314,
	0x2585a1a4, 0x0f8f838c, 0x03030300, 0x3b4b7378,
	0x3b8bb3b8, 0x13031310, 0x12c2d2d0, 0x2ecee2ec,
	0x30407070, 0x0c8c808c, 0x3f0f333c, 0x2888a0a8,
	0x32023230, 0x1dcdd1dc, 0x36c6f2f4, 0x34447074,
	0x2ccce0ec, 0x15859194, 0x0b0b0308, 0x17475354,
	0x1c4c505c, 0x1b4b5358, 0x3d8db1bc, 0x01010100,
	0x24042024, 0x1c0c101c, 0x33437370, 0x18889098,
	0x10001010, 0x0cccc0cc, 0x32c2f2f0, 0x19c9d1d8,
	0x2c0c202c, 0x27c7e3e4, 0x32427270, 0x03838380,
	0x1b8b9398, 0x11c1d1d0, 0x06868284, 0x09c9c1c8,
	0x20406060, 0x10405050, 0x2383a3a0, 0x2bcbe3e8,
	0x0d0d010c, 0x3686b2b4, 0x1e8e929c, 0x0f4f434c,
	0x3787b3b4, 0x1a4a5258, 0x06c6c2c4, 0x38487078,
	0x2686a2a4, 0x12021210, 0x2f8fa3ac, 0x15c5d1d4,
	0x21416160, 0x03c3c3c0, 0x3484b0b4, 0x01414140,
	0x12425250, 0x3d4d717c, 0x0d8d818c, 0x08080008,
	0x1f0f131c, 0x19899198, 0x00000000, 0x19091118,
	0x04040004, 0x13435350, 0x37c7f3f4, 0x21c1e1e0,
	0x3dcdf1fc, 0x36467274, 0x2f0f232c, 0x27072324,
	0x3080b0b0, 0x0b8b8388, 0x0e0e020c, 0x2b8ba3a8,
	0x2282a2a0, 0x2e4e626c, 0x13839390, 0x0d4d414c,
	0x29496168, 0x3c4c707c, 0x09090108, 0x0a0a0208,
	0x3f8fb3bc, 0x2fcfe3ec, 0x33c3f3f0, 0x05c5c1c4,
	0x07878384, 0x14041014, 0x3ecef2fc, 0x24446064,
	0x1eced2dc, 0x2e0e222c, 0x0b4b4348, 0x1a0a1218,
	0x06060204, 0x21012120, 0x2b4b6368, 0x26466264,
	0x02020200, 0x35c5f1f4, 0x12829290, 0x0a8a8288,
	0x0c0c000c, 0x3383b3b0, 0x3e4e727c, 0x10c0d0d0,
	0x3a4a7278, 0x07474344, 0x16869294, 0x25c5e1e4,
	0x26062224, 0x00808080, 0x2d8da1ac, 0x1fcfd3dc,
	0x2181a1a0, 0x30003030, 0x37073334, 0x2e8ea2ac,
	0x36063234, 0x15051114, 0x22022220, 0x38083038,
	0x34c4f0f4, 0x2787a3a4, 0x05454144, 0x0c4c404c,
	0x01818180, 0x29c9e1e8, 0x04848084, 0x17879394,
	0x35053134, 0x0bcbc3c8, 0x0ecec2cc, 0x3c0c303c,
	0x31417170, 0x11011110, 0x07c7c3c4, 0x09898188,
	0x35457174, 0x3bcbf3f8, 0x1acad2d8, 0x38c8f0f8,
	0x14849094, 0x19495158, 0x02828280, 0x04c4c0c4,
	0x3fcff3fc, 0x09494148, 0x39093138, 0x27476364,
	0x00c0c0c0, 0x0fcfc3cc, 0x17c7d3d4, 0x3888b0b8,
	0x0f0f030c, 0x0e8e828c, 0x02424240, 0x23032320,
	0x11819190, 0x2c4c606c, 0x1bcbd3d8, 0x2484a0a4,
	0x34043034, 0x31c1f1f0, 0x08484048, 0x02c2c2c0,
	0x2f4f636c, 0x3d0d313c, 0x2d0d212c, 0x00404040,
	0x3e8eb2bc, 0x3e0e323c, 0x3c8cb0bc, 0x01c1c1c0,
	0x2a8aa2a8, 0x3a8ab2b8, 0x0e4e424c, 0x15455154,
	0x3b0b3338, 0x1cccd0dc, 0x28486068, 0x3f4f737c,
	0x1c8c909c, 0x18c8d0d8, 0x0a4a4248, 0x16465254,
	0x37477374, 0x2080a0a0, 0x2dcde1ec, 0x06464244,
	0x3585b1b4, 0x2b0b2328, 0x25456164, 0x3acaf2f8,
	0x23c3e3e0, 0x3989b1b8, 0x3181b1b0, 0x1f8f939c,
	0x1e4e525c, 0x39c9f1f8, 0x26c6e2e4, 0x3282b2b0,
	0x31013130, 0x2acae2e8, 0x2d4d616c, 0x1f4f535c,
	0x24c4e0e4, 0x30c0f0f0, 0x0dcdc1cc, 0x08888088,
	0x16061214, 0x3a0a3238, 0x18485058, 0x14c4d0d4,
	0x22426260, 0x29092128, 0x07070304, 0x33033330,
	0x28c8e0e8, 0x1b0b1318, 0x05050104, 0x39497178,
	0x10809090, 0x2a4a6268, 0x2a0a2228, 0x1a8a9298,
};

static const u32 SS1[256] = {
	0x38380830, 0xe828c8e0, 0x2c2d0d21, 0xa42686a2,
	0xcc0fcfc3, 0xdc1eced2, 0xb03383b3, 0xb83888b0,
	0xac2f8fa3, 0x60204060, 0x54154551, 0xc407c7c3,
	0x44044440, 0x6c2f4f63, 0x682b4b63, 0x581b4b53,
	0xc003c3c3, 0x60224262, 0x30330333, 0xb43585b1,
	0x28290921, 0xa02080a0, 0xe022c2e2, 0xa42787a3,
	0xd013c3d3, 0x90118191, 0x10110111, 0x04060602,
	0x1c1c0c10, 0xbc3c8cb0, 0x34360632, 0x480b4b43,
	0xec2fcfe3, 0x88088880, 0x6c2c4c60, 0xa82888a0,
	0x14170713, 0xc404c4c0, 0x14160612, 0xf434c4f0,
	0xc002c2c2, 0x44054541, 0xe021c1e1, 0xd416c6d2,
	0x3c3f0f33, 0x3c3d0d31, 0x8c0e8e82, 0x98188890,
	0x28280820, 0x4c0e4e42, 0xf436c6f2, 0x3c3e0e32,
	0xa42585a1, 0xf839c9f1, 0x0c0d0d01, 0xdc1fcfd3,
	0xd818c8d0, 0x282b0b23, 0x64264662, 0x783a4a72,
	0x24270723, 0x2c2f0f23, 0xf031c1f1, 0x70324272,
	0x40024242, 0xd414c4d0, 0x40014141, 0xc000c0c0,
	0x70334373, 0x64274763, 0xac2c8ca0, 0x880b8b83,
	0xf437c7f3, 0xac2d8da1, 0x80008080, 0x1c1f0f13,
	0xc80acac2, 0x2c2c0c20, 0xa82a8aa2, 0x34340430,
	0xd012c2d2, 0x080b0b03, 0xec2ecee2, 0xe829c9e1,
	0x5c1d4d51, 0x94148490, 0x18180810, 0xf838c8f0,
	0x54174753, 0xac2e8ea2, 0x08080800, 0xc405c5c1,
	0x10130313, 0xcc0dcdc1, 0x84068682, 0xb83989b1,
	0xfc3fcff3, 0x7c3d4d71, 0xc001c1c1, 0x30310131,
	0xf435c5f1, 0x880a8a82, 0x682a4a62, 0xb03181b1,
	0xd011c1d1, 0x20200020, 0xd417c7d3, 0x00020202,
	0x20220222, 0x04040400, 0x68284860, 0x70314171,
	0x04070703, 0xd81bcbd3, 0x9c1d8d91, 0x98198991,
	0x60214161, 0xbc3e8eb2, 0xe426c6e2, 0x58194951,
	0xdc1dcdd1, 0x50114151, 0x90108090, 0xdc1cccd0,
	0x981a8a92, 0xa02383a3, 0xa82b8ba3, 0xd010c0d0,
	0x80018181, 0x0c0f0f03, 0x44074743, 0x181a0a12,
	0xe023c3e3, 0xec2ccce0, 0x8c0d8d81, 0xbc3f8fb3,
	0x94168692, 0x783b4b73, 0x5c1c4c50, 0xa02282a2,
	0xa02181a1, 0x60234363, 0x20230323, 0x4c0d4d41,
	0xc808c8c0, 0x9c1e8e92, 0x9c1c8c90, 0x383a0a32,
	0x0c0c0c00, 0x2c2e0e22, 0xb83a8ab2, 0x6c2e4e62,
	0x9c1f8f93, 0x581a4a52, 0xf032c2f2, 0x90128292,
	0xf033c3f3, 0x48094941, 0x78384870, 0xcc0cccc0,
	0x14150511, 0xf83bcbf3, 0x70304070, 0x74354571,
	0x7c3f4f73, 0x34350531, 0x10100010, 0x00030303,
	0x64244460, 0x6c2d4d61, 0xc406c6c2, 0x74344470,
	0xd415c5d1, 0xb43484b0, 0xe82acae2, 0x08090901,
	0x74364672, 0x18190911, 0xfc3ecef2, 0x40004040,
	0x10120212, 0xe020c0e0, 0xbc3d8db1, 0x04050501,
	0xf83acaf2, 0x00010101, 0xf030c0f0, 0x282a0a22,
	0x5c1e4e52, 0xa82989a1, 0x54164652, 0x40034343,
	0x84058581, 0x14140410, 0x88098981, 0x981b8b93,
	0xb03080b0, 0xe425c5e1, 0x48084840, 0x78394971,
	0x94178793, 0xfc3cccf0, 0x1c1e0e12, 0x80028282,
	0x20210121, 0x8c0c8c80, 0x181b0b13, 0x5c1f4f53,
	0x74374773, 0x54144450, 0xb03282b2, 0x1c1d0d11,
	0x24250521, 0x4c0f4f43, 0x00000000, 0x44064642,
	0xec2dcde1, 0x58184850, 0x50124252, 0xe82bcbe3,
	0x7c3e4e72, 0xd81acad2, 0xc809c9c1, 0xfc3dcdf1,
	0x30300030, 0x94158591, 0x64254561, 0x3c3c0c30,
	0xb43686b2, 0xe424c4e0, 0xb83b8bb3, 0x7c3c4c70,
	0x0c0e0e02, 0x50104050, 0x38390931, 0x24260622,
	0x30320232, 0x84048480, 0x68294961, 0x90138393,
	0x34370733, 0xe427c7e3, 0x24240420, 0xa42484a0,
	0xc80bcbc3, 0x50134353, 0x080a0a02, 0x84078783,
	0xd819c9d1, 0x4c0c4c40, 0x80038383, 0x8c0f8f83,
	0xcc0ecec2, 0x383b0b33, 0x480a4a42, 0xb43787b3,
};

static const u32 SS2[256] = {
	0xa1a82989, 0x81840585, 0xd2d416c6, 0xd3d013c3,
	0x50541444, 0x111c1d0d, 0xa0ac2c8c, 0x21242505,
	0x515c1d4d, 0x43400343, 0x10181808, 0x121c1e0e,
	0x51501141, 0xf0fc3ccc, 0xc2c80aca, 0x63602343,
	0x20282808, 0x40440444, 0x20202000, 0x919c1d8d,
	0xe0e020c0, 0xe2e022c2, 0xc0c808c8, 0x13141707,
	0xa1a42585, 0x838c0f8f, 0x03000303, 0x73783b4b,
	0xb3b83b8b, 0x13101303, 0xd2d012c2, 0xe2ec2ece,
	0x70703040, 0x808c0c8c, 0x333c3f0f, 0xa0a82888,
	0x32303202, 0xd1dc1dcd, 0xf2f436c6, 0x70743444,
	0xe0ec2ccc, 0x91941585, 0x03080b0b, 0x53541747,
	0x505c1c4c, 0x53581b4b, 0xb1bc3d8d, 0x01000101,
	0x20242404, 0x101c1c0c, 0x73703343, 0x90981888,
	0x10101000, 0xc0cc0ccc, 0xf2f032c2, 0xd1d819c9,
	0x202c2c0c, 0xe3e427c7, 0x72703242, 0x83800383,
	0x93981b8b, 0xd1d011c1, 0x82840686, 0xc1c809c9,
	0x60602040, 0x50501040, 0xa3a02383, 0xe3e82bcb,
	0x010c0d0d, 0xb2b43686, 0x929c1e8e, 0x434c0f4f,
	0xb3b43787, 0x52581a4a, 0xc2c406c6, 0x70783848,
	0xa2a42686, 0x12101202, 0xa3ac2f8f, 0xd1d415c5,
	0x61602141, 0xc3c003c3, 0xb0b43484, 0x41400141,
	0x52501242, 0x717c3d4d, 0x818c0d8d, 0x00080808,
	0x131c1f0f, 0x91981989, 0x00000000, 0x11181909,
	0x00040404, 0x53501343, 0xf3f437c7, 0xe1e021c1,
	0xf1fc3dcd, 0x72743646, 0x232c2f0f, 0x23242707,
	0xb0b03080, 0x83880b8b, 0x020c0e0e, 0xa3a82b8b,
	0xa2a02282, 0x626c2e4e, 0x93901383, 0x414c0d4d,
	0x61682949, 0x707c3c4c, 0x01080909, 0x02080a0a,
	0xb3bc3f8f, 0xe3ec2fcf, 0xf3f033c3, 0xc1c405c5,
	0x83840787, 0x10141404, 0xf2fc3ece, 0x60642444,
	0xd2dc1ece, 0x222c2e0e, 0x43480b4b, 0x12181a0a,
	0x02040606, 0x21202101, 0x63682b4b, 0x62642646,
	0x02000202, 0xf1f435c5, 0x92901282, 0x82880a8a,
	0x000c0c0c, 0xb3b03383, 0x727c3e4e, 0xd0d010c0,
	0x72783a4a, 0x43440747, 0x92941686, 0xe1e425c5,
	0x22242606, 0x80800080, 0xa1ac2d8d, 0xd3dc1fcf,
	0xa1a02181, 0x30303000, 0x33343707, 0xa2ac2e8e,
	0x32343606, 0x11141505, 0x22202202, 0x30383808,
	0xf0f434c4, 0xa3a42787, 0x41440545, 0x404c0c4c,
	0x81800181, 0xe1e829c9, 0x80840484, 0x93941787,
	0x31343505, 0xc3c80bcb, 0xc2cc0ece, 0x303c3c0c,
	0x71703141, 0x11101101, 0xc3c407c7, 0x81880989,
	0x71743545, 0xf3f83bcb, 0xd2d81aca, 0xf0f838c8,
	0x90941484, 0x51581949, 0x82800282, 0xc0c404c4,
	0xf3fc3fcf, 0x41480949, 0x31383909, 0x63642747,
	0xc0c000c0, 0xc3cc0fcf, 0xd3d417c7, 0xb0b83888,
	0x030c0f0f, 0x828c0e8e, 0x42400242, 0x23202303,
	0x91901181, 0x606c2c4c, 0xd3d81bcb, 0xa0a42484,
	0x30343404, 0xf1f031c1, 0x40480848, 0xc2c002c2,
	0x636c2f4f, 0x313c3d0d, 0x212c2d0d, 0x40400040,
	0xb2bc3e8e, 0x323c3e0e, 0xb0bc3c8c, 0xc1c001c1,
	0xa2a82a8a, 0xb2b83a8a, 0x424c0e4e, 0x51541545,
	0x33383b0b, 0xd0dc1ccc, 0x60682848, 0x737c3f4f,
	0x909c1c8c, 0xd0d818c8, 0x42480a4a, 0x52541646,
	0x73743747, 0xa0a02080, 0xe1ec2dcd, 0x42440646,
	0xb1b43585, 0x23282b0b, 0x61642545, 0xf2f83aca,
	0xe3e023c3, 0xb1b83989, 0xb1b03181, 0x939c1f8f,
	0x525c1e4e, 0xf1f839c9, 0xe2e426c6, 0xb2b03282,
	0x31303101, 0xe2e82aca, 0x616c2d4d, 0x535c1f4f,
	0xe0e424c4, 0xf0f030c0, 0xc1cc0dcd, 0x80880888,
	0x12141606, 0x32383a0a, 0x50581848, 0xd0d414c4,
	0x62602242, 0x21282909, 0x03040707, 0x33303303,
	0xe0e828c8, 0x13181b0b, 0x01040505, 0x71783949,
	0x90901080, 0x62682a4a, 0x22282a0a, 0x92981a8a,
};

static const u32 SS3[256] = {
	0x08303838, 0xc8e0e828, 0x0d212c2d, 0x86a2a426,
	0xcfc3cc0f, 0xced2dc1e, 0x83b3b033, 0x88b0b838,
	0x8fa3ac2f, 0x40606020, 0x45515415, 0xc7c3c407,
	0x44404404, 0x4f636c2f, 0x4b63682b, 0x4b53581b,
	0xc3c3c003, 0x42626022, 0x03333033, 0x85b1b435,
	0x09212829, 0x80a0a020, 0xc2e2e022, 0x87a3a427,
	0xc3d3d013, 0x81919011, 0x01111011, 0x06020406,
	0x0c101c1c, 0x8cb0bc3c, 0x06323436, 0x4b43480b,
	0xcfe3ec2f, 0x88808808, 0x4c606c2c, 0x88a0a828,
	0x07131417, 0xc4c0c404, 0x06121416, 0xc4f0f434,
	0xc2c2c002, 0x45414405, 0xc1e1e021, 0xc6d2d416,
	0x0f333c3f, 0x0d313c3d, 0x8e828c0e, 0x88909818,
	0x08202828, 0x4e424c0e, 0xc6f2f436, 0x0e323c3e,
	0x85a1a425, 0xc9f1f839, 0x0d010c0d, 0xcfd3dc1f,
	0xc8d0d818, 0x0b23282b, 0x46626426, 0x4a72783a,
	0x07232427, 0x0f232c2f, 0xc1f1f031, 0x42727032,
	0x42424002, 0xc4d0d414, 0x41414001, 0xc0c0c000,
	0x43737033, 0x47636427, 0x8ca0ac2c, 0x8b83880b,
	0xc7f3f437, 0x8da1ac2d, 0x80808000, 0x0f131c1f,
	0xcac2c80a, 0x0c202c2c, 0x8aa2a82a, 0x04303434,
	0xc2d2d012, 0x0b03080b, 0xcee2ec2e, 0xc9e1e829,
	0x4d515c1d, 0x84909414, 0x08101818, 0xc8f0f838,
	0x47535417, 0x8ea2ac2e, 0x08000808, 0xc5c1c405,
	0x03131013, 0xcdc1cc0d, 0x86828406, 0x89b1b839,
	0xcff3fc3f, 0x4d717c3d, 0xc1c1c001, 0x01313031,
	0xc5f1f435, 0x8a82880a, 0x4a62682a, 0x81b1b031,
	0xc1d1d011, 0x00202020, 0xc7d3d417, 0x02020002,
	0x02222022, 0x04000404, 0x48606828, 0x41717031,
	0x07030407, 0xcbd3d81b, 0x8d919c1d, 0x89919819,
	0x41616021, 0x8eb2bc3e, 0xc6e2e426, 0x49515819,
	0xcdd1dc1d, 0x41515011, 0x80909010, 0xccd0dc1c,
	0x8a92981a, 0x83a3a023, 0x8ba3a82b, 0xc0d0d010,
	0x81818001, 0x0f030c0f, 0x47434407, 0x0a12181a,
	0xc3e3e023, 0xcce0ec2c, 0x8d818c0d, 0x8fb3bc3f,
	0x86929416, 0x4b73783b, 0x4c505c1c, 0x82a2a022,
	0x81a1a021, 0x43636023, 0x03232023, 0x4d414c0d,
	0xc8c0c808, 0x8e929c1e, 0x8c909c1c, 0x0a32383a,
	0x0c000c0c, 0x0e222c2e, 0x8ab2b83a, 0x4e626c2e,
	0x8f939c1f, 0x4a52581a, 0xc2f2f032, 0x82929012,
	0xc3f3f033, 0x49414809, 0x48707838, 0xccc0cc0c,
	0x05111415, 0xcbf3f83b, 0x40707030, 0x45717435,
	0x4f737c3f, 0x05313435, 0x00101010, 0x03030003,
	0x44606424, 0x4d616c2d, 0xc6c2c406, 0x44707434,
	0xc5d1d415, 0x84b0b434, 0xcae2e82a, 0x09010809,
	0x46727436, 0x09111819, 0xcef2fc3e, 0x40404000,
	0x02121012, 0xc0e0e020, 0x8db1bc3d, 0x05010405,
	0xcaf2f83a, 0x01010001, 0xc0f0f030, 0x0a22282a,
	0x4e525c1e, 0x89a1a829, 0x46525416, 0x43434003,
	0x85818405, 0x04101414, 0x89818809, 0x8b93981b,
	0x80b0b030, 0xc5e1e425, 0x48404808, 0x49717839,
	0x87939417, 0xccf0fc3c, 0x0e121c1e, 0x82828002,
	0x01212021, 0x8c808c0c, 0x0b13181b, 0x4f535c1f,
	0x47737437, 0x44505414, 0x82b2b032, 0x0d111c1d,
	0x05212425, 0x4f434c0f, 0x00000000, 0x46424406,
	0xcde1ec2d, 0x48505818, 0x42525012, 0xcbe3e82b,
	0x4e727c3e, 0xcad2d81a, 0xc9c1c809, 0xcdf1fc3d,
	0x00303030, 0x85919415, 0x45616425, 0x0c303c3c,
	0x86b2b436, 0xc4e0e424, 0x8bb3b83b, 0x4c707c3c,
	0x0e020c0e, 0x40505010, 0x09313839, 0x06222426,
	0x02323032, 0x84808404, 0x49616829, 0x83939013,
	0x07333437, 0xc7e3e427, 0x04202424, 0x84a0a424,
	0xcbc3c80b, 0x43535013, 0x0a02080a, 0x87838407,
	0xc9d1d819, 0x4c404c0c, 0x83838003, 0x8f838c0f,
	0xcec2cc0e, 0x0b33383b, 0x4a42480a, 0x87b3b437,
};

static const u32 KC[SEED_NUM_KCONSTANTS] = {
	0x9e3779b9, 0x3c6ef373, 0x78dde6e6, 0xf1bbcdcc,
	0xe3779b99, 0xc6ef3733, 0x8dde6e67, 0x1bbcdccf,
	0x3779b99e, 0x6ef3733c, 0xdde6e678, 0xbbcdccf1,
	0x779b99e3, 0xef3733c6, 0xde6e678d, 0xbcdccf1b,
};

#define OP(X1, X2, X3, X4, rbase)			\
	t0 = X3 ^ ks[rbase];				\
	t1 = X4 ^ ks[rbase+1];				\
	t1 ^= t0;					\
	t1 = SS0[byte(t1, 0)] ^ SS1[byte(t1, 1)] ^	\
		SS2[byte(t1, 2)] ^ SS3[byte(t1, 3)];	\
	t0 += t1;					\
	t0 = SS0[byte(t0, 0)] ^ SS1[byte(t0, 1)] ^	\
		SS2[byte(t0, 2)] ^ SS3[byte(t0, 3)];	\
	t1 += t0;					\
	t1 = SS0[byte(t1, 0)] ^ SS1[byte(t1, 1)] ^	\
		SS2[byte(t1, 2)] ^ SS3[byte(t1, 3)];	\
	t0 += t1;					\
	X1 ^= t0;					\
	X2 ^= t1;

static int seed_set_key(struct crypto_tfm *tfm, const u8 *in_key,
		        unsigned int key_len)
{
	struct seed_ctx *ctx = crypto_tfm_ctx(tfm);
	u32 *keyout = ctx->keysched;
	const __be32 *key = (const __be32 *)in_key;
	u32 i, t0, t1, x1, x2, x3, x4;

	x1 = be32_to_cpu(key[0]);
	x2 = be32_to_cpu(key[1]);
	x3 = be32_to_cpu(key[2]);
	x4 = be32_to_cpu(key[3]);

	for (i = 0; i < SEED_NUM_KCONSTANTS; i++) {
		t0 = x1 + x3 - KC[i];
		t1 = x2 + KC[i] - x4;
		*(keyout++) = SS0[byte(t0, 0)] ^ SS1[byte(t0, 1)] ^
				SS2[byte(t0, 2)] ^ SS3[byte(t0, 3)];
		*(keyout++) = SS0[byte(t1, 0)] ^ SS1[byte(t1, 1)] ^
				SS2[byte(t1, 2)] ^ SS3[byte(t1, 3)];

		if (i % 2 == 0) {
			t0 = x1;
			x1 = (x1 >> 8) ^ (x2 << 24);
			x2 = (x2 >> 8) ^ (t0 << 24);
		} else {
			t0 = x3;
			x3 = (x3 << 8) ^ (x4 >> 24);
			x4 = (x4 << 8) ^ (t0 >> 24);
		}
	}

	return 0;
}

/* encrypt a block of text */

static void seed_encrypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
{
	const struct seed_ctx *ctx = crypto_tfm_ctx(tfm);
	const __be32 *src = (const __be32 *)in;
	__be32 *dst = (__be32 *)out;
	u32 x1, x2, x3, x4, t0, t1;
	const u32 *ks = ctx->keysched;

	x1 = be32_to_cpu(src[0]);
	x2 = be32_to_cpu(src[1]);
	x3 = be32_to_cpu(src[2]);
	x4 = be32_to_cpu(src[3]);

	OP(x1, x2, x3, x4, 0);
	OP(x3, x4, x1, x2, 2);
	OP(x1, x2, x3, x4, 4);
	OP(x3, x4, x1, x2, 6);
	OP(x1, x2, x3, x4, 8);
	OP(x3, x4, x1, x2, 10);
	OP(x1, x2, x3, x4, 12);
	OP(x3, x4, x1, x2, 14);
	OP(x1, x2, x3, x4, 16);
	OP(x3, x4, x1, x2, 18);
	OP(x1, x2, x3, x4, 20);
	OP(x3, x4, x1, x2, 22);
	OP(x1, x2, x3, x4, 24);
	OP(x3, x4, x1, x2, 26);
	OP(x1, x2, x3, x4, 28);
	OP(x3, x4, x1, x2, 30);

	dst[0] = cpu_to_be32(x3);
	dst[1] = cpu_to_be32(x4);
	dst[2] = cpu_to_be32(x1);
	dst[3] = cpu_to_be32(x2);
}

/* decrypt a block of text */

static void seed_decrypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
{
	const struct seed_ctx *ctx = crypto_tfm_ctx(tfm);
	const __be32 *src = (const __be32 *)in;
	__be32 *dst = (__be32 *)out;
	u32 x1, x2, x3, x4, t0, t1;
	const u32 *ks = ctx->keysched;

	x1 = be32_to_cpu(src[0]);
	x2 = be32_to_cpu(src[1]);
	x3 = be32_to_cpu(src[2]);
	x4 = be32_to_cpu(src[3]);

	OP(x1, x2, x3, x4, 30);
	OP(x3, x4, x1, x2, 28);
	OP(x1, x2, x3, x4, 26);
	OP(x3, x4, x1, x2, 24);
	OP(x1, x2, x3, x4, 22);
	OP(x3, x4, x1, x2, 20);
	OP(x1, x2, x3, x4, 18);
	OP(x3, x4, x1, x2, 16);
	OP(x1, x2, x3, x4, 14);
	OP(x3, x4, x1, x2, 12);
	OP(x1, x2, x3, x4, 10);
	OP(x3, x4, x1, x2, 8);
	OP(x1, x2, x3, x4, 6);
	OP(x3, x4, x1, x2, 4);
	OP(x1, x2, x3, x4, 2);
	OP(x3, x4, x1, x2, 0);

	dst[0] = cpu_to_be32(x3);
	dst[1] = cpu_to_be32(x4);
	dst[2] = cpu_to_be32(x1);
	dst[3] = cpu_to_be32(x2);
}


static struct crypto_alg seed_alg = {
	.cra_name		=	"seed",
	.cra_driver_name	=	"seed-generic",
	.cra_priority		=	100,
	.cra_flags		=	CRYPTO_ALG_TYPE_CIPHER,
	.cra_blocksize		=	SEED_BLOCK_SIZE,
	.cra_ctxsize		=	sizeof(struct seed_ctx),
	.cra_alignmask		=	3,
	.cra_module		=	THIS_MODULE,
	.cra_u			=	{
		.cipher = {
			.cia_min_keysize	=	SEED_KEY_SIZE,
			.cia_max_keysize	=	SEED_KEY_SIZE,
			.cia_setkey		=	seed_set_key,
			.cia_encrypt		=	seed_encrypt,
			.cia_decrypt		=	seed_decrypt
		}
	}
};

static int __init seed_init(void)
{
	return crypto_register_alg(&seed_alg);
}

static void __exit seed_fini(void)
{
	crypto_unregister_alg(&seed_alg);
}

module_init(seed_init);
module_exit(seed_fini);

MODULE_DESCRIPTION("SEED Cipher Algorithm");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hye-Shik Chang <perky@FreeBSD.org>, Kim Hyun <hkim@kisa.or.kr>");
MODULE_ALIAS_CRYPTO("seed");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * seqiv: Sequence Number IV Generator
 *
 * This generator generates an IV based on a sequence number by xoring it
 * with a salt.  This algorithm is mainly useful for CTR and similar modes.
 *
 * Copyright (c) 2007 Herbert Xu <herbert@gondor.apana.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#include <crypto/internal/geniv.h>
#include <crypto/internal/skcipher.h>
#include <crypto/rng.h>
#include <crypto/scatterwalk.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>

struct seqiv_ctx {
	spinlock_t lock;
	u8 salt[] __attribute__ ((aligned(__alignof__(u32))));
};

static void seqiv_free(struct crypto_instance *inst);

static void seqiv_complete2(struct skcipher_givcrypt_request *req, int err)
{
	struct ablkcipher_request *subreq = skcipher_givcrypt_reqctx(req);
	struct crypto_ablkcipher *geniv;

	if (err == -EINPROGRESS)
		return;

	if (err)
		goto out;

	geniv = skcipher_givcrypt_reqtfm(req);
	memcpy(req->creq.info, subreq->info, crypto_ablkcipher_ivsize(geniv));

out:
	kfree(subreq->info);
}

static void seqiv_complete(struct crypto_async_request *base, int err)
{
	struct skcipher_givcrypt_request *req = base->data;

	seqiv_complete2(req, err);
	skcipher_givcrypt_complete(req, err);
}

static void seqiv_aead_encrypt_complete2(struct aead_request *req, int err)
{
	struct aead_request *subreq = aead_request_ctx(req);
	struct crypto_aead *geniv;

	if (err == -EINPROGRESS)
		return;

	if (err)
		goto out;

	geniv = crypto_aead_reqtfm(req);
	memcpy(req->iv, subreq->iv, crypto_aead_ivsize(geniv));

out:
	kzfree(subreq->iv);
}

static void seqiv_aead_encrypt_complete(struct crypto_async_request *base,
					int err)
{
	struct aead_request *req = base->data;

	seqiv_aead_encrypt_complete2(req, err);
	aead_request_complete(req, err);
}

static void seqiv_geniv(struct seqiv_ctx *ctx, u8 *info, u64 seq,
			unsigned int ivsize)
{
	unsigned int len = ivsize;

	if (ivsize > sizeof(u64)) {
		memset(info, 0, ivsize - sizeof(u64));
		len = sizeof(u64);
	}
	seq = cpu_to_be64(seq);
	memcpy(info + ivsize - len, &seq, len);
	crypto_xor(info, ctx->salt, ivsize);
}

static int seqiv_givencrypt(struct skcipher_givcrypt_request *req)
{
	struct crypto_ablkcipher *geniv = skcipher_givcrypt_reqtfm(req);
	struct seqiv_ctx *ctx = crypto_ablkcipher_ctx(geniv);
	struct ablkcipher_request *subreq = skcipher_givcrypt_reqctx(req);
	crypto_completion_t compl;
	void *data;
	u8 *info;
	unsigned int ivsize;
	int err;

	ablkcipher_request_set_tfm(subreq, skcipher_geniv_cipher(geniv));

	compl = req->creq.base.complete;
	data = req->creq.base.data;
	info = req->creq.info;

	ivsize = crypto_ablkcipher_ivsize(geniv);

	if (unlikely(!IS_ALIGNED((unsigned long)info,
				 crypto_ablkcipher_alignmask(geniv) + 1))) {
		info = kmalloc(ivsize, req->creq.base.flags &
				       CRYPTO_TFM_REQ_MAY_SLEEP ? GFP_KERNEL:
								  GFP_ATOMIC);
		if (!info)
			return -ENOMEM;

		compl = seqiv_complete;
		data = req;
	}

	ablkcipher_request_set_callback(subreq, req->creq.base.flags, compl,
					data);
	ablkcipher_request_set_crypt(subreq, req->creq.src, req->creq.dst,
				     req->creq.nbytes, info);

	seqiv_geniv(ctx, info, req->seq, ivsize);
	memcpy(req->giv, info, ivsize);

	err = crypto_ablkcipher_encrypt(subreq);
	if (unlikely(info != req->creq.info))
		seqiv_complete2(req, err);
	return err;
}

static int seqiv_aead_encrypt(struct aead_request *req)
{
	struct crypto_aead *geniv = crypto_aead_reqtfm(req);
	struct aead_geniv_ctx *ctx = crypto_aead_ctx(geniv);
	struct aead_request *subreq = aead_request_ctx(req);
	crypto_completion_t compl;
	void *data;
	u8 *info;
	unsigned int ivsize = 8;
	int err;

	if (req->cryptlen < ivsize)
		return -EINVAL;

	aead_request_set_tfm(subreq, ctx->child);

	compl = req->base.complete;
	data = req->base.data;
	info = req->iv;

	if (req->src != req->dst) {
		struct blkcipher_desc desc = {
			.tfm = ctx->null,
		};

		err = crypto_blkcipher_encrypt(&desc, req->dst, req->src,
					       req->assoclen + req->cryptlen);
		if (err)
			return err;
	}

	if (unlikely(!IS_ALIGNED((unsigned long)info,
				 crypto_aead_alignmask(geniv) + 1))) {
		info = kmalloc(ivsize, req->base.flags &
				       CRYPTO_TFM_REQ_MAY_SLEEP ? GFP_KERNEL:
								  GFP_ATOMIC);
		if (!info)
			return -ENOMEM;

		memcpy(info, req->iv, ivsize);
		compl = seqiv_aead_encrypt_complete;
		data = req;
	}

	aead_request_set_callback(subreq, req->base.flags, compl, data);
	aead_request_set_crypt(subreq, req->dst, req->dst,
			       req->cryptlen - ivsize, info);
	aead_request_set_ad(subreq, req->assoclen + ivsize);

	crypto_xor(info, ctx->salt, ivsize);
	scatterwalk_map_and_copy(info, req->dst, req->assoclen, ivsize, 1);

	err = crypto_aead_encrypt(subreq);
	if (unlikely(info != req->iv))
		seqiv_aead_encrypt_complete2(req, err);
	return err;
}

static int seqiv_aead_decrypt(struct aead_request *req)
{
	struct crypto_aead *geniv = crypto_aead_reqtfm(req);
	struct aead_geniv_ctx *ctx = crypto_aead_ctx(geniv);
	struct aead_request *subreq = aead_request_ctx(req);
	crypto_completion_t compl;
	void *data;
	unsigned int ivsize = 8;

	if (req->cryptlen < ivsize + crypto_aead_authsize(geniv))
		return -EINVAL;

	aead_request_set_tfm(subreq, ctx->child);

	compl = req->base.complete;
	data = req->base.data;

	aead_request_set_callback(subreq, req->base.flags, compl, data);
	aead_request_set_crypt(subreq, req->src, req->dst,
			       req->cryptlen - ivsize, req->iv);
	aead_request_set_ad(subreq, req->assoclen + ivsize);

	scatterwalk_map_and_copy(req->iv, req->src, req->assoclen, ivsize, 0);

	return crypto_aead_decrypt(subreq);
}

static int seqiv_init(struct crypto_tfm *tfm)
{
	struct crypto_ablkcipher *geniv = __crypto_ablkcipher_cast(tfm);
	struct seqiv_ctx *ctx = crypto_ablkcipher_ctx(geniv);
	int err;

	spin_lock_init(&ctx->lock);

	tfm->crt_ablkcipher.reqsize = sizeof(struct ablkcipher_request);

	err = 0;
	if (!crypto_get_default_rng()) {
		crypto_ablkcipher_crt(geniv)->givencrypt = seqiv_givencrypt;
		err = crypto_rng_get_bytes(crypto_default_rng, ctx->salt,
					   crypto_ablkcipher_ivsize(geniv));
		crypto_put_default_rng();
	}

	return err ?: skcipher_geniv_init(tfm);
}

static int seqiv_ablkcipher_create(struct crypto_template *tmpl,
				   struct rtattr **tb)
{
	struct crypto_instance *inst;
	int err;

	inst = skcipher_geniv_alloc(tmpl, tb, 0, 0);

	if (IS_ERR(inst))
		return PTR_ERR(inst);

	err = -EINVAL;
	if (inst->alg.cra_ablkcipher.ivsize < sizeof(u64))
		goto free_inst;

	inst->alg.cra_init = seqiv_init;
	inst->alg.cra_exit = skcipher_geniv_exit;

	inst->alg.cra_ctxsize += inst->alg.cra_ablkcipher.ivsize;
	inst->alg.cra_ctxsize += sizeof(struct seqiv_ctx);

	inst->alg.cra_alignmask |= __alignof__(u32) - 1;

	err = crypto_register_instance(tmpl, inst);
	if (err)
		goto free_inst;

out:
	return err;

free_inst:
	skcipher_geniv_free(inst);
	goto out;
}

static int seqiv_aead_create(struct crypto_template *tmpl, struct rtattr **tb)
{
	struct aead_instance *inst;
	struct crypto_aead_spawn *spawn;
	struct aead_alg *alg;
	int err;

	inst = aead_geniv_alloc(tmpl, tb, 0, 0);

	if (IS_ERR(inst))
		return PTR_ERR(inst);

	inst->alg.base.cra_alignmask |= __alignof__(u32) - 1;

	spawn = aead_instance_ctx(inst);
	alg = crypto_spawn_aead_alg(spawn);

	err = -EINVAL;
	if (inst->alg.ivsize != sizeof(u64))
		goto free_inst;

	inst->alg.encrypt = seqiv_aead_encrypt;
	inst->alg.decrypt = seqiv_aead_decrypt;

	inst->alg.init = aead_init_geniv;
	inst->alg.exit = aead_exit_geniv;

	inst->alg.base.cra_ctxsize = sizeof(struct aead_geniv_ctx);
	inst->alg.base.cra_ctxsize += inst->alg.ivsize;

	err = aead_register_instance(tmpl, inst);
	if (err)
		goto free_inst;

out:
	return err;

free_inst:
	aead_geniv_free(inst);
	goto out;
}

static int seqiv_create(struct crypto_template *tmpl, struct rtattr **tb)
{
	struct crypto_attr_type *algt;
	int err;

	algt = crypto_get_attr_type(tb);
	if (IS_ERR(algt))
		return PTR_ERR(algt);

	if ((algt->type ^ CRYPTO_ALG_TYPE_AEAD) & CRYPTO_ALG_TYPE_MASK)
		err = seqiv_ablkcipher_create(tmpl, tb);
	else
		err = seqiv_aead_create(tmpl, tb);

	return err;
}

static void seqiv_free(struct crypto_instance *inst)
{
	if ((inst->alg.cra_flags ^ CRYPTO_ALG_TYPE_AEAD) & CRYPTO_ALG_TYPE_MASK)
		skcipher_geniv_free(inst);
	else
		aead_geniv_free(aead_instance(inst));
}

static struct crypto_template seqiv_tmpl = {
	.name = "seqiv",
	.create = seqiv_create,
	.free = seqiv_free,
	.module = THIS_MODULE,
};

static int __init seqiv_module_init(void)
{
	return crypto_register_template(&seqiv_tmpl);
}

static void __exit seqiv_module_exit(void)
{
	crypto_unregister_template(&seqiv_tmpl);
}

module_init(seqiv_module_init);
module_exit(seqiv_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Sequence Number IV Generator");
MODULE_ALIAS_CRYPTO("seqiv");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*
 * Cryptographic API.
 *
 * Serpent Cipher Algorithm.
 *
 * Copyright (C) 2002 Dag Arne Osvik <osvik@ii.uib.no>
 *               2003 Herbert Valerio Riedel <hvr@gnu.org>
 *
 * Added tnepres support:
 *		Ruben Jesus Garcia Hernandez <ruben@ugr.es>, 18.10.2004
 *              Based on code by hvr
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <asm/byteorder.h>
#include <linux/crypto.h>
#include <linux/types.h>
#include <crypto/serpent.h>

/* Key is padded to the maximum of 256 bits before round key generation.
 * Any key length <= 256 bits (32 bytes) is allowed by the algorithm.
 */

#define PHI 0x9e3779b9UL

#define keyiter(a, b, c, d, i, j) \
	({ b ^= d; b ^= c; b ^= a; b ^= PHI ^ i; b = rol32(b, 11); k[j] = b; })

#define loadkeys(x0, x1, x2, x3, i) \
	({ x0 = k[i]; x1 = k[i+1]; x2 = k[i+2]; x3 = k[i+3]; })

#define storekeys(x0, x1, x2, x3, i) \
	({ k[i] = x0; k[i+1] = x1; k[i+2] = x2; k[i+3] = x3; })

#define store_and_load_keys(x0, x1, x2, x3, s, l) \
	({ storekeys(x0, x1, x2, x3, s); loadkeys(x0, x1, x2, x3, l); })

#define K(x0, x1, x2, x3, i) ({				\
	x3 ^= k[4*(i)+3];        x2 ^= k[4*(i)+2];	\
	x1 ^= k[4*(i)+1];        x0 ^= k[4*(i)+0];	\
	})

#define LK(x0, x1, x2, x3, x4, i) ({					   \
							x0 = rol32(x0, 13);\
	x2 = rol32(x2, 3);	x1 ^= x0;		x4  = x0 << 3;	   \
	x3 ^= x2;		x1 ^= x2;				   \
	x1 = rol32(x1, 1);	x3 ^= x4;				   \
	x3 = rol32(x3, 7);	x4  = x1;				   \
	x0 ^= x1;		x4 <<= 7;		x2 ^= x3;	   \
	x0 ^= x3;		x2 ^= x4;		x3 ^= k[4*i+3];	   \
	x1 ^= k[4*i+1];		x0 = rol32(x0, 5);	x2 = rol32(x2, 22);\
	x0 ^= k[4*i+0];		x2 ^= k[4*i+2];				   \
	})

#define KL(x0, x1, x2, x3, x4, i) ({					   \
	x0 ^= k[4*i+0];		x1 ^= k[4*i+1];		x2 ^= k[4*i+2];	   \
	x3 ^= k[4*i+3];		x0 = ror32(x0, 5);	x2 = ror32(x2, 22);\
	x4 =  x1;		x2 ^= x3;		x0 ^= x3;	   \
	x4 <<= 7;		x0 ^= x1;		x1 = ror32(x1, 1); \
	x2 ^= x4;		x3 = ror32(x3, 7);	x4 = x0 << 3;	   \
	x1 ^= x0;		x3 ^= x4;		x0 = ror32(x0, 13);\
	x1 ^= x2;		x3 ^= x2;		x2 = ror32(x2, 3); \
	})

#define S0(x0, x1, x2, x3, x4) ({			\
					x4  = x3;	\
	x3 |= x0;	x0 ^= x4;	x4 ^= x2;	\
	x4 = ~x4;	x3 ^= x1;	x1 &= x0;	\
	x1 ^= x4;	x2 ^= x0;	x0 ^= x3;	\
	x4 |= x0;	x0 ^= x2;	x2 &= x1;	\
	x3 ^= x2;	x1 = ~x1;	x2 ^= x4;	\
	x1 ^= x2;					\
	})

#define S1(x0, x1, x2, x3, x4) ({			\
					x4  = x1;	\
	x1 ^= x0;	x0 ^= x3;	x3 = ~x3;	\
	x4 &= x1;	x0 |= x1;	x3 ^= x2;	\
	x0 ^= x3;	x1 ^= x3;	x3 ^= x4;	\
	x1 |= x4;	x4 ^= x2;	x2 &= x0;	\
	x2 ^= x1;	x1 |= x0;	x0 = ~x0;	\
	x0 ^= x2;	x4 ^= x1;			\
	})

#define S2(x0, x1, x2, x3, x4) ({			\
					x3 = ~x3;	\
	x1 ^= x0;	x4  = x0;	x0 &= x2;	\
	x0 ^= x3;	x3 |= x4;	x2 ^= x1;	\
	x3 ^= x1;	x1 &= x0;	x0 ^= x2;	\
	x2 &= x3;	x3 |= x1;	x0 = ~x0;	\
	x3 ^= x0;	x4 ^= x0;	x0 ^= x2;	\
	x1 |= x2;					\
	})

#define S3(x0, x1, x2, x3, x4) ({			\
					x4  = x1;	\
	x1 ^= x3;	x3 |= x0;	x4 &= x0;	\
	x0 ^= x2;	x2 ^= x1;	x1 &= x3;	\
	x2 ^= x3;	x0 |= x4;	x4 ^= x3;	\
	x1 ^= x0;	x0 &= x3;	x3 &= x4;	\
	x3 ^= x2;	x4 |= x1;	x2 &= x1;	\
	x4 ^= x3;	x0 ^= x3;	x3 ^= x2;	\
	})

#define S4(x0, x1, x2, x3, x4) ({			\
					x4  = x3;	\
	x3 &= x0;	x0 ^= x4;			\
	x3 ^= x2;	x2 |= x4;	x0 ^= x1;	\
	x4 ^= x3;	x2 |= x0;			\
	x2 ^= x1;	x1 &= x0;			\
	x1 ^= x4;	x4 &= x2;	x2 ^= x3;	\
	x4 ^= x0;	x3 |= x1;	x1 = ~x1;	\
	x3 ^= x0;					\
	})

#define S5(x0, x1, x2, x3, x4) ({			\
	x4  = x1;	x1 |= x0;			\
	x2 ^= x1;	x3 = ~x3;	x4 ^= x0;	\
	x0 ^= x2;	x1 &= x4;	x4 |= x3;	\
	x4 ^= x0;	x0 &= x3;	x1 ^= x3;	\
	x3 ^= x2;	x0 ^= x1;	x2 &= x4;	\
	x1 ^= x2;	x2 &= x0;			\
	x3 ^= x2;					\
	})

#define S6(x0, x1, x2, x3, x4) ({			\
					x4  = x1;	\
	x3 ^= x0;	x1 ^= x2;	x2 ^= x0;	\
	x0 &= x3;	x1 |= x3;	x4 = ~x4;	\
	x0 ^= x1;	x1 ^= x2;			\
	x3 ^= x4;	x4 ^= x0;	x2 &= x0;	\
	x4 ^= x1;	x2 ^= x3;	x3 &= x1;	\
	x3 ^= x0;	x1 ^= x2;			\
	})

#define S7(x0, x1, x2, x3, x4) ({			\
					x1 = ~x1;	\
	x4  = x1;	x0 = ~x0;	x1 &= x2;	\
	x1 ^= x3;	x3 |= x4;	x4 ^= x2;	\
	x2 ^= x3;	x3 ^= x0;	x0 |= x1;	\
	x2 &= x0;	x0 ^= x4;	x4 ^= x3;	\
	x3 &= x0;	x4 ^= x1;			\
	x2 ^= x4;	x3 ^= x1;	x4 |= x0;	\
	x4 ^= x1;					\
	})

#define SI0(x0, x1, x2, x3, x4) ({			\
			x4  = x3;	x1 ^= x0;	\
	x3 |= x1;	x4 ^= x1;	x0 = ~x0;	\
	x2 ^= x3;	x3 ^= x0;	x0 &= x1;	\
	x0 ^= x2;	x2 &= x3;	x3 ^= x4;	\
	x2 ^= x3;	x1 ^= x3;	x3 &= x0;	\
	x1 ^= x0;	x0 ^= x2;	x4 ^= x3;	\
	})

#define SI1(x0, x1, x2, x3, x4) ({			\
	x1 ^= x3;	x4  = x0;			\
	x0 ^= x2;	x2 = ~x2;	x4 |= x1;	\
	x4 ^= x3;	x3 &= x1;	x1 ^= x2;	\
	x2 &= x4;	x4 ^= x1;	x1 |= x3;	\
	x3 ^= x0;	x2 ^= x0;	x0 |= x4;	\
	x2 ^= x4;	x1 ^= x0;			\
	x4 ^= x1;					\
	})

#define SI2(x0, x1, x2, x3, x4) ({			\
	x2 ^= x1;	x4  = x3;	x3 = ~x3;	\
	x3 |= x2;	x2 ^= x4;	x4 ^= x0;	\
	x3 ^= x1;	x1 |= x2;	x2 ^= x0;	\
	x1 ^= x4;	x4 |= x3;	x2 ^= x3;	\
	x4 ^= x2;	x2 &= x1;			\
	x2 ^= x3;	x3 ^= x4;	x4 ^= x0;	\
	})

#define SI3(x0, x1, x2, x3, x4) ({			\
					x2 ^= x1;	\
	x4  = x1;	x1 &= x2;			\
	x1 ^= x0;	x0 |= x4;	x4 ^= x3;	\
	x0 ^= x3;	x3 |= x1;	x1 ^= x2;	\
	x1 ^= x3;	x0 ^= x2;	x2 ^= x3;	\
	x3 &= x1;	x1 ^= x0;	x0 &= x2;	\
	x4 ^= x3;	x3 ^= x0;	x0 ^= x1;	\
	})

#define SI4(x0, x1, x2, x3, x4) ({			\
	x2 ^= x3;	x4  = x0;	x0 &= x1;	\
	x0 ^= x2;	x2 |= x3;	x4 = ~x4;	\
	x1 ^= x0;	x0 ^= x2;	x2 &= x4;	\
	x2 ^= x0;	x0 |= x4;			\
	x0 ^= x3;	x3 &= x2;			\
	x4 ^= x3;	x3 ^= x1;	x1 &= x0;	\
	x4 ^= x1;	x0 ^= x3;			\
	})

#define SI5(x0, x1, x2, x3, x4) ({			\
			x4  = x1;	x1 |= x2;	\
	x2 ^= x4;	x1 ^= x3;	x3 &= x4;	\
	x2 ^= x3;	x3 |= x0;	x0 = ~x0;	\
	x3 ^= x2;	x2 |= x0;	x4 ^= x1;	\
	x2 ^= x4;	x4 &= x0;	x0 ^= x1;	\
	x1 ^= x3;	x0 &= x2;	x2 ^= x3;	\
	x0 ^= x2;	x2 ^= x4;	x4 ^= x3;	\
	})

#define SI6(x0, x1, x2, x3, x4) ({			\
			x0 ^= x2;			\
	x4  = x0;	x0 &= x3;	x2 ^= x3;	\
	x0 ^= x2;	x3 ^= x1;	x2 |= x4;	\
	x2 ^= x3;	x3 &= x0;	x0 = ~x0;	\
	x3 ^= x1;	x1 &= x2;	x4 ^= x0;	\
	x3 ^= x4;	x4 ^= x2;	x0 ^= x1;	\
	x2 ^= x0;					\
	})

#define SI7(x0, x1, x2, x3, x4) ({			\
	x4  = x3;	x3 &= x0;	x0 ^= x2;	\
	x2 |= x4;	x4 ^= x1;	x0 = ~x0;	\
	x1 |= x3;	x4 ^= x0;	x0 &= x2;	\
	x0 ^= x1;	x1 &= x2;	x3 ^= x2;	\
	x4 ^= x3;	x2 &= x3;	x3 |= x0;	\
	x1 ^= x4;	x3 ^= x4;	x4 &= x0;	\
	x4 ^= x2;					\
	})

int __serpent_setkey(struct serpent_ctx *ctx, const u8 *key,
		     unsigned int keylen)
{
	u32 *k = ctx->expkey;
	u8  *k8 = (u8 *)k;
	u32 r0, r1, r2, r3, r4;
	int i;

	/* Copy key, add padding */

	for (i = 0; i < keylen; ++i)
		k8[i] = key[i];
	if (i < SERPENT_MAX_KEY_SIZE)
		k8[i++] = 1;
	while (i < SERPENT_MAX_KEY_SIZE)
		k8[i++] = 0;

	/* Expand key using polynomial */

	r0 = le32_to_cpu(k[3]);
	r1 = le32_to_cpu(k[4]);
	r2 = le32_to_cpu(k[5]);
	r3 = le32_to_cpu(k[6]);
	r4 = le32_to_cpu(k[7]);

	keyiter(le32_to_cpu(k[0]), r0, r4, r2, 0, 0);
	keyiter(le32_to_cpu(k[1]), r1, r0, r3, 1, 1);
	keyiter(le32_to_cpu(k[2]), r2, r1, r4, 2, 2);
	keyiter(le32_to_cpu(k[3]), r3, r2, r0, 3, 3);
	keyiter(le32_to_cpu(k[4]), r4, r3, r1, 4, 4);
	keyiter(le32_to_cpu(k[5]), r0, r4, r2, 5, 5);
	keyiter(le32_to_cpu(k[6]), r1, r0, r3, 6, 6);
	keyiter(le32_to_cpu(k[7]), r2, r1, r4, 7, 7);

	keyiter(k[0], r3, r2, r0, 8, 8);
	keyiter(k[1], r4, r3, r1, 9, 9);
	keyiter(k[2], r0, r4, r2, 10, 10);
	keyiter(k[3], r1, r0, r3, 11, 11);
	keyiter(k[4], r2, r1, r4, 12, 12);
	keyiter(k[5], r3, r2, r0, 13, 13);
	keyiter(k[6], r4, r3, r1, 14, 14);
	keyiter(k[7], r0, r4, r2, 15, 15);
	keyiter(k[8], r1, r0, r3, 16, 16);
	keyiter(k[9], r2, r1, r4, 17, 17);
	keyiter(k[10], r3, r2, r0, 18, 18);
	keyiter(k[11], r4, r3, r1, 19, 19);
	keyiter(k[12], r0, r4, r2, 20, 20);
	keyiter(k[13], r1, r0, r3, 21, 21);
	keyiter(k[14], r2, r1, r4, 22, 22);
	keyiter(k[15], r3, r2, r0, 23, 23);
	keyiter(k[16], r4, r3, r1, 24, 24);
	keyiter(k[17], r0, r4, r2, 25, 25);
	keyiter(k[18], r1, r0, r3, 26, 26);
	keyiter(k[19], r2, r1, r4, 27, 27);
	keyiter(k[20], r3, r2, r0, 28, 28);
	keyiter(k[21], r4, r3, r1, 29, 29);
	keyiter(k[22], r0, r4, r2, 30, 30);
	keyiter(k[23], r1, r0, r3, 31, 31);

	k += 50;

	keyiter(k[-26], r2, r1, r4, 32, -18);
	keyiter(k[-25], r3, r2, r0, 33, -17);
	keyiter(k[-24], r4, r3, r1, 34, -16);
	keyiter(k[-23], r0, r4, r2, 35, -15);
	keyiter(k[-22], r1, r0, r3, 36, -14);
	keyiter(k[-21], r2, r1, r4, 37, -13);
	keyiter(k[-20], r3, r2, r0, 38, -12);
	keyiter(k[-19], r4, r3, r1, 39, -11);
	keyiter(k[-18], r0, r4, r2, 40, -10);
	keyiter(k[-17], r1, r0, r3, 41, -9);
	keyiter(k[-16], r2, r1, r4, 42, -8);
	keyiter(k[-15], r3, r2, r0, 43, -7);
	keyiter(k[-14], r4, r3, r1, 44, -6);
	keyiter(k[-13], r0, r4, r2, 45, -5);
	keyiter(k[-12], r1, r0, r3, 46, -4);
	keyiter(k[-11], r2, r1, r4, 47, -3);
	keyiter(k[-10], r3, r2, r0, 48, -2);
	keyiter(k[-9], r4, r3, r1, 49, -1);
	keyiter(k[-8], r0, r4, r2, 50, 0);
	keyiter(k[-7], r1, r0, r3, 51, 1);
	keyiter(k[-6], r2, r1, r4, 52, 2);
	keyiter(k[-5], r3, r2, r0, 53, 3);
	keyiter(k[-4], r4, r3, r1, 54, 4);
	keyiter(k[-3], r0, r4, r2, 55, 5);
	keyiter(k[-2], r1, r0, r3, 56, 6);
	keyiter(k[-1], r2, r1, r4, 57, 7);
	keyiter(k[0], r3, r2, r0, 58, 8);
	keyiter(k[1], r4, r3, r1, 59, 9);
	keyiter(k[2], r0, r4, r2, 60, 10);
	keyiter(k[3], r1, r0, r3, 61, 11);
	keyiter(k[4], r2, r1, r4, 62, 12);
	keyiter(k[5], r3, r2, r0, 63, 13);
	keyiter(k[6], r4, r3, r1, 64, 14);
	keyiter(k[7], r0, r4, r2, 65, 15);
	keyiter(k[8], r1, r0, r3, 66, 16);
	keyiter(k[9], r2, r1, r4, 67, 17);
	keyiter(k[10], r3, r2, r0, 68, 18);
	keyiter(k[11], r4, r3, r1, 69, 19);
	keyiter(k[12], r0, r4, r2, 70, 20);
	keyiter(k[13], r1, r0, r3, 71, 21);
	keyiter(k[14], r2, r1, r4, 72, 22);
	keyiter(k[15], r3, r2, r0, 73, 23);
	keyiter(k[16], r4, r3, r1, 74, 24);
	keyiter(k[17], r0, r4, r2, 75, 25);
	keyiter(k[18], r1, r0, r3, 76, 26);
	keyiter(k[19], r2, r1, r4, 77, 27);
	keyiter(k[20], r3, r2, r0, 78, 28);
	keyiter(k[21], r4, r3, r1, 79, 29);
	keyiter(k[22], r0, r4, r2, 80, 30);
	keyiter(k[23], r1, r0, r3, 81, 31);

	k += 50;

	keyiter(k[-26], r2, r1, r4, 82, -18);
	keyiter(k[-25], r3, r2, r0, 83, -17);
	keyiter(k[-24], r4, r3, r1, 84, -16);
	keyiter(k[-23], r0, r4, r2, 85, -15);
	keyiter(k[-22], r1, r0, r3, 86, -14);
	keyiter(k[-21], r2, r1, r4, 87, -13);
	keyiter(k[-20], r3, r2, r0, 88, -12);
	keyiter(k[-19], r4, r3, r1, 89, -11);
	keyiter(k[-18], r0, r4, r2, 90, -10);
	keyiter(k[-17], r1, r0, r3, 91, -9);
	keyiter(k[-16], r2, r1, r4, 92, -8);
	keyiter(k[-15], r3, r2, r0, 93, -7);
	keyiter(k[-14], r4, r3, r1, 94, -6);
	keyiter(k[-13], r0, r4, r2, 95, -5);
	keyiter(k[-12], r1, r0, r3, 96, -4);
	keyiter(k[-11], r2, r1, r4, 97, -3);
	keyiter(k[-10], r3, r2, r0, 98, -2);
	keyiter(k[-9], r4, r3, r1, 99, -1);
	keyiter(k[-8], r0, r4, r2, 100, 0);
	keyiter(k[-7], r1, r0, r3, 101, 1);
	keyiter(k[-6], r2, r1, r4, 102, 2);
	keyiter(k[-5], r3, r2, r0, 103, 3);
	keyiter(k[-4], r4, r3, r1, 104, 4);
	keyiter(k[-3], r0, r4, r2, 105, 5);
	keyiter(k[-2], r1, r0, r3, 106, 6);
	keyiter(k[-1], r2, r1, r4, 107, 7);
	keyiter(k[0], r3, r2, r0, 108, 8);
	keyiter(k[1], r4, r3, r1, 109, 9);
	keyiter(k[2], r0, r4, r2, 110, 10);
	keyiter(k[3], r1, r0, r3, 111, 11);
	keyiter(k[4], r2, r1, r4, 112, 12);
	keyiter(k[5], r3, r2, r0, 113, 13);
	keyiter(k[6], r4, r3, r1, 114, 14);
	keyiter(k[7], r0, r4, r2, 115, 15);
	keyiter(k[8], r1, r0, r3, 116, 16);
	keyiter(k[9], r2, r1, r4, 117, 17);
	keyiter(k[10], r3, r2, r0, 118, 18);
	keyiter(k[11], r4, r3, r1, 119, 19);
	keyiter(k[12], r0, r4, r2, 120, 20);
	keyiter(k[13], r1, r0, r3, 121, 21);
	keyiter(k[14], r2, r1, r4, 122, 22);
	keyiter(k[15], r3, r2, r0, 123, 23);
	keyiter(k[16], r4, r3, r1, 124, 24);
	keyiter(k[17], r0, r4, r2, 125, 25);
	keyiter(k[18], r1, r0, r3, 126, 26);
	keyiter(k[19], r2, r1, r4, 127, 27);
	keyiter(k[20], r3, r2, r0, 128, 28);
	keyiter(k[21], r4, r3, r1, 129, 29);
	keyiter(k[22], r0, r4, r2, 130, 30);
	keyiter(k[23], r1, r0, r3, 131, 31);

	/* Apply S-boxes */

	S3(r3, r4, r0, r1, r2); store_and_load_keys(r1, r2, r4, r3, 28, 24);
	S4(r1, r2, r4, r3, r0); store_and_load_keys(r2, r4, r3, r0, 24, 20);
	S5(r2, r4, r3, r0, r1); store_and_load_keys(r1, r2, r4, r0, 20, 16);
	S6(r1, r2, r4, r0, r3); store_and_load_keys(r4, r3, r2, r0, 16, 12);
	S7(r4, r3, r2, r0, r1); store_and_load_keys(r1, r2, r0, r4, 12, 8);
	S0(r1, r2, r0, r4, r3); store_and_load_keys(r0, r2, r4, r1, 8, 4);
	S1(r0, r2, r4, r1, r3); store_and_load_keys(r3, r4, r1, r0, 4, 0);
	S2(r3, r4, r1, r0, r2); store_and_load_keys(r2, r4, r3, r0, 0, -4);
	S3(r2, r4, r3, r0, r1); store_and_load_keys(r0, r1, r4, r2, -4, -8);
	S4(r0, r1, r4, r2, r3); store_and_load_keys(r1, r4, r2, r3, -8, -12);
	S5(r1, r4, r2, r3, r0); store_and_load_keys(r0, r1, r4, r3, -12, -16);
	S6(r0, r1, r4, r3, r2); store_and_load_keys(r4, r2, r1, r3, -16, -20);
	S7(r4, r2, r1, r3, r0); store_and_load_keys(r0, r1, r3, r4, -20, -24);
	S0(r0, r1, r3, r4, r2); store_and_load_keys(r3, r1, r4, r0, -24, -28);
	k -= 50;
	S1(r3, r1, r4, r0, r2); store_and_load_keys(r2, r4, r0, r3, 22, 18);
	S2(r2, r4, r0, r3, r1); store_and_load_keys(r1, r4, r2, r3, 18, 14);
	S3(r1, r4, r2, r3, r0); store_and_load_keys(r3, r0, r4, r1, 14, 10);
	S4(r3, r0, r4, r1, r2); store_and_load_keys(r0, r4, r1, r2, 10, 6);
	S5(r0, r4, r1, r2, r3); store_and_load_keys(r3, r0, r4, r2, 6, 2);
	S6(r3, r0, r4, r2, r1); store_and_load_keys(r4, r1, r0, r2, 2, -2);
	S7(r4, r1, r0, r2, r3); store_and_load_keys(r3, r0, r2, r4, -2, -6);
	S0(r3, r0, r2, r4, r1); store_and_load_keys(r2, r0, r4, r3, -6, -10);
	S1(r2, r0, r4, r3, r1); store_and_load_keys(r1, r4, r3, r2, -10, -14);
	S2(r1, r4, r3, r2, r0); store_and_load_keys(r0, r4, r1, r2, -14, -18);
	S3(r0, r4, r1, r2, r3); store_and_load_keys(r2, r3, r4, r0, -18, -22);
	k -= 50;
	S4(r2, r3, r4, r0, r1); store_and_load_keys(r3, r4, r0, r1, 28, 24);
	S5(r3, r4, r0, r1, r2); store_and_load_keys(r2, r3, r4, r1, 24, 20);
	S6(r2, r3, r4, r1, r0); store_and_load_keys(r4, r0, r3, r1, 20, 16);
	S7(r4, r0, r3, r1, r2); store_and_load_keys(r2, r3, r1, r4, 16, 12);
	S0(r2, r3, r1, r4, r0); store_and_load_keys(r1, r3, r4, r2, 12, 8);
	S1(r1, r3, r4, r2, r0); store_and_load_keys(r0, r4, r2, r1, 8, 4);
	S2(r0, r4, r2, r1, r3); store_and_load_keys(r3, r4, r0, r1, 4, 0);
	S3(r3, r4, r0, r1, r2); storekeys(r1, r2, r4, r3, 0);

	return 0;
}
EXPORT_SYMBOL_GPL(__serpent_setkey);

int serpent_setkey(struct crypto_tfm *tfm, const u8 *key, unsigned int keylen)
{
	return __serpent_setkey(crypto_tfm_ctx(tfm), key, keylen);
}
EXPORT_SYMBOL_GPL(serpent_setkey);

void __serpent_encrypt(struct serpent_ctx *ctx, u8 *dst, const u8 *src)
{
	const u32 *k = ctx->expkey;
	const __le32 *s = (const __le32 *)src;
	__le32	*d = (__le32 *)dst;
	u32	r0, r1, r2, r3, r4;

/*
 * Note: The conversions between u8* and u32* might cause trouble
 * on architectures with stricter alignment rules than x86
 */

	r0 = le32_to_cpu(s[0]);
	r1 = le32_to_cpu(s[1]);
	r2 = le32_to_cpu(s[2]);
	r3 = le32_to_cpu(s[3]);

					K(r0, r1, r2, r3, 0);
	S0(r0, r1, r2, r3, r4);		LK(r2, r1, r3, r0, r4, 1);
	S1(r2, r1, r3, r0, r4);		LK(r4, r3, r0, r2, r1, 2);
	S2(r4, r3, r0, r2, r1);		LK(r1, r3, r4, r2, r0, 3);
	S3(r1, r3, r4, r2, r0);		LK(r2, r0, r3, r1, r4, 4);
	S4(r2, r0, r3, r1, r4);		LK(r0, r3, r1, r4, r2, 5);
	S5(r0, r3, r1, r4, r2);		LK(r2, r0, r3, r4, r1, 6);
	S6(r2, r0, r3, r4, r1);		LK(r3, r1, r0, r4, r2, 7);
	S7(r3, r1, r0, r4, r2);		LK(r2, r0, r4, r3, r1, 8);
	S0(r2, r0, r4, r3, r1);		LK(r4, r0, r3, r2, r1, 9);
	S1(r4, r0, r3, r2, r1);		LK(r1, r3, r2, r4, r0, 10);
	S2(r1, r3, r2, r4, r0);		LK(r0, r3, r1, r4, r2, 11);
	S3(r0, r3, r1, r4, r2);		LK(r4, r2, r3, r0, r1, 12);
	S4(r4, r2, r3, r0, r1);		LK(r2, r3, r0, r1, r4, 13);
	S5(r2, r3, r0, r1, r4);		LK(r4, r2, r3, r1, r0, 14);
	S6(r4, r2, r3, r1, r0);		LK(r3, r0, r2, r1, r4, 15);
	S7(r3, r0, r2, r1, r4);		LK(r4, r2, r1, r3, r0, 16);
	S0(r4, r2, r1, r3, r0);		LK(r1, r2, r3, r4, r0, 17);
	S1(r1, r2, r3, r4, r0);		LK(r0, r3, r4, r1, r2, 18);
	S2(r0, r3, r4, r1, r2);		LK(r2, r3, r0, r1, r4, 19);
	S3(r2, r3, r0, r1, r4);		LK(r1, r4, r3, r2, r0, 20);
	S4(r1, r4, r3, r2, r0);		LK(r4, r3, r2, r0, r1, 21);
	S5(r4, r3, r2, r0, r1);		LK(r1, r4, r3, r0, r2, 22);
	S6(r1, r4, r3, r0, r2);		LK(r3, r2, r4, r0, r1, 23);
	S7(r3, r2, r4, r0, r1);		LK(r1, r4, r0, r3, r2, 24);
	S0(r1, r4, r0, r3, r2);		LK(r0, r4, r3, r1, r2, 25);
	S1(r0, r4, r3, r1, r2);		LK(r2, r3, r1, r0, r4, 26);
	S2(r2, r3, r1, r0, r4);		LK(r4, r3, r2, r0, r1, 27);
	S3(r4, r3, r2, r0, r1);		LK(r0, r1, r3, r4, r2, 28);
	S4(r0, r1, r3, r4, r2);		LK(r1, r3, r4, r2, r0, 29);
	S5(r1, r3, r4, r2, r0);		LK(r0, r1, r3, r2, r4, 30);
	S6(r0, r1, r3, r2, r4);		LK(r3, r4, r1, r2, r0, 31);
	S7(r3, r4, r1, r2, r0);		K(r0, r1, r2, r3, 32);

	d[0] = cpu_to_le32(r0);
	d[1] = cpu_to_le32(r1);
	d[2] = cpu_to_le32(r2);
	d[3] = cpu_to_le32(r3);
}
EXPORT_SYMBOL_GPL(__serpent_encrypt);

static void serpent_encrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{
	struct serpent_ctx *ctx = crypto_tfm_ctx(tfm);

	__serpent_encrypt(ctx, dst, src);
}

void __serpent_decrypt(struct serpent_ctx *ctx, u8 *dst, const u8 *src)
{
	const u32 *k = ctx->expkey;
	const __le32 *s = (const __le32 *)src;
	__le32	*d = (__le32 *)dst;
	u32	r0, r1, r2, r3, r4;

	r0 = le32_to_cpu(s[0]);
	r1 = le32_to_cpu(s[1]);
	r2 = le32_to_cpu(s[2]);
	r3 = le32_to_cpu(s[3]);

					K(r0, r1, r2, r3, 32);
	SI7(r0, r1, r2, r3, r4);	KL(r1, r3, r0, r4, r2, 31);
	SI6(r1, r3, r0, r4, r2);	KL(r0, r2, r4, r1, r3, 30);
	SI5(r0, r2, r4, r1, r3);	KL(r2, r3, r0, r4, r1, 29);
	SI4(r2, r3, r0, r4, r1);	KL(r2, r0, r1, r4, r3, 28);
	SI3(r2, r0, r1, r4, r3);	KL(r1, r2, r3, r4, r0, 27);
	SI2(r1, r2, r3, r4, r0);	KL(r2, r0, r4, r3, r1, 26);
	SI1(r2, r0, r4, r3, r1);	KL(r1, r0, r4, r3, r2, 25);
	SI0(r1, r0, r4, r3, r2);	KL(r4, r2, r0, r1, r3, 24);
	SI7(r4, r2, r0, r1, r3);	KL(r2, r1, r4, r3, r0, 23);
	SI6(r2, r1, r4, r3, r0);	KL(r4, r0, r3, r2, r1, 22);
	SI5(r4, r0, r3, r2, r1);	KL(r0, r1, r4, r3, r2, 21);
	SI4(r0, r1, r4, r3, r2);	KL(r0, r4, r2, r3, r1, 20);
	SI3(r0, r4, r2, r3, r1);	KL(r2, r0, r1, r3, r4, 19);
	SI2(r2, r0, r1, r3, r4);	KL(r0, r4, r3, r1, r2, 18);
	SI1(r0, r4, r3, r1, r2);	KL(r2, r4, r3, r1, r0, 17);
	SI0(r2, r4, r3, r1, r0);	KL(r3, r0, r4, r2, r1, 16);
	SI7(r3, r0, r4, r2, r1);	KL(r0, r2, r3, r1, r4, 15);
	SI6(r0, r2, r3, r1, r4);	KL(r3, r4, r1, r0, r2, 14);
	SI5(r3, r4, r1, r0, r2);	KL(r4, r2, r3, r1, r0, 13);
	SI4(r4, r2, r3, r1, r0);	KL(r4, r3, r0, r1, r2, 12);
	SI3(r4, r3, r0, r1, r2);	KL(r0, r4, r2, r1, r3, 11);
	SI2(r0, r4, r2, r1, r3);	KL(r4, r3, r1, r2, r0, 10);
	SI1(r4, r3, r1, r2, r0);	KL(r0, r3, r1, r2, r4, 9);
	SI0(r0, r3, r1, r2, r4);	KL(r1, r4, r3, r0, r2, 8);
	SI7(r1, r4, r3, r0, r2);	KL(r4, r0, r1, r2, r3, 7);
	SI6(r4, r0, r1, r2, r3);	KL(r1, r3, r2, r4, r0, 6);
	SI5(r1, r3, r2, r4, r0);	KL(r3, r0, r1, r2, r4, 5);
	SI4(r3, r0, r1, r2, r4);	KL(r3, r1, r4, r2, r0, 4);
	SI3(r3, r1, r4, r2, r0);	KL(r4, r3, r0, r2, r1, 3);
	SI2(r4, r3, r0, r2, r1);	KL(r3, r1, r2, r0, r4, 2);
	SI1(r3, r1, r2, r0, r4);	KL(r4, r1, r2, r0, r3, 1);
	SI0(r4, r1, r2, r0, r3);	K(r2, r3, r1, r4, 0);

	d[0] = cpu_to_le32(r2);
	d[1] = cpu_to_le32(r3);
	d[2] = cpu_to_le32(r1);
	d[3] = cpu_to_le32(r4);
}
EXPORT_SYMBOL_GPL(__serpent_decrypt);

static void serpent_decrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{
	struct serpent_ctx *ctx = crypto_tfm_ctx(tfm);

	__serpent_decrypt(ctx, dst, src);
}

static int tnepres_setkey(struct crypto_tfm *tfm, const u8 *key,
			  unsigned int keylen)
{
	u8 rev_key[SERPENT_MAX_KEY_SIZE];
	int i;

	for (i = 0; i < keylen; ++i)
		rev_key[keylen - i - 1] = key[i];

	return serpent_setkey(tfm, rev_key, keylen);
}

static void tnepres_encrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{
	const u32 * const s = (const u32 * const)src;
	u32 * const d = (u32 * const)dst;

	u32 rs[4], rd[4];

	rs[0] = swab32(s[3]);
	rs[1] = swab32(s[2]);
	rs[2] = swab32(s[1]);
	rs[3] = swab32(s[0]);

	serpent_encrypt(tfm, (u8 *)rd, (u8 *)rs);

	d[0] = swab32(rd[3]);
	d[1] = swab32(rd[2]);
	d[2] = swab32(rd[1]);
	d[3] = swab32(rd[0]);
}

static void tnepres_decrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{
	const u32 * const s = (const u32 * const)src;
	u32 * const d = (u32 * const)dst;

	u32 rs[4], rd[4];

	rs[0] = swab32(s[3]);
	rs[1] = swab32(s[2]);
	rs[2] = swab32(s[1]);
	rs[3] = swab32(s[0]);

	serpent_decrypt(tfm, (u8 *)rd, (u8 *)rs);

	d[0] = swab32(rd[3]);
	d[1] = swab32(rd[2]);
	d[2] = swab32(rd[1]);
	d[3] = swab32(rd[0]);
}

static struct crypto_alg srp_algs[2] = { {
	.cra_name		=	"serpent",
	.cra_driver_name	=	"serpent-generic",
	.cra_priority		=	100,
	.cra_flags		=	CRYPTO_ALG_TYPE_CIPHER,
	.cra_blocksize		=	SERPENT_BLOCK_SIZE,
	.cra_ctxsize		=	sizeof(struct serpent_ctx),
	.cra_alignmask		=	3,
	.cra_module		=	THIS_MODULE,
	.cra_u			=	{ .cipher = {
	.cia_min_keysize	=	SERPENT_MIN_KEY_SIZE,
	.cia_max_keysize	=	SERPENT_MAX_KEY_SIZE,
	.cia_setkey		=	serpent_setkey,
	.cia_encrypt		=	serpent_encrypt,
	.cia_decrypt		=	serpent_decrypt } }
}, {
	.cra_name		=	"tnepres",
	.cra_flags		=	CRYPTO_ALG_TYPE_CIPHER,
	.cra_blocksize		=	SERPENT_BLOCK_SIZE,
	.cra_ctxsize		=	sizeof(struct serpent_ctx),
	.cra_alignmask		=	3,
	.cra_module		=	THIS_MODULE,
	.cra_u			=	{ .cipher = {
	.cia_min_keysize	=	SERPENT_MIN_KEY_SIZE,
	.cia_max_keysize	=	SERPENT_MAX_KEY_SIZE,
	.cia_setkey		=	tnepres_setkey,
	.cia_encrypt		=	tnepres_encrypt,
	.cia_decrypt		=	tnepres_decrypt } }
} };

static int __init serpent_mod_init(void)
{
	return crypto_register_algs(srp_algs, ARRAY_SIZE(srp_algs));
}

static void __exit serpent_mod_fini(void)
{
	crypto_unregister_algs(srp_algs, ARRAY_SIZE(srp_algs));
}

module_init(serpent_mod_init);
module_exit(serpent_mod_fini);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Serpent and tnepres (kerneli compatible serpent reversed) Cipher Algorithm");
MODULE_AUTHOR("Dag Arne Osvik <osvik@ii.uib.no>");
MODULE_ALIAS_CRYPTO("tnepres");
MODULE_ALIAS_CRYPTO("serpent");
MODULE_ALIAS_CRYPTO("serpent-generic");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * Cryptographic API.
 *
 * SHA1 Secure Hash Algorithm.
 *
 * Derived from cryptoapi implementation, adapted for in-place
 * scatterlist interface.
 *
 * Copyright (c) Alan Smithee.
 * Copyright (c) Andrew McDonald <andrew@mcdonald.org.uk>
 * Copyright (c) Jean-Francois Dive <jef@linuxbe.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */
#include <crypto/internal/hash.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/cryptohash.h>
#include <linux/types.h>
#include <crypto/sha.h>
#include <crypto/sha1_base.h>
#include <asm/byteorder.h>

static void sha1_generic_block_fn(struct sha1_state *sst, u8 const *src,
				  int blocks)
{
	u32 temp[SHA_WORKSPACE_WORDS];

	while (blocks--) {
		sha_transform(sst->state, src, temp);
		src += SHA1_BLOCK_SIZE;
	}
	memzero_explicit(temp, sizeof(temp));
}

int crypto_sha1_update(struct shash_desc *desc, const u8 *data,
		       unsigned int len)
{
	return sha1_base_do_update(desc, data, len, sha1_generic_block_fn);
}
EXPORT_SYMBOL(crypto_sha1_update);

static int sha1_final(struct shash_desc *desc, u8 *out)
{
	sha1_base_do_finalize(desc, sha1_generic_block_fn);
	return sha1_base_finish(desc, out);
}

int crypto_sha1_finup(struct shash_desc *desc, const u8 *data,
		      unsigned int len, u8 *out)
{
	sha1_base_do_update(desc, data, len, sha1_generic_block_fn);
	return sha1_final(desc, out);
}
EXPORT_SYMBOL(crypto_sha1_finup);

static struct shash_alg alg = {
	.digestsize	=	SHA1_DIGEST_SIZE,
	.init		=	sha1_base_init,
	.update		=	crypto_sha1_update,
	.final		=	sha1_final,
	.finup		=	crypto_sha1_finup,
	.descsize	=	sizeof(struct sha1_state),
	.base		=	{
		.cra_name	=	"sha1",
		.cra_driver_name=	"sha1-generic",
		.cra_flags	=	CRYPTO_ALG_TYPE_SHASH,
		.cra_blocksize	=	SHA1_BLOCK_SIZE,
		.cra_module	=	THIS_MODULE,
	}
};

static int __init sha1_generic_mod_init(void)
{
	return crypto_register_shash(&alg);
}

static void __exit sha1_generic_mod_fini(void)
{
	crypto_unregister_shash(&alg);
}

module_init(sha1_generic_mod_init);
module_exit(sha1_generic_mod_fini);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SHA1 Secure Hash Algorithm");

MODULE_ALIAS_CRYPTO("sha1");
MODULE_ALIAS_CRYPTO("sha1-generic");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * Cryptographic API.
 *
 * SHA-256, as specified in
 * http://csrc.nist.gov/groups/STM/cavp/documents/shs/sha256-384-512.pdf
 *
 * SHA-256 code by Jean-Luc Cooke <jlcooke@certainkey.com>.
 *
 * Copyright (c) Jean-Luc Cooke <jlcooke@certainkey.com>
 * Copyright (c) Andrew McDonald <andrew@mcdonald.org.uk>
 * Copyright (c) 2002 James Morris <jmorris@intercode.com.au>
 * SHA224 Support Copyright 2007 Intel Corporation <jonathan.lynch@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of th