#include "NanoWindow.h"
#include "NanoConfig.h"

#include <stdlib.h>
#include <stdio.h>

ERR CleanUpWindow(NanoWindow* window){
    if(!window || !window->m_isInit){
        return NOT_INITIALIZED;
    }
    glfwDestroyWindow(window->_window);
    glfwTerminate();
    free(window->_window);
    window = NULL;
    return OK;
}

NanoWindow* InitWindow(NanoWindow* window, const int32_t width, const int32_t height, bool forceReInit){
    if(window && window->m_isInit && !forceReInit){
        return window;
    }

    CleanUpWindow(window);
    window = (NanoWindow*)malloc(sizeof(NanoWindow));

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window->_window = glfwCreateWindow(width, height, APP_NAME, NULL, NULL);
    if(window->_window){
        window->m_isInit = true;
    } else {
        window->m_isInit = false;
        return NULL;
    }
    return window;
}

void PollEvents(NanoWindow window){
    if(window._window && window.m_isInit){
        glfwPollEvents();
    } else {
        fprintf(stderr, "Polling GLFW events of unintialized glfwwindow");
    }
}

bool ShouldWindowClose(NanoWindow window){
    if(window._window || !window.m_isInit){
        return true;
    }
    return glfwWindowShouldClose(window._window);
}

