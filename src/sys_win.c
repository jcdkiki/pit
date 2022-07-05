#include "sys.h"

#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>

#include "lodepng.h"
#include "logic.h"

static char CLASS_NAME[] = "pit_win";
const char WIN_TITLE[] = "pit";

HWND        hWnd = NULL; // window
HINSTANCE   hInstance;   // app instance

float old_time;

Renderer *ren;

SysData sys = {
	.done_running = false,
    .width = 640, .height = 480,
    .bits = 32,
    .fullscreen = false,
    .active = true
};

// ---------------------- //
// Legacy OpenGL Renderer //
// ---------------------- //

HDC         hDC = NULL;  // GDI device ctx
HGLRC       hRC = NULL;  // GL rendering ctx

typedef struct {
	int width, height;
	unsigned int id;
} GLTexture;

Texture *lgl_ren_load_texture(char *filename) {
	GLTexture *tx = malloc(sizeof(GLTexture));
	unsigned char *img;

	int error = lodepng_decode32_file(&img, &tx->width, &tx->height, filename);
	if (error) Sys_FatalError("lgl_ren_load_texture -> %s", lodepng_error_text(error));

	glGenTextures(1, &tx->id);
	glBindTexture(GL_TEXTURE_2D, tx->id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tx->width, tx->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
	//glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);
	free(img);
	return (Texture*)tx;
}

void lgl_ren_free_texture(Texture **tx) {
	GLTexture *gtx = (GLTexture*)*tx;
	glDeleteTextures(1, &gtx->id);
	free(*tx);
}

bool lgl_ren_attach() {
    GLuint PixelFormat;
    PIXELFORMATDESCRIPTOR pfd =  {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
        sys.bits,
        0, 0, 0, 0, 0, 0,                           // Color Bits Ignored
        0,                                          // No Alpha Buffer
        0,                                          // Shift Bit Ignored
        0,                                          // No Accumulation Buffer
        0, 0, 0, 0,                                 // Accumulation Bits Ignored
        16,                                         // 16Bit Z-Buffer (Depth Buffer)  
        0,                                          // No Stencil Buffer
        0,                                          // No Auxiliary Buffer
        PFD_MAIN_PLANE,                             // Main Drawing Layer
        0,                                          // Reserved
        0, 0, 0                                     // Layer Masks Ignored
    };

    hDC = GetDC(hWnd);
    if (!hDC) goto failed;
    
    PixelFormat = ChoosePixelFormat(hDC, &pfd);
    if (!PixelFormat) goto failed;
    if (!SetPixelFormat(hDC, PixelFormat, &pfd)) goto failed;

    hRC = wglCreateContext(hDC);
    if (!hRC) goto failed;
    if (!wglMakeCurrent(hDC, hRC)) goto failed;
    
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
failed:
    return false;
}

bool lgl_ren_destroy() {
    if (hRC) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hRC);
        hRC = NULL;
    }

    if (hDC) {
    	ReleaseDC(hWnd, hDC);
    	hDC = NULL;
   	}
}

bool lgl_ren_onresize() {
    if (sys.height == 0)
        sys.height = 1;

    glViewport(0, 0, sys.width, sys.height);

    glMatrixMode(GL_PROJECTION); // Select projection matrix
    glLoadIdentity(); // Reset it
    gluOrtho2D(0, sys.width, sys.height, 0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void lgl_ren_fill_rect(float x, float y, float w, float h, col32 color) {
    glBegin(GL_QUADS);

    glColor4ub(color.r, color.g, color.b, color.a);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);

    glEnd();
}

void lgl_ren_draw_texture(float x, float y, float w, float h, Texture *tx, col32 color) {
	glBindTexture(GL_TEXTURE_2D, ((GLTexture*)tx)->id);
	glBegin(GL_QUADS);

    glColor4ub(color.r, color.g, color.b, color.a);

    glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(x + w, y);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(x + w, y + h);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + h);

    glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
}

Renderer lgl_ren = {
    .attach = lgl_ren_attach,
    .destroy = lgl_ren_destroy,
    .onresize = lgl_ren_onresize,

    .fill_rect = lgl_ren_fill_rect,
    .load_texture = lgl_ren_load_texture,
    .free_texture = lgl_ren_free_texture,
    .draw_texture = lgl_ren_draw_texture
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void Sys_FatalError(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    printf("Fatal error - ");
    vprintf(fmt, args);
    putc('\n', stdout);

    va_end(args);

    KillWindow();
    exit(-1);
}

void KillWindow() {
	L_OnShutdown();
	
    if (sys.fullscreen) {
        ChangeDisplaySettings(NULL, 0); // switch to desktop
        ShowCursor(true);
    }

    ren->destroy();

    if (hWnd) DestroyWindow(hWnd);
    UnregisterClass(CLASS_NAME, hInstance);
}

bool Sys_CreateWindow() {
    WNDCLASS    wc;
    DWORD       dwExStyle;
    DWORD       dwStyle;
    RECT        WindowRect;

    WindowRect.left = 0;
    WindowRect.right = sys.width;
    WindowRect.top = 0;
    WindowRect.bottom = sys.height;

    hInstance           = GetModuleHandle(NULL);
    wc.lpszClassName    = CLASS_NAME;
    wc.style            = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc      = (WNDPROC)WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = NULL;
    wc.lpszMenuName     = NULL;

    if (!RegisterClass(&wc)) Sys_FatalError("RegisterClass failed");

    if (sys.fullscreen) {
        DEVMODE dmScreenSettings;
        memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
        dmScreenSettings.dmSize = sizeof(dmScreenSettings);
        dmScreenSettings.dmPelsWidth = sys.width;
        dmScreenSettings.dmPelsHeight = sys.height;
        dmScreenSettings.dmBitsPerPel = sys.bits;
        dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
            if (MessageBox(NULL, "The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?", "fuck", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
                sys.fullscreen = false;
            else
                return false;
        }
    }

    if (sys.fullscreen) {
        dwExStyle = WS_EX_APPWINDOW;
        dwStyle = WS_POPUP;
        ShowCursor(false);
    }
    else {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dwStyle = WS_OVERLAPPEDWINDOW;
    }

    AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

    hWnd = CreateWindowEx(
        dwExStyle, CLASS_NAME, WIN_TITLE,
        dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0,
        WindowRect.right - WindowRect.left,
        WindowRect.bottom - WindowRect.top,
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) {
        KillWindow();
        MessageBox(NULL, "Window Creation Error.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);

    if (!ren->attach()) Sys_FatalError("Failed to attach renderer.");
    ren->onresize();

	L_OnCreate();
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_ACTIVATE:
            sys.active = !(HIWORD(wParam));
            return 0;

        case WM_SYSCOMMAND: {
            switch (wParam) {
                case SC_SCREENSAVE: // trying to open screensaver
                case SC_MONITORPOWER: // trying to enter save power mode?
                return 0; // no dude, i won`t let you do that
            }
            break;
        }

        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;

        case WM_KEYDOWN:
            sys.keystate[wParam] |= KEYSTATE_PRESSED;
            sys.keystate[wParam] |= KEYSTATE_DOWN;
            return 0;

        case WM_KEYUP:
            sys.keystate[wParam] |= KEYSTATE_RELEASED;
            sys.keystate[wParam] &= ~KEYSTATE_DOWN;
            return 0;

        case WM_SIZE:
            sys.width = LOWORD(lParam);
            sys.height = HIWORD(lParam);
            ren->onresize();
            return 0;
    }

    // unhandled message goes here
    return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	(void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
    MSG msg;

    ren = &lgl_ren;

    if (!Sys_CreateWindow()) return 0;
    
    old_time = sys.time = GetTickCount() / 1000.f;

    while(!sys.done_running) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) sys.done_running = true;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        old_time = sys.time;
        sys.time = GetTickCount() / 1000.f;
        if (sys.time - old_time > 0) sys.delta = sys.time - old_time; // why > 0? idk?

        if (sys.active) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			L_RenderFrame();
            SwapBuffers(hDC);
        }

        // reset pressed & released flags
        for (int i = 0; i < 256; i++)
            sys.keystate[i] &= ~(KEYSTATE_PRESSED | KEYSTATE_RELEASED);
    }

    KillWindow();
    return 0;
}
