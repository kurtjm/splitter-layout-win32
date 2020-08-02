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

#pragma once

class Application {
public:
  
  Application() {}

  Application(Application const&) = delete;

  Application& operator=(Application const&) = delete;

  ~Application();

  bool init(HINSTANCE hinstance);

private:

  bool create_layout_windows(HINSTANCE hinstance);

  void update_layout_windows();

  void splitter_select(HWND hwnd, int x, int y);

  void splitter_clear_selection();

  void splitter_mouse_move(HWND hwnd, int x, int y);
  
  void set_cursor(Layout::Select_type const& select);

  static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
  
  HWND m_hwnd = {};
    
  HCURSOR m_cursor_default = {};
  HCURSOR m_cursor_vertical = {};
  HCURSOR m_cursor_horizontal = {};
  HCURSOR m_cursor_both = {};

  Layout m_layout;

  std::unordered_map<int, HWND> m_windows;
};
