
#include <stdbool.h>
#include <stdlib.h>

#include <app_error.h>
#include <nrf_soc.h>

#include <onex-kernel/random.h>
#include <onex-kernel/log.h>

// Thanks to Espruino for these

unsigned int get_rand() {
  unsigned int r=0;
  uint8_t bytes_needed=sizeof(r);
  uint8_t bytes_avail=0;
  unsigned int c;
  for(c=600000; c && bytes_avail < bytes_needed; c--) sd_rand_application_bytes_available_get(&bytes_avail);
  if(bytes_avail >= bytes_needed) sd_rand_application_vector_get((uint8_t*)&r, bytes_needed);
  else log_write("Timeout on get_rand\n");
  return r;
}

uint8_t random_byte()
{
  return get_rand() & 0xFF;
}

unsigned int rand_m_w = 0xDEADBEEF;
unsigned int rand_m_z = 0xCAFEBABE;

void srand(unsigned int seed) {
  rand_m_w = (seed&0xFFFF) | (seed<<16);
  rand_m_z = (seed&0xFFFF0000) | (seed>>16);
}

int rand() {
  rand_m_z = 36969 * (rand_m_z & 65535) + (rand_m_z >> 16);
  rand_m_w = 18000 * (rand_m_w & 65535) + (rand_m_w >> 16);
  return (int)RAND_MAX & (int)((rand_m_z << 16) + rand_m_w);
}

static bool random_initialised=false;

uint8_t random_ish_byte(){
  if(!random_initialised){
    srand(get_rand());
    random_initialised=true;
  }
  return rand() & 0xFF;
}


