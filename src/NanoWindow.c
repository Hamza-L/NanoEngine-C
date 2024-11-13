#include "NanoWindow.h"
#include "NanoConfig.h"

#include <stdlib.h>
#include <stdio.h>

static NanoWindow* s_window;

ERR CleanUp(){
    if(!s_window || !s_window->m_isInit){
        return NOT_INITIALIZED;
    }
    glfwDestroyWindow(s_window->window);
    glfwTerminate();
    free(s_window);
    s_window = NULL;
    return OK;
}

NanoWindow* InitWindow(const int32_t width, const int32_t height, bool forceReInit){
    if(s_window && s_window->m_isInit && !forceReInit){
        return s_window;
    }

    CleanUp();
    s_window = (NanoWindow*)malloc(sizeof(NanoWindow));

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    s_window->window = glfwCreateWindow(width, height, APP_NAME, NULL, NULL);
    if(s_window->window){
        s_window->m_isInit = true;
    } else {
        s_window->m_isInit = false;
        return NULL;
    }
    return s_window;
}

void PollEvents(){
    if(s_window && s_window->m_isInit){
        glfwPollEvents();
    } else {
        fprintf(stderr, "Polling GLFW events of unintialized glfwwindow");
    }
}

bool ShouldWindowClose(){
    if(!s_window || !s_window->m_isInit){
        return true;
    }
    return glfwWindowShouldClose(s_window->window);
}

GLFWwindow* getGLFWwindow(){
    return s_window && s_window->m_isInit ? s_window->window : NULL;
}

