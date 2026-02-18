#define SGL_IMPLEMENTATION
#include "../sgl.h"
#include <math.h>

int main(int argc, char** argv) {
	sgl_InitWindow(800, 600, "SGL: 3D Cubes & 2D HUD");

	// --- Setup 3D Camera ---
	// Position: (0, 10, 10) -> High up and back
	// Target: (0, 0, 0) -> Looking at the center
	// Up: (0, 1, 0) -> Y is up
	SGL_Camera3D cam3d = {
		.position = { 0.0f, 5.0f, 100.0f },
		.target = { 0.0f, 0.0f, 0.0f },
		.up = { 0.0f, 1.0f, 0.0f },
		.fovy = 45.0f,
		.projection = 0 // Perspective
	};

	// --- Setup 2D Camera (For UI) ---
	SGL_Camera cam2d;
	sgl_CameraInit(&cam2d, 0.0f, 0.0f, 1.0f);

	while (!sgl_WindowShouldClose()) {
		float time = SDL_GetTicks() / 1000.0f;

		// Simple Orbital Camera Animation
		float radius = 10.0f;
		cam3d.position.x = sinf(time) * radius;
		cam3d.position.z = cosf(time) * radius;

		sgl_BeginDrawing();

		// --- 3D PASS ---
		sgl_BeginMode3D(&cam3d);

		sgl_DrawCube((Vec3){ 0, 0, 0 }, 2.0f, (SGL_COLOR){ 255, 0, 0, 255 });
		sgl_DrawCube((Vec3){ 5, 0, 0 }, 2.0f, (SGL_COLOR){ 0, 0, 255, 255 });

		sgl_DrawCube((Vec3){ 0, -5.0f, 0 }, 50.0f, (SGL_COLOR){ 40, 40, 40, 255 });

		sgl_EndMode3D();

		// --- 2D PASS (UI/Overlay) ---

		sgl_BeginMode2D(&cam2d);
		// Draw a "Health Bar" or UI element in top-left
		sgl_DrawRectangle(20, 20, 200, 30, (SGL_COLOR){ 100, 100, 100, 200 });
		sgl_DrawRectangle(25, 25, 150, 20, (SGL_COLOR){ 0, 255, 0, 255 }); // Green bar

		// Draw a circle following the mouse (Screen Space)
		// Note: You'd need SDL_GetMouseState for real mouse pos, using dummy value here
		sgl_DrawCircle(750, 50, 20, (SGL_COLOR){ 255, 255, 0, 255 });

		sgl_EndMode2D();

		// ------------------------------------------------
		// END FRAME
		// ------------------------------------------------
		sgl_EndDrawing();
	}

	sgl_Shutdown();
	return 0;
}
