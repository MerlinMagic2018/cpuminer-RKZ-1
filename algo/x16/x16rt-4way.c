#include "x16r-gate.h"

#if defined (X16R_4WAY)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "algo/blake/blake-hash-4way.h"
#include "algo/bmw/bmw-hash-4way.h"
#include "algo/groestl/aes_ni/hash-groestl.h"
#include "algo/groestl/aes_ni/hash-groestl.h"
#include "algo/skein/skein-hash-4way.h"
#include "algo/jh/jh-hash-4way.h"
#include "algo/keccak/keccak-hash-4way.h"
#include "algo/shavite/sph_shavite.h"
#include "algo/luffa/luffa-hash-2way.h"
#include "algo/cubehash/cubehash_sse2.h"
#include "algo/simd/simd-hash-2way.h"
#include "algo/echo/aes_ni/hash_api.h"
#include "algo/hamsi/hamsi-hash-4way.h"
#include "algo/fugue/sph_fugue.h"
#include "algo/shabal/shabal-hash-4way.h"
#include "algo/whirlpool/sph_whirlpool.h"
#include "algo/sha/sha-hash-4way.h"

static __thread uint32_t s_ntime = UINT32_MAX;
static __thread bool s_implemented = false;
static __thread char hashOrder[X16R_HASH_FUNC_COUNT + 1] = { 0 };

union _x16rt_4way_context_overlay
{
    blake512_4way_context   blake;
    bmw512_4way_context     bmw;
    hashState_echo          echo;
    hashState_groestl       groestl;
    skein512_4way_context   skein;
    jh512_4way_context      jh;
    keccak512_4way_context  keccak;
    luffa_2way_context      luffa;
    cubehashParam           cube;
    sph_shavite512_context  shavite;
    simd_2way_context       simd;
    hamsi512_4way_context   hamsi;
    sph_fugue512_context    fugue;
    shabal512_4way_context  shabal;
    sph_whirlpool_context   whirlpool;
    sha512_4way_context     sha512;
};
typedef union _x16rt_4way_context_overlay x16rt_4way_context_overlay;

void x16rt_4way_hash( void* output, const void* input )
{
   uint32_t hash0[24] __attribute__ ((aligned (64)));
   uint32_t hash1[24] __attribute__ ((aligned (64)));
   uint32_t hash2[24] __attribute__ ((aligned (64)));
   uint32_t hash3[24] __attribute__ ((aligned (64)));
   uint32_t vhash[24*4] __attribute__ ((aligned (64)));
   x16rt_4way_context_overlay ctx;
   void *in0 = (void*) hash0;
   void *in1 = (void*) hash1;
   void *in2 = (void*) hash2;
   void *in3 = (void*) hash3;
   int size = 80;

   dintrlv_4x64( hash0, hash1, hash2, hash3, input, 640 );
 
/*
   void *in = (void*) input;
   uint32_t *in32 = (uint32_t*) hash0;
   uint32_t ntime = in32[17];
   if ( s_ntime == UINT32_MAX )
   {
      uint32_t _ALIGN(64) timeHash[8];
      x16rt_getTimeHash(ntime, &timeHash);
      x16rt_getAlgoString(&timeHash[0], hashOrder);
   }
*/

   // Input data is both 64 bit interleaved (input)
   // and deinterleaved in inp0-3.
   // If First function uses 64 bit data it is not required to interleave inp
   // first. It may use the inerleaved data dmost convenient, ie 4way 64 bit.
   // All other functions assume data is deinterleaved in hash0-3
   // All functions must exit with data deinterleaved in hash0-3.
   // Alias in0-3 points to either inp0-3 or hash0-3 according to
   // its hashOrder position. Size is also set accordingly.
   for ( int i = 0; i < 16; i++ )
   {
      const char elem = hashOrder[i];
      const uint8_t algo = elem >= 'A' ? elem - 'A' + 10 : elem - '0';

      switch ( algo )
      {
         case BLAKE:
            blake512_4way_init( &ctx.blake );
            if ( i == 0 )
               blake512_4way( &ctx.blake, input, size );
            else
            {
               intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
               blake512_4way( &ctx.blake, vhash, size );
            }
            blake512_4way_close( &ctx.blake, vhash );
            dintrlv_4x64( hash0, hash1, hash2, hash3, vhash, 512 );
         break;
         case BMW:
            bmw512_4way_init( &ctx.bmw );
            if ( i == 0 )
               bmw512_4way( &ctx.bmw, input, size );
            else
            {
               intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
               bmw512_4way( &ctx.bmw, vhash, size );
            }
            bmw512_4way_close( &ctx.bmw, vhash );
            dintrlv_4x64( hash0, hash1, hash2, hash3, vhash, 512 );
         break;
         case GROESTL:
               init_groestl( &ctx.groestl, 64 );
               update_and_final_groestl( &ctx.groestl, (char*)hash0,
                                                 (const char*)in0, size<<3 );
               init_groestl( &ctx.groestl, 64 );
               update_and_final_groestl( &ctx.groestl, (char*)hash1,
                                                 (const char*)in1, size<<3 );
               init_groestl( &ctx.groestl, 64 );
               update_and_final_groestl( &ctx.groestl, (char*)hash2,
                                                 (const char*)in2, size<<3 );
               init_groestl( &ctx.groestl, 64 );
               update_and_final_groestl( &ctx.groestl, (char*)hash3,
                                                 (const char*)in3, size<<3 );
         break;
         case SKEIN:
            skein512_4way_init( &ctx.skein );
            if ( i == 0 )
               skein512_4way( &ctx.skein, input, size );
            else
            {
               intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
               skein512_4way( &ctx.skein, vhash, size );
            }
            skein512_4way_close( &ctx.skein, vhash );
            dintrlv_4x64( hash0, hash1, hash2, hash3, vhash, 512 );
         break;
         case JH:
            jh512_4way_init( &ctx.jh );
            if ( i == 0 )
               jh512_4way( &ctx.jh, input, size );
            else
            {
               intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
               jh512_4way( &ctx.jh, vhash, size );
            }
            jh512_4way_close( &ctx.jh, vhash );
            dintrlv_4x64( hash0, hash1, hash2, hash3, vhash, 512 );
         break;
         case KECCAK:
            keccak512_4way_init( &ctx.keccak );
            if ( i == 0 )
               keccak512_4way( &ctx.keccak, input, size );
            else
            {
               intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
               keccak512_4way( &ctx.keccak, vhash, size );
            }
            keccak512_4way_close( &ctx.keccak, vhash );
            dintrlv_4x64( hash0, hash1, hash2, hash3, vhash, 512 );
         break;
         case LUFFA:
            intrlv_2x128( vhash, in0, in1, size<<3 );
            luffa_2way_init( &ctx.luffa, 512 );
            luffa_2way_update_close( &ctx.luffa, vhash, vhash, size );
            dintrlv_2x128( hash0, hash1, vhash, 512 );
            intrlv_2x128( vhash, in2, in3, size<<3 );
            luffa_2way_init( &ctx.luffa, 512 );
            luffa_2way_update_close( &ctx.luffa, vhash, vhash, size);
            dintrlv_2x128( hash2, hash3, vhash, 512 );
         break;
         case CUBEHASH:
            cubehashInit( &ctx.cube, 512, 16, 32 );
            cubehashUpdateDigest( &ctx.cube, (byte*) hash0,
                                  (const byte*)in0, size );
            cubehashInit( &ctx.cube, 512, 16, 32 );
            cubehashUpdateDigest( &ctx.cube, (byte*) hash1,
                                  (const byte*)in1, size );
            cubehashInit( &ctx.cube, 512, 16, 32 );
            cubehashUpdateDigest( &ctx.cube, (byte*) hash2,
                                  (const byte*)in2, size );
            cubehashInit( &ctx.cube, 512, 16, 32 );
            cubehashUpdateDigest( &ctx.cube, (byte*) hash3,
                                  (const byte*)in3, size );
         break;
         case SHAVITE:
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in0, size );
            sph_shavite512_close( &ctx.shavite, hash0 );
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in1, size );
            sph_shavite512_close( &ctx.shavite, hash1 );
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in2, size );
            sph_shavite512_close( &ctx.shavite, hash2 );
            sph_shavite512_init( &ctx.shavite );
            sph_shavite512( &ctx.shavite, in3, size );
            sph_shavite512_close( &ctx.shavite, hash3 );
         break;
         case SIMD:
            intrlv_2x128( vhash, in0, in1, size<<3 );
            simd_2way_init( &ctx.simd, 512 );
            simd_2way_update_close( &ctx.simd, vhash, vhash, size<<3 );
            dintrlv_2x128( hash0, hash1, vhash, 512 );
            intrlv_2x128( vhash, in2, in3, size<<3 );
            simd_2way_init( &ctx.simd, 512 );
            simd_2way_update_close( &ctx.simd, vhash, vhash, size<<3 );
            dintrlv_2x128( hash2, hash3, vhash, 512 );
         break;
         case ECHO:
             init_echo( &ctx.echo, 512 );
             update_final_echo ( &ctx.echo, (BitSequence *)hash0,
                                (const BitSequence*)in0, size<<3 );
             init_echo( &ctx.echo, 512 );
             update_final_echo ( &ctx.echo, (BitSequence *)hash1,
                                (const BitSequence*)in1, size<<3 );
             init_echo( &ctx.echo, 512 );
             update_final_echo ( &ctx.echo, (BitSequence *)hash2,
                                (const BitSequence*)in2, size<<3 );
             init_echo( &ctx.echo, 512 );
             update_final_echo ( &ctx.echo, (BitSequence *)hash3,
                                (const BitSequence*)in3, size<<3 );
         break;
         case HAMSI:
             intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
             hamsi512_4way_init( &ctx.hamsi );
             hamsi512_4way( &ctx.hamsi, vhash, size );
             hamsi512_4way_close( &ctx.hamsi, vhash );
             dintrlv_4x64( hash0, hash1, hash2, hash3, vhash, 512 );
         break;
         case FUGUE:
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in0, size );
             sph_fugue512_close( &ctx.fugue, hash0 );
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in1, size );
             sph_fugue512_close( &ctx.fugue, hash1 );
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in2, size );
             sph_fugue512_close( &ctx.fugue, hash2 );
             sph_fugue512_init( &ctx.fugue );
             sph_fugue512( &ctx.fugue, in3, size );
             sph_fugue512_close( &ctx.fugue, hash3 );
         break;
         case SHABAL:
             intrlv_4x32( vhash, in0, in1, in2, in3, size<<3 );
             shabal512_4way_init( &ctx.shabal );
             shabal512_4way( &ctx.shabal, vhash, size );
             shabal512_4way_close( &ctx.shabal, vhash );
             dintrlv_4x32( hash0, hash1, hash2, hash3, vhash, 512 );
         break;
         case WHIRLPOOL:
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in0, size );
             sph_whirlpool_close( &ctx.whirlpool, hash0 );
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in1, size );
             sph_whirlpool_close( &ctx.whirlpool, hash1 );
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in2, size );
             sph_whirlpool_close( &ctx.whirlpool, hash2 );
             sph_whirlpool_init( &ctx.whirlpool );
             sph_whirlpool( &ctx.whirlpool, in3, size );
             sph_whirlpool_close( &ctx.whirlpool, hash3 );
         break;
         case SHA_512:
             intrlv_4x64( vhash, in0, in1, in2, in3, size<<3 );
             sha512_4way_init( &ctx.sha512 );
             sha512_4way( &ctx.sha512, vhash, size );
             sha512_4way_close( &ctx.sha512, vhash );
             dintrlv_4x64( hash0, hash1, hash2, hash3, vhash, 512 );
         break;
      }
      size = 64;
   }
   memcpy( output,    hash0, 32 );
   memcpy( output+32, hash1, 32 );
   memcpy( output+64, hash2, 32 );
   memcpy( output+96, hash3, 32 );
}

int scanhash_x16rt_4way( struct work *work, uint32_t max_nonce,
                        uint64_t *hashes_done, struct thr_info *mythr)
{
   uint32_t hash[4*16] __attribute__ ((aligned (64)));
   uint32_t vdata[24*4] __attribute__ ((aligned (64)));
   uint32_t endiandata[20] __attribute__((aligned(64)));
   uint32_t _ALIGN(64) timeHash[4*8];
   uint32_t *pdata = work->data;
   uint32_t *ptarget = work->target;
   const uint32_t Htarg = ptarget[7];
   const uint32_t first_nonce = pdata[19];
   uint32_t n = first_nonce;
   int thr_id = mythr->id;  // thr_id arg is deprecated
    __m256i  *noncev = (__m256i*)vdata + 9;   // aligned
   volatile uint8_t *restart = &(work_restart[thr_id].restart);

   casti_m256i( endiandata, 0 ) = mm256_bswap_32( casti_m256i( pdata, 0 ) );
   casti_m256i( endiandata, 1 ) = mm256_bswap_32( casti_m256i( pdata, 1 ) );
   casti_m128i( endiandata, 4 ) = mm128_bswap_32( casti_m128i( pdata, 4 ) );

   uint32_t ntime = swab32( pdata[17] );
   if ( s_ntime != ntime )
   {
      x16rt_getTimeHash( ntime, &timeHash );
      x16rt_getAlgoString( &timeHash[0], hashOrder );
      s_ntime = ntime;
      s_implemented = true;
      if ( opt_debug && !thr_id )
          applog( LOG_INFO, "hash order: %s time: (%08x) time hash: (%08x)",
                               hashOrder, ntime, timeHash );
   }
   if ( !s_implemented )
   {
      applog( LOG_WARNING, "s not implemented");
      sleep(1);
      return 0;
   }

   if ( opt_benchmark )
      ptarget[7] = 0x0cff;

   uint64_t *edata = (uint64_t*)endiandata;
   intrlv_4x64( (uint64_t*)vdata, edata, edata, edata, edata, 640 );

   do
   {
      *noncev = mm256_intrlv_blend_32( mm256_bswap_32(
               _mm256_set_epi32( n+3, 0, n+2, 0, n+1, 0, n, 0 ) ), *noncev );

      x16rt_4way_hash( hash, vdata );
      pdata[19] = n;

      for ( int i = 0; i < 4; i++ )  if ( (hash+(i<<3))[7] <= Htarg )
      if( fulltest( hash+(i<<3), ptarget ) && !opt_benchmark )
      {
         pdata[19] = n+i;
         submit_lane_solution( work, hash+(i<<3), mythr, i );
      }
      n += 4;
   } while ( (  n < max_nonce ) && !(*restart) );

   *hashes_done = n - first_nonce + 1;
   return 0;
}

#endif
