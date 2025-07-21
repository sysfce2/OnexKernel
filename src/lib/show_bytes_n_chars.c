
#include <string.h>

#include <inttypes.h>
#define FMT_UINT32 PRIu32

#include <onex-kernel/log.h>
#include <onex-kernel/lib.h>
#include <onex-kernel/mem.h>

#define LINE_LEN 32

static void log_line(uint32_t addr, const uint8_t* data, uint16_t len) {
    log_write("%08x:", addr);
    for(uint16_t i = 0; i < LINE_LEN; i++){
        if(i < len) log_write(" %02x", data[i]);
        else        log_write("   ");
    }
    log_write("  |");
    for(uint16_t i = 0; i < LINE_LEN; i++){
        if(i < len) log_write("%c", isprint(data[i])? data[i]: '.');
        else        log_write(" ");
    }
    log_write("|\n");
}

void show_bytes_and_chars(uint32_t base_addr, uint8_t* buf, uint16_t size) {

    unsigned char curr[LINE_LEN];
    unsigned char prev[LINE_LEN];

    bool have_prev = false;
    bool skipping = false;
    uint16_t skip_count = 0;

    for(uint32_t offset = 0; offset < size; offset += LINE_LEN) {

        uint16_t n = (size - offset < LINE_LEN)? size - offset: LINE_LEN;
        memcpy(curr, buf + offset, n);

        if(have_prev && n == LINE_LEN && memcmp(curr, prev, LINE_LEN) == 0) {
            if(!skipping) {
                skipping = true;
                skip_count = 1;
            } else {
                skip_count++;
            }
        } else {

            if(skipping) {
                if(skip_count > 1) {
                    log_write("-------  <repeats %u times>  -------\n", skip_count - 1);
                }
                log_line(base_addr + offset - LINE_LEN, prev, LINE_LEN);
                skipping = false;
            }
            log_line(base_addr + offset, curr, n);
        }

        memcpy(prev, curr, n);
        have_prev = true;
    }
    if(skipping) {
        if(skip_count > 1) {
            log_write("-------  <repeats %u times>  -------\n", skip_count - 1);
        }
        log_line(base_addr + size - LINE_LEN, prev, LINE_LEN);
    }
}


