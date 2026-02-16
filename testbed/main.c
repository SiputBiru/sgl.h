#define SGL_IMPLEMENTATION
#include "../sgl.h"

int main(int argc, char** argv) {
	sgl_InitWindow(800, 600, "SGL: Raylib Inspired graphics library");

	SGL_Camera cam;
	sgl_CameraInit(&cam, 0.0f, 0.0f, 1.0f);

	while (!sgl_WindowShouldClose()) {

		sgl_CameraUpdate(&cam);

		sgl_Begin();
		sgl_SetCamera(&cam);

		sgl_DrawRectangle(10, 10, 100, 100, (SGL_COLOR){ 0, 255, 0, 255 });
		sgl_DrawTriangle(400, 300, 50, (SGL_COLOR){ 255, 0, 0, 255 });

		float time = SDL_GetTicks() / 1000.0f;
		sgl_DrawRectangle(600 + SDL_sinf(time) * 100, 300, 50, 50, (SGL_COLOR){ 0, 0, 255, 255 });

		sgl_DrawCircle(800 / 2, 600 / 2, 50, (SGL_COLOR){ 0, 0, 255, 255 });

		sgl_End();
	}

	sgl_Shutdown();
	return 0;
}
