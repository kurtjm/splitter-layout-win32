// Copyright (c) 2020 Kurt Miller (kurtjm@gmx.com)
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#include <windows.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

#include "Layout.h"
#include "Application.h"

int WINAPI WinMain(
  HINSTANCE hinstance,
  HINSTANCE hinstance_prev,
  PSTR cmdline,
  INT cmdshow
) {

  Application app;
  if (!app.init(hinstance)) {
    MessageBox(0, L"Unable to initialize application.", L"Error", MB_ICONEXCLAMATION|MB_OK);
    return 0;
  }

  auto msg = MSG{};
  auto status = BOOL{};

  while ((status = GetMessage(&msg, 0, 0, 0)) != 0) {
    if (status == -1) {
      break;
    }
    TranslateMessage(&msg); 
    DispatchMessage(&msg);
  }
  return static_cast<int>(msg.wParam);
}
