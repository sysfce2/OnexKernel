#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>

#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  "OnexApp", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN,  "OnexApp", __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "OnexApp", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "OnexApp", __VA_ARGS__))

extern "C" {
#include <onf.h>
#include <assert.h>
extern void run_properties_tests();
extern void run_onf_tests();
}

android_app* androidApp;

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
      LOGD("APP_CMD_SAVE_STATE");
      break;
    case APP_CMD_INIT_WINDOW:
      LOGD("APP_CMD_INIT_WINDOW");
      if (androidApp->window != NULL)
      {
      }
      else
      {
        LOGE("No window assigned!");
      }
      break;
    case APP_CMD_LOST_FOCUS:
      LOGD("APP_CMD_LOST_FOCUS");
      break;
    case APP_CMD_GAINED_FOCUS:
      LOGD("APP_CMD_GAINED_FOCUS");
      break;
    case APP_CMD_TERM_WINDOW:
      LOGD("APP_CMD_TERM_WINDOW");
      break;
    }
  }

  static int32_t handleAppInput(struct android_app* app, AInputEvent* event)
  {
    OnexApp* onexapp = reinterpret_cast<OnexApp*>(app->userData);
    return 0;
  }

  void run()
  {
    LOGD("---------------OnexKernel tests----------------------\n");
    run_properties_tests();
    run_onf_tests();
    int failures=onex_assert_summary();
    LOGD("---------------%d failures---------------------------\n", failures);
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

