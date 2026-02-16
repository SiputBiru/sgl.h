#define SGL_IMPLEMENTATION
#include "../sgl.h"

int main(int argc, char** argv) {
	sgl_InitWindow(800, 600, "SGL Texture Test");

	// Load Texture (Must be a .bmp file)
	SGL_Texture* myTex = sgl_LoadTexture("Tex.bmp");

	if (!myTex) {
		SGL_Error("Could not load texture! Make sure test.bmp exists.");
	}

	SGL_Camera cam;
	sgl_CameraInit(&cam, 0, 0, 1.0f);

	while (!sgl_WindowShouldClose()) {
		sgl_CameraUpdate(&cam);

		sgl_Begin();
		sgl_SetCamera(&cam);

		sgl_DrawRectangle(50, 50, 200, 200, (SGL_COLOR){ 100, 100, 100, 255 });

		if (myTex) {
			// Draw Normal
			sgl_DrawTexture(myTex, 300, 100, 128, 128, (SGL_COLOR){ 255, 255, 255, 255 });

			// Draw Tinted (Red)
			sgl_DrawTexture(myTex, 500, 100, 128, 128, (SGL_COLOR){ 255, 0, 0, 255 });

			// Draw Stretched
			sgl_DrawTexture(myTex, 300, 300, 300, 100, (SGL_COLOR){ 255, 255, 255, 255 });
		}

		sgl_End();
	}

	// Cleanup
	sgl_DestroyTexture(myTex);
	sgl_Shutdown();
	return 0;
}
