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
#include <assert.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <memory>

#include "Layout.h"

static const int k_splitter_size = 6;
static const wchar_t* k_layout_child_wnd_class = L"layout_child_wnd";

static bool split_child_layout_strings(std::string const& input, std::string& ch1, std::string& ch2) {
  auto len = static_cast<int>(input.length());
  auto depth = 0;
  for (auto i = 0; i < len; i++) {
    if (input[i] == '{') {
      depth++;
    }
    else
    if (input[i] == '}') {
      depth--;
      if (depth == 0) {
        auto pivot = i + 1;
        assert(pivot < (len - 1));
        ch1 = input.substr(0, pivot);
        ch2 = input.substr(pivot + 1, len - pivot);
        return true;
      }
    }
  }
  return false;
}

static RECT shrink_rect(RECT const& rect, int padding) {
  auto padded = rect;
  padded.left += padding;
  padded.top += padding;
  padded.right -= padding;
  padded.bottom -= padding;
  return padded;
}

static bool split_layout_rect(Layout::Panel* current, RECT& cr1, RECT& cr2) {
  assert(current);
  cr1 = cr2 = current->rect;
  if (current->type == Layout::Panel_type::Splitter_vertical) {
    cr1.right = cr2.left = (cr1.left + current->splitter.position);
    return true;
  }
  else
  if (current->type == Layout::Panel_type::Splitter_horizontal) {
    cr1.bottom = cr2.top = (cr1.top + current->splitter.position);
    return true;
  }
  return false;
}

static void update_splitter_rects(Layout::Panel* current) {
  assert(current);
  auto rect = current->rect;
  auto splitter_offset = k_splitter_size / 2;
  if (current->type == Layout::Panel_type::Splitter_vertical) {
    auto pivot = rect.left + current->splitter.position;
    current->splitter.rect = rect;
    current->splitter.rect.left = pivot - splitter_offset;
    current->splitter.rect.right = pivot + splitter_offset;
  }
  else
  if (current->type == Layout::Panel_type::Splitter_horizontal) {
    auto pivot = rect.top + current->splitter.position;
    current->splitter.rect = rect;
    current->splitter.rect.top = pivot - splitter_offset;
    current->splitter.rect.bottom = pivot + splitter_offset;
  }
}

bool Layout::create_layout(Panel* current, std::string const& layout, RECT const& rect) {
  assert(current);
  auto slen = static_cast<int>(layout.length());
  if (slen < 3) { // minimum: T{}
    return false;
  }

  current->rect = rect;

  auto brace_s = 1;
  auto brace_e = slen - 1;
  auto brace_content_len = brace_e - brace_s - 1;

  auto is_valid = (layout[brace_s] == '{') && (layout[brace_e] == '}') && (brace_content_len > 0);
  if (!is_valid) {
    return false;
  }

  auto content = layout.substr(brace_s + 1, brace_content_len);
  auto type_id = layout.at(0);

  if (type_id == 'W') {
    current->type = Panel_type::Window;
    current->id = std::stoi(content);
    current->rect = shrink_rect(current->rect, k_splitter_size / 2);
    assert(m_panels.find(current->id) == m_panels.end());
    m_panels[current->id] = current;
    return true;
  }

  if (type_id == 'V') {
    current->type = Panel_type::Splitter_vertical;
    current->splitter.position = (rect.right - rect.left) / 2;
  }
  else
  if (type_id == 'H') {
    current->type = Panel_type::Splitter_horizontal;
    current->splitter.position = (rect.bottom - rect.top) / 2;
  }
  else {
    return false;
  }

  m_splitters.push_back(current);

  update_splitter_rects(current);

  auto ch1 = std::string{};
  auto ch2 = std::string{};

  if (!split_child_layout_strings(content, ch1, ch2)) {
    return false;
  }

  auto cr1 = RECT{};
  auto cr2 = RECT{};

  if (!split_layout_rect(current, cr1, cr2)) {
    return false;
  }

  current->children = std::make_pair(std::make_unique<Panel>(), std::make_unique<Panel>());
  if (!create_layout(current->children.first.get(), ch1, cr1)) {
    return false;
  }

  if (!create_layout(current->children.second.get(), ch2, cr2)) {
    return false;
  }
  return true;
}

bool Layout::update_layout(Panel* current, RECT const& rect) {
  assert(current);

  if (current->type == Layout::Panel_type::Window) {
    current->rect = shrink_rect(rect, k_splitter_size / 2);
    return true;
  }

  current->rect = rect;
  update_splitter_rects(current);

  auto cr1 = RECT{};
  auto cr2 = RECT{};

  if (!split_layout_rect(current, cr1, cr2)) {
    return false;
  }

  if (!update_layout(current->children.first.get(), cr1)) {
    return false;
  }

  if (!update_layout(current->children.second.get(), cr2)) {
    return false;
  }
  return true;
}

bool Layout::init(std::string const& layout, RECT const& rect) {
  return create_layout(&m_root_panel, layout, shrink_rect(rect, k_splitter_size));
}

bool Layout::update(RECT const& rect) {
  return update_layout(&m_root_panel, shrink_rect(rect, k_splitter_size));
}

static void splitter_find_indices(int x, int y, std::vector<Layout::Panel*> const& splitters, std::vector<int>& selected) {
  selected.clear();
  auto count = static_cast<int>(splitters.size());
  for (auto i = 0; i < count; i++) {
    auto const padding = k_splitter_size / 2; // selection padding to make shared intersections 'snap' more intuitively;
    auto rect = shrink_rect(splitters[i]->splitter.rect, -padding);
    if ((x >= rect.left) && (x <= rect.right) && (y >= rect.top) && (y <= rect.bottom)) {
      selected.push_back(i);
    }
  }
}

Layout::Select_type Layout::splitter_select(int x, int y, bool save_selected) {
  splitter_find_indices(x, y, m_splitters, m_selected_splitters);
  auto type = Layout::Select_type::None;
  if (!m_selected_splitters.empty()) {
    // sort out the selection type based on what matched;
    for (auto const& i : m_selected_splitters) {
      if (m_splitters[i]->type == Layout::Panel_type::Splitter_vertical) {
        type = (type == Layout::Select_type::Horizontal) ? Layout::Select_type::Both : Layout::Select_type::Vertical;
      }
      else
      if (m_splitters[i]->type == Layout::Panel_type::Splitter_horizontal) {
        type = (type == Layout::Select_type::Vertical) ? Layout::Select_type::Both : Layout::Select_type::Horizontal;
      }

      if (type == Layout::Select_type::Both) {
        break;
      }
    }

    if (!save_selected) {
      m_selected_splitters.clear();
    }
  }
  return type;
}

bool Layout::splitter_has_selected() const {
  return !m_selected_splitters.empty();
}

std::pair<int, int> Layout::get_splitter_boundaries(Layout::Panel* selected, RECT const& rect) {
  assert(selected);
  assert(selected->type != Layout::Panel_type::Window);

  // for a given splitter type, this determines a 'low' to 'high' range to restrict
  // the movement of the splitter beyond the region or other splitter boundaries.
  auto is_vertical = (selected->type == Layout::Panel_type::Splitter_vertical);
  auto low = k_splitter_size + (k_splitter_size / 2);
  auto high = static_cast<int>((is_vertical ? rect.right : rect.bottom) - low);
  auto splitter_pos = static_cast<int>(is_vertical ? selected->rect.left : selected->rect.top) + selected->splitter.position;

  auto splitter_count = static_cast<int>(m_splitters.size());
  for (int i = 0; i < splitter_count; i++) {
    auto current = m_splitters[i];
    if (selected == current) {
      continue;
    }

    // perform potential collision checks against splitters of equal type;
    if (current->type == selected->type) {
      auto const& compare_rect = current->splitter.rect;

      if (is_vertical) {
        if ((compare_rect.top < selected->splitter.rect.bottom) && (selected->splitter.rect.top < compare_rect.bottom)) {
          if (compare_rect.right < splitter_pos) {
            low = (std::max)(low, static_cast<int>(compare_rect.right));
          }

          if (compare_rect.left > splitter_pos) {
            high = (std::min)(high, static_cast<int>(compare_rect.left));
          }
        }
      }
      else {
        if ((compare_rect.left < selected->splitter.rect.right) && (selected->splitter.rect.left < compare_rect.right)) {
          if (compare_rect.bottom < splitter_pos) {
            low = (std::max)(low, static_cast<int>(compare_rect.bottom));
          }

          if (compare_rect.top > splitter_pos) {
            high = (std::min)(high, static_cast<int>(compare_rect.top));
          }
        }
      }
    }
  }
  return std::make_pair(low, high);
}

void Layout::splitter_update_selected(int x, int y, RECT const& window_rect) {
  if (!splitter_has_selected()) {
    return;
  }

  for (auto const& selected_index : m_selected_splitters) {
    auto selected = m_splitters[selected_index];

    auto is_vertical = (selected->type == Layout::Panel_type::Splitter_vertical);
    auto split_value = is_vertical ? x : y;
    auto split_offset = static_cast<int>(is_vertical ? -selected->rect.left : -selected->rect.top);

    auto boundary = get_splitter_boundaries(selected, window_rect);

    auto splitter_padding = k_splitter_size * 2;
    auto splitter_pos_prev = selected->splitter.position;
    selected->splitter.position = split_offset + (std::max)(boundary.first + splitter_padding, (std::min)(split_value, boundary.second - splitter_padding));

    // aesthetic preference: lock the position of the second child's splitter when the type is the same;
    if (selected->type == selected->children.second->type) {
      selected->children.second->splitter.position += (splitter_pos_prev - selected->splitter.position);
    }
  }
}

void Layout::splitter_clear_selected() {
  m_selected_splitters.clear();
}
