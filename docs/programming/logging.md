# Logging

ACE has very verbose logging that allows you to inspect every single step that the library does.
Emitting that amount of logs however comes with serious performance cost, hence it is advised to enable it only in debug builds.

Logs can be emitted in 3 ways:

- via outputing to file (slowest)
- via UAE logging (fastest, limited to emulators)
- via serial port (fast-ish, most reasonable on real hardware)

Note that usually you just want to enable one log output to minimize the performance hit.

## Enabling logs

To enable logs and sanity checks in ACE, add `-DACE_DEBUG=ON` parameter to your cmake call.
This will enable logging itself, as well as file output if it's properly configured in code.

> [!NOTE]
> You need `ACE_DEBUG` switch enabled even with other logging methods.

### Enabling file logs

The `logOpen()` function accepts the file name parameter.
If you want to enable file-based logging, set it to a path for output log file.
If you want to disable file logs, set it to null.

If you're using `#include <ace/generic/main.h>`, you'll notice that the log manager is opened inside this file.
To control the log file path, define the `GENERIC_MAIN_LOG_PATH` *before* the include, such as:

```c
#define GENERIC_MAIN_LOG_PATH "game.log"
#include <ace/generic/main.h>
```

### Enabling UAE logs

If you want to enable UAE logging, add `-DACE_DEBUG_UAE=ON`.
If you're using Bartman suite, the log should be visible in the "Debug Console" bottom panel of the IDE whenever you launch the game in integrated emulator.
If you're using Bebbo compiler, you're probably using stock WinUAE.
In order for logging to work, you need to launch the emulator with `-log` parameter so that it opens the console window, as well as enable "Debug memory space" in Miscellaneous section.

### Enabling serial port logs

In case you want to enable serial logging, add `-DACE_DEBUG_SERIAL=ON`.
This will cause the game to take over the serial port and use it for outputting log messages.

The transmission parameters are hardcoded to **115200 baud, 8 data bits, 1 stop bit, no parity, no flow control**.

> [!WARNING]
> Amiga serial port uses RS-232 transmission that outputs data using -12V and 12V voltages.
> Be sure to connect to it with a compatible RS-232 adapter / PC port.
>
> If you try to connect the 3.3V/5V UART dongle to it, you will damage the hardware!

In emulated environments, it is possible to configure WinUAE to use any UART/RS-232 port/dongle attached to your system - be sure to run the emulator as administrator in order to allow it to use the actual hardware.

WinUAE also has an option of outputting any data that would go to the serial port to emulator's console window.
In order to achieve this, run it with `-log -serlog` parameters.

## Controlling the logs from VSCode

While developing with code editors, you will rarely execute the CMake command by hand.
Enabling logs can also be automated to some extent.

In order to have a convenient switch for logs in VSCode, create a `.vscode/tasks.json` file with following contents:

```jsonc
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Configure Game: Debug ON",
      "type": "shell",
      "command": "cmake",
      "group": "build",
      "args": [
        "..",
        "-DACE_DEBUG=ON",
        "-DACE_DEBUG_UAE=ON",
        // Mix and match CMake switches of your choice
      ],
      "options": {
        "cwd": "${workspaceFolder}/build",
      },
      "problemMatcher": [],
    },
    {
      "label": "Configure Game: Debug OFF",
      "type": "shell",
      "command": "cmake",
      "group": "build",
      "args": [
        "..",
        "-DACE_DEBUG=OFF",
        "-DACE_DEBUG_UAE=OFF",
        // Ditto
      ],
      "options": {
        "cwd": "${workspaceFolder}/build",
      },
      "problemMatcher": [],
    },
  ]
}
```

This will create two build tasks that allow you switch in or out the debug facilities of your choice.
To run them, hit <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>B</kbd> and select one from the dropdown.
Observe the CMake output to see which ACE switches are enabled.

> [!NOTE]
> Calling such partial CMake configures works correctly only if the project is already configured.
>
> Before doing such calls, make sure that CMake has ran at least once and the game can be built.
