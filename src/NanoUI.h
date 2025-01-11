#ifndef NANOUI_H_
#define NANOUI_H_

#include "NanoScene.h"

typedef enum UIState UIState;
typedef struct UIElement UIElement;
typedef struct NanoUI NanoUI;

enum UIState{
STATE_DISABLED = 0x00000000,
STATE_ENABLED = 0x00000001,
STATE_FOCUSED = 0x00000010,
STATE_PRESSED = 0x00000100,
};

struct UIElement{
    RenderableNode element;
    UIState state;

    void (*Action)(void* self, void* data);
};

struct NanoUI{
    RenderableScene scene;

    UIElement* elements;
    uint32_t numUIElements;
};


#endif // NANOUI_H_
