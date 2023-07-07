
#include <stdbool.h>
#include <stdlib.h>
#include <core_pins.h>
#include <imxrt.h>
#include <variant.h>
#include <spi.h>
#include <gpio.h>
#include <serial.h>

const uint32_t miso_mux        = 3 | 0x10;
const uint8_t  miso_select_val = 0;

const uint32_t mosi_mux        = 3 | 0x10;
const uint8_t  mosi_select_val = 0;

const uint32_t sck_mux         = 3 | 0x10;
const uint8_t  sck_select_val  = 0;

void spi_begin(uint8_t sclk, uint8_t mosi, uint8_t miso) {

  CCM_CCGR1 &= ~CCM_CCGR1_LPSPI4(CCM_CCGR_ON);

  CCM_CBCMR = (CCM_CBCMR & ~(CCM_CBCMR_LPSPI_PODF_MASK | CCM_CBCMR_LPSPI_CLK_SEL_MASK)) |
               CCM_CBCMR_LPSPI_PODF(2) |
               CCM_CBCMR_LPSPI_CLK_SEL(1);

  uint32_t fastio = IOMUXC_PAD_DSE(7) | IOMUXC_PAD_SPEED(2);

  *(portControlRegister(PIN_SPI_MISO)) = fastio;
  *(portControlRegister(PIN_SPI_MOSI)) = fastio;
  *(portControlRegister(PIN_SPI_SCLK)) = fastio;

  CCM_CCGR1 |= CCM_CCGR1_LPSPI4(CCM_CCGR_ON);

  *(portConfigRegister(PIN_SPI_MISO)) = miso_mux;
  *(portConfigRegister(PIN_SPI_MOSI)) = mosi_mux;
  *(portConfigRegister(PIN_SPI_SCLK)) = sck_mux;

  IOMUXC_LPSPI4_SDI_SELECT_INPUT = miso_select_val;
  IOMUXC_LPSPI4_SDO_SELECT_INPUT = mosi_select_val;
  IOMUXC_LPSPI4_SCK_SELECT_INPUT = sck_select_val;

  IMXRT_LPSPI4_S.CR  = LPSPI_CR_RST;
  IMXRT_LPSPI4_S.FCR = LPSPI_FCR_TXWATER(15);

  IMXRT_LPSPI4_S.CR = 0;
  IMXRT_LPSPI4_S.CFGR1 = LPSPI_CFGR1_MASTER | LPSPI_CFGR1_SAMPLE;
  IMXRT_LPSPI4_S.TCR = LPSPI_TCR_FRAMESZ(7);
  IMXRT_LPSPI4_S.CR = LPSPI_CR_MEN;
}

void spi_set_bit_order(uint8_t bit_order) {

  CCM_CCGR1 |= CCM_CCGR1_LPSPI4(CCM_CCGR_ON);

  if (bit_order == LSBFIRST) {
    IMXRT_LPSPI4_S.TCR |= LPSPI_TCR_LSBF;
  } else {
    IMXRT_LPSPI4_S.TCR &= ~LPSPI_TCR_LSBF;
  }
}

void spi_set_data_mode(uint8_t data_mode) {

  CCM_CCGR1 |= CCM_CCGR1_LPSPI4(CCM_CCGR_ON);

  uint32_t tcr = IMXRT_LPSPI4_S.TCR & ~(LPSPI_TCR_CPOL | LPSPI_TCR_CPHA);

  if (data_mode & 0x08) tcr |= LPSPI_TCR_CPOL;
  if (data_mode & 0x04) tcr |= LPSPI_TCR_CPHA;

  IMXRT_LPSPI4_S.TCR = tcr;
}

void spi_set_clock_div(uint32_t div) {

    IMXRT_LPSPI4_S.CR = 0;
    IMXRT_LPSPI4_S.CCR = LPSPI_CCR_SCKDIV(div) | LPSPI_CCR_DBT(div/2) | LPSPI_CCR_PCSSCK(div/2);
    IMXRT_LPSPI4_S.CR  = LPSPI_CR_MEN;
}

void spi_set_speed(uint32_t kbps) {

    uint32_t clock = kbps * 1000;

    static const uint32_t clk_sel[4] = {
      664615384,  // PLL3 PFD1
      720000000,  // PLL3 PFD0
      528000000,  // PLL2
      396000000   // PLL2 PFD2
    };

    uint32_t cbcmr = CCM_CBCMR;
    uint32_t clkhz = clk_sel[(cbcmr >> 4) & 0x03] / (((cbcmr >> 26 ) & 0x07 ) + 1);

    uint32_t d, div;
    d = clock? clkhz/clock : clkhz;

    if (d && clkhz/d > clock) d++;
    if (d > 257) d= 257;
    if (d > 2) {
      div = d-2;
    } else {
      div = 0;
    }

    IMXRT_LPSPI4_S.CR = 0;
    IMXRT_LPSPI4_S.CCR = LPSPI_CCR_SCKDIV(div) | LPSPI_CCR_DBT(div/2) | LPSPI_CCR_PCSSCK(div/2);
    IMXRT_LPSPI4_S.CR  = LPSPI_CR_MEN;
}

uint8_t spi_transfer(uint8_t data) {
  IMXRT_LPSPI4_S.TDR = data;
  while(!((IMXRT_LPSPI4_S.FSR >> 16) & 0x1F));
  return IMXRT_LPSPI4_S.RDR;
}

uint8_t spi_transfer_8(uint8_t data) {
  IMXRT_LPSPI4_S.TDR = data;
  while(IMXRT_LPSPI4_S.RSR & LPSPI_RSR_RXEMPTY);
  return IMXRT_LPSPI4_S.RDR;
}

uint16_t spi_transfer_16(uint16_t data) {
  uint32_t tcr = IMXRT_LPSPI4_S.TCR;
  IMXRT_LPSPI4_S.TCR = (tcr & 0xfffff000) | LPSPI_TCR_FRAMESZ(15);
  IMXRT_LPSPI4_S.TDR = data;
  while(IMXRT_LPSPI4_S.RSR & LPSPI_RSR_RXEMPTY);
  IMXRT_LPSPI4_S.TCR = tcr;
  return IMXRT_LPSPI4_S.RDR;
}

uint32_t spi_transfer_32(uint32_t data) {
  uint32_t tcr = IMXRT_LPSPI4_S.TCR;
  IMXRT_LPSPI4_S.TCR = (tcr & 0xfffff000) | LPSPI_TCR_FRAMESZ(31);
  IMXRT_LPSPI4_S.TDR = data;
  while(IMXRT_LPSPI4_S.RSR & LPSPI_RSR_RXEMPTY);
  IMXRT_LPSPI4_S.TCR = tcr;
  return IMXRT_LPSPI4_S.RDR;
}

void spi_transfer_buf_16(uint16_t* buf, size_t n) {
  uint32_t tcr = IMXRT_LPSPI4_S.TCR;
  IMXRT_LPSPI4_S.TCR = (tcr & 0xfffff000) | LPSPI_TCR_FRAMESZ(15);
  for(size_t i=0; i<n; i++){
    IMXRT_LPSPI4_S.TDR = buf[i];
    while(IMXRT_LPSPI4_S.RSR & LPSPI_RSR_RXEMPTY);
    uint16_t r=IMXRT_LPSPI4_S.RDR;
  }
  IMXRT_LPSPI4_S.TCR = tcr;
}

