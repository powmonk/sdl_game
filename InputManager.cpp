#include "InputManager.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>

// Keyboard input implementation
class KeyboardInput : public InputDevice {
private:
    const Uint8* keyboardState;
    Uint8 previousKeyboardState[SDL_NUM_SCANCODES];
    
public:
    KeyboardInput() : keyboardState(nullptr) {
        memset(previousKeyboardState, 0, sizeof(previousKeyboardState));
    }
    
    void update() override {
        int numKeys;
        keyboardState = SDL_GetKeyboardState(&numKeys);
        
        // Store previous state
        if (keyboardState) {
            memcpy(previousKeyboardState, keyboardState, SDL_NUM_SCANCODES);
        }
    }
    
    bool isAvailable() const override {
        return keyboardState != nullptr;
    }
    
    bool isKeyPressed(SDL_Scancode key) const {
        if (!isAvailable()) return false;
        return keyboardState[key] == 1;
    }
    
    bool isKeyJustPressed(SDL_Scancode key) const {
        if (!isAvailable()) return false;
        return keyboardState[key] == 1 && previousKeyboardState[key] == 0;
    }
};

// Mouse input implementation
class MouseInput : public InputDevice {
private:
    int mouseX, mouseY;
    Uint32 mouseState;
    Uint32 previousMouseState;
    
public:
    MouseInput() : mouseX(0), mouseY(0), mouseState(0), previousMouseState(0) {}
    
    void update() override {
        previousMouseState = mouseState;
        mouseState = SDL_GetMouseState(&mouseX, &mouseY);
    }
    
    bool isAvailable() const override {
        // SDL can usually provide mouse state even if no mouse is connected,
        // but this is a simple placeholder for detecting actual mouse input
        return true;
    }
    
    bool isButtonPressed(int button) const {
        if (!isAvailable()) return false;
        return (mouseState & SDL_BUTTON(button)) != 0;
    }
    
    bool isButtonJustPressed(int button) const {
        if (!isAvailable()) return false;
        return (mouseState & SDL_BUTTON(button)) != 0 && 
               (previousMouseState & SDL_BUTTON(button)) == 0;
    }
    
    void getPosition(int& x, int& y) const {
        x = mouseX;
        y = mouseY;
    }
};

// Gamepad input implementation
class GamepadInput : public InputDevice {
private:
    SDL_GameController* controller;
    bool controllerWasConnected;
    
    // Store previous button states
    bool previousButtonStates[SDL_CONTROLLER_BUTTON_MAX];
    bool currentButtonStates[SDL_CONTROLLER_BUTTON_MAX];
    
    // Store axis values
    Sint16 axisValues[SDL_CONTROLLER_AXIS_MAX];
    
public:
    GamepadInput() : controller(nullptr), controllerWasConnected(false) {
        memset(previousButtonStates, 0, sizeof(previousButtonStates));
        memset(currentButtonStates, 0, sizeof(currentButtonStates));
        memset(axisValues, 0, sizeof(axisValues));
    }
    
    ~GamepadInput() {
        if (controller) {
            SDL_GameControllerClose(controller);
        }
    }
    
    void update() override {
        // Copy current to previous state
        memcpy(previousButtonStates, currentButtonStates, sizeof(currentButtonStates));
        
        // Check if we need to initialize the controller
        if (!controller) {
            for (int i = 0; i < SDL_NumJoysticks(); ++i) {
                if (SDL_IsGameController(i)) {
                    controller = SDL_GameControllerOpen(i);
                    if (controller) {
                        break;
                    }
                }
            }
        }
        
        // Update controller state
        if (controller) {
            if (!SDL_GameControllerGetAttached(controller)) {
                SDL_GameControllerClose(controller);
                controller = nullptr;
                controllerWasConnected = false;
            } else {
                controllerWasConnected = true;
                
                // Update button states
                for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
                    SDL_GameControllerButton button = static_cast<SDL_GameControllerButton>(i);
                    currentButtonStates[i] = SDL_GameControllerGetButton(controller, button) == 1;
                }
                
                // Update axis values
                for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i) {
                    SDL_GameControllerAxis axis = static_cast<SDL_GameControllerAxis>(i);
                    axisValues[i] = SDL_GameControllerGetAxis(controller, axis);
                }
            }
        }
    }
    
    bool isAvailable() const override {
        return controller != nullptr;
    }
    
    bool wasAvailable() const {
        return controllerWasConnected;
    }
    
    bool isButtonPressed(SDL_GameControllerButton button) const {
        if (!isAvailable() || button >= SDL_CONTROLLER_BUTTON_MAX) return false;
        return currentButtonStates[button];
    }
    
    bool isButtonJustPressed(SDL_GameControllerButton button) const {
        if (!isAvailable() || button >= SDL_CONTROLLER_BUTTON_MAX) return false;
        return currentButtonStates[button] && !previousButtonStates[button];
    }
    
    float getAxisValue(SDL_GameControllerAxis axis) const {
        if (!isAvailable() || axis >= SDL_CONTROLLER_AXIS_MAX) return 0.0f;
        
        // Convert from Sint16 (-32768 to 32767) to float (-1.0 to 1.0)
        constexpr float normalizeFactor = 1.0f / 32768.0f;
        float normalizedValue = static_cast<float>(axisValues[axis]) * normalizeFactor;
        
        // Apply deadzone
        constexpr float deadzone = 0.15f;
        if (std::abs(normalizedValue) < deadzone) {
            return 0.0f;
        }
        
        // Adjust for deadzone and normalize to -1.0 to 1.0 range
        float sign = (normalizedValue > 0) ? 1.0f : -1.0f;
        return sign * (std::abs(normalizedValue) - deadzone) / (1.0f - deadzone);
    }
};

// Steam Deck specific input handling
class SteamDeckInput : public InputDevice {
private:
    // Steam Deck has built-in gamepad, touchpads, and gyro
    std::unique_ptr<GamepadInput> gamepad;
    bool hasGyro;
    
    // Gyro/acceleration values
    float gyroX, gyroY, gyroZ;
    float accelX, accelY, accelZ;
    
public:
    SteamDeckInput() : gamepad(std::make_unique<GamepadInput>()), hasGyro(false),
                      gyroX(0), gyroY(0), gyroZ(0), accelX(0), accelY(0), accelZ(0) {
        // Try to initialize Steam Input API for best Steam Deck support
        // This is a placeholder - actual implementation would use Steam Input API
        hasGyro = true; // Assume gyro is available on Steam Deck
    }
    
    void update() override {
        // Update gamepad (Steam Deck built-in controls)
        gamepad->update();
        
        // Update gyro and accelerometer data
        // In a real implementation, this would read from Steam Input API
        if (hasGyro) {
            // Placeholder for actual gyro values from Steam Input API
            // These would be populated with real data
        }
    }
    
    bool isAvailable() const override {
        // The Steam Deck controller is always available when running on Steam Deck
        return gamepad->isAvailable();
    }
    
    // Delegate gamepad button/axis methods to the gamepad object
    bool isButtonPressed(SDL_GameControllerButton button) const {
        return gamepad->isButtonPressed(button);
    }
    
    bool isButtonJustPressed(SDL_GameControllerButton button) const {
        return gamepad->isButtonJustPressed(button);
    }
    
    float getAxisValue(SDL_GameControllerAxis axis) const {
        return gamepad->getAxisValue(axis);
    }
    
    // Gyro-specific methods
    bool hasGyroSupport() const {
        return hasGyro;
    }
    
    void getGyroData(float& x, float& y, float& z) const {
        if (!hasGyro) {
            x = y = z = 0.0f;
            return;
        }
        
        x = gyroX;
        y = gyroY;
        z = gyroZ;
    }
    
    void getAccelerometerData(float& x, float& y, float& z) const {
        if (!hasGyro) {
            x = y = z = 0.0f;
            return;
        }
        
        x = accelX;
        y = accelY;
        z = accelZ;
    }
};

// InputManager implementation
InputManager::InputManager() : initialized(false), isSteamDeckHardware(false) {}

InputManager::~InputManager() {
    // These will clean up automatically due to unique_ptr
    if (initialized) {
        SDL_Quit();
    }
}

bool InputManager::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    keyboard = std::make_unique<KeyboardInput>();
    mouse = std::make_unique<MouseInput>();
    gamepad = std::make_unique<GamepadInput>();
    
    // Check if we're running on Steam Deck
    // This is a placeholder - actual implementation would detect Steam Deck
    // You could use SDL_GetPlatform() or check environment variables
    const char* envVar = SDL_getenv("SteamDeck");
    isSteamDeckHardware = (envVar != nullptr && strcmp(envVar, "1") == 0);
    
    if (isSteamDeckHardware) {
        steamDeck = std::make_unique<SteamDeckInput>();
    }
    
    initialized = true;
    return true;
}

void InputManager::update() {
    // Process SDL events to keep the event queue clean
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Handle connection/disconnection events
        if (event.type == SDL_CONTROLLERDEVICEADDED || 
            event.type == SDL_CONTROLLERDEVICEREMOVED) {
            // Force gamepad to re-detect on next update
            if (gamepad) {
                gamepad = std::make_unique<GamepadInput>();
            }
        }
    }
    
    // Update all input devices
    if (keyboard) keyboard->update();
    if (mouse) mouse->update();
    if (gamepad) gamepad->update();
    if (steamDeck) steamDeck->update();
}

bool InputManager::isKeyboardAvailable() const {
    return keyboard && keyboard->isAvailable();
}

bool InputManager::isMouseAvailable() const {
    return mouse && mouse->isAvailable();
}

bool InputManager::isGamepadAvailable() const {
    return gamepad && gamepad->isAvailable();
}

bool InputManager::isSteamDeckAvailable() const {
    return steamDeck && steamDeck->isAvailable();
}

bool InputManager::isRunningOnSteamDeck() const {
    return isSteamDeckHardware;
}

bool InputManager::isKeyPressed(SDL_Scancode key) const {
    return isKeyboardAvailable() && keyboard->isKeyPressed(key);
}

bool InputManager::isKeyJustPressed(SDL_Scancode key) const {
    return isKeyboardAvailable() && keyboard->isKeyJustPressed(key);
}

bool InputManager::isMouseButtonPressed(int button) const {
    return isMouseAvailable() && mouse->isButtonPressed(button);
}

bool InputManager::isMouseButtonJustPressed(int button) const {
    return isMouseAvailable() && mouse->isButtonJustPressed(button);
}

void InputManager::getMousePosition(int& x, int& y) const {
    if (isMouseAvailable()) {
        mouse->getPosition(x, y);
    } else {
        x = y = 0;
    }
}

bool InputManager::isGamepadButtonPressed(SDL_GameControllerButton button) const {
    if (isSteamDeckAvailable()) {
        return steamDeck->isButtonPressed(button);
    }
    return isGamepadAvailable() && gamepad->isButtonPressed(button);
}

bool InputManager::isGamepadButtonJustPressed(SDL_GameControllerButton button) const {
    if (isSteamDeckAvailable()) {
        return steamDeck->isButtonJustPressed(button);
    }
    return isGamepadAvailable() && gamepad->isButtonJustPressed(button);
}

float InputManager::getGamepadAxisValue(SDL_GameControllerAxis axis) const {
    if (isSteamDeckAvailable()) {
        return steamDeck->getAxisValue(axis);
    }
    return isGamepadAvailable() ? gamepad->getAxisValue(axis) : 0.0f;
}

bool InputManager::hasSteamDeckGyroSupport() const {
    return steamDeck && steamDeck->hasGyroSupport();
}

void InputManager::getSteamDeckGyroData(float& x, float& y, float& z) const {
    if (steamDeck) {
        steamDeck->getGyroData(x, y, z);
    } else {
        x = y = z = 0.0f;
    }
}

void InputManager::getSteamDeckAccelerometerData(float& x, float& y, float& z) const {
    if (steamDeck) {
        steamDeck->getAccelerometerData(x, y, z);
    } else {
        x = y = z = 0.0f;
    }
}