
/**
 *
 * @file video.cpp
 *
 * Part of the OpenJazz project
 *
 * @par History:
 * - 23rd August 2005: Created main.c
 * - 22nd July 2008: Created util.c from parts of main.c
 * - 3rd February 2009: Renamed util.c to util.cpp
 * - 13th July 2009: Created graphics.cpp from parts of util.cpp
 * - 26th July 2009: Renamed graphics.cpp to video.cpp
 *
 * @par Licence:
 * Copyright (c) 2005-2017 Alister Thomson
 *
 * OpenJazz is distributed under the terms of
 * the GNU General Public License, version 2.0
 *
 * @par Description:
 * Contains graphics utility functions.
 *
 */


#include "paletteeffects.h"
#include "video.h"

#include "util.h"

#include <string.h>


/**
 * Creates a surface.
 *
 * @param pixels Pixel data to copy into the surface. Can be NULL.
 * @param width Width of the pixel data and of the surface to be created
 * @param height Height of the pixel data and of the surface to be created
 *
 * @return The completed surface
 */
SDL_Surface* createSurface (unsigned char * pixels, int width, int height) {

	SDL_Surface *ret;
	int y;

	// Create the surface
	ret = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);

	// Set the surface's palette
	video.restoreSurfacePalette(ret);

	if (pixels) {

		// Upload pixel data to the surface
		if (SDL_MUSTLOCK(ret)) SDL_LockSurface(ret);

		for (y = 0; y < height; y++)
			memcpy(((unsigned char *)(ret->pixels)) + (ret->pitch * y),
				pixels + (width * y), width);

		if (SDL_MUSTLOCK(ret)) SDL_UnlockSurface(ret);

	}

	return ret;

}


/**
 * Create the video output object.
 */
Video::Video () {

	int count;

	screen = NULL;

	// Generate the logical palette
	for (count = 0; count < 256; count++)
		logicalPalette[count].r = logicalPalette[count].g =
 			logicalPalette[count].b = count;

	currentPalette = logicalPalette;

	return;

}


/**
 * Initialise video output.
 *
 * @param width Width of the window or screen
 * @param height Height of the window or screen
 * @param startFullscreen Whether or not to start in full-screen mode
 *
 * @return Success
 */
bool Video::init (int width, int height, bool startFullscreen) {

	fullscreen = startFullscreen;

	Uint32 windowFlags = SDL_WINDOW_OPENGL;

	if (fullscreen) {
		windowFlags |= SDL_WINDOW_FULLSCREEN;
		SDL_ShowCursor(SDL_DISABLE);
	}
	else {
		windowFlags |= SDL_WINDOW_RESIZABLE;
	}

	int windowWidth = fullscreen ? 0 : width;
	int windowHeight = fullscreen ? 0 : height;

	window = SDL_CreateWindow("OpenJazz",
														SDL_WINDOWPOS_UNDEFINED,
														SDL_WINDOWPOS_UNDEFINED,
														windowWidth, windowHeight,
														windowFlags);

	renderer = SDL_CreateRenderer(window, -1, 0);

	if (!reset(width, height)) {

		logError("Could not set video mode", SDL_GetError());

		return false;

	}

	return true;

}


/**
 * Sets the size of the video window or the resolution of the screen.
 *
 * @param width New width of the window or screen
 * @param height New height of the window or screen
 *
 * @return Success
 */
bool Video::reset (int width, int height) {

	screenW = width;
	screenH = height;

	if (canvas != screen) SDL_FreeSurface(canvas);

	screen = SDL_CreateRGBSurface(0, width, height, 32,
                                        0x00FF0000,
                                        0x0000FF00,
                                        0x000000FF,
                                        0xFF000000);

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	
// 	screen = SDL_SetVideoMode(screenW, screenH, 8, fullscreen? FULLSCREEN_FLAGS: WINDOWED_FLAGS);

	if (!screen) return false;

	canvasW = screenW;
	canvasH = screenH;
	canvas = createSurface(NULL, canvasW, canvasH);


	/* A real 8-bit display is quite likely if the user has the right video
	card, the right video drivers, the right version of DirectX/whatever, and
	the right version of SDL. In other words, it's not likely enough. If a real
	palette is assumed when
	a) there really is a real palette, there will be an extremely small speed
		gain.
	b) the palette is emulated, there will be a HUGE speed loss.
	Therefore, assume the palette is emulated. */
	/// @todo Find a better way to determine if palette is emulated
	fakePalette = true;

	return true;

}


/**
 * Sets the display palette.
 *
 * @param palette The new palette
 */
void Video::setPalette (SDL_Color *palette) {

	// Make palette changes invisible until the next draw. Hopefully.
	clearScreen(SDL_MapRGB(canvas->format, 0, 0, 0));
	flip(0);

	SDL_SetPaletteColors(canvas->format->palette, palette, 0, 256);
	currentPalette = palette;

	return;

}


/**
 * Returns the current display palette.
 *
 * @return The current display palette
 */
SDL_Color* Video::getPalette () {

	return currentPalette;

}


/**
 * Sets some colours of the display palette.
 *
 * @param palette The palette containing the new colours
 * @param first The index of the first colour in both the display palette and the specified palette
 * @param amount The number of colours
 */
void Video::changePalette (SDL_Color *palette, unsigned char first, unsigned int amount) {

	SDL_SetPaletteColors(canvas->format->palette, palette, first, amount);

	return;

}


/**
 * Restores a surface's palette.
 *
 * @param surface Surface with a modified palette
 */
void Video::restoreSurfacePalette (SDL_Surface* surface) {

	SDL_SetPaletteColors(surface->format->palette, logicalPalette, 0, 256);

	return;

}


/**
 * Returns the maximum possible screen width.
 *
 * @return The maximum width
 */
int Video::getMaxWidth () {

	return maxW;

}


/**
 * Returns the maximum possible screen height.
 *
 * @return The maximum height
 */
int Video::getMaxHeight () {

	return maxH;

}


/**
 * Returns the current width of the window or screen.
 *
 * @return The width
 */
int Video::getWidth () {

	return screenW;

}


/**
 * Returns the current height of the window or screen.
 *
 * @return The height
 */
int Video::getHeight () {

	return screenH;

}


#ifndef FULLSCREEN_ONLY
/**
 * Determines whether or not full-screen mode is being used.
 *
 * @return Whether or not full-screen mode is being used
 */
bool Video::isFullscreen () {

	return fullscreen;

}
#endif


/**
 * Update video based on a system event.
 *
 * @param event The system event. Events not affecting video will be ignored
 */
void Video::update (SDL_Event *event) {

#if !defined(FULLSCREEN_ONLY)
	switch (event->type) {

	#ifndef FULLSCREEN_ONLY
		case SDL_KEYDOWN:

			// If Alt + Enter has been pressed, switch between windowed and full-screen mode.
			if ((event->key.keysym.sym == SDLK_RETURN) &&
				(event->key.keysym.mod & KMOD_ALT)) {

				fullscreen = !fullscreen;

				if (fullscreen) SDL_ShowCursor(SDL_DISABLE);

				reset(screenW, screenH);

				if (!fullscreen) SDL_ShowCursor(SDL_ENABLE);

			}

			break;
    #endif

	}
#endif

	return;

}


/**
 * Draw graphics to screen.
 *
 * @param mspf Ticks per frame
 * @param paletteEffects Palette effects to use
 * @param effectsStopped Whether the effects should be applied without advancing
 */
void Video::flip (int mspf, PaletteEffect* paletteEffects, bool effectsStopped) {

	SDL_Color shownPalette[256];

	// Apply palette effects
	if (paletteEffects) {

		/* If the palette is being emulated, compile all palette changes and
		apply them all at once.
		If the palette is being used directly, apply all palette effects
		directly. */

		if (fakePalette) {

			memcpy(shownPalette, currentPalette, sizeof(SDL_Color) * 256);

			paletteEffects->apply(shownPalette, false, mspf, effectsStopped);

			SDL_SetPaletteColors(canvas->format->palette, shownPalette, 0, 256);

		} else {

			paletteEffects->apply(shownPalette, true, mspf, effectsStopped);

		}

	}

	// Show what has been drawn
	SDL_BlitSurface(canvas, NULL, screen, NULL);
	SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);// screen->w * sizeof (Uint32));

	SDL_RenderClear(renderer);

	int windowWidth, windowHeight;
	SDL_GetRendererOutputSize(renderer, &windowWidth, &windowHeight);
	float textureAspectRatio = 640.0f / 480.0f;// (float)video_state.w / video_state.h;
	float screenAspectRatio = (float)windowWidth / windowHeight;

	SDL_Rect dstRect;

	if (textureAspectRatio > screenAspectRatio) {
			dstRect.x = 0;
			dstRect.w = windowWidth;
			dstRect.h = dstRect.w / textureAspectRatio;
			dstRect.y = (windowHeight - dstRect.h) >> 1;
	}
	else {
			dstRect.y = 0;
			dstRect.h = windowHeight;
			dstRect.w = dstRect.h * textureAspectRatio;
			dstRect.x = (windowWidth - dstRect.w) >> 1;
	}

	// video_state.viewportX = dstRect.x;
	// video_state.viewportY = dstRect.y;
	// video_state.viewportWidth = dstRect.w;
	// video_state.viewportHeight = dstRect.h;

	SDL_RenderCopy(renderer, texture, NULL, &dstRect);
	SDL_RenderPresent(renderer);

	return;

}


/**
 * Fill the screen with a colour.
 *
 * @param index Index of the colour to use
 */
void Video::clearScreen (int index) {

#if defined(CAANOO) || defined(WIZ) || defined(GP2X) || defined(GAMESHELL)
	// always 240 lines cleared to black
	memset(video.screen->pixels, index, 320*240);
#else
	SDL_FillRect(canvas, NULL, index);
#endif

	return;

}


/**
 * Fill a specified rectangle of the screen with a colour.
 *
 * @param x X-coordinate of the left side of the rectangle
 * @param y Y-coordinate of the top of the rectangle
 * @param width Width of the rectangle
 * @param height Height of the rectangle
 * @param index Index of the colour to use
 */
void drawRect (int x, int y, int width, int height, int index) {

	SDL_Rect dst;

	dst.x = x;
	dst.y = y;
	dst.w = width;
	dst.h = height;

	SDL_FillRect(canvas, &dst, index);

	return;

}

