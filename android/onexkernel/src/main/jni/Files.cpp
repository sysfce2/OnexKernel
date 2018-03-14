#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>

extern android_app* androidApp;

char* getExternalStorageDirectory()
{
  JNIEnv* env; androidApp->activity->vm->AttachCurrentThread(&env, 0);

  jclass osEnvClass = env->FindClass("android/os/Environment");
  jmethodID getExternalStorageDirectoryMethod = env->GetStaticMethodID(osEnvClass, "getExternalStorageDirectory", "()Ljava/io/File;");
  jobject extStorage = env->CallStaticObjectMethod(osEnvClass, getExternalStorageDirectoryMethod);

  jclass extStorageClass = env->GetObjectClass(extStorage);
  jmethodID getAbsolutePathMethod = env->GetMethodID(extStorageClass, "getAbsolutePath", "()Ljava/lang/String;");
  jstring stringPath = (jstring)env->CallObjectMethod(extStorage, getAbsolutePathMethod);

  char* dir=(char*)(env->GetStringUTFChars(stringPath, NULL));

  androidApp->activity->vm->DetachCurrentThread();

  return dir;
}

