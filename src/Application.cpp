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
#include <windowsx.h>
#include <assert.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <assert.h>

#include "Layout.h"
#include "Application.h"

static const wchar_t* k_app_wnd_title = L"Splitter Window Layout";
static const wchar_t* k_app_wnd_class = L"app_wnd";
static const wchar_t* k_app_wnd_layout_class = L"layout_wnd";

static const int k_app_wnd_width = 1280;
static const int k_app_wnd_height = 800;

Application::~Application() {
  for (auto const& window : m_windows) {
    DestroyWindow(window.second);
  }

  if (m_hwnd) {
    DestroyWindow(m_hwnd);
    m_hwnd = {};
  }
}

bool Application::init(HINSTANCE hinstance) {
  assert(hinstance);
  assert(!m_hwnd);

  m_cursor_default = LoadCursor(NULL, IDC_ARROW);
  m_cursor_vertical = LoadCursor(NULL, IDC_SIZEWE);
  m_cursor_horizontal = LoadCursor(NULL, IDC_SIZENS);
  m_cursor_both = LoadCursor(NULL, IDC_SIZEALL);

  auto wndclass = WNDCLASS{};
  wndclass.style = CS_OWNDC;
  wndclass.lpfnWndProc = Application::window_proc;
  wndclass.hInstance = hinstance;
  wndclass.lpszClassName = k_app_wnd_class;
  wndclass.hbrBackground = (HBRUSH)COLOR_WINDOW;

  if (!RegisterClass(&wndclass)) {
    return false;
  }

  auto window_width = k_app_wnd_width;
  auto window_height = k_app_wnd_height;
  auto window_x = (GetSystemMetrics(SM_CXSCREEN) - window_width) / 2;
  auto window_y = (GetSystemMetrics(SM_CYSCREEN) - window_height) / 2;
  auto window_rect = RECT{ window_x, window_y, window_x + window_width, window_y + window_height };
  auto window_style = WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

  AdjustWindowRect(&window_rect, window_style, FALSE);

  auto style_ex = DWORD{ WS_EX_TRANSPARENT };

  m_hwnd = CreateWindowEx(
    style_ex,
    wndclass.lpszClassName,
    k_app_wnd_title,
    window_style,
    window_rect.left,
    window_rect.top,
    window_rect.right - window_rect.left,
    window_rect.bottom - window_rect.top,
    NULL,
    NULL,
    hinstance,
    0L
  );

  if (!m_hwnd) {
    return false;
  }

  auto client_rect = RECT{};
  GetClientRect(m_hwnd, &client_rect);

  auto layout_config = std::string{ "V{H{V{W{1}:W{2}}:H{W{3}:V{W{4}:W{5}}}}:V{W{6}:H{W{7}:W{8}}}}" };
  if (!m_layout.init(layout_config, client_rect)) {
    return false;
  }

  if (!create_layout_windows(hinstance)) {
    return false;
  }

  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
  ShowWindow(m_hwnd, SW_SHOW);
  return true;
}

bool Application::create_layout_windows(HINSTANCE hinstance) {
  auto wndclass = WNDCLASS{};
  wndclass.style = CS_OWNDC|CS_VREDRAW|CS_HREDRAW;
  wndclass.lpfnWndProc = DefWindowProc;
  wndclass.hInstance = hinstance;
  wndclass.lpszClassName = k_app_wnd_layout_class;
  wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW);

  if (!RegisterClass(&wndclass)) {
    return false;
  }

  auto style = DWORD{ WS_CHILD };
  auto style_ex = DWORD{ WS_EX_CLIENTEDGE };

  auto const& panels = m_layout.panels();
  for (auto const& panel : panels) {
    auto const& id = panel.second->id;
    auto const& rect = panel.second->rect;

    auto hwnd = CreateWindowEx(
      style_ex,
      k_app_wnd_layout_class,
      L"",
      style,
      rect.left,
      rect.top,
      rect.right - rect.left,
      rect.bottom - rect.top,
      m_hwnd,
      NULL,
      hinstance,
      0L
    );

    if (!hwnd) {
      return false;
    }
    assert(m_windows.find(id) == m_windows.end());
    m_windows[id] = hwnd;
    ShowWindow(hwnd, SW_SHOW);
  }
  return true;
}

void Application::update_layout_windows() {
  auto client_rect = RECT{};
  GetClientRect(m_hwnd, &client_rect);

  if (!m_layout.update(client_rect)) {
    PostQuitMessage(1);  // handle errors;
    return;
  }

  auto const& panels = m_layout.panels();
  for (auto const& panel : panels) {
    auto const& id = panel.second->id;
    auto const& rect = panel.second->rect;

    assert(m_windows.find(id) != m_windows.end());
    MoveWindow(m_windows[id], rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
  }
  InvalidateRect(m_hwnd, NULL, TRUE);
}

void Application::splitter_select(HWND hwnd, int x, int y) {
  auto select = m_layout.splitter_select(x, y, true);
  if (select != Layout::Select_type::None) {
    SetCapture(hwnd);
  }
}

void Application::splitter_clear_selection() {
  if (m_layout.splitter_has_selected()) {
    m_layout.splitter_clear_selected();
    ReleaseCapture();
  }
}

void Application::splitter_mouse_move(HWND hwnd, int x, int y) {
  if (m_layout.splitter_has_selected()) {
    auto client_rect = RECT{};
    GetClientRect(m_hwnd, &client_rect);

    m_layout.splitter_update_selected(x, y, client_rect);
    update_layout_windows();
  }
  else {
    auto select = m_layout.splitter_select(x, y, false);
    if (select != Layout::Select_type::None) {
      set_cursor(select);
    }
  }
}

void Application::set_cursor(Layout::Select_type const& select) {
  if (select == Layout::Select_type::None) {
    SetCursor(m_cursor_default);
  }
  else
  if (select == Layout::Select_type::Vertical) {
    SetCursor(m_cursor_vertical);
  }
  else
  if (select == Layout::Select_type::Horizontal) {
    SetCursor(m_cursor_horizontal);
  }
  else
  if (select == Layout::Select_type::Both) {
    SetCursor(m_cursor_both);
  }
}

LRESULT CALLBACK Application::window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  auto app_window = reinterpret_cast<Application*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

  if (app_window) {
    static bool is_tracking = false;
    switch (message) {
      case WM_LBUTTONDOWN: {
        app_window->splitter_select(hwnd, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
      } break;

      case WM_LBUTTONUP: {
        app_window->splitter_clear_selection();
      } break;

      case WM_MOUSEMOVE: {
        app_window->splitter_mouse_move(hwnd, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));

        if (!is_tracking) {
          auto tme = TRACKMOUSEEVENT{ sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
          is_tracking = (TrackMouseEvent(&tme) == TRUE);
        }
      } break;

      case WM_MOUSELEAVE: {
        app_window->set_cursor(Layout::Select_type::None);
        is_tracking = false;
      } break;

      case WM_SIZE: {
        app_window->update_layout_windows();
      } break;

      case WM_CLOSE: {
        PostQuitMessage(0);
      } break;
    }
  }
  return DefWindowProc(hwnd, message, wparam, lparam);
}
