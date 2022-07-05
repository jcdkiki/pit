#ifndef PIT_SYS_H
#define PIT_SYS_H

#include "common.h"

#ifdef _WIN32
	#include <Windows.h>
	#define KEY_F1 VK_F1
	#define KEY_F2 VK_F2
	#define KEY_F3 VK_F3
	#define KEY_F4 VK_F4
	#define KEY_F5 VK_F5
	#define KEY_F6 VK_F6
	#define KEY_F7 VK_F7
	#define KEY_F8 VK_F8
	#define KEY_F9 VK_F9
	#define KEY_F10 VK_F10
	#define KEY_F11 VK_F11
	#define KEY_F12 VK_F12
	#define KEY_ESC VK_ESCAPE
#elif
	#error unsupported sorry((
#endif

typedef struct {
	int width, height;
} Texture;

typedef struct {
    char *name;
    bool (*attach)();
    bool (*destroy)();
    
    bool (*onresize)();
    void (*fill_rect)(float x, float y, float w, float h, col32 color);

    Texture *(*load_texture)(char *filename);
    void (*free_texture)(Texture **tx); // it free(*tx) too    
	void (*draw_texture)(float x, float y, float w, float h, Texture *tx, col32 color);
} Renderer;

#define KEYSTATE_DOWN 1
#define KEYSTATE_PRESSED 2
#define KEYSTATE_RELEASED 4

typedef struct {
	bool done_running;
    int width, height;
    int bits;
    bool fullscreen;
    char keystate[256];
    bool active;
    float delta, time;
} SysData;

extern Renderer *ren;
extern SysData sys;

void Sys_FatalError(char *fmt, ...);

static inline bool Sys_IsKeyDown(int key) {
    return sys.keystate[key] & KEYSTATE_DOWN;
}

static inline bool Sys_IsKeyPressed(int key) {
    return sys.keystate[key] & KEYSTATE_PRESSED;
}

static inline bool Sys_IsKeyReleased(int key) {
    return sys.keystate[key] & KEYSTATE_RELEASED;
}

#endif // PIT_SYS_H
