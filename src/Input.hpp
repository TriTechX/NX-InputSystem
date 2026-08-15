#pragma once

#include <switch.h>
#include <string>
#include <optional>
#include <unordered_map>

#include <System/Signal.hpp>

enum class InputState{
    Begin,
    End,
    Hold,
    None
};

enum class InputType{
    Gamepad,
    Keyboard
};

/**
 * @brief `keyCode` can be either `HidNpadButton` OR `HidKeyboardKey`
**/
class InputObject{
public:
    InputType inputType;
    int keyCode;

    InputObject(InputType inputType, int keyCode);
};

class InputSystem {
private:
    InputSystem()=default;

    PadState gamepad;

    HidKeyboardState previousKeyboardState;
    HidKeyboardState currentKeyboardState;

    std::unordered_map<std::string, InputObject> bindings;
    std::unordered_map<std::string, Signal<InputSystem&, InputState>> signals;
    std::vector<Connection<InputSystem&, InputState>> connections;
public:
    static InputSystem& getService();

    InputSystem(const InputSystem&)=delete;
    InputSystem& operator=(const InputSystem&)=delete;

    Signal<InputSystem&, InputState, InputObject> inputChanged;
    Signal<InputSystem&, InputObject> inputBegan;
    Signal<InputSystem&, InputObject> inputEnded;

    void init();
    void update();

    bool buttonPressed(const InputObject& inputObject);
    bool buttonHeld(const InputObject& inputObject);
    bool buttonReleased(const InputObject& inputObject);

    InputState getButtonState(const InputObject& inputObject);
    InputState getBindState(const std::string& bindID);
    int getBindButton(const std::string& bindID);

    std::string HidUsageToString(int hidCode);

    void bind(std::string bindID, const InputObject& inputObject);
    void unbind(const std::string& bindID);

    Connection<InputSystem&, InputState>& connect(const std::string& bindID, void (*function)(InputSystem&, InputState));
};
