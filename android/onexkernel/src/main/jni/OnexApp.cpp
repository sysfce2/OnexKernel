#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <sys/system_properties.h>

extern "C" {
#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <onf.h>
#include <assert.h>
extern void run_properties_tests();
extern void run_list_tests();
extern void run_onf_tests();
}

android_app* androidApp;

bool button_pressed=false;

bool evaluate_button(object* button)
{
  char* s=(char*)(button_pressed? "down": "up");
  object_property_set(button, (char*)"state", s);
  log_write("evaluate_button: "); object_log(button);
  return true;
}

extern "C" {

void serial_send(char* b)
{
  log_write("serial_send %s\n", b);
}

}

class OnexApp
{
public:

  OnexApp()
  {
  }

  ~OnexApp()
  {
  }

  static void handleAppCommand(android_app* app, int32_t cmd)
  {
    OnexApp* onexapp = reinterpret_cast<OnexApp*>(app->userData);
    switch (cmd)
    {
    case APP_CMD_SAVE_STATE:
      log_write("APP_CMD_SAVE_STATE");
      break;
    case APP_CMD_INIT_WINDOW:
      log_write("APP_CMD_INIT_WINDOW");
      break;
    case APP_CMD_LOST_FOCUS:
      log_write("APP_CMD_LOST_FOCUS");
      break;
    case APP_CMD_GAINED_FOCUS:
      log_write("APP_CMD_GAINED_FOCUS");
      break;
    case APP_CMD_TERM_WINDOW:
      log_write("APP_CMD_TERM_WINDOW");
      break;
    }
  }

  static int32_t handleAppInput(struct android_app* app, AInputEvent* event)
  {
    OnexApp* onexapp = reinterpret_cast<OnexApp*>(app->userData);
    return 0;
  }

  object* button;

  void run()
  {
    log_init(0);
    time_init();
    onex_init();

    log_write("---------------OnexKernel tests----------------------\n");

    run_properties_tests();
    run_list_tests();
    run_onf_tests();

    int failures=onex_assert_summary();

    log_write("---------------%d failures---------------------------\n", failures);

    log_write("\n------Starting Button Test-----\n");

    button=object_new((char*)"uid-1-2-3", (char*)"button", evaluate_button, 4);

    int lasttime=0;

    while(1){

      onex_loop();

      if(time_ms() > lasttime+1000){
         lasttime=time_ms();
         button_pressed=!button_pressed;
         onex_run_evaluators(button);
      }
    }
  }
};

OnexApp* onexapp;

void android_main(android_app* state)
{
  onexapp = new OnexApp();
  state->userData = onexapp;
  state->onAppCmd = OnexApp::handleAppCommand;
  state->onInputEvent = OnexApp::handleAppInput;
  androidApp = state;
  onexapp->run();
  delete(onexapp);
}

