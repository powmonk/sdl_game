#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <iostream>

// Forward declaration
class Game;

/**
 * InputManager: Handles all input methods for Steam Deck
 * - Gamepad controls
 * - Touchscreen input
 * - Keyboard/mouse fallback
 */
class InputManager {
public:
    enum class InputType {
        KEYBOARD,
        MOUSE,
        GAMEPAD,
        TOUCHSCREEN
    };

    enum class GamepadButton {
        A, B, X, Y,
        BACK, GUIDE, START,
        LEFTSTICK, RIGHTSTICK,
        LEFTSHOULDER, RIGHTSHOULDER,
        DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT,
        MISC1, PADDLE1, PADDLE2, PADDLE3, PADDLE4, // Steam Deck back buttons
        TOUCHPAD, // Steam Deck touchpad click
        NONE
    };

    enum class GamepadAxis {
        LEFTX, LEFTY,
        RIGHTX, RIGHTY,
        TRIGGERLEFT,
        TRIGGERRIGHT,
        NONE
    };

    struct TouchPoint {
        int id;
        float x, y;
        bool pressed;
    };

    using CommandFunction = std::function<void(Game*, float)>;

    InputManager(Game* game);
    ~InputManager();

    // Initialize input systems
    bool init();
    void shutdown();
    
    // Update input state
    void update(float deltaTime);

    // Process SDL events (call this in your main event loop)
    void processEvent(const SDL_Event& event);

    // Bind commands to inputs
    void bindKeyCommand(SDL_Keycode key, CommandFunction command);
    void bindMouseCommand(int button, CommandFunction command);
    void bindGamepadButtonCommand(GamepadButton button, CommandFunction command);
    void bindGamepadAxisCommand(GamepadAxis axis, CommandFunction command, float deadzone = 0.25f);
    void bindTouchCommand(CommandFunction command);

    // Input state access methods
    bool isKeyDown(SDL_Keycode key) const;
    bool isKeyPressed(SDL_Keycode key) const;
    bool isKeyReleased(SDL_Keycode key) const;
    
    bool isMouseButtonDown(int button) const;
    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonReleased(int button) const;
    void getMousePosition(int& x, int& y) const;
    void getMouseDelta(int& dx, int& dy) const;
    
    bool isGamepadButtonDown(GamepadButton button) const;
    bool isGamepadButtonPressed(GamepadButton button) const;
    bool isGamepadButtonReleased(GamepadButton button) const;
    float getGamepadAxisValue(GamepadAxis axis) const;

    const std::vector<TouchPoint>& getTouchPoints() const;
    bool isTouchActive() const;
    
    // Get active input method (last used)
    InputType getActiveInputMethod() const;

    // Helper functions
    static const char* getGamepadButtonName(GamepadButton button);
    static const char* getGamepadAxisName(GamepadAxis axis);
    static GamepadButton sdlButtonToGamepadButton(Uint8 button);
    static GamepadAxis sdlAxisToGamepadAxis(Uint8 axis);

private:
    Game* m_game;
    SDL_GameController* m_gameController;
    int m_gameControllerIndex;
    
    // Active input method
    InputType m_activeInputMethod;
    
    // Keyboard state
    const Uint8* m_currentKeyboardState;
    std::unordered_map<SDL_Keycode, bool> m_previousKeyState;
    std::unordered_map<SDL_Keycode, CommandFunction> m_keyCommands;
    
    // Mouse state
    Uint32 m_currentMouseState;
    Uint32 m_previousMouseState;
    int m_mouseX, m_mouseY;
    int m_mouseDeltaX, m_mouseDeltaY;
    std::unordered_map<int, CommandFunction> m_mouseCommands;
    
    // Gamepad state
    std::unordered_map<GamepadButton, bool> m_currentGamepadButtonState;
    std::unordered_map<GamepadButton, bool> m_previousGamepadButtonState;
    std::unordered_map<GamepadAxis, float> m_gamepadAxisValues;
    std::unordered_map<GamepadButton, CommandFunction> m_gamepadButtonCommands;
    std::unordered_map<GamepadAxis, std::pair<CommandFunction, float>> m_gamepadAxisCommands;
    
    // Touch state
    std::vector<TouchPoint> m_touchPoints;
    CommandFunction m_touchCommand;
    
    // Utility methods
    void updateKeyboardState();
    void updateMouseState();
    void updateGamepadState();
};

// Implementation file follows below
/**************************************************************/

#include "input_manager.h"
// Assuming there's a Game class that this input manager works with
#include "game.h"

InputManager::InputManager(Game* game) 
    : m_game(game), 
      m_gameController(nullptr),
      m_gameControllerIndex(-1),
      m_activeInputMethod(InputType::KEYBOARD),
      m_currentKeyboardState(nullptr),
      m_currentMouseState(0),
      m_previousMouseState(0),
      m_mouseX(0), m_mouseY(0),
      m_mouseDeltaX(0), m_mouseDeltaY(0) {
}

InputManager::~InputManager() {
    shutdown();
}

bool InputManager::init() {
    // Initialize SDL gamepad system if not already done
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
            std::cerr << "Failed to initialize SDL gamepad system: " << SDL_GetError() << std::endl;
            return false;
        }
    }

    // Load the Steam Deck gamepad mapping if not built into SDL
    // SDL_GameControllerAddMapping("0,Steam Deck Controller,a:b0,b:b1,x:b2,y:b3,back:b7,guide:b8,start:b9,leftstick:b10,rightstick:b11,leftshoulder:b4,rightshoulder:b5,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,platform:Linux");

    // Get keyboard state pointer
    m_currentKeyboardState = SDL_GetKeyboardState(NULL);

    // Open the first available gamecontroller
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            m_gameController = SDL_GameControllerOpen(i);
            if (m_gameController) {
                m_gameControllerIndex = i;
                std::cout << "Found gamepad: " << SDL_GameControllerName(m_gameController) << std::endl;
                break;
            }
        }
    }

    // Enable touch events
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
    
    return true;
}

void InputManager::shutdown() {
    if (m_gameController) {
        SDL_GameControllerClose(m_gameController);
        m_gameController = nullptr;
    }
}

void InputManager::update(float deltaTime) {
    // Store previous states
    updateKeyboardState();
    updateMouseState();
    updateGamepadState();
    
    // Process keyboard commands
    for (const auto& [key, command] : m_keyCommands) {
        if (isKeyDown(key)) {
            m_activeInputMethod = InputType::KEYBOARD;
            command(m_game, deltaTime);
        }
    }
    
    // Process mouse commands
    for (const auto& [button, command] : m_mouseCommands) {
        if (isMouseButtonDown(button)) {
            m_activeInputMethod = InputType::MOUSE;
            command(m_game, deltaTime);
        }
    }
    
    // Process gamepad button commands
    for (const auto& [button, command] : m_gamepadButtonCommands) {
        if (isGamepadButtonDown(button)) {
            m_activeInputMethod = InputType::GAMEPAD;
            command(m_game, deltaTime);
        }
    }
    
    // Process gamepad axis commands
    for (const auto& [axis, data] : m_gamepadAxisCommands) {
        float value = getGamepadAxisValue(axis);
        float deadzone = data.second;
        if (std::abs(value) > deadzone) {
            m_activeInputMethod = InputType::GAMEPAD;
            data.first(m_game, value);
        }
    }
    
    // Process touch commands
    if (isTouchActive() && m_touchCommand) {
        m_activeInputMethod = InputType::TOUCHSCREEN;
        m_touchCommand(m_game, deltaTime);
    }
}

void InputManager::processEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            m_activeInputMethod = InputType::KEYBOARD;
            break;
            
        case SDL_MOUSEMOTION:
            m_mouseX = event.motion.x;
            m_mouseY = event.motion.y;
            m_mouseDeltaX = event.motion.xrel;
            m_mouseDeltaY = event.motion.yrel;
            m_activeInputMethod = InputType::MOUSE;
            break;
            
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            m_activeInputMethod = InputType::MOUSE;
            break;
            
        case SDL_CONTROLLERDEVICEADDED:
            if (m_gameController == nullptr) {
                m_gameController = SDL_GameControllerOpen(event.cdevice.which);
                m_gameControllerIndex = event.cdevice.which;
                std::cout << "Gamepad connected: " << SDL_GameControllerName(m_gameController) << std::endl;
            }
            break;
            
        case SDL_CONTROLLERDEVICEREMOVED:
            if (m_gameController != nullptr && event.cdevice.which == m_gameControllerIndex) {
                SDL_GameControllerClose(m_gameController);
                m_gameController = nullptr;
                m_gameControllerIndex = -1;
                std::cout << "Gamepad disconnected" << std::endl;
                
                // Try to find another controller
                for (int i = 0; i < SDL_NumJoysticks(); i++) {
                    if (SDL_IsGameController(i)) {
                        m_gameController = SDL_GameControllerOpen(i);
                        if (m_gameController) {
                            m_gameControllerIndex = i;
                            std::cout << "Found another gamepad: " << SDL_GameControllerName(m_gameController) << std::endl;
                            break;
                        }
                    }
                }
            }
            break;
            
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            m_activeInputMethod = InputType::GAMEPAD;
            break;
            
        case SDL_CONTROLLERAXISMOTION:
            m_activeInputMethod = InputType::GAMEPAD;
            break;
            
        case SDL_FINGERDOWN:
            {
                TouchPoint point;
                point.id = static_cast<int>(event.tfinger.fingerId);
                point.x = event.tfinger.x;
                point.y = event.tfinger.y;
                point.pressed = true;
                
                // Add or update touch point
                bool found = false;
                for (auto& tp : m_touchPoints) {
                    if (tp.id == point.id) {
                        tp = point;
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    m_touchPoints.push_back(point);
                }
                
                m_activeInputMethod = InputType::TOUCHSCREEN;
            }
            break;
            
        case SDL_FINGERUP:
            {
                // Remove touch point
                for (auto it = m_touchPoints.begin(); it != m_touchPoints.end(); ) {
                    if (it->id == static_cast<int>(event.tfinger.fingerId)) {
                        it = m_touchPoints.erase(it);
                    } else {
                        ++it;
                    }
                }
                
                m_activeInputMethod = InputType::TOUCHSCREEN;
            }
            break;
            
        case SDL_FINGERMOTION:
            {
                // Update touch point
                for (auto& tp : m_touchPoints) {
                    if (tp.id == static_cast<int>(event.tfinger.fingerId)) {
                        tp.x = event.tfinger.x;
                        tp.y = event.tfinger.y;
                        break;
                    }
                }
                
                m_activeInputMethod = InputType::TOUCHSCREEN;
            }
            break;
    }
}

void InputManager::bindKeyCommand(SDL_Keycode key, CommandFunction command) {
    m_keyCommands[key] = command;
}

void InputManager::bindMouseCommand(int button, CommandFunction command) {
    m_mouseCommands[button] = command;
}

void InputManager::bindGamepadButtonCommand(GamepadButton button, CommandFunction command) {
    m_gamepadButtonCommands[button] = command;
}

void InputManager::bindGamepadAxisCommand(GamepadAxis axis, CommandFunction command, float deadzone) {
    m_gamepadAxisCommands[axis] = std::make_pair(command, deadzone);
}

void InputManager::bindTouchCommand(CommandFunction command) {
    m_touchCommand = command;
}

bool InputManager::isKeyDown(SDL_Keycode key) const {
    SDL_Scancode scancode = SDL_GetScancodeFromKey(key);
    return m_currentKeyboardState[scancode] != 0;
}

bool InputManager::isKeyPressed(SDL_Keycode key) const {
    SDL_Scancode scancode = SDL_GetScancodeFromKey(key);
    auto it = m_previousKeyState.find(key);
    bool prevState = (it != m_previousKeyState.end()) ? it->second : false;
    return m_currentKeyboardState[scancode] != 0 && !prevState;
}

bool InputManager::isKeyReleased(SDL_Keycode key) const {
    SDL_Scancode scancode = SDL_GetScancodeFromKey(key);
    auto it = m_previousKeyState.find(key);
    bool prevState = (it != m_previousKeyState.end()) ? it->second : false;
    return m_currentKeyboardState[scancode] == 0 && prevState;
}

bool InputManager::isMouseButtonDown(int button) const {
    return (m_currentMouseState & SDL_BUTTON(button)) != 0;
}

bool InputManager::isMouseButtonPressed(int button) const {
    return (m_currentMouseState & SDL_BUTTON(button)) != 0 && 
           (m_previousMouseState & SDL_BUTTON(button)) == 0;
}

bool InputManager::isMouseButtonReleased(int button) const {
    return (m_currentMouseState & SDL_BUTTON(button)) == 0 && 
           (m_previousMouseState & SDL_BUTTON(button)) != 0;
}

void InputManager::getMousePosition(int& x, int& y) const {
    x = m_mouseX;
    y = m_mouseY;
}

void InputManager::getMouseDelta(int& dx, int& dy) const {
    dx = m_mouseDeltaX;
    dy = m_mouseDeltaY;
}

bool InputManager::isGamepadButtonDown(GamepadButton button) const {
    auto it = m_currentGamepadButtonState.find(button);
    return (it != m_currentGamepadButtonState.end()) ? it->second : false;
}

bool InputManager::isGamepadButtonPressed(GamepadButton button) const {
    auto currIt = m_currentGamepadButtonState.find(button);
    auto prevIt = m_previousGamepadButtonState.find(button);
    
    bool currState = (currIt != m_currentGamepadButtonState.end()) ? currIt->second : false;
    bool prevState = (prevIt != m_previousGamepadButtonState.end()) ? prevIt->second : false;
    
    return currState && !prevState;
}

bool InputManager::isGamepadButtonReleased(GamepadButton button) const {
    auto currIt = m_currentGamepadButtonState.find(button);
    auto prevIt = m_previousGamepadButtonState.find(button);
    
    bool currState = (currIt != m_currentGamepadButtonState.end()) ? currIt->second : false;
    bool prevState = (prevIt != m_previousGamepadButtonState.end()) ? prevIt->second : false;
    
    return !currState && prevState;
}

float InputManager::getGamepadAxisValue(GamepadAxis axis) const {
    auto it = m_gamepadAxisValues.find(axis);
    return (it != m_gamepadAxisValues.end()) ? it->second : 0.0f;
}

const std::vector<InputManager::TouchPoint>& InputManager::getTouchPoints() const {
    return m_touchPoints;
}

bool InputManager::isTouchActive() const {
    return !m_touchPoints.empty();
}

InputManager::InputType InputManager::getActiveInputMethod() const {
    return m_activeInputMethod;
}

void InputManager::updateKeyboardState() {
    // Store previous key states
    for (int i = 0; i < SDL_NUM_SCANCODES; ++i) {
        SDL_Keycode key = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(i));
        if (key != SDLK_UNKNOWN) {
            m_previousKeyState[key] = m_currentKeyboardState[i] != 0;
        }
    }
}

void InputManager::updateMouseState() {
    m_previousMouseState = m_currentMouseState;
    m_currentMouseState = SDL_GetMouseState(&m_mouseX, &m_mouseY);
}

void InputManager::updateGamepadState() {
    if (m_gameController) {
        // Store previous button states
        m_previousGamepadButtonState = m_currentGamepadButtonState;
        
        // Update button states
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            GamepadButton button = sdlButtonToGamepadButton(i);
            if (button != GamepadButton::NONE) {
                bool pressed = SDL_GameControllerGetButton(m_gameController, static_cast<SDL_GameControllerButton>(i)) != 0;
                m_currentGamepadButtonState[button] = pressed;
            }
        }
        
        // Handle Steam Deck specific buttons if available
        // Note: These would require custom mappings as SDL doesn't expose them directly
        
        // Update axis values
        for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i) {
            GamepadAxis axis = sdlAxisToGamepadAxis(i);
            if (axis != GamepadAxis::NONE) {
                float value = SDL_GameControllerGetAxis(m_gameController, static_cast<SDL_GameControllerAxis>(i)) / 32767.0f;
                m_gamepadAxisValues[axis] = value;
            }
        }
    }
}

const char* InputManager::getGamepadButtonName(GamepadButton button) {
    switch (button) {
        case GamepadButton::A: return "A";
        case GamepadButton::B: return "B";
        case GamepadButton::X: return "X";
        case GamepadButton::Y: return "Y";
        case GamepadButton::BACK: return "Back";
        case GamepadButton::GUIDE: return "Guide";
        case GamepadButton::START: return "Start";
        case GamepadButton::LEFTSTICK: return "Left Stick";
        case GamepadButton::RIGHTSTICK: return "Right Stick";
        case GamepadButton::LEFTSHOULDER: return "Left Shoulder";
        case GamepadButton::RIGHTSHOULDER: return "Right Shoulder";
        case GamepadButton::DPAD_UP: return "D-Pad Up";
        case GamepadButton::DPAD_DOWN: return "D-Pad Down";
        case GamepadButton::DPAD_LEFT: return "D-Pad Left";
        case GamepadButton::DPAD_RIGHT: return "D-Pad Right";
        case GamepadButton::MISC1: return "Misc";
        case GamepadButton::PADDLE1: return "Paddle 1";
        case GamepadButton::PADDLE2: return "Paddle 2";
        case GamepadButton::PADDLE3: return "Paddle 3";
        case GamepadButton::PADDLE4: return "Paddle 4";
        case GamepadButton::TOUCHPAD: return "Touchpad";
        default: return "Unknown";
    }
}

const char* InputManager::getGamepadAxisName(GamepadAxis axis) {
    switch (axis) {
        case GamepadAxis::LEFTX: return "Left X";
        case GamepadAxis::LEFTY: return "Left Y";
        case GamepadAxis::RIGHTX: return "Right X";
        case GamepadAxis::RIGHTY: return "Right Y";
        case GamepadAxis::TRIGGERLEFT: return "Left Trigger";
        case GamepadAxis::TRIGGERRIGHT: return "Right Trigger";
        default: return "Unknown";
    }
}

InputManager::GamepadButton InputManager::sdlButtonToGamepadButton(Uint8 button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return GamepadButton::A;
        case SDL_CONTROLLER_BUTTON_B: return GamepadButton::B;
        case SDL_CONTROLLER_BUTTON_X: return GamepadButton::X;
        case SDL_CONTROLLER_BUTTON_Y: return GamepadButton::Y;
        case SDL_CONTROLLER_BUTTON_BACK: return GamepadButton::BACK;
        case SDL_CONTROLLER_BUTTON_GUIDE: return GamepadButton::GUIDE;
        case SDL_CONTROLLER_BUTTON_START: return GamepadButton::START;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: return GamepadButton::LEFTSTICK;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return GamepadButton::RIGHTSTICK;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return GamepadButton::LEFTSHOULDER;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return GamepadButton::RIGHTSHOULDER;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return GamepadButton::DPAD_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return GamepadButton::DPAD_DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return GamepadButton::DPAD_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return GamepadButton::DPAD_RIGHT;
        case SDL_CONTROLLER_BUTTON_MISC1: return GamepadButton::MISC1;
        // Steam Deck specific mappings would need additional configuration
        default: return GamepadButton::NONE;
    }
}

InputManager::GamepadAxis InputManager::sdlAxisToGamepadAxis(Uint8 axis) {
    switch (axis) {
        case SDL_CONTROLLER_AXIS_LEFTX: return GamepadAxis::LEFTX;
        case SDL_CONTROLLER_AXIS_LEFTY: return GamepadAxis::LEFTY;
        case SDL_CONTROLLER_AXIS_RIGHTX: return GamepadAxis::RIGHTX;
        case SDL_CONTROLLER_AXIS_RIGHTY: return GamepadAxis::RIGHTY;
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT: return GamepadAxis::TRIGGERLEFT;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: return GamepadAxis::TRIGGERRIGHT;
        default: return GamepadAxis::NONE;
    }
}
