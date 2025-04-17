
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
static volatile bool thread_running=true;
static void* timer_thread(void* data);

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

#define MAX_TIMER_COUNT 1000

uint16_t topid=1000;

typedef struct timer_node {
  int                id;
  int                fd;
  time_up_cb         callback;
  void*              callback_arg;
  t_timer            type;
  struct timer_node* next;
} timer_node;

static timer_node* timer_list = 0;

timer_node* timer_by_fd(int fd)
{
  timer_node* timer = timer_list;
  while(timer) {
    if(timer->fd==fd) return timer;
    timer = timer->next;
  }
  return 0;
}

timer_node* timer_by_id(uint16_t id)
{
  timer_node* timer = timer_list;
  while(timer) {
    if(timer->id==id) return timer;
    timer = timer->next;
  }
  return 0;
}

uint16_t create_timer(time_up_cb cb, void* arg, t_timer type)
{
  timer_node* timer = (timer_node*)malloc(sizeof(timer_node));

  if(!timer) return 0;

  timer->id           = topid++;
  timer->callback     = cb;
  timer->callback_arg = arg;
  timer->type         = type;
  timer->fd           = timerfd_create(CLOCK_REALTIME, 0);

  if(timer->fd == -1) { free(timer); return 0; }

  timer->next = timer_list;
  timer_list = timer;

  return timer->id;
}

uint16_t time_ticker(time_up_cb cb, void* arg, uint32_t every)
{
  uint32_t id=create_timer(cb, arg, TIMER_PERIODIC);
  time_start_timer(id, every);
  return id;
}

uint16_t time_timeout(time_up_cb cb, void* arg)
{
  return create_timer(cb, arg, TIMER_SINGLE_SHOT);
}

void time_start_timer(uint16_t id, uint32_t timeout)
{
  timer_node* timer=timer_by_id(id);
  if(!timer) return;

  struct itimerspec timerspec;

  timerspec.it_value.tv_sec = timeout / 1000;
  timerspec.it_value.tv_nsec = (timeout % 1000)* 1000000;

  if(timer->type==TIMER_PERIODIC) {
    timerspec.it_interval.tv_sec= timeout / 1000;
    timerspec.it_interval.tv_nsec = (timeout %1000) * 1000000;
  } else {
    timerspec.it_interval.tv_sec= 0;
    timerspec.it_interval.tv_nsec = 0;
  }

  timerfd_settime(timer->fd, 0, &timerspec, 0);
}

void time_stop_timer(uint16_t id)
{
  time_start_timer(id, 0);
}

static void remove_timer(timer_node* timer)
{
  if(!timer) return;
  close(timer->fd);
  if(timer == timer_list) {
    timer_list = timer_list->next;
  } else {
    timer_node* t = timer_list;
    while(t && t->next != timer) t = t->next;
    if(t) t->next = t->next->next;
  }
  free(timer);
}

void time_end()
{
  while(timer_list){ remove_timer(timer_list); }
  thread_running=false;
  pthread_join(thread_id, 0);
}

void* timer_thread(void* data)
{
  struct pollfd ufds[MAX_TIMER_COUNT] = {{0}};
  int fdn = 0;
  timer_node* timer = 0;
  int read_fds=0, i, s;
  uint64_t exp;

  while(thread_running) {

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
    pthread_testcancel();
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);

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
        if(timer && timer->callback) timer->callback(timer->callback_arg);
      }
    }
  }
  return 0;
}
