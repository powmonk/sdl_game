#include <SDL2/SDL.h>
#include <stdio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

int main(int argc, char* args[]) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    // Create window
    window = SDL_CreateWindow("SDL Platform Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    // Main game loop
    bool quit = false;
    SDL_Event e;
    
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
        
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Draw here
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect rect = {SCREEN_WIDTH/2 - 25, SCREEN_HEIGHT/2 - 25, 50, 50};
        SDL_RenderFillRect(renderer, &rect);
        
        // Update screen
        SDL_RenderPresent(renderer);
    }
    
    // Clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
