#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL_image.h>
#include "InputManager.h"

#define VIRTUAL_WIDTH 1920   // Design resolution width
#define VIRTUAL_HEIGHT 1080  // Design resolution height

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool isFullScreen;
    float scaleX;
    float scaleY;
    int windowWidth;
    int windowHeight;
} GameDisplay;

// Initialize the display system
bool initDisplay(GameDisplay* display) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    // Initialize SDL_image
    //if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) != (IMG_INIT_PNG | IMG_INIT_JPG)) {
    //  printf("SDL_image could not initialize! IMG_Error: %s\n", IMG_GetError());
    //  SDL_Quit();
    //  return -1;
    //}    

    // Create window
    display->window = SDL_CreateWindow("SDL Platform Game", 
                                      SDL_WINDOWPOS_UNDEFINED, 
                                      SDL_WINDOWPOS_UNDEFINED, 
                                      VIRTUAL_WIDTH, VIRTUAL_HEIGHT, 
                                      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (display->window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    // Create renderer
    display->renderer = SDL_CreateRenderer(display->window, -1, 
                                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (display->renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    // Initialize display variables
    display->isFullScreen = false;
    display->scaleX = 1.0f;
    display->scaleY = 1.0f;
    display->windowWidth = VIRTUAL_WIDTH;
    display->windowHeight = VIRTUAL_HEIGHT;
    
    // Set up logical size for resolution independence
    SDL_RenderSetLogicalSize(display->renderer, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    
    return true;
}

// Toggle fullscreen mode
void toggleFullScreen(GameDisplay* display) {
    display->isFullScreen = !display->isFullScreen;
    
    if (display->isFullScreen) {
        // Enter fullscreen mode
        SDL_SetWindowFullscreen(display->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        // Exit fullscreen mode
        SDL_SetWindowFullscreen(display->window, 0);
        
        // Restore original window size
        SDL_SetWindowSize(display->window, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    }
    
    // Update window size information
    SDL_GetWindowSize(display->window, &display->windowWidth, &display->windowHeight);
    
    // Calculate new scale factors
    display->scaleX = (float)display->windowWidth / VIRTUAL_WIDTH;
    display->scaleY = (float)display->windowHeight / VIRTUAL_HEIGHT;
}

// Convert screen coordinates to game world coordinates
void screenToGameCoordinates(GameDisplay* display, int screenX, int screenY, int* gameX, int* gameY) {
    *gameX = (int)((float)screenX / display->scaleX);
    *gameY = (int)((float)screenY / display->scaleY);
}

// Clean up display resources
void closeDisplay(GameDisplay* display) {
    SDL_DestroyRenderer(display->renderer);
    SDL_DestroyWindow(display->window);
    //    SDL_DestroyTexture(texture);
    //SDL_FreeSurface(image);
    SDL_Quit();

}

int main(int argc, char* args[]) {
  InputManager inputManager;
  inputManager.initialize();
  
    GameDisplay display;
    
    // Initialize display
    if (!initDisplay(&display)) {
        printf("Failed to initialize display!\n");
        return 1;
    }
    
    // Player position and size
    SDL_Rect playerRect = {VIRTUAL_WIDTH/2 - 25, VIRTUAL_HEIGHT/2 - 25, 50, 50};
    SDL_Rect mapTile = {VIRTUAL_WIDTH/2 - 25, VIRTUAL_HEIGHT/2 - 25, 50, 50};
    SDL_Surface* floorTile00 = SDL_LoadBMP("images/floor_tile_01.bmp");
    if (floorTile00 == NULL) {
        printf("Unable to load image! IMG_Error: %s\n", IMG_GetError());
	SDL_Quit();
	return -1;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(display.renderer, floorTile00);

    // Main game loop
    bool quit = false;
    SDL_Event e;    

    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
	    
	    if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_f:  // F key to toggle fullscreen
                    case SDLK_F11: // Or F11
                        toggleFullScreen(&display);
                        break;
                    case SDLK_ESCAPE:
                        quit = true;
                        break;
                    // Add player movement controls
                    case SDLK_LEFT:
                        playerRect.x -= 10;
                        break;
                    case SDLK_RIGHT:
                        playerRect.x += 10;
                        break;
                    case SDLK_UP:
                        playerRect.y -= 10;
                        break;
                    case SDLK_DOWN:
                        playerRect.y += 10;
                        break;
                }
            }	    
            else if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    // Update display parameters on window resize
                    display.windowWidth = e.window.data1;
                    display.windowHeight = e.window.data2;
                    display.scaleX = (float)display.windowWidth / VIRTUAL_WIDTH;
                    display.scaleY = (float)display.windowHeight / VIRTUAL_HEIGHT;
                }
            }
        }
        
        // Clear screen
        SDL_SetRenderDrawColor(display.renderer, 0, 0, 128, 255); // Dark blue background
        SDL_RenderClear(display.renderer);
        
        // Draw a grid to demonstrate resolution independence
        SDL_SetRenderDrawColor(display.renderer, 50, 50, 50, 255);
        for (int x = 0; x < VIRTUAL_WIDTH; x += 50) {
            SDL_RenderDrawLine(display.renderer, x, 0, x, VIRTUAL_HEIGHT);
        }
        for (int y = 0; y < VIRTUAL_HEIGHT; y += 50) {
            SDL_RenderDrawLine(display.renderer, 0, y, VIRTUAL_WIDTH, y);
        }
	
	//initial attempt at creating a floor
        for (int y = 0; y < VIRTUAL_HEIGHT; y += 50) {
	    for(int x = 0; x< VIRTUAL_WIDTH; x +=50) {
	      SDL_SetRenderDrawColor(display.renderer, 0,255,0, 255);
	      if(y>500){
		mapTile.y = y;
	      };
	      mapTile.x = x;
	      //SDL_RenderFillRect(display.renderer, &mapTile);
	      //Display an image
	      SDL_RenderCopy(display.renderer, texture, NULL, &mapTile);
	    }
	}
        // Draw the player
        SDL_SetRenderDrawColor(display.renderer, 255, 100, 100, 255);
        SDL_RenderFillRect(display.renderer, &playerRect);
        
        // Update screen
        SDL_RenderPresent(display.renderer);

	//Set a delay for consisten FPS
	SDL_Delay(1000 / 60);
    }
    
    // Clean up
    closeDisplay(&display);

    return 0;
}
