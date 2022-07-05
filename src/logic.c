#include "logic.h"
#include "sys.h"

Texture *tx;

void L_OnCreate() {
	tx = ren->load_texture("gfx/tiles.png");
}

void L_RenderFrame() {
	// ---------------- //
	// PRE-RENDER STUFF //
	// ---------------- //
	
	if (Sys_IsKeyPressed(KEY_F1)) {
		KillWindow();
		sys.fullscreen = !sys.fullscreen;

		if (!Sys_CreateWindow())
		return 0;
	}

	if (Sys_IsKeyPressed(KEY_ESC))
		sys.done_running = true;

    // ---------------- //
	// ACTUAL RENDERING //
	// ---------------- //

	ren->fill_rect(0.f, 0.f, sys.width / 2.f, sys.height / 2.f, Col32(255, 0, 0, 255));
    ren->fill_rect(sys.width / 2.f, sys.height / 2.f, sys.width / 2.f, sys.height / 2.f, Col32(0, 255, 0, 255));
	ren->draw_texture(10.f, 10.f, tx->width, tx->height, tx, Col32(255, 255, 255, 127));
}

void L_OnShutdown() {
	ren->free_texture(&tx);
}
