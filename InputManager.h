#pragma once

#include <memory>
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>

// Forward declarations
class KeyboardInput;
class MouseInput;
class GamepadInput;
class SteamDeckInput;

// Abstract base class for input devices
class InputDevice {
public:
    virtual ~InputDevice() = default;
    virtual void update() = 0;
    virtual bool isAvailable() const = 0;
};

// Main input manager class
class InputManager {
private:
    bool initialized;
    std::unique_ptr<KeyboardInput> keyboard;
    std::unique_ptr<MouseInput> mouse;
    std::unique_ptr<GamepadInput> gamepad;
    std::unique_ptr<SteamDeckInput> steamDeck;
    bool isSteamDeckHardware;
    
public:
    InputManager();
    ~InputManager();
    
    // Initialize the input system
    bool initialize();
    
    // Update all input devices (call once per frame)
    void update();
    
    // Device availability checks
    bool isKeyboardAvailable() const;
    bool isMouseAvailable() const;
    bool isGamepadAvailable() const;
    bool isSteamDeckAvailable() const;
    bool isRunningOnSteamDeck() const;
    
    // Keyboard input methods
    bool isKeyPressed(SDL_Scancode key) const;
    bool isKeyJustPressed(SDL_Scancode key) const;
    
    // Mouse input methods
    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonJustPressed(int button) const;
    void getMousePosition(int& x, int& y) const;
    
    // Gamepad input methods
    bool isGamepadButtonPressed(SDL_GameControllerButton button) const;
    bool isGamepadButtonJustPressed(SDL_GameControllerButton button) const;
    float getGamepadAxisValue(SDL_GameControllerAxis axis) const;
    
    // Steam Deck specific methods
    bool hasSteamDeckGyroSupport() const;
    void getSteamDeckGyroData(float& x, float& y, float& z) const;
    void getSteamDeckAccelerometerData(float& x, float& y, float& z) const;
};
