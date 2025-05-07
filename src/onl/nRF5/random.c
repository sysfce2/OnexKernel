
#include <stdbool.h>
#include <stdlib.h>

#include <nrf_soc.h>
#include <nrf_crypto.h>

#include <onex-kernel/random.h>
#include <onex-kernel/log.h>

static uint32_t get_rand() {
  uint32_t r=0;
  uint8_t  n=sizeof(r);
  ret_code_t err_code = nrf_crypto_rng_vector_generate((uint8_t*)&r, n);
  if(err_code != NRF_SUCCESS) log_write("Crypto rng failure %x\n", err_code);
  return r;
}

uint8_t random_byte()
{
  return get_rand() & 0xFF;
}

static uint32_t rand_m_w = 0xDEADBEEF;
static uint32_t rand_m_z = 0xCAFEBABE;

static void seed_rand(uint32_t seed) {
  rand_m_w = (seed&0xFFFF) | (seed<<16);
  rand_m_z = (seed&0xFFFF0000) | (seed>>16);
}

static int gen_rand() {
  rand_m_z = 36969 * (rand_m_z & 65535) + (rand_m_z >> 16);
  rand_m_w = 18000 * (rand_m_w & 65535) + (rand_m_w >> 16);
  return (int)RAND_MAX & (int)((rand_m_z << 16) + rand_m_w);
}

static bool initialised=false;

void random_init()
{
  if(initialised) return;

  ret_code_t err_code = nrf_crypto_init();
  if(err_code != NRF_SUCCESS) log_write("nrf_crypto_init failed: %x\n", err_code);

  seed_rand(get_rand());

  initialised=true;
}

uint8_t random_ish_byte()
{
  return gen_rand() & 0xFF;
}


