#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>
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

	bool isMouseLocked = true;
	bool tabWasDown = false;

	// IMPORTANT: Lock/Hide mouse for 3D controls
	sgl_SetMouseLock(isMouseLocked);

	uint64_t NOW = sgl_GetPerfCount();
	uint64_t LAST = 0;
	f32 deltaTime = 0.0f;

	SGL_Texture* crateTex = sgl_LoadTexture("Tex.bmp");

	while (!sgl_WindowShouldClose()) {
		LAST = NOW;

		NOW = sgl_GetPerfCount();
		deltaTime = (f32)((NOW - LAST) / (double)sgl_GetPerfFreq());
		// Update Camera

		const bool* keys = SDL_GetKeyboardState(NULL);
		bool tabIsDown = keys[SDL_SCANCODE_TAB];

		if (tabIsDown && !tabWasDown) {
			isMouseLocked = !isMouseLocked;
			sgl_SetMouseLock(isMouseLocked);
		}
		tabWasDown = tabIsDown;

		sgl_BeginDrawing();
		sgl_BeginMode3D(&cam);

		SGL_Frustum frustum;
		sgl_ExtractFrustum(sgl_GetCurrentMatrix(), &frustum);

		int cubesDrawn = 0;

		if (isMouseLocked) {
			sgl_Camera3DUpdate(&cam, CAMERA_FREE, deltaTime);
		}

		for (int x = -50; x < 50; x++) {
			for (int z = -50; z < 50; z++) {

				Vec3 cubePos = { (f32)x * 2.0f, 0.0f, (f32)z * 2.0f };

				if (sgl_FrustumContainsSphere(&frustum, cubePos, 1.5f)) {
					sgl_DrawCube(cubePos, 1.0f, NULL, (SGL_COLOR){ 0, 255, 0, 255 });
					cubesDrawn++;
				}
			}
		}

		sgl_EndMode3D();
		sgl_EndDrawing();

		SGL_Log("Cubes Drawn: %d / 100000", cubesDrawn);
	}

	sgl_Shutdown();
	return 0;
}
