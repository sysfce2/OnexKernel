
#include <stdint.h>
#include <string.h>
#include <variant.h>
#include <time.h>
#include <gpio.h>
#include <serial.h>
#include <tft.h>

#define DISPLAYWIDTH 128
#define DISPLAYHEIGHT 128

#define ROWHEIGHT  32

typedef void (*select_callback)(void*);

typedef struct view_item {

  const char*       label;
  int               colour;
  uint8_t           size;
  struct view_item* link;
  select_callback   call;

} view_item;

view_item strip[] =  { { .label="Red",     .colour=TFT_RED,     .size=2, .link=0, .call=0 },
                       { .label="Yellow",  .colour=TFT_YELLOW,  .size=2, .link=0, .call=0 },
                       { .label="Green",   .colour=TFT_GREEN,   .size=2, .link=0, .call=0 },
                       { .label="Cyan",    .colour=TFT_CYAN,    .size=2, .link=0, .call=0 },
                       { .label="Blue",    .colour=TFT_BLUE,    .size=2, .link=0, .call=0 },
                       { .label=0,         .colour=0,           .size=0, .link=0, .call=0 } };

view_item home[] =   { { .label="RGB Strip",   .colour=TFT_YELLOW, .size=2, .link=strip, .call=0 },
                       { .label="Soil Sensor", .colour=TFT_GREEN,  .size=2, .link=0,     .call=0 },
                       { .label="Tag Button",  .colour=TFT_BLUE,   .size=2, .link=0,     .call=0 },
                       { .label=0,             .colour=0,          .size=0, .link=0,     .call=0 } };

view_item* current_view=home;
int        current_row=0;


int number_of_rows=-1;

float get_row_height(int r) {

  float scale=(1+current_view[r].size/3.0)/2;
  return (int)(ROWHEIGHT*scale);
}

float total_row_height(int r) {

  float t=0;
  for(int i=0; i<r; i++) t+=get_row_height(i);
  return t;
}

void draw_selection() {

  tft_draw_rectangle(0, total_row_height(current_row),   DISPLAYWIDTH,   get_row_height(current_row),   TFT_ORANGE);
  tft_draw_rectangle(1, total_row_height(current_row)+1, DISPLAYWIDTH-2, get_row_height(current_row)-2, TFT_ORANGE);
}

void fill_rows() {

  view_item* items=current_view;
  tft_set_text_colour(TFT_BLACK);
  int currh=0;
  for(int i=0; i<9; i++){
      if(!items[i].label && !items[i].colour && !items[i].link && !items[i].call){
        number_of_rows=i;
        tft_fill_rectangle(0, currh, DISPLAYWIDTH, DISPLAYHEIGHT-currh, TFT_GREY);
        draw_selection();
        return;
      }
      int h=get_row_height(i);
      tft_fill_rectangle( 0, currh, DISPLAYWIDTH, h, items[i].colour);
      tft_draw_rectangle( 0, currh, DISPLAYWIDTH, h, TFT_GREY);
      tft_set_cursor(0+2, currh+ROWHEIGHT/2-10);
      tft_set_text_size(items[i].size);
      tft_draw_text(items[i].label);
      currh+=h;
  }
}

void handle_select() {

  serial_printf("select\n");
  if(current_view[current_row].call || current_view[current_row].link){
    if(current_view[current_row].call){
      current_view[current_row].call(&current_view[current_row]);
    }
    if(current_view[current_row].link){
      current_view=current_view[current_row].link;
      current_row=0;
      fill_rows();
    }
  }
}

void handle_back() {

  serial_printf("back\n");
  current_view=home;
  current_row=0;
  fill_rows();
}

int main(void) {

  time_init();
  serial_init(0,9600);

  tft_init(PIN_SPI_SS, PIN_TFT_DC);
  tft_set_rotation(3);
  tft_fill_screen(TFT_BLACK);
  fill_rows();

  while(1) {
    time_delay_ms(100);
    static uint32_t loop_count=0;
    loop_count++;
    if(!(loop_count % 10)) handle_back();
    else
    if(!(loop_count % 5))  handle_select();
  }
}



