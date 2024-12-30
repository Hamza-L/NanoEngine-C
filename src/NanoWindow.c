#include "NanoWindow.h"
#include "NanoInput.h"
#include "GLFW/glfw3.h"
#include "NanoConfig.h"
#include "NanoEngine.h"

#include <stdlib.h>
#include <stdio.h>

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    NanoEngine* nanoEngine = glfwGetWindowUserPointer(window);
    nanoEngine->m_Window.framebufferResized = true;

    if(nanoEngine->framebufferResize_callback)
        nanoEngine->framebufferResize_callback(nanoEngine, &nanoEngine->m_Renderer.m_pNanoContext->m_frameData);

    PreDrawFrame(&nanoEngine->m_Renderer, &nanoEngine->m_Window);
    DrawFrame(&nanoEngine->m_Renderer, &nanoEngine->m_Window);
}

static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    NanoEngine* nanoEngine = glfwGetWindowUserPointer(window);

    if(nanoEngine->cursorPos_callback)
        nanoEngine->cursorPos_callback(nanoEngine, &nanoEngine->m_Renderer.m_pNanoContext->m_frameData);
}

void drop_callback(GLFWwindow* window, int count, const char** paths) {
    NanoEngine* nanoEngine = glfwGetWindowUserPointer(window);

    for (int i = 0; i < count; i++){
        SetDroppedInFile(paths[i]);
    }

    if(nanoEngine->fileDrop_callback)
        nanoEngine->fileDrop_callback(nanoEngine, &nanoEngine->m_Renderer.m_pNanoContext->m_frameData);

        /* handle_dropped_file(paths[i]); */
}

void cursor_enter_callback(GLFWwindow* window, int entered)
{

    if (entered)
    {
        // The cursor entered the content area of the window
        LOG_MSG(stderr, "entered window\n");
    }
    else
    {
        // The cursor left the content area of the window
    }
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
    glfwSetFramebufferSizeCallback(nanoWindow->_window, framebufferResizeCallback);
    glfwSetKeyCallback(nanoWindow->_window,key_callback);
    glfwSetMouseButtonCallback(nanoWindow->_window, mouse_callback);
    glfwSetCursorPosCallback(nanoWindow->_window, cursorPositionCallback);
    glfwSetScrollCallback(nanoWindow->_window, scroll_callback);
    glfwSetDropCallback(nanoWindow->_window, drop_callback);
    glfwSetCursorEnterCallback(nanoWindow->_window, cursor_enter_callback);

    InitNanoInput();
    return nanoWindow;
}

void PollEvents(NanoWindow* window){
    if(!window->m_isInit){
        LOG_MSG(stderr, "Polling GLFW events of unintialized glfwwindow");
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

