// system
#include <switch.h>
#include <unordered_map>
#include <string>
#include <vector>

#include <System/Input.hpp>
#include <System/Signal.hpp>

// The InputObject constructor
InputObject::InputObject(InputType inputType, int keyCode){
    this->inputType=inputType;
    this->keyCode=keyCode;
}

void InputSystem::update(){
    padUpdate(&gamepad);

    previousKeyboardState = currentKeyboardState;
    size_t stateCount = hidGetKeyboardStates(&currentKeyboardState, 1);

    // fire generic input events for gamepad
    {
        u64 down = padGetButtonsDown(&gamepad);
        u64 up   = padGetButtonsUp(&gamepad);

        // inputs must have changed
        if (down != 0 || up != 0){
            for (int bit = 0; bit <= 30; bit++){
                // check if the bit is in the up/down set
                u64 bitl = BITL(bit);
                
                // construct the input object
                InputObject changedObject(InputType::Gamepad, HidNpadButton(bitl));

                if (down & bitl){ // the button with this binary value was pressed
                    inputChanged.fire(*this, InputState::Begin, changedObject);
                    inputBegan.fire(*this, changedObject);
                }
                if (up & bitl){ // the button with this binary value was released#
                    inputChanged.fire(*this, InputState::End, changedObject);
                    inputEnded.fire(*this, changedObject);
                }
            }
        }
    }

    // fire generic input events for keyboard
    {
        if (stateCount > 0){
            for (int keyIdx = 0; keyIdx < 4; keyIdx ++){
                // check if anything changed with a XOR mask
                u64 changed = previousKeyboardState.keys[keyIdx] ^ currentKeyboardState.keys[keyIdx];
                if (!changed){
                    continue;
                }

                // iterate through each key
                for (int bit = 0; bit < 64; bit++){
                    if (!(changed & BITL(bit))){ // only iterate through changed keys
                        continue;
                    }

                    InputObject changedObject(InputType::Keyboard, HidKeyboardKey((keyIdx*64)+bit));

                    if (currentKeyboardState.keys[keyIdx] & BITL(bit)){
                        inputChanged.fire(*this, InputState::Begin, changedObject);
                        inputBegan.fire(*this, changedObject);
                    }
                    else {
                        inputChanged.fire(*this, InputState::End, changedObject);
                        inputEnded.fire(*this, changedObject);
                    }
                }
            }
        }
    }

    // run signals
    for (const auto& [bindID, inputObject] : bindings){
        // check if there is a connection
        if (signals.find(bindID) == signals.end()){
            continue;
        }
        // check the state
        InputState buttonState = getButtonState(inputObject);

        if (buttonState != InputState::None){
            // log("[INPUT] - Running bound functions for '"+bindID+"'");

            // fire the signal
            Signal<InputSystem&, InputState>& signal = signals[bindID];
            signal.fire(*this, buttonState);
        }
    }
}

void InputSystem::init(){
    // log("[INPUT] - Initialising InputSystem...");

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&gamepad);

    // also initialise keyboard
    hidInitializeKeyboard();

    // log("[INPUT] - InputSystem initialised successfully!");
}

// nice abstractions
bool InputSystem::buttonPressed(const InputObject& inputObject){
    InputType inputType = inputObject.inputType;
    int keyCode = inputObject.keyCode;

    if (inputType == InputType::Gamepad){
        HidNpadButton button = HidNpadButton(keyCode);
        return (padGetButtonsDown(&gamepad)&button) != 0; // shut up compiler
    }
    else if (inputType == InputType::Keyboard){
        HidKeyboardKey key = HidKeyboardKey(keyCode);
        
        int keyIdx = key / 64;
        if (keyIdx < 0 || keyIdx > 3){
            return false;
        }

        int bitIdx = key % 64;

        // get current and previous states
        bool isCurrHeld  = (currentKeyboardState.keys[keyIdx] & BITL(bitIdx)) != 0; // true if currently held
        bool wasPrevHeld = (previousKeyboardState.keys[keyIdx] & BITL(bitIdx)) != 0; // false if previously not held

        if (isCurrHeld && !wasPrevHeld){
            // it must've been pressed
            return true;
        }
    }

    return false;
}

bool InputSystem::buttonHeld(const InputObject& inputObject){
    InputType inputType = inputObject.inputType;
    int keyCode = inputObject.keyCode;

    if (inputType == InputType::Gamepad){
        HidNpadButton button = HidNpadButton(keyCode);
        return (padGetButtons(&gamepad)&button) != 0; // shut up compiler
    }
    else if (inputType == InputType::Keyboard){
        HidKeyboardKey key = HidKeyboardKey(keyCode);

        // get array index of key
        // keyboardState.keys output:
        //  [64 states] [64 states] [64 states] [64 states]
        //  0           1           2           3
        int keyIdx = key / 64; // cannot be greater than 3 or less than 0
        if (keyIdx < 0 || keyIdx > 3){
            return false;
        }

        int bitIdx = key % 64;

        // do the AND mask check
        return (this->currentKeyboardState.keys[keyIdx] & BITL(bitIdx)) != 0;
    }

    return false;
}

bool InputSystem::buttonReleased(const InputObject& inputObject){
    InputType inputType = inputObject.inputType;
    int keyCode = inputObject.keyCode;

    if (inputType == InputType::Gamepad){
        HidNpadButton button = HidNpadButton(keyCode);
        return (padGetButtonsUp(&gamepad)&button) != 0; // shut up compiler
    }
    else if (inputType == InputType::Keyboard){
        HidKeyboardKey key = HidKeyboardKey(keyCode);
        
        int keyIdx = key / 64;
        if (keyIdx < 0 || keyIdx > 3){
            return false;
        }

        int bitIdx = key % 64;

        // get current and previous states
        bool isCurrHeld  = (currentKeyboardState.keys[keyIdx] & BITL(bitIdx)) != 0; // false if not currently held
        bool wasPrevHeld = (previousKeyboardState.keys[keyIdx] & BITL(bitIdx)) != 0; // true if previously held

        if (!isCurrHeld && wasPrevHeld){ // inverse to buttonPressed
            // it must've been pressed
            return true;
        }
    }

    return false;
}

// state checks
InputState InputSystem::getButtonState(const InputObject& inputObject){
    bool isPressed  = this->buttonPressed(inputObject);
    bool isHeld     = this->buttonHeld(inputObject);
    bool isReleased = this->buttonReleased(inputObject);

    if (isPressed){
        return InputState::Begin;
    }
    else if(isHeld){
        return InputState::Hold;
    }
    else if (isReleased){
        return InputState::End;
    }
    
    return InputState::None;
}

InputState InputSystem::getBindState(const std::string& bindID){
    // check if there is a bind
    if (bindings.find(bindID) != bindings.end()){ // fuck you compiler
        // if there is one, check all button states to see which one it is covered by
        InputObject& inputObject = bindings.at(bindID);
        return getButtonState(inputObject);
    }

    return InputState::None;
}

// bindings
int InputSystem::getBindButton(const std::string& bindID){
    // look for the button
    InputObject inputObject = bindings.at(bindID);
    int keyCode = inputObject.keyCode;

    if (inputObject.inputType == InputType::Gamepad){
        return HidNpadButton(keyCode);
    }
    else if (inputObject.inputType == InputType::Keyboard){
        // do keyboard logic
    }

    return 0;
}

// Get the string from a HID usage code
// probably don't use this it's very raw and only supports one keyboard layout
std::string InputSystem::HidUsageToString(int hidCode){
    // check if shift is pressed
    InputState shiftState = getButtonState(InputObject(InputType::Keyboard, HidKeyboardKey_LeftShift)); 

    bool shiftPressed = (shiftState == InputState::Begin) || (shiftState == InputState::Hold);
    if (hidCode >= 4 && hidCode <= 29){
        char letter = (hidCode - 4) + (shiftPressed ? 'A' : 'a');
        return std::string(1, letter);
    }

    static const char numbers[] = "1234567890";
    static const char symbols[] = "!\"#$%^&*()";

    if (hidCode >= 30 && hidCode <= 39) {
        int index = hidCode - 30;
        return std::string(1, shiftPressed ? symbols[index] : numbers[index]);
    }

    if (hidCode == 44){
        // space
        return " ";
    }
    if (hidCode == 40){
        // return
        return "\n";
    }

    return "";
}

void InputSystem::bind(std::string bindID, const InputObject& inputObject){
    // set the new binding      
    bindings.insert_or_assign(bindID, inputObject);
    // log("[INPUT] - Successfully bound '"+bindID+"'!"); 
}

void InputSystem::unbind(const std::string& bindID){
    // erase button bindings
    bindings.erase(bindID);
    
    // clear the signal
    if (signals.find(bindID) != signals.end()){ // fuck you compiler x2
        signals.at(bindID).clear();
        signals.erase(bindID);
    }
}

// connections
Connection<InputSystem&, InputState>& InputSystem::connect(const std::string& bindID, void (*function)(InputSystem&, InputState)){
    // check if the bind exists
    if (bindings.find(bindID) != bindings.end()){ // compiler genuinely please
        // this makes a new signal if one doesn't exist
       Signal<InputSystem&, InputState>& mySignal = signals[bindID];

        // make a new connection
        return mySignal.connect(function);
    }
    else{
        std::abort();
    }
}

InputSystem& InputSystem::getService(){
    static InputSystem instance;
    return instance;
}
