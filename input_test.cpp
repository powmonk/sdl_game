#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>

int main(int argc, char* args[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    // Initialize SDL_gamecontroller (optional: for controllers)
    SDL_GameController* controller = NULL;
    if (SDL_NumJoysticks() > 0) {
        controller = SDL_GameControllerOpen(0);
        if (!controller) {
            printf("Warning: Could not open controller! SDL_Error: %s\n", SDL_GetError());
        }
    } else {
        printf("Warning: No controller detected.\n");
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("SDL Event Handling", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    int quit = 0;
    SDL_Event e;
    int xPos = 320, yPos = 240; // Initial position of a sprite (e.g., a rectangle)
    int speed = 5;  // Movement speed

    while (!quit) {
        // Handle events in the event queue
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }

        // Keyboard input - checking multiple key presses simultaneously
        const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

        // Moving with arrow keys (WASD, Arrow keys)
        if (currentKeyStates[SDL_SCANCODE_W]) {
            yPos -= speed;  // Move up
        }
        if (currentKeyStates[SDL_SCANCODE_S]) {
            yPos += speed;  // Move down
        }
        if (currentKeyStates[SDL_SCANCODE_A]) {
            xPos -= speed;  // Move left
        }
        if (currentKeyStates[SDL_SCANCODE_D]) {
            xPos += speed;  // Move right
        }

        // Controller input - Checking controller button presses
        if (controller) {  // Only check if controller exists
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A)) {
                printf("A button on the controller is pressed\n");
            }

            // Controller joystick input (moving a sprite with the controller)
            Sint16 joystickX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
            Sint16 joystickY = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);

            // Normalize joystick values to the range of -100 to 100
            if (joystickX < -3200) {
                xPos -= speed;
            } else if (joystickX > 3200) {
                xPos += speed;
            }

            if (joystickY < -3200) {
                yPos -= speed;
            } else if (joystickY > 3200) {
                yPos += speed;
            }
        }

        // Rendering
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Red color

        // Draw a rectangle (representing the sprite) at xPos, yPos
        SDL_Rect fillRect = { xPos, yPos, 50, 50 };  // 50x50 size
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Red color
        SDL_RenderFillRect(renderer, &fillRect);

        // Update the screen
        SDL_RenderPresent(renderer);

        // Delay to make the loop run at a consistent speed (for 60 FPS)
        SDL_Delay(1000 / 60);
    }

    // Clean up
    if (controller) {
        SDL_GameControllerClose(controller);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
