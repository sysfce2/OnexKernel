
#include <stdint.h>
#include <string.h>
#include <sys/timerfd.h>
#include <pthread.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>

#include <onex-kernel/time.h>

static bool initialised=false;

uint64_t startus;

uint64_t get_time_us()
{
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return t.tv_sec*1000000+t.tv_nsec/1000;
}

static pthread_t thread_id;
static void * timer_thread(void * data);

void time_init()
{
  if(initialised) return;
  startus=get_time_us();
  pthread_create(&thread_id, 0, timer_thread, 0);
  initialised=true;
}

uint64_t time_us()
{
  time_init();
  return get_time_us()-startus;
}

uint64_t time_ms()
{
  time_init();
  return time_us()/1000;
}

static uint32_t offset=0;

uint64_t time_es()
{
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return t.tv_sec-offset;
}

void time_es_set(uint64_t es)
{
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  offset=t.tv_sec-es;
}

/* THANK YOU to https://qnaplus.com/implement-periodic-timer-linux/ !! */

typedef enum {
  TIMER_SINGLE_SHOT = 0,
  TIMER_PERIODIC
} t_timer;

static uint64_t start_timer(uint32_t interval, time_up_cb handler, t_timer type);
/*
static void stop_timer(uint64_t timer_id);
static void finalize();
*/

#define MAX_TIMER_COUNT 1000

typedef struct timer_node {
  int                fd;
  time_up_cb         callback;
  uint32_t           interval;
  t_timer            type;
  struct timer_node* next;
} timer_node;

static timer_node* timer_list = 0;

uint64_t start_timer(uint32_t interval, time_up_cb handler, t_timer type)
{
    timer_node* new_node = (timer_node*)malloc(sizeof(timer_node));

    if(!new_node) return 0;

    new_node->callback  = handler;
    new_node->interval  = interval;
    new_node->type      = type;

    new_node->fd = timerfd_create(CLOCK_REALTIME, 0);

    if(new_node->fd == -1) {
      free(new_node);
      return 0;
    }

    struct itimerspec timerspec;

    timerspec.it_value.tv_sec = interval / 1000;
    timerspec.it_value.tv_nsec = (interval % 1000)* 1000000;

    if(type==TIMER_PERIODIC) {
      timerspec.it_interval.tv_sec= interval / 1000;
      timerspec.it_interval.tv_nsec = (interval %1000) * 1000000;
    } else {
      timerspec.it_interval.tv_sec= 0;
      timerspec.it_interval.tv_nsec = 0;
    }

    timerfd_settime(new_node->fd, 0, &timerspec, 0);

    new_node->next = timer_list;
    timer_list = new_node;

    return (uint64_t)new_node;
}

/*
void stop_timer(uint64_t timer_id)
{
    timer_node* this_timer = (timer_node*)timer_id;
    if (!this_timer) return;

    close(this_timer->fd);

    if(this_timer == timer_list) {
      timer_list = timer_list->next;
    } else {
      timer_node* timer = timer_list;
      while(timer && timer->next != this_timer) timer = timer->next;
      if(timer) timer->next = timer->next->next;
    }
    if(this_timer) free(this_timer);
}


void finalize()
{
  while(timer_list) stop_timer((uint64_t)timer_list);
  pthread_cancel(thread_id);
  pthread_join(thread_id, 0);
}
*/

timer_node* timer_by_fd(int fd)
{
  timer_node* timer = timer_list;
  while(timer) {
    if(timer->fd==fd) return timer;
    timer = timer->next;
  }
  return 0;
}

void* timer_thread(void * data)
{
  struct pollfd ufds[MAX_TIMER_COUNT] = {{0}};
  int fdn = 0;
  timer_node* timer = 0;
  int read_fds=0, i, s;
  uint64_t exp;

  while(1) {

#if !defined(__ANDROID__)
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
    pthread_testcancel();
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
#else
#endif

    memset(ufds, 0, sizeof(struct pollfd)*MAX_TIMER_COUNT);
    timer = timer_list;
    fdn = 0;
    while(timer && fdn<MAX_TIMER_COUNT) {
      ufds[fdn].fd = timer->fd;
      ufds[fdn].events = POLLIN;
      fdn++;
      timer = timer->next;
    }
    read_fds = poll(ufds, fdn, 100);

    if(read_fds <= 0) continue;

    for(i=0; i<fdn; i++) {
      if(ufds[i].revents & POLLIN) {
        s = read(ufds[i].fd, &exp, sizeof(uint64_t));
        if(s!=sizeof(uint64_t)) continue;
        timer = timer_by_fd(ufds[i].fd);
        if(timer && timer->callback) timer->callback();
      }
    }
  }
  return 0;
}

void time_ticker(time_up_cb cb, uint32_t every)
{
  start_timer(every, cb, TIMER_PERIODIC);
}
