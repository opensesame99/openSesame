/* crypto/dh/dhtest.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 *
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 *
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be reopensesamed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 *
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

/*
 * Until the key-gen callbacks are modified to use newer prototypes, we allow
 * deprecated functions for openssl-internal code
 */
#ifdef OPENSSL_NO_DEPRECATED
# undef OPENSSL_NO_DEPRECATED
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../e_os.h"

#include <openssl/crypto.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#ifdef OPENSSL_NO_DH
int main(int argc, char *argv[])
{
    printf("No DH support\n");
    return (0);
}
#else
# include <openssl/dh.h>

# ifdef OPENSSL_SYS_WIN16
#  define MS_CALLBACK     _far _loadds
# else
#  define MS_CALLBACK
# endif

static int MS_CALLBACK cb(int p, int n, BN_GENCB *arg);

static const char rnd_seed[] =
    "string to make the random number generator think it has entropy";

static int run_rfc5114_tests(void);

int main(int argc, char *argv[])
{
    BN_GENCB _cb;
    DH *a;
    DH *b = NULL;
    char buf[12];
    unsigned char *abuf = NULL, *bbuf = NULL;
    int i, alen, blen, aout, bout, ret = 1;
    BIO *out;

    CRYPTO_malloc_debug_init();
    CRYPTO_dbg_set_options(V_CRYPTO_MDEBUG_ALL);
    CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);

# ifdef OPENSSL_SYS_WIN32
    CRYPTO_malloc_init();
# endif

    RAND_seed(rnd_seed, sizeof rnd_seed);

    out = BIO_new(BIO_s_file());
    if (out == NULL)
        EXIT(1);
    BIO_set_fp(out, stdout, BIO_NOCLOSE);

    BN_GENCB_set(&_cb, &cb, out);
    if (((a = DH_new()) == NULL) || !DH_generate_parameters_ex(a, 64,
                                                               DH_GENERATOR_5,
                                                               &_cb))
        goto err;

    if (!DH_check(a, &i))
        goto err;
    if (i & DH_CHECK_P_NOT_PRIME)
        BIO_puts(out, "p value is not prime\n");
    if (i & DH_CHECK_P_NOT_SAFE_PRIME)
        BIO_puts(out, "p value is not a safe prime\n");
    if (i & DH_UNABLE_TO_CHECK_GENERATOR)
        BIO_puts(out, "unable to check the generator value\n");
    if (i & DH_NOT_SUITABLE_GENERATOR)
        BIO_puts(out, "the g value is not a generator\n");

    BIO_puts(out, "\np    =");
    BN_print(out, a->p);
    BIO_puts(out, "\ng    =");
    BN_print(out, a->g);
    BIO_puts(out, "\n");

    b = DH_new();
    if (b == NULL)
        goto err;

    b->p = BN_dup(a->p);
    b->g = BN_dup(a->g);
    if ((b->p == NULL) || (b->g == NULL))
        goto err;

    /* Set a to run with normal modexp and b to use constant time */
    a->flags &= ~DH_FLAG_NO_EXP_CONSTTIME;
    b->flags |= DH_FLAG_NO_EXP_CONSTTIME;

    if (!DH_generate_key(a))
        goto err;
    BIO_puts(out, "pri 1=");
    BN_print(out, a->priv_key);
    BIO_puts(out, "\npub 1=");
    BN_print(out, a->pub_key);
    BIO_puts(out, "\n");

    if (!DH_generate_key(b))
        goto err;
    BIO_puts(out, "pri 2=");
    BN_print(out, b->priv_key);
    BIO_puts(out, "\npub 2=");
    BN_print(out, b->pub_key);
    BIO_puts(out, "\n");

    alen = DH_size(a);
    abuf = (unsigned char *)OPENSSL_malloc(alen);
    aout = DH_compute_key(abuf, b->pub_key, a);

    BIO_puts(out, "key1 =");
    for (i = 0; i < aout; i++) {
        sprintf(buf, "%02X", abuf[i]);
        BIO_puts(out, buf);
    }
    BIO_puts(out, "\n");

    blen = DH_size(b);
    bbuf = (unsigned char *)OPENSSL_malloc(blen);
    bout = DH_compute_key(bbuf, a->pub_key, b);

    BIO_puts(out, "key2 =");
    for (i = 0; i < bout; i++) {
        sprintf(buf, "%02X", bbuf[i]);
        BIO_puts(out, buf);
    }
    BIO_puts(out, "\n");
    if ((aout < 4) || (bout != aout) || (memcmp(abuf, bbuf, aout) != 0)) {
        fprintf(stderr, "Error in DH routines\n");
        ret = 1;
    } else
        ret = 0;
    if (!run_rfc5114_tests())
        ret = 1;
 err:
    ERR_print_errors_fp(stderr);

    if (abuf != NULL)
        OPENSSL_free(abuf);
    if (bbuf != NULL)
        OPENSSL_free(bbuf);
    if (b != NULL)
        DH_free(b);
    if (a != NULL)
        DH_free(a);
    BIO_free(out);
# ifdef OPENSSL_SYS_NETWARE
    if (ret)
        printf("ERROR: %d\n", ret);
# endif
    EXIT(ret);
    return (ret);
}

static int MS_CALLBACK cb(int p, int n, BN_GENCB *arg)
{
    char c = '*';

    if (p == 0)
        c = '.';
    if (p == 1)
        c = '+';
    if (p == 2)
        c = '*';
    if (p == 3)
        c = '\n';
    BIO_write(arg->arg, &c, 1);
    (void)BIO_flush(arg->arg);
# ifdef LINT
    p = n;
# endif
    return 1;
}

/* Test data from RFC 5114 */

static const unsigned char dhtest_1024_160_xA[] = {
    0xB9, 0xA3, 0xB3, 0xAE, 0x8F, 0xEF, 0xC1, 0xA2, 0x93, 0x04, 0x96, 0x50,
    0x70, 0x86, 0xF8, 0x45, 0x5D, 0x48, 0x94, 0x3E
};

static const unsigned char dhtest_1024_160_yA[] = {
    0x2A, 0x85, 0x3B, 0x3D, 0x92, 0x19, 0x75, 0x01, 0xB9, 0x01, 0x5B, 0x2D,
    0xEB, 0x3E, 0xD8, 0x4F, 0x5E, 0x02, 0x1D, 0xCC, 0x3E, 0x52, 0xF1, 0x09,
    0xD3, 0x27, 0x3D, 0x2B, 0x75, 0x21, 0x28, 0x1C, 0xBA, 0xBE, 0x0E, 0x76,
    0xFF, 0x57, 0x27, 0xFA, 0x8A, 0xCC, 0xE2, 0x69, 0x56, 0xBA, 0x9A, 0x1F,
    0xCA, 0x26, 0xF2, 0x02, 0x28, 0xD8, 0x69, 0x3F, 0xEB, 0x10, 0x84, 0x1D,
    0x84, 0xA7, 0x36, 0x00, 0x54, 0xEC, 0xE5, 0xA7, 0xF5, 0xB7, 0xA6, 0x1A,
    0xD3, 0xDF, 0xB3, 0xC6, 0x0D, 0x2E, 0x43, 0x10, 0x6D, 0x87, 0x27, 0xDA,
    0x37, 0xDF, 0x9C, 0xCE, 0x95, 0xB4, 0x78, 0x75, 0x5D, 0x06, 0xBC, 0xEA,
    0x8F, 0x9D, 0x45, 0x96, 0x5F, 0x75, 0xA5, 0xF3, 0xD1, 0xDF, 0x37, 0x01,
    0x16, 0x5F, 0xC9, 0xE5, 0x0C, 0x42, 0x79, 0xCE, 0xB0, 0x7F, 0x98, 0x95,
    0x40, 0xAE, 0x96, 0xD5, 0xD8, 0x8E, 0xD7, 0x76
};

static const unsigned char dhtest_1024_160_xB[] = {
    0x93, 0x92, 0xC9, 0xF9, 0xEB, 0x6A, 0x7A, 0x6A, 0x90, 0x22, 0xF7, 0xD8,
    0x3E, 0x72, 0x23, 0xC6, 0x83, 0x5B, 0xBD, 0xDA
};

static const unsigned char dhtest_1024_160_yB[] = {
    0x71, 0x7A, 0x6C, 0xB0, 0x53, 0x37, 0x1F, 0xF4, 0xA3, 0xB9, 0x32, 0x94,
    0x1C, 0x1E, 0x56, 0x63, 0xF8, 0x61, 0xA1, 0xD6, 0xAD, 0x34, 0xAE, 0x66,
    0x57, 0x6D, 0xFB, 0x98, 0xF6, 0xC6, 0xCB, 0xF9, 0xDD, 0xD5, 0xA5, 0x6C,
    0x78, 0x33, 0xF6, 0xBC, 0xFD, 0xFF, 0x09, 0x55, 0x82, 0xAD, 0x86, 0x8E,
    0x44, 0x0E, 0x8D, 0x09, 0xFD, 0x76, 0x9E, 0x3C, 0xEC, 0xCD, 0xC3, 0xD3,
    0xB1, 0xE4, 0xCF, 0xA0, 0x57, 0x77, 0x6C, 0xAA, 0xF9, 0x73, 0x9B, 0x6A,
    0x9F, 0xEE, 0x8E, 0x74, 0x11, 0xF8, 0xD6, 0xDA, 0xC0, 0x9D, 0x6A, 0x4E,
    0xDB, 0x46, 0xCC, 0x2B, 0x5D, 0x52, 0x03, 0x09, 0x0E, 0xAE, 0x61, 0x26,
    0x31, 0x1E, 0x53, 0xFD, 0x2C, 0x14, 0xB5, 0x74, 0xE6, 0xA3, 0x10, 0x9A,
    0x3D, 0xA1, 0xBE, 0x41, 0xBD, 0xCE, 0xAA, 0x18, 0x6F, 0x5C, 0xE0, 0x67,
    0x16, 0xA2, 0xB6, 0xA0, 0x7B, 0x3C, 0x33, 0xFE
};

static const unsigned char dhtest_1024_160_Z[] = {
    0x5C, 0x80, 0x4F, 0x45, 0x4D, 0x30, 0xD9, 0xC4, 0xDF, 0x85, 0x27, 0x1F,
    0x93, 0x52, 0x8C, 0x91, 0xDF, 0x6B, 0x48, 0xAB, 0x5F, 0x80, 0xB3, 0xB5,
    0x9C, 0xAA, 0xC1, 0xB2, 0x8F, 0x8A, 0xCB, 0xA9, 0xCD, 0x3E, 0x39, 0xF3,
    0xCB, 0x61, 0x45, 0x25, 0xD9, 0x52, 0x1D, 0x2E, 0x64, 0x4C, 0x53, 0xB8,
    0x07, 0xB8, 0x10, 0xF3, 0x40, 0x06, 0x2F, 0x25, 0x7D, 0x7D, 0x6F, 0xBF,
    0xE8, 0xD5, 0xE8, 0xF0, 0x72, 0xE9, 0xB6, 0xE9, 0xAF, 0xDA, 0x94, 0x13,
    0xEA, 0xFB, 0x2E, 0x8B, 0x06, 0x99, 0xB1, 0xFB, 0x5A, 0x0C, 0xAC, 0xED,
    0xDE, 0xAE, 0xAD, 0x7E, 0x9C, 0xFB, 0xB3, 0x6A, 0xE2, 0xB4, 0x20, 0x83,
    0x5B, 0xD8, 0x3A, 0x19, 0xFB, 0x0B, 0x5E, 0x96, 0xBF, 0x8F, 0xA4, 0xD0,
    0x9E, 0x34, 0x55, 0x25, 0x16, 0x7E, 0xCD, 0x91, 0x55, 0x41, 0x6F, 0x46,
    0xF4, 0x08, 0xED, 0x31, 0xB6, 0x3C, 0x6E, 0x6D
};

static const unsigned char dhtest_2048_224_xA[] = {
    0x22, 0xE6, 0x26, 0x01, 0xDB, 0xFF, 0xD0, 0x67, 0x08, 0xA6, 0x80, 0xF7,
    0x47, 0xF3, 0x61, 0xF7, 0x6D, 0x8F, 0x4F, 0x72, 0x1A, 0x05, 0x48, 0xE4,
    0x83, 0x29, 0x4B, 0x0C
};

static const unsigned char dhtest_2048_224_yA[] = {
    0x1B, 0x3A, 0x63, 0x45, 0x1B, 0xD8, 0x86, 0xE6, 0x99, 0xE6, 0x7B, 0x49,
    0x4E, 0x28, 0x8B, 0xD7, 0xF8, 0xE0, 0xD3, 0x70, 0xBA, 0xDD, 0xA7, 0xA0,
    0xEF, 0xD2, 0xFD, 0xE7, 0xD8, 0xF6, 0x61, 0x45, 0xCC, 0x9F, 0x28, 0x04,
    0x19, 0x97, 0x5E, 0xB8, 0x08, 0x87, 0x7C, 0x8A, 0x4C, 0x0C, 0x8E, 0x0B,
    0xD4, 0x8D, 0x4A, 0x54, 0x01, 0xEB, 0x1E, 0x87, 0x76, 0xBF, 0xEE, 0xE1,
    0x34, 0xC0, 0x38, 0x31, 0xAC, 0x27, 0x3C, 0xD9, 0xD6, 0x35, 0xAB, 0x0C,
    0xE0, 0x06, 0xA4, 0x2A, 0x88, 0x7E, 0x3F, 0x52, 0xFB, 0x87, 0x66, 0xB6,
    0x50, 0xF3, 0x80, 0x78, 0xBC, 0x8E, 0xE8, 0x58, 0x0C, 0xEF, 0xE2, 0x43,
    0x96, 0x8C, 0xFC, 0x4F, 0x8D, 0xC3, 0xDB, 0x08, 0x45, 0x54, 0x17, 0x1D,
    0x41, 0xBF, 0x2E, 0x86, 0x1B, 0x7B, 0xB4, 0xD6, 0x9D, 0xD0, 0xE0, 0x1E,
    0xA3, 0x87, 0xCB, 0xAA, 0x5C, 0xA6, 0x72, 0xAF, 0xCB, 0xE8, 0xBD, 0xB9,
    0xD6, 0x2D, 0x4C, 0xE1, 0x5F, 0x17, 0xDD, 0x36, 0xF9, 0x1E, 0xD1, 0xEE,
    0xDD, 0x65, 0xCA, 0x4A, 0x06, 0x45, 0x5C, 0xB9, 0x4C, 0xD4, 0x0A, 0x52,
    0xEC, 0x36, 0x0E, 0x84, 0xB3, 0xC9, 0x26, 0xE2, 0x2C, 0x43, 0x80, 0xA3,
    0xBF, 0x30, 0x9D, 0x56, 0x84, 0x97, 0x68, 0xB7, 0xF5, 0x2C, 0xFD, 0xF6,
    0x55, 0xFD, 0x05, 0x3A, 0x7E, 0xF7, 0x06, 0x97, 0x9E, 0x7E, 0x58, 0x06,
    0xB1, 0x7D, 0xFA, 0xE5, 0x3A, 0xD2, 0xA5, 0xBC, 0x56, 0x8E, 0xBB, 0x52,
    0x9A, 0x7A, 0x61, 0xD6, 0x8D, 0x25, 0x6F, 0x8F, 0xC9, 0x7C, 0x07, 0x4A,
    0x86, 0x1D, 0x82, 0x7E, 0x2E, 0xBC, 0x8C, 0x61, 0x34, 0x55, 0x31, 0x15,
    0xB7, 0x0E, 0x71, 0x03, 0x92, 0x0A, 0xA1, 0x6D, 0x85, 0xE5, 0x2B, 0xCB,
    0xAB, 0x8D, 0x78, 0x6A, 0x68, 0x17, 0x8F, 0xA8, 0xFF, 0x7C, 0x2F, 0x5C,
    0x71, 0x64, 0x8D, 0x6F
};

static const unsigned char dhtest_2048_224_xB[] = {
    0x4F, 0xF3, 0xBC, 0x96, 0xC7, 0xFC, 0x6A, 0x6D, 0x71, 0xD3, 0xB3, 0x63,
    0x80, 0x0A, 0x7C, 0xDF, 0xEF, 0x6F, 0xC4, 0x1B, 0x44, 0x17, 0xEA, 0x15,
    0x35, 0x3B, 0x75, 0x90
};

static const unsigned char dhtest_2048_224_yB[] = {
    0x4D, 0xCE, 0xE9, 0x92, 0xA9, 0x76, 0x2A, 0x13, 0xF2, 0xF8, 0x38, 0x44,
    0xAD, 0x3D, 0x77, 0xEE, 0x0E, 0x31, 0xC9, 0x71, 0x8B, 0x3D, 0xB6, 0xC2,
    0x03, 0x5D, 0x39, 0x61, 0x18, 0x2C, 0x3E, 0x0B, 0xA2, 0x47, 0xEC, 0x41,
    0x82, 0xD7, 0x60, 0xCD, 0x48, 0xD9, 0x95, 0x99, 0x97, 0x06, 0x22, 0xA1,
    0x88, 0x1B, 0xBA, 0x2D, 0xC8, 0x22, 0x93, 0x9C, 0x78, 0xC3, 0x91, 0x2C,
    0x66, 0x61, 0xFA, 0x54, 0x38, 0xB2, 0x07, 0x66, 0x22, 0x2B, 0x75, 0xE2,
    0x4C, 0x2E, 0x3A, 0xD0, 0xC7, 0x28, 0x72, 0x36, 0x12, 0x95, 0x25, 0xEE,
    0x15, 0xB5, 0xDD, 0x79, 0x98, 0xAA, 0x04, 0xC4, 0xA9, 0x69, 0x6C, 0xAC,
    0xD7, 0x17, 0x20, 0x83, 0xA9, 0x7A, 0x81, 0x66, 0x4E, 0xAD, 0x2C, 0x47,
    0x9E, 0x44, 0x4E, 0x4C, 0x06, 0x54, 0xCC, 0x19, 0xE2, 0x8D, 0x77, 0x03,
    0xCE, 0xE8, 0xDA, 0xCD, 0x61, 0x26, 0xF5, 0xD6, 0x65, 0xEC, 0x52, 0xC6,
    0x72, 0x55, 0xDB, 0x92, 0x01, 0x4B, 0x03, 0x7E, 0xB6, 0x21, 0xA2, 0xAC,
    0x8E, 0x36, 0x5D, 0xE0, 0x71, 0xFF, 0xC1, 0x40, 0x0A, 0xCF, 0x07, 0x7A,
    0x12, 0x91, 0x3D, 0xD8, 0xDE, 0x89, 0x47, 0x34, 0x37, 0xAB, 0x7B, 0xA3,
    0x46, 0x74, 0x3C, 0x1B, 0x21, 0x5D, 0xD9, 0xC1, 0x21, 0x64, 0xA7, 0xE4,
    0x05, 0x31, 0x18, 0xD1, 0x99, 0xBE, 0xC8, 0xEF, 0x6F, 0xC5, 0x61, 0x17,
    0x0C, 0x84, 0xC8, 0x7D, 0x10, 0xEE, 0x9A, 0x67, 0x4A, 0x1F, 0xA8, 0xFF,
    0xE1, 0x3B, 0xDF, 0xBA, 0x1D, 0x44, 0xDE, 0x48, 0x94, 0x6D, 0x68, 0xDC,
    0x0C, 0xDD, 0x77, 0x76, 0x35, 0xA7, 0xAB, 0x5B, 0xFB, 0x1E, 0x4B, 0xB7,
    0xB8, 0x56, 0xF9, 0x68, 0x27, 0x73, 0x4C, 0x18, 0x41, 0x38, 0xE9, 0x15,
    0xD9, 0xC3, 0x00, 0x2E, 0xBC, 0xE5, 0x31, 0x20, 0x54, 0x6A, 0x7E, 0x20,
    0x02, 0x14, 0x2B, 0x6C
};

static const unsigned char dhtest_2048_224_Z[] = {
    0x34, 0xD9, 0xBD, 0xDC, 0x1B, 0x42, 0x17, 0x6C, 0x31, 0x3F, 0xEA, 0x03,
    0x4C, 0x21, 0x03, 0x4D, 0x07, 0x4A, 0x63, 0x13, 0xBB, 0x4E, 0xCD, 0xB3,
    0x70, 0x3F, 0xFF, 0x42, 0x45, 0x67, 0xA4, 0x6B, 0xDF, 0x75, 0x53, 0x0E,
    0xDE, 0x0A, 0x9D, 0xA5, 0x22, 0x9D, 0xE7, 0xD7, 0x67, 0x32, 0x28, 0x6C,
    0xBC, 0x0F, 0x91, 0xDA, 0x4C, 0x3C, 0x85, 0x2F, 0xC0, 0x99, 0xC6, 0x79,
    0x53, 0x1D, 0x94, 0xC7, 0x8A, 0xB0, 0x3D, 0x9D, 0xEC, 0xB0, 0xA4, 0xE4,
    0xCA, 0x8B, 0x2B, 0xB4, 0x59, 0x1C, 0x40, 0x21, 0xCF, 0x8C, 0xE3, 0xA2,
    0x0A, 0x54, 0x1D, 0x33, 0x99, 0x40, 0x17, 0xD0, 0x20, 0x0A, 0xE2, 0xC9,
    0x51, 0x6E, 0x2F, 0xF5, 0x14, 0x57, 0x79, 0x26, 0x9E, 0x86, 0x2B, 0x0F,
    0xB4, 0x74, 0xA2, 0xD5, 0x6D, 0xC3, 0x1E, 0xD5, 0x69, 0xA7, 0x70, 0x0B,
    0x4C, 0x4A, 0xB1, 0x6B, 0x22, 0xA4, 0x55, 0x13, 0x53, 0x1E, 0xF5, 0x23,
    0xD7, 0x12, 0x12, 0x07, 0x7B, 0x5A, 0x16, 0x9B, 0xDE, 0xFF, 0xAD, 0x7A,
    0xD9, 0x60, 0x82, 0x84, 0xC7, 0x79, 0x5B, 0x6D, 0x5A, 0x51, 0x83, 0xB8,
    0x70, 0x66, 0xDE, 0x17, 0xD8, 0xD6, 0x71, 0xC9, 0xEB, 0xD8, 0xEC, 0x89,
    0x54, 0x4D, 0x45, 0xEC, 0x06, 0x15, 0x93, 0xD4, 0x42, 0xC6, 0x2A, 0xB9,
    0xCE, 0x3B, 0x1C, 0xB9, 0x94, 0x3A, 0x1D, 0x23, 0xA5, 0xEA, 0x3B, 0xCF,
    0x21, 0xA0, 0x14, 0x71, 0xE6, 0x7E, 0x00, 0x3E, 0x7F, 0x8A, 0x69, 0xC7,
    0x28, 0xBE, 0x49, 0x0B, 0x2F, 0xC8, 0x8C, 0xFE, 0xB9, 0x2D, 0xB6, 0xA2,
    0x15, 0xE5, 0xD0, 0x3C, 0x17, 0xC4, 0x64, 0xC9, 0xAC, 0x1A, 0x46, 0xE2,
    0x03, 0xE1, 0x3F, 0x95, 0x29, 0x95, 0xFB, 0x03, 0xC6, 0x9D, 0x3C, 0xC4,
    0x7F, 0xCB, 0x51, 0x0B, 0x69, 0x98, 0xFF, 0xD3, 0xAA, 0x6D, 0xE7, 0x3C,
    0xF9, 0xF6, 0x38, 0x69
};

static const unsigned char dhtest_2048_256_xA[] = {
    0x08, 0x81, 0x38, 0x2C, 0xDB, 0x87, 0x66, 0x0C, 0x6D, 0xC1, 0x3E, 0x61,
    0x49, 0x38, 0xD5, 0xB9, 0xC8, 0xB2, 0xF2, 0x48, 0x58, 0x1C, 0xC5, 0xE3,
    0x1B, 0x35, 0x45, 0x43, 0x97, 0xFC, 0xE5, 0x0E
};

static const unsigned char dhtest_2048_256_yA[] = {
    0x2E, 0x93, 0x80, 0xC8, 0x32, 0x3A, 0xF9, 0x75, 0x45, 0xBC, 0x49, 0x41,
    0xDE, 0xB0, 0xEC, 0x37, 0x42, 0xC6, 0x2F, 0xE0, 0xEC, 0xE8, 0x24, 0xA6,
    0xAB, 0xDB, 0xE6, 0x6C, 0x59, 0xBE, 0xE0, 0x24, 0x29, 0x11, 0xBF, 0xB9,
    0x67, 0x23, 0x5C, 0xEB, 0xA3, 0x5A, 0xE1, 0x3E, 0x4E, 0xC7, 0x52, 0xBE,
    0x63, 0x0B, 0x92, 0xDC, 0x4B, 0xDE, 0x28, 0x47, 0xA9, 0xC6, 0x2C, 0xB8,
    0x15, 0x27, 0x45, 0x42, 0x1F, 0xB7, 0xEB, 0x60, 0xA6, 0x3C, 0x0F, 0xE9,
    0x15, 0x9F, 0xCC, 0xE7, 0x26, 0xCE, 0x7C, 0xD8, 0x52, 0x3D, 0x74, 0x50,
    0x66, 0x7E, 0xF8, 0x40, 0xE4, 0x91, 0x91, 0x21, 0xEB, 0x5F, 0x01, 0xC8,
    0xC9, 0xB0, 0xD3, 0xD6, 0x48, 0xA9, 0x3B, 0xFB, 0x75, 0x68, 0x9E, 0x82,
    0x44, 0xAC, 0x13, 0x4A, 0xF5, 0x44, 0x71, 0x1C, 0xE7, 0x9A, 0x02, 0xDC,
    0xC3, 0x42, 0x26, 0x68, 0x47, 0x80, 0xDD, 0xDC, 0xB4, 0x98, 0x59, 0x41,
    0x06, 0xC3, 0x7F, 0x5B, 0xC7, 0x98, 0x56, 0x48, 0x7A, 0xF5, 0xAB, 0x02,
    0x2A, 0x2E, 0x5E, 0x42, 0xF0, 0x98, 0x97, 0xC1, 0xA8, 0x5A, 0x11, 0xEA,
    0x02, 0x12, 0xAF, 0x04, 0xD9, 0xB4, 0xCE, 0xBC, 0x93, 0x7C, 0x3C, 0x1A,
    0x3E, 0x15, 0xA8, 0xA0, 0x34, 0x2E, 0x33, 0x76, 0x15, 0xC8, 0x4E, 0x7F,
    0xE3, 0xB8, 0xB9, 0xB8, 0x7F, 0xB1, 0xE7, 0x3A, 0x15, 0xAF, 0x12, 0xA3,
    0x0D, 0x74, 0x6E, 0x06, 0xDF, 0xC3, 0x4F, 0x29, 0x0D, 0x79, 0x7C, 0xE5,
    0x1A, 0xA1, 0x3A, 0xA7, 0x85, 0xBF, 0x66, 0x58, 0xAF, 0xF5, 0xE4, 0xB0,
    0x93, 0x00, 0x3C, 0xBE, 0xAF, 0x66, 0x5B, 0x3C, 0x2E, 0x11, 0x3A, 0x3A,
    0x4E, 0x90, 0x52, 0x69, 0x34, 0x1D, 0xC0, 0x71, 0x14, 0x26, 0x68, 0x5F,
    0x4E, 0xF3, 0x7E, 0x86, 0x8A, 0x81, 0x26, 0xFF, 0x3F, 0x22, 0x79, 0xB5,
    0x7C, 0xA6, 0x7E, 0x29
};

static const unsigned char dhtest_2048_256_xB[] = {
    0x7D, 0x62, 0xA7, 0xE3, 0xEF, 0x36, 0xDE, 0x61, 0x7B, 0x13, 0xD1, 0xAF,
    0xB8, 0x2C, 0x78, 0x0D, 0x83, 0xA2, 0x3B, 0xD4, 0xEE, 0x67, 0x05, 0x64,
    0x51, 0x21, 0xF3, 0x71, 0xF5, 0x46, 0xA5, 0x3D
};

static const unsigned char dhtest_2048_256_yB[] = {
    0x57, 0x5F, 0x03, 0x51, 0xBD, 0x2B, 0x1B, 0x81, 0x74, 0x48, 0xBD, 0xF8,
    0x7A, 0x6C, 0x36, 0x2C, 0x1E, 0x28, 0x9D, 0x39, 0x03, 0xA3, 0x0B, 0x98,
    0x32, 0xC5, 0x74, 0x1F, 0xA2, 0x50, 0x36, 0x3E, 0x7A, 0xCB, 0xC7, 0xF7,
    0x7F, 0x3D, 0xAC, 0xBC, 0x1F, 0x13, 0x1A, 0xDD, 0x8E, 0x03, 0x36, 0x7E,
    0xFF, 0x8F, 0xBB, 0xB3, 0xE1, 0xC5, 0x78, 0x44, 0x24, 0x80, 0x9B, 0x25,
    0xAF, 0xE4, 0xD2, 0x26, 0x2A, 0x1A, 0x6F, 0xD2, 0xFA, 0xB6, 0x41, 0x05,
    0xCA, 0x30, 0xA6, 0x74, 0xE0, 0x7F, 0x78, 0x09, 0x85, 0x20, 0x88, 0x63,
    0x2F, 0xC0, 0x49, 0x23, 0x37, 0x91, 0xAD, 0x4E, 0xDD, 0x08, 0x3A, 0x97,
    0x8B, 0x88, 0x3E, 0xE6, 0x18, 0xBC, 0x5E, 0x0D, 0xD0, 0x47, 0x41, 0x5F,
    0x2D, 0x95, 0xE6, 0x83, 0xCF, 0x14, 0x82, 0x6B, 0x5F, 0xBE, 0x10, 0xD3,
    0xCE, 0x41, 0xC6, 0xC1, 0x20, 0xC7, 0x8A, 0xB2, 0x00, 0x08, 0xC6, 0x98,
    0xBF, 0x7F, 0x0B, 0xCA, 0xB9, 0xD7, 0xF4, 0x07, 0xBE, 0xD0, 0xF4, 0x3A,
    0xFB, 0x29, 0x70, 0xF5, 0x7F, 0x8D, 0x12, 0x04, 0x39, 0x63, 0xE6, 0x6D,
    0xDD, 0x32, 0x0D, 0x59, 0x9A, 0xD9, 0x93, 0x6C, 0x8F, 0x44, 0x13, 0x7C,
    0x08, 0xB1, 0x80, 0xEC, 0x5E, 0x98, 0x5C, 0xEB, 0xE1, 0x86, 0xF3, 0xD5,
    0x49, 0x67, 0x7E, 0x80, 0x60, 0x73, 0x31, 0xEE, 0x17, 0xAF, 0x33, 0x80,
    0xA7, 0x25, 0xB0, 0x78, 0x23, 0x17, 0xD7, 0xDD, 0x43, 0xF5, 0x9D, 0x7A,
    0xF9, 0x56, 0x8A, 0x9B, 0xB6, 0x3A, 0x84, 0xD3, 0x65, 0xF9, 0x22, 0x44,
    0xED, 0x12, 0x09, 0x88, 0x21, 0x93, 0x02, 0xF4, 0x29, 0x24, 0xC7, 0xCA,
    0x90, 0xB8, 0x9D, 0x24, 0xF7, 0x1B, 0x0A, 0xB6, 0x97, 0x82, 0x3D, 0x7D,
    0xEB, 0x1A, 0xFF, 0x5B, 0x0E, 0x8E, 0x4A, 0x45, 0xD4, 0x9F, 0x7F, 0x53,
    0x75, 0x7E, 0x19, 0x13
};

static const unsigned char dhtest_2048_256_Z[] = {
    0x86, 0xC7, 0x0B, 0xF8, 0xD0, 0xBB, 0x81, 0xBB, 0x01, 0x07, 0x8A, 0x17,
    0x21, 0x9C, 0xB7, 0xD2, 0x72, 0x03, 0xDB, 0x2A, 0x19, 0xC8, 0x77, 0xF1,
    0xD1, 0xF1, 0x9F, 0xD7, 0xD7, 0x7E, 0xF2, 0x25, 0x46, 0xA6, 0x8F, 0x00,
    0x5A, 0xD5, 0x2D, 0xC8, 0x45, 0x53, 0xB7, 0x8F, 0xC6, 0x03, 0x30, 0xBE,
    0x51, 0xEA, 0x7C, 0x06, 0x72, 0xCA, 0xC1, 0x51, 0x5E, 0x4B, 0x35, 0xC0,
    0x47, 0xB9, 0xA5, 0x51, 0xB8, 0x8F, 0x39, 0xDC, 0x26, 0xDA, 0x14, 0xA0,
    0x9E, 0xF7, 0x47, 0x74, 0xD4, 0x7C, 0x76, 0x2D, 0xD1, 0x77, 0xF9, 0xED,
    0x5B, 0xC2, 0xF1, 0x1E, 0x52, 0xC8, 0x79, 0xBD, 0x95, 0x09, 0x85, 0x04,
    0xCD, 0x9E, 0xEC, 0xD8, 0xA8, 0xF9, 0xB3, 0xEF, 0xBD, 0x1F, 0x00, 0x8A,
    0xC5, 0x85, 0x30, 0x97, 0xD9, 0xD1, 0x83, 0x7F, 0x2B, 0x18, 0xF7, 0x7C,
    0xD7, 0xBE, 0x01, 0xAF, 0x80, 0xA7, 0xC7, 0xB5, 0xEA, 0x3C, 0xA5, 0x4C,
    0xC0, 0x2D, 0x0C, 0x11, 0x6F, 0xEE, 0x3F, 0x95, 0xBB, 0x87, 0x39, 0x93,
    0x85, 0x87, 0x5D, 0x7E, 0x86, 0x74, 0x7E, 0x67, 0x6E, 0x72, 0x89, 0x38,
    0xAC, 0xBF, 0xF7, 0x09, 0x8E, 0x05, 0xBE, 0x4D, 0xCF, 0xB2, 0x40, 0x52,
    0xB8, 0x3A, 0xEF, 0xFB, 0x14, 0x78, 0x3F, 0x02, 0x9A, 0xDB, 0xDE, 0x7F,
    0x53, 0xFA, 0xE9, 0x20, 0x84, 0x22, 0x40, 0x90, 0xE0, 0x07, 0xCE, 0xE9,
    0x4D, 0x4B, 0xF2, 0xBA, 0xCE, 0x9F, 0xFD, 0x4B, 0x57, 0xD2, 0xAF, 0x7C,
    0x72, 0x4D, 0x0C, 0xAA, 0x19, 0xBF, 0x05, 0x01, 0xF6, 0xF1, 0x7B, 0x4A,
    0xA1, 0x0F, 0x42, 0x5E, 0x3E, 0xA7, 0x60, 0x80, 0xB4, 0xB9, 0xD6, 0xB3,
    0xCE, 0xFE, 0xA1, 0x15, 0xB2, 0xCE, 0xB8, 0x78, 0x9B, 0xB8, 0xA3, 0xB0,
    0xEA, 0x87, 0xFE, 0xBE, 0x63, 0xB6, 0xC8, 0xF8, 0x46, 0xEC, 0x6D, 0xB0,
    0xC2, 0x6C, 0x5D, 0x7C
};

static const unsigned char dhtest_rfc5114_2048_224_bad_y[] = {
    0x45, 0x32, 0x5F, 0x51, 0x07, 0xE5, 0xDF, 0x1C, 0xD6, 0x02, 0x82, 0xB3,
    0x32, 0x8F, 0xA4, 0x0F, 0x87, 0xB8, 0x41, 0xFE, 0xB9, 0x35, 0xDE, 0xAD,
    0xC6, 0x26, 0x85, 0xB4, 0xFF, 0x94, 0x8C, 0x12, 0x4C, 0xBF, 0x5B, 0x20,
    0xC4, 0x46, 0xA3, 0x26, 0xEB, 0xA4, 0x25, 0xB7, 0x68, 0x8E, 0xCC, 0x67,
    0xBA, 0xEA, 0x58, 0xD0, 0xF2, 0xE9, 0xD2, 0x24, 0x72, 0x60, 0xDA, 0x88,
    0x18, 0x9C, 0xE0, 0x31, 0x6A, 0xAD, 0x50, 0x6D, 0x94, 0x35, 0x8B, 0x83,
    0x4A, 0x6E, 0xFA, 0x48, 0x73, 0x0F, 0x83, 0x87, 0xFF, 0x6B, 0x66, 0x1F,
    0xA8, 0x82, 0xC6, 0x01, 0xE5, 0x80, 0xB5, 0xB0, 0x52, 0xD0, 0xE9, 0xD8,
    0x72, 0xF9, 0x7D, 0x5B, 0x8B, 0xA5, 0x4C, 0xA5, 0x25, 0x95, 0x74, 0xE2,
    0x7A, 0x61, 0x4E, 0xA7, 0x8F, 0x12, 0xE2, 0xD2, 0x9D, 0x8C, 0x02, 0x70,
    0x34, 0x44, 0x32, 0xC7, 0xB2, 0xF3, 0xB9, 0xFE, 0x17, 0x2B, 0xD6, 0x1F,
    0x8B, 0x7E, 0x4A, 0xFA, 0xA3, 0xB5, 0x3E, 0x7A, 0x81, 0x9A, 0x33, 0x66,
    0x62, 0xA4, 0x50, 0x18, 0x3E, 0xA2, 0x5F, 0x00, 0x07, 0xD8, 0x9B, 0x22,
    0xE4, 0xEC, 0x84, 0xD5, 0xEB, 0x5A, 0xF3, 0x2A, 0x31, 0x23, 0xD8, 0x44,
    0x22, 0x2A, 0x8B, 0x37, 0x44, 0xCC, 0xC6, 0x87, 0x4B, 0xBE, 0x50, 0x9D,
    0x4A, 0xC4, 0x8E, 0x45, 0xCF, 0x72, 0x4D, 0xC0, 0x89, 0xB3, 0x72, 0xED,
    0x33, 0x2C, 0xBC, 0x7F, 0x16, 0x39, 0x3B, 0xEB, 0xD2, 0xDD, 0xA8, 0x01,
    0x73, 0x84, 0x62, 0xB9, 0x29, 0xD2, 0xC9, 0x51, 0x32, 0x9E, 0x7A, 0x6A,
    0xCF, 0xC1, 0x0A, 0xDB, 0x0E, 0xE0, 0x62, 0x77, 0x6F, 0x59, 0x62, 0x72,
    0x5A, 0x69, 0xA6, 0x5B, 0x70, 0xCA, 0x65, 0xC4, 0x95, 0x6F, 0x9A, 0xC2,
    0xDF, 0x72, 0x6D, 0xB1, 0x1E, 0x54, 0x7B, 0x51, 0xB4, 0xEF, 0x7F, 0x89,
    0x93, 0x74, 0x89, 0x59
};

typedef struct {
    DH *(*get_param) (void);
    const unsigned char *xA;
    size_t xA_len;
    const unsigned char *yA;
    size_t yA_len;
    const unsigned char *xB;
    size_t xB_len;
    const unsigned char *yB;
    size_t yB_len;
    const unsigned char *Z;
    size_t Z_len;
} rfc5114_td;

# define make_rfc5114_td(pre) { \
        DH_get_##pre, \
        dhtest_##pre##_xA, sizeof(dhtest_##pre##_xA), \
        dhtest_##pre##_yA, sizeof(dhtest_##pre##_yA), \
        dhtest_##pre##_xB, sizeof(dhtest_##pre##_xB), \
        dhtest_##pre##_yB, sizeof(dhtest_##pre##_yB), \
        dhtest_##pre##_Z, sizeof(dhtest_##pre##_Z) \
        }

static const rfc5114_td rfctd[] = {
        make_rfc5114_td(1024_160),
        make_rfc5114_td(2048_224),
        make_rfc5114_td(2048_256)
};

static int run_rfc5114_tests(void)
{
    int i;
    DH *dhA = NULL;
    DH *dhB = NULL;
    unsigned char *Z1 = NULL;
    unsigned char *Z2 = NULL;
    const rfc5114_td *td = NULL;
    BIGNUM *bady = NULL;

    for (i = 0; i < (int)(sizeof(rfctd) / sizeof(rfc5114_td)); i++) {
        td = rfctd + i;
        /* Set up DH structures setting key components */
        dhA = td->get_param();
        dhB = td->get_param();
        if (!dhA || !dhB)
            goto bad_err;

        dhA->priv_key = BN_bin2bn(td->xA, td->xA_len, NULL);
        dhA->pub_key = BN_bin2bn(td->yA, td->yA_len, NULL);

        dhB->priv_key = BN_bin2bn(td->xB, td->xB_len, NULL);
        dhB->pub_key = BN_bin2bn(td->yB, td->yB_len, NULL);

        if (!dhA->priv_key || !dhA->pub_key
            || !dhB->priv_key || !dhB->pub_key)
            goto bad_err;

        if ((td->Z_len != (size_t)DH_size(dhA))
            || (td->Z_len != (size_t)DH_size(dhB)))
            goto err;

        Z1 = OPENSSL_malloc(DH_size(dhA));
        Z2 = OPENSSL_malloc(DH_size(dhB));
        /*
         * Work out shared secrets using both sides and compare with expected
         * values.
         */
        if (DH_compute_key(Z1, dhB->pub_key, dhA) == -1)
            goto bad_err;
        if (DH_compute_key(Z2, dhA->pub_key, dhB) == -1)
            goto bad_err;

        if (memcmp(Z1, td->Z, td->Z_len))
            goto err;
        if (memcmp(Z2, td->Z, td->Z_len))
            goto err;

        printf("RFC5114 parameter test %d OK\n", i + 1);

        DH_free(dhA);
        DH_free(dhB);
        OPENSSL_free(Z1);
        OPENSSL_free(Z2);
        dhA = NULL;
        dhB = NULL;
        Z1 = NULL;
        Z2 = NULL;
    }

    /* Now i == OSSL_NELEM(rfctd) */
    /* RFC5114 uses unsafe primes, so now test an invalid y value */
    dhA = DH_get_2048_224();
    if (dhA == NULL)
        goto bad_err;
    Z1 = OPENSSL_malloc(DH_size(dhA));
    if (Z1 == NULL)
        goto bad_err;

    bady = BN_bin2bn(dhtest_rfc5114_2048_224_bad_y,
                     sizeof(dhtest_rfc5114_2048_224_bad_y), NULL);
    if (bady == NULL)
        goto bad_err;

    if (!DH_generate_key(dhA))
        goto bad_err;

    if (DH_compute_key(Z1, bady, dhA) != -1) {
        /*
         * DH_compute_key should fail with -1. If we get here we unexpectedly
         * allowed an invalid y value
         */
        goto err;
    }
    /* We'll have a stale error on the queue from the above test so clear it */
    ERR_clear_error();

    printf("RFC5114 parameter test %d OK\n", i + 1);

    BN_free(bady);
    DH_free(dhA);
    OPENSSL_free(Z1);

    return 1;
 bad_err:
    BN_free(bady);
    DH_free(dhA);
    DH_free(dhB);
    OPENSSL_free(Z1);
    OPENSSL_free(Z2);

    fprintf(stderr, "Initalisation error RFC5114 set %d\n", i + 1);
    ERR_print_errors_fp(stderr);
    return 0;
 err:
    BN_free(bady);
    DH_free(dhA);
    DH_free(dhB);
    OPENSSL_free(Z1);
    OPENSSL_free(Z2);

    fprintf(stderr, "Test failed RFC5114 set %d\n", i + 1);
    return 0;
}

#endif
