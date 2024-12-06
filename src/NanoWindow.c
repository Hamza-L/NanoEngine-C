#include "NanoWindow.h"
#include "NanoInput.h"
#include "GLFW/glfw3.h"
#include "NanoConfig.h"

#include <stdlib.h>
#include <stdio.h>

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    NanoWindow* nanoWindow = glfwGetWindowUserPointer(window);
    nanoWindow->framebufferResized = true;
}

ERR CleanUpWindow(NanoWindow* nanoWindow){
    if(!nanoWindow || !nanoWindow->m_isInit){
        return NOT_INITIALIZED;
    }
    glfwDestroyWindow(nanoWindow->_window);
    glfwTerminate();
    nanoWindow->m_isInit = false;
    nanoWindow->width = 0;
    nanoWindow->height = 0;
    nanoWindow->_window = NULL;
    return OK;
}

NanoWindow* InitWindow(NanoWindow* nanoWindow, const int32_t width, const int32_t height, bool forceReInit){
    if(nanoWindow && nanoWindow->m_isInit && !forceReInit){
        return nanoWindow;
    }

    CleanUpWindow(nanoWindow);
    nanoWindow->width = width;
    nanoWindow->height = height;
    nanoWindow->m_isInit = false;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    nanoWindow->_window = glfwCreateWindow(width, height, APP_NAME, NULL, NULL);
    if(nanoWindow->_window){
        nanoWindow->m_isInit = true;
    } else {
        nanoWindow->m_isInit = false;
        return NULL;
    }
    glfwSetWindowUserPointer(nanoWindow->_window, nanoWindow);
    glfwSetFramebufferSizeCallback(nanoWindow->_window, framebufferResizeCallback);
    glfwSetKeyCallback(nanoWindow->_window,key_callback);
    glfwSetMouseButtonCallback(nanoWindow->_window, mouse_callback);
    glfwSetScrollCallback(nanoWindow->_window, scroll_callback);
    return nanoWindow;
}

void PollEvents(NanoWindow* window){
    if(!window->m_isInit){
        fprintf(stderr, "Polling GLFW events of unintialized glfwwindow");
    } else {
        glfwPollEvents();
    }
}

bool ShouldWindowClose(NanoWindow* nanoWindow){
    if(!nanoWindow->m_isInit){
        return true;
    }
    return glfwWindowShouldClose(nanoWindow->_window);
}

