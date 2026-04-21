#pragma once

#include "deck.hpp"

#include <imgui.h>

#include <array>
#include <cstdint>
#include <string>

class CardTextures {
 public:
  CardTextures() = default;
  CardTextures(const CardTextures&) = delete;
  CardTextures& operator=(const CardTextures&) = delete;
  CardTextures(CardTextures&&) = delete;
  CardTextures& operator=(CardTextures&&) = delete;
  ~CardTextures();

  bool load_from_directory(const std::string& dir, std::string* error_message);

  ImTextureID texture_for(Card c) const;
  ImTextureID back_texture() const;
  bool has_back() const { return back_texture_ != 0U; }

  int face_width() const { return face_width_; }
  int face_height() const { return face_height_; }
  int back_width() const { return back_width_; }
  int back_height() const { return back_height_; }

 private:
  static std::size_t card_index(Card c);
  void destroy_gl();

  std::array<std::uint32_t, 52> face_textures_{};
  std::uint32_t back_texture_ = 0;
  int face_width_ = 0;
  int face_height_ = 0;
  int back_width_ = 0;
  int back_height_ = 0;
  bool loaded_ = false;
};
