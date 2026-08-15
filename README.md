# NX-InputSystem
A basic input system for Nintendo Switch written in C++

Supports both gamepad and keyboard inputs.

Usage:
```cpp
static InputSystem& inputService = InputSystem::getService(); // get the system

inputService.bind("Jump", HidNpadButton_A)
auto& jumpConn = inputService.connect("Jump", [](InputSystem& inputService, InputState inputState){
  printf("Jumped"); // or whatever you want to do when A is pressed
}); // multiple connections can be made per bind

// remove the connection
jumpConn.disconnect();

// also has several generic listener events
auto& inputChangedConn = inputService.inputChanged.connect([](InputSystem& inputService, InputState inputState, InputObject inputObject){
  // you can check how the input changed
  if (inputState == InputState::Begin){
    printf("Input began");
  } else if (inputState == InputState::End){
    printf("Input ended");
  }

  // or get its HID usage keycode
  printf(std::to_string(inputObject.keyCode))

  // you can also get its input type
  if (inputObject.inputType == InputType::Gamepad){
    printf("Input was from a controller");

    // from here, you can convert to the right enum
    HidNpadButton button = HidNpadButton(inputObject.keyCode);
  } else if (inputObject.inputType == InputType::Keyboard){
    printf("Input was from a keyboard");

    // you can also convert keycodes to HidKeyboardKey
    HidKeyboardKey key = HidKeyboardKey(inputObject.keyCode);
  }
});

// can also be disconnected
inputChangedConn.disconnect();

inputService.inputBegan.connect([](InputSystem& inputService, InputObject inputObject){});
inputService.inputEnded.connect([](InputSystem& inputService, InputObject inputObject){});
```
