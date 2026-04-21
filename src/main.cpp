#include "card_textures.hpp"
#include "deck.hpp"

#include <optional>
#include <string>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cstdio>

#include <GLFW/glfw3.h>

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

namespace {

void glfw_error_callback(int error, const char* description) {
  std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

ImVec2 fitted_size(int w, int h, float max_h) {
  if (w <= 0 || h <= 0) {
    return ImVec2(140.0F, 200.0F);
  }
  const float hh = max_h;
  const float ww = hh * (static_cast<float>(w) / static_cast<float>(h));
  return ImVec2(ww, hh);
}

}  // namespace

#ifndef POCKER_ASSETS_DIR
#define POCKER_ASSETS_DIR "assets"
#endif

int main() {
  glfwSetErrorCallback(glfw_error_callback);
  if (glfwInit() == 0) {
    return 1;
  }

  const char* glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

  GLFWwindow* window =
      glfwCreateWindow(800, 600, "Poker — deck", nullptr, nullptr);
  if (window == nullptr) {
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  {
    CardTextures textures;
    std::string load_error;
    const std::string assets_dir(POCKER_ASSETS_DIR);
    const bool textures_ok =
        textures.load_from_directory(assets_dir, &load_error);

    Deck deck;
    deck.shuffle();

    char status[256] = "Click Deal for two cards. Shuffle refills to 52.";
    std::optional<std::pair<Card, Card>> last_hole;

    while (glfwWindowShouldClose(window) == 0) {
      glfwPollEvents();

      int display_w = 0;
      int display_h = 0;
      glfwGetFramebufferSize(window, &display_w, &display_h);

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      ImGui::SetNextWindowPos(ImVec2(0.0F, 0.0F), ImGuiCond_Always);
      ImGui::SetNextWindowSize(ImVec2(static_cast<float>(display_w),
                                      static_cast<float>(display_h)),
                               ImGuiCond_Always);

      ImGuiWindowFlags window_flags =
          ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus;

      ImGui::Begin("Deck", nullptr, window_flags);

      if (!textures_ok) {
        ImGui::TextColored(ImVec4(1.0F, 0.45F, 0.45F, 1.0F), "%s",
                           load_error.c_str());
        ImGui::TextUnformatted("Expected PNGs like clubs_A.png in assets/.");
      }

      if (ImGui::Button("Shuffle", ImVec2(140.0F, 0.0F))) {
        deck.shuffle();
        last_hole.reset();
        std::snprintf(status, sizeof(status),
                      "Shuffled full deck. %zu cards.", deck.remaining());
      }

      ImGui::SameLine();

      if (ImGui::Button("Deal", ImVec2(140.0F, 0.0F))) {
        const auto dealt = deck.deal();
        if (dealt.has_value()) {
          last_hole = *dealt;
          std::snprintf(status, sizeof(status), "%zu cards left in deck.",
                        deck.remaining());
        } else {
          last_hole.reset();
          std::snprintf(status, sizeof(status),
                        "Not enough cards — press Shuffle.");
        }
      }

      ImGui::Separator();
      ImGui::Text("%s", status);
      ImGui::Spacing();

      const float max_card_h = 280.0F;
      const ImVec2 face_sz = fitted_size(textures.face_width(),
                                         textures.face_height(), max_card_h);
      const ImVec2 back_sz = fitted_size(textures.back_width(),
                                         textures.back_height(), max_card_h);

      if (ImGui::BeginTable(
              "cards", 2,
              ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableNextColumn();
        ImGui::BeginChild("card_a",
                          ImVec2(face_sz.x + 24.0F, max_card_h + 40.0F), true,
                          ImGuiWindowFlags_None);
        ImGui::TextUnformatted("Card A");
        ImGui::Spacing();
        if (textures_ok) {
          if (last_hole.has_value()) {
            ImGui::Image(textures.texture_for(last_hole->first), face_sz);
            ImGui::TextWrapped("%s",
                               format_card(last_hole->first).c_str());
          } else {
            ImGui::Image(textures.back_texture(), back_sz);
          }
        }
        ImGui::EndChild();

        ImGui::TableNextColumn();
        ImGui::BeginChild("card_b",
                          ImVec2(face_sz.x + 24.0F, max_card_h + 40.0F), true,
                          ImGuiWindowFlags_None);
        ImGui::TextUnformatted("Card B");
        ImGui::Spacing();
        if (textures_ok) {
          if (last_hole.has_value()) {
            ImGui::Image(textures.texture_for(last_hole->second), face_sz);
            ImGui::TextWrapped("%s",
                               format_card(last_hole->second).c_str());
          } else {
            ImGui::Image(textures.back_texture(), back_sz);
          }
        }
        ImGui::EndChild();

        ImGui::EndTable();
      }

      ImGui::End();

      ImGui::Render();
      glViewport(0, 0, display_w, display_h);
      glClearColor(0.12F, 0.12F, 0.14F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      glfwSwapBuffers(window);
    }
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
