{
  "targets": [
    {
      "target_name": "progress_bar",
      "include_dirs": ["<!(node -e \"require('nan')\")"],
      "conditions": [
        ['OS=="mac"', {
          "sources": [ 
            "src/progress_bar.cpp",
            "src/progress_bar_macos.mm"
          ],
          "libraries": ["-framework Cocoa"],
          "xcode_settings": {
            "OTHER_CPLUSPLUSFLAGS": ["-std=c++17", "-stdlib=libc++"],
            "OTHER_LDFLAGS": ["-framework Cocoa"],
            "MACOSX_DEPLOYMENT_TARGET": "11.0",
            "CLANG_ENABLE_OBJC_ARC": "YES",
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "GCC_ENABLE_CPP_RTTI": "YES"
          }
        }],
        ['OS=="win"', {
          "sources": [
            "src/progress_bar.cpp",
            "src/progress_bar_windows.cpp"
          ],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "AdditionalOptions": ["/std:c++17"]
            }
          },
          "libraries": [
            "Shcore.lib"
          ]
        }]
      ]
    }
  ]
}
