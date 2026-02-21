#include <stdint.h>
#define SGL_IMPLEMENTATION
#include "../sgl.h"

int main() {
	sgl_InitWindow(800, 600, "SGL 3D Camera");

	// Initialize 3D Camera
	SGL_Camera3D cam = { 0 };
	cam.position = (Vec3){ 0.0f, 2.0f, 5.0f };
	cam.target = (Vec3){ 0.0f, 0.0f, 0.0f };
	cam.up = (Vec3){ 0.0f, 1.0f, 0.0f };
	cam.fovy = 45.0f;
	cam.projection = 0; // Perspective
	cam.speed = 5.0f;

	// IMPORTANT: Lock/Hide mouse for 3D controls
	sgl_SetMouseLock(true);

	uint64_t NOW = sgl_GetPerfCount();
	uint64_t LAST = 0;
	f32 deltaTime = 0.0f;

	SGL_Texture* crateTex = sgl_LoadTexture("Tex.bmp");

	while (!sgl_WindowShouldClose()) {
		LAST = NOW;

		NOW = sgl_GetPerfCount();
		deltaTime = (f32)((NOW - LAST) / (double)sgl_GetPerfFreq());
		// Update Camera
		sgl_Camera3DUpdate(&cam, CAMERA_ORBITAL, deltaTime);

		sgl_BeginDrawing();
		sgl_BeginMode3D(&cam);

		sgl_DrawCube((Vec3){ 0, 0, 0 }, 1.0f, crateTex, (SGL_COLOR){ 255, 255, 255, 255 });

		// Draw a floor grid for reference
		for (int x = -5; x <= 5; x++) {
			for (int z = -5; z <= 5; z++) {
				sgl_DrawCube(
					(Vec3){ (f32)x, -1.0f, (f32)z },
					0.1f,
					NULL,
					(SGL_COLOR){ 100, 100, 100, 255 }
				);
			}
		}

		sgl_EndMode3D();
		sgl_EndDrawing();
	}

	sgl_Shutdown();
	return 0;
}
