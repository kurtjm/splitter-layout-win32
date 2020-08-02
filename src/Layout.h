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

class Layout {
public:
  
  Layout() {}

  Layout(Layout const&) = delete;

  Layout& operator=(Layout const&) = delete;

  ~Layout() {}

  bool init(std::string const& layout, RECT const& rect);

  bool update(RECT const& rect);
    
  enum class Panel_type {
    Window,
    Splitter_vertical,
    Splitter_horizontal
  };

  struct Splitter_properties {
    RECT rect = {};
    int position = {};
  };

  struct Panel {
    int id = {};
    Panel_type type = {};
    RECT rect = {};
    Splitter_properties splitter = {};

    std::pair<std::unique_ptr<Panel>, std::unique_ptr<Panel>> children = {};
  };

  enum class Select_type {
    None,
    Vertical,
    Horizontal,
    Both,
  };
  
  Select_type splitter_select(int x, int y, bool save_selected);
  
  bool splitter_has_selected() const;

  void splitter_update_selected(int x, int y, RECT const& rect);

  void splitter_clear_selected();

  const std::unordered_map<int, Panel*>& panels() const { return m_panels; };

private:

  bool create_layout(Panel* current, std::string const& layout, RECT const& rect);

  bool update_layout(Panel* current, RECT const& rect);

  std::pair<int, int> get_splitter_boundaries(Layout::Panel* splitter, RECT const& rect);

  std::unordered_map<int, Panel*> m_panels;

  std::vector<Panel*> m_splitters;

  std::vector<int> m_selected_splitters;

  Panel m_root_panel;
};
