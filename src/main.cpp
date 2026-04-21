#include "card_textures.hpp"
#include "deck.hpp"
#include "preflop_odds.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
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

std::random_device rd;
std::mt19937 gen(rd());

void glfw_error_callback(int error, const char* description) {
  std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

char suit_code(Suit suit) {
  switch (suit) {
    case Suit::Clubs:
      return 'c';
    case Suit::Diamonds:
      return 'd';
    case Suit::Hearts:
      return 'h';
    case Suit::Spades:
      return 's';
  }
  return 'c';
}

std::string rank_code(Rank rank) {
  switch (rank) {
    case Rank::Ace:
      return "A";
    case Rank::King:
      return "K";
    case Rank::Queen:
      return "Q";
    case Rank::Jack:
      return "J";
    case Rank::Ten:
      return "T";
    case Rank::Nine:
      return "9";
    case Rank::Eight:
      return "8";
    case Rank::Seven:
      return "7";
    case Rank::Six:
      return "6";
    case Rank::Five:
      return "5";
    case Rank::Four:
      return "4";
    case Rank::Three:
      return "3";
    case Rank::Two:
      return "2";
  }
  return "?";
}

std::string card_code(const Card& card) {
  return rank_code(card.rank) + suit_code(card.suit);
}

ImVec2 fitted_size(int w, int h, float max_h) {
  if (w <= 0 || h <= 0) {
    return ImVec2(140.0F, 200.0F);
  }
  const float hh = max_h;
  const float ww = hh * (static_cast<float>(w) / static_cast<float>(h));
  return ImVec2(ww, hh);
}

// Flip U so card art matches source PNGs (OpenGL/ImGui default sampling was mirrored).
constexpr ImVec2 kCardUv0(1.0F, 0.0F);
constexpr ImVec2 kCardUv1(0.0F, 1.0F);

void image_card_texture(ImTextureID tex, const ImVec2& size) {
  ImGui::Image(tex, size, kCardUv0, kCardUv1);
}

void append_mc_benchmark_tsv(const std::string& tank_dir, int trials,
                             const std::string& hero1, const std::string& hero2,
                             std::size_t deck_remaining, double elapsed_ms,
                             const PreflopOddsResult& r) {
  namespace fs = std::filesystem;
  const fs::path dir(tank_dir);
  std::error_code ec;
  fs::create_directories(dir, ec);
  if (ec) {
    return;
  }
  const fs::path file = dir / "mc_preflop.tsv";
  bool need_header = true;
  if (fs::exists(file)) {
    const auto sz = fs::file_size(file, ec);
    need_header = static_cast<bool>(ec) || sz == 0;
  }
  std::ofstream out(file, std::ios::out | std::ios::app);
  if (!out) {
    return;
  }
  if (need_header) {
    out << "# trials\thero1\thero2\tdeck_remaining\telapsed_ms\twin_pct\t"
           "tie_pct\tlose_pct\ttrials_per_s\n";
  }
  const double denom = static_cast<double>(r.trials);
  const double win_pct = 100.0 * static_cast<double>(r.wins) / denom;
  const double tie_pct = 100.0 * static_cast<double>(r.ties) / denom;
  const double lose_pct = 100.0 * static_cast<double>(r.losses) / denom;
  const double trials_per_s =
      elapsed_ms > 0.0 ? (denom / (elapsed_ms / 1000.0)) : 0.0;
  out << trials << '\t' << hero1 << '\t' << hero2 << '\t' << deck_remaining
      << '\t' << elapsed_ms << '\t' << win_pct << '\t' << tie_pct << '\t'
      << lose_pct << '\t' << trials_per_s << '\n';
}

void draw_card_back_placeholder(const ImVec2& size) {
  const ImVec2 p0 = ImGui::GetCursorScreenPos();
  const ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);
  ImDrawList* const dl = ImGui::GetWindowDrawList();
  dl->AddRectFilled(p0, p1, IM_COL32(36, 44, 58, 255), 6.0F);
  dl->AddRect(p0, p1, IM_COL32(90, 100, 120, 255), 6.0F, 0, 2.0F);
  const char* label = "Back";
  const ImVec2 ts = ImGui::CalcTextSize(label);
  dl->AddText(ImVec2(p0.x + (size.x - ts.x) * 0.5F, p0.y + (size.y - ts.y) * 0.5F),
              IM_COL32(210, 215, 230, 255), label);
  ImGui::Dummy(size);
}

}  // namespace

#ifndef POCKER_ASSETS_DIR
#define POCKER_ASSETS_DIR "assets"
#endif

#ifndef POCKER_TANK_DIR
#define POCKER_TANK_DIR "tank"
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
    deck.shuffle(gen);

    char status[256] = "Click Deal for two cards. Shuffle refills to 52.";
    std::optional<std::pair<Card, Card>> last_hole;
    char hero_card_a_input[8] = "As";
    char hero_card_b_input[8] = "Ah";
    int monte_carlo_trials = 20000;
    char odds_status[256] = "Enter hero hand and click Estimate Odds.";
    PreflopOddsResult odds_result{};
    bool has_odds_result = false;
    double last_mc_ms = 0.0;
    const std::string tank_dir(POCKER_TANK_DIR);

    auto run_odds_calc = [&](const Card& hero_a, const Card& hero_b,
                             std::size_t deck_remaining, bool log_to_tank) {
      const auto t0 = std::chrono::steady_clock::now();
      odds_result = estimate_preflop_vs_random(
          hero_a, hero_b, monte_carlo_trials, std::random_device{}());
      const auto t1 = std::chrono::steady_clock::now();
      const double elapsed_ms =
          std::chrono::duration<double, std::milli>(t1 - t0).count();
      last_mc_ms = elapsed_ms;
      has_odds_result = true;
      std::snprintf(
          odds_status, sizeof(odds_status),
          "Estimated with %d trials in %.2f ms (%.0f trials/s).",
          odds_result.trials, elapsed_ms,
          elapsed_ms > 0.0
              ? static_cast<double>(odds_result.trials) / (elapsed_ms / 1000.0)
              : 0.0);
      if (log_to_tank) {
        append_mc_benchmark_tsv(tank_dir, monte_carlo_trials,
                                card_code(hero_a), card_code(hero_b),
                                deck_remaining, elapsed_ms, odds_result);
      }
    };

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
        ImGui::TextUnformatted("Expected PNGs like CA.png, H10.png in assets/.");
      }

      if (ImGui::Button("Shuffle", ImVec2(140.0F, 0.0F))) {
        deck.shuffle(gen);
        last_hole.reset();
        std::snprintf(status, sizeof(status),
                      "Shuffled full deck. %zu cards.", deck.remaining());
      }

      ImGui::SameLine();

      if (ImGui::Button("Deal", ImVec2(140.0F, 0.0F))) {
        const auto dealt = deck.deal();
        if (dealt.has_value()) {
          last_hole = *dealt;
          const std::string code_a = card_code(last_hole->first);
          const std::string code_b = card_code(last_hole->second);
          std::snprintf(hero_card_a_input, sizeof(hero_card_a_input), "%s",
                        code_a.c_str());
          std::snprintf(hero_card_b_input, sizeof(hero_card_b_input), "%s",
                        code_b.c_str());
          const std::size_t remaining_after_deal = deck.remaining();
          run_odds_calc(last_hole->first, last_hole->second, remaining_after_deal,
                        true);
          std::snprintf(status, sizeof(status), "%zu cards left in deck.",
                        remaining_after_deal);
        } else {
          last_hole.reset();
          has_odds_result = false;
          std::snprintf(odds_status, sizeof(odds_status),
                        "Not enough cards to assign hero hand.");
          std::snprintf(status, sizeof(status),
                        "Not enough cards — press Shuffle.");
        }
      }

      ImGui::Separator();
      ImGui::TextUnformatted("Preflop Monte Carlo (heads-up vs random)");
      ImGui::SetNextItemWidth(80.0F);
      ImGui::InputText("Card 1", hero_card_a_input, sizeof(hero_card_a_input));
      ImGui::SameLine();
      ImGui::SetNextItemWidth(80.0F);
      ImGui::InputText("Card 2", hero_card_b_input, sizeof(hero_card_b_input));
      ImGui::SetNextItemWidth(140.0F);
      ImGui::InputInt("Trials", &monte_carlo_trials, 1000, 10000);
      if (monte_carlo_trials < 1000) {
        monte_carlo_trials = 1000;
      }
      if (ImGui::Button("Estimate Odds", ImVec2(180.0F, 0.0F))) {
        Card hero_a{};
        Card hero_b{};
        const bool parsed_a =
            parse_card_code(std::string(hero_card_a_input), &hero_a);
        const bool parsed_b =
            parse_card_code(std::string(hero_card_b_input), &hero_b);
        if (!parsed_a || !parsed_b) {
          std::snprintf(odds_status, sizeof(odds_status),
                        "Use codes like As, Kh, Td, 9c.");
          has_odds_result = false;
        } else if (hero_a.rank == hero_b.rank && hero_a.suit == hero_b.suit) {
          std::snprintf(odds_status, sizeof(odds_status),
                        "Card 1 and Card 2 must be different.");
          has_odds_result = false;
        } else {
          run_odds_calc(hero_a, hero_b, deck.remaining(), true);
        }
      }
      
      ImGui::TextWrapped("%s", odds_status);
      if (last_mc_ms > 0.0) {
        ImGui::TextDisabled("Last MC wall time: %.2f ms", last_mc_ms);
      }
      if (has_odds_result && odds_result.trials > 0) {
        const float denom = static_cast<float>(odds_result.trials);
        const float win_pct = 100.0F * static_cast<float>(odds_result.wins) / denom;
        const float tie_pct = 100.0F * static_cast<float>(odds_result.ties) / denom;
        const float lose_pct =
            100.0F * static_cast<float>(odds_result.losses) / denom;
        ImGui::Text("Win: %.2f%%", win_pct);
        ImGui::SameLine();
        ImGui::Text("Tie: %.2f%%", tie_pct);
        ImGui::SameLine();
        ImGui::Text("Lose: %.2f%%", lose_pct);
      }

      ImGui::Separator();
      ImGui::Text("%s", status);
      ImGui::Spacing();

      const float max_card_h = 280.0F;
      const ImVec2 face_sz = fitted_size(textures.face_width(),
                                         textures.face_height(), max_card_h);
      const ImVec2 back_sz =
          textures.has_back()
              ? fitted_size(textures.back_width(), textures.back_height(),
                            max_card_h)
              : face_sz;

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
            image_card_texture(textures.texture_for(last_hole->first), face_sz);
            ImGui::TextWrapped("%s",
                               format_card(last_hole->first).c_str());
          } else if (textures.has_back()) {
            image_card_texture(textures.back_texture(), back_sz);
          } else {
            draw_card_back_placeholder(face_sz);
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
            image_card_texture(textures.texture_for(last_hole->second), face_sz);
            ImGui::TextWrapped("%s",
                               format_card(last_hole->second).c_str());
          } else if (textures.has_back()) {
            image_card_texture(textures.back_texture(), back_sz);
          } else {
            draw_card_back_placeholder(face_sz);
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
