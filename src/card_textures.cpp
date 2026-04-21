#include "card_textures.hpp"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace {

char suit_file_letter(Suit s) {
  switch (s) {
    case Suit::Clubs:
      return 'C';
    case Suit::Diamonds:
      return 'D';
    case Suit::Hearts:
      return 'H';
    case Suit::Spades:
      return 'S';
  }
  return 'C';
}

std::string rank_file_suffix(Rank r) {
  switch (r) {
    case Rank::Ace:
      return "A";
    case Rank::Two:
      return "2";
    case Rank::Three:
      return "3";
    case Rank::Four:
      return "4";
    case Rank::Five:
      return "5";
    case Rank::Six:
      return "6";
    case Rank::Seven:
      return "7";
    case Rank::Eight:
      return "8";
    case Rank::Nine:
      return "9";
    case Rank::Ten:
      return "10";
    case Rank::Jack:
      return "J";
    case Rank::Queen:
      return "Q";
    case Rank::King:
      return "K";
  }
  return "A";
}

bool load_png_rgba(const std::string& path, int* out_w, int* out_h,
                   std::vector<unsigned char>* rgba, std::string* err) {
  int w = 0;
  int h = 0;
  int channels = 0;
  stbi_set_flip_vertically_on_load(1);
  unsigned char* data =
      stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
  if (data == nullptr) {
    if (err != nullptr) {
      *err = std::string("Failed to load image: ") + path + " — " +
             (stbi_failure_reason() != nullptr ? stbi_failure_reason() : "");
    }
    return false;
  }
  *out_w = w;
  *out_h = h;
  rgba->assign(data, data + static_cast<std::size_t>(w * h * 4));
  stbi_image_free(data);
  return true;
}

std::uint32_t create_texture_rgba(int w, int h, const unsigned char* rgba) {
  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               rgba);
  glBindTexture(GL_TEXTURE_2D, 0);
  return static_cast<std::uint32_t>(tex);
}

}  // namespace

std::size_t CardTextures::card_index(Card c) {
  const auto s = static_cast<std::size_t>(c.suit);
  const auto r = static_cast<std::size_t>(c.rank);
  return s * 13U + r;
}

CardTextures::~CardTextures() { destroy_gl(); }

void CardTextures::destroy_gl() {
  for (std::uint32_t id : face_textures_) {
    if (id != 0U) {
      GLuint tex = static_cast<GLuint>(id);
      glDeleteTextures(1, &tex);
    }
  }
  face_textures_.fill(0U);
  if (back_texture_ != 0U) {
    GLuint tex = static_cast<GLuint>(back_texture_);
    glDeleteTextures(1, &tex);
    back_texture_ = 0U;
  }
  loaded_ = false;
  face_width_ = 0;
  face_height_ = 0;
  back_width_ = 0;
  back_height_ = 0;
}

bool CardTextures::load_from_directory(const std::string& dir,
                                       std::string* error_message) {
  destroy_gl();

  namespace fs = std::filesystem;
  const fs::path root(dir);
  if (!fs::is_directory(root)) {
    if (error_message != nullptr) {
      *error_message = "Not a directory: " + dir;
    }
    return false;
  }

  std::vector<unsigned char> rgba;
  std::string err;

  for (std::uint8_t si = 0; si < 4; ++si) {
    for (std::uint8_t ri = 0; ri < 13; ++ri) {
      const Card c{static_cast<Rank>(ri), static_cast<Suit>(si)};
      const fs::path path =
          root / (std::string(1, suit_file_letter(c.suit)) +
                  rank_file_suffix(c.rank) + ".png");
      int w = 0;
      int h = 0;
      if (!load_png_rgba(path.string(), &w, &h, &rgba, &err)) {
        destroy_gl();
        if (error_message != nullptr) {
          *error_message = err;
        }
        return false;
      }
      if (face_width_ == 0) {
        face_width_ = w;
        face_height_ = h;
      } else if (w != face_width_ || h != face_height_) {
        destroy_gl();
        if (error_message != nullptr) {
          *error_message = "Face image size mismatch: " + path.string();
        }
        return false;
      }
      face_textures_[card_index(c)] =
          create_texture_rgba(w, h, rgba.data());
    }
  }

  const fs::path back_candidates[] = {root / "back.png",
                                       root / "back_dark.png"};
  back_texture_ = 0U;
  back_width_ = 0;
  back_height_ = 0;
  for (const fs::path& back_path : back_candidates) {
    int bw = 0;
    int bh = 0;
    if (fs::is_regular_file(back_path) &&
        load_png_rgba(back_path.string(), &bw, &bh, &rgba, &err)) {
      back_texture_ = create_texture_rgba(bw, bh, rgba.data());
      back_width_ = bw;
      back_height_ = bh;
      break;
    }
  }

  loaded_ = true;
  return true;
}

ImTextureID CardTextures::texture_for(Card c) const {
  if (!loaded_) {
    return static_cast<ImTextureID>(0);
  }
  const std::uint32_t id = face_textures_[card_index(c)];
  return static_cast<ImTextureID>(static_cast<std::uintptr_t>(id));
}

ImTextureID CardTextures::back_texture() const {
  if (!loaded_ || back_texture_ == 0U) {
    return static_cast<ImTextureID>(0);
  }
  return static_cast<ImTextureID>(static_cast<std::uintptr_t>(back_texture_));
}
