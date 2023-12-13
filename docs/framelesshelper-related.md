# FramelessHelper 2.x

Cross-platform window customization framework for Qt Widgets and Qt Quick. Supports Windows, Linux and macOS.

## Join with Us :triangular_flag_on_post:

You can join our [Discord channel](https://discord.gg/grrM4Tmesy) to communicate with us. You can share your findings, thoughts and ideas on improving / implementing FramelessHelper functionalities on more platforms and apps!

## More

### Title Bar Design Guidance

- Microsoft: <https://docs.microsoft.com/en-us/windows/apps/design/basics/titlebar-design>
- KDE: <https://develop.kde.org/hig/>
- GNOME: <https://developer.gnome.org/hig/patterns/containers/header-bars.html>
- Apple: <https://developer.apple.com/design/human-interface-guidelines/macos/windows-and-views/window-anatomy/>

## Platform Notes

### Windows

- If DWM composition is disabled in some very rare cases (only possible on Windows 7), the top-left corner and top-right corner will appear in round shape. The round corners can be restored to square if you re-enable DWM composition.
- There's an OpenGL driver bug which will cause some frameless windows have a strange black bar right on top of your homemade title bar, and it also makes the controls in your windows shifted to the bottom-right corner for some pixels. It's a bug of your graphics card driver, specifically, your OpenGL driver, not FramelessHelper. There are some solutions provided by our users but some of them may not work in all conditions, you can pick one from them:

  Solution | Principle
  -------- | ---------
  Upgrade the graphics driver | Try to use a newer driver which may ship with the fix
  Change the system theme to "Basic" (in contrary to "Windows Aero") | Let Windows use pure software rendering
  If there are multiple graphics cards, use another one instead | Try to use a different driver which may don't have such bug at all
  Upgrade the system to at least Windows 11 | Windows 11 redesigned the windowing system so the bug can no longer be triggered
  Remove the `WS_THICKFRAME` and `WS_OVERLAPPED` styles from the window, and maybe also add the `WS_POPUP` style at the same time, and don't do anything inside the `WM_NCCALCSIZE` block (just return `false` directly or remove/comment out the whole block) | Try to mirror Qt's `FramelessWindowHint`'s behavior
  Use `Qt::FramelessWindowHint` instead of doing the `WM_NCCALCSIZE` trick | Qt's rendering code path is totally different between these two solutions
  Force Qt to use the ANGLE backend instead of the Desktop OpenGL | ANGLE will translate OpenGL directives into D3D ones
  Force Qt to use pure software rendering instead of rendering through OpenGL | Qt is not using OpenGL at all
  Force Qt to use the Mesa 3D libraries instead of normal OpenGL | Try to use a different OpenGL implementation
  Use Direct3D/Vulkan/Metal instead of OpenGL | Just don't use the buggy OpenGL

  If you are lucky enough, one of them may fix the issue for you. If not, you may try to use multiple solutions together. **But I can't guarantee the issue can 100% be fixed.**
- Due to there are many sub-versions of Windows 10, it's highly recommended to use the latest version of Windows 10, at least **no older than Windows 10 1809**. If you try to use this framework on some very old Windows 10 versions such as 1507 or 1607, there may be some compatibility issues. Using this framework on Windows 7 is also supported but not recommended. To get the most stable behavior and the best appearance, you should use it on the latest version of Windows 10 or Windows 11.
- To make the snap layout work as expected, there are some additional rules for your homemade system buttons to follow:
  - **Add a manifest file to your application. In the manifest file, you need to claim your application supports Windows 11 explicitly. This step is VERY VERY IMPORTANT. Without this step, the snap layout feature can't be enabled.**
  - Call `setSystemButton()` for each button (it can be any *QWidget* or *QQuickItem*) to let FramelessHelper know which is the minimize/maximize/close button.

### Linux

- FramelessHelper will force your application to use the _XCB_ platform plugin when running on Wayland.
- The resize area is inside of the window.

### macOS

- Some users reported that the window is not resizable on some old macOS versions.

## Special Thanks

*Ordered by first contribution time (it may not be very accurate, sorry)*

- [Yuhang Zhao](https://github.com/wangwenx190): Help me create this project. This project is mainly based on his code.
- [Julien](https://github.com/JulienMaille): Help me test this library on many various environments and help me fix the bugs we found. Contributed many code to improve this library. The MainWindow example is mostly based on his code.
- [Altair Wei](https://github.com/altairwei): Help me fix quite some small bugs and give me many important suggestions, the 2.x version is also inspired by his idea during our discussions.
- [Kenji Mouri](https://github.com/MouriNaruto): Give me a lot of help on Win32 native developing.
- [Dylan Liu](https://github.com/mentalfl0w): Help me improve the build process on macOS.
- [SineStriker](https://github.com/SineStriker): Spent over a whole week helping me improve the Snap Layout implementation, fixing potential bugs and also give me a lot of professional and useful suggestions. Without his great effort, the new implementation may never come.
- And also thanks to other contributors not listed here! Without their valuable help, this library wouldn't have such good quality and user experience!

## License

```text
MIT License

Copyright (C) 2021-2023 by wangwenx190 (Yuhang Zhao)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```