#ifndef NANOWINDOW_H_
#define NANOWINDOW_H_

#include <stdint.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "NanoError.h"

#include <stdbool.h>

struct NanoWindow {
  GLFWwindow* window;
  int32_t width;
  int32_t height;
  bool m_isInit;
};
typedef struct NanoWindow NanoWindow;

NanoWindow* InitWindow(const int32_t width, const int32_t height, bool forceReInit);
ERR CleanUp();
void PollEvents();
bool ShouldWindowClose();
GLFWwindow* getGLFWwindow();


#endif // NANOWINDOW_H_
