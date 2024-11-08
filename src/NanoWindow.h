#ifndef NANOWINDOW_H_
#define NANOWINDOW_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "NanoError.h"

struct NanoWindow {
  ERR Init(const int32_t width, const int32_t height, bool forceReInit);
  ERR Init();
  void PollEvents();
  bool ShouldWindowClose();
  ERR CleanUp();
  GLFWwindow* getGLFWwindow();

  bool m_isInit = false;
};

#endif // NANOWINDOW_H_
