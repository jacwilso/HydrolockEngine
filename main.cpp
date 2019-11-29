#include <iostream>

#include "Renderer.h"

int main()
{
    Renderer renderer;
    renderer.run();

    return EXIT_SUCCESS;
}


// TODO: old include files (eventually purge)
/*
,
"/Users/jacwilso/Documents/vulkansdk-macos-1.1.126.0/macOS/include",
"/usr/local/include"
*/

/*
/Library/Developer/CommandLineTools/SDKs/MacOSX10.14.sdk/System/Library/Frameworks
/System/Library/Frameworks
/Library/Frameworks
/usr/local/lib
/Users/jacwilso/Documents/vulkansdk-macos-1.1.126.0/macOS/lib
*/

/*
,
        {
            "name": "(lldb) Launch",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/main.out",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "externalConsole": true,
            "MIMode": "lldb",
            "logging": {
                "trace": true,
                "traceResponse": true,
                "engineLogging": true
            },
            "environment": [
                {
                    "name": "VK_ICD_FILENAMES",
                    "value": "/Users/jacwilso/Documents/vulkansdk-macos-1.1.126.0/macOS/etc/vulkan/icd.d/MoltenVK_icd.json"
                },
                {
                    "name": "VK_LAYER_PATH",
                    "value": "/Users/jacwilso/Documents/vulkansdk-macos-1.1.126.0/macOS/etc/vulkan/explicit_layer.d"
                }
            ]
        }
*/