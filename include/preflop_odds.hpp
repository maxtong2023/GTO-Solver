#pragma once

#include "deck.hpp"

#include <cstdint>
#include <optional>
#include <string>

struct PreflopOddsResult {
  int trials = 0;
  int wins = 0;
  int ties = 0;
  int losses = 0;
};

bool parse_card_code(const std::string& code, Card* out_card);

PreflopOddsResult estimate_preflop_vs_random(const Card& hero_a,
                                             const Card& hero_b,
                                             int trials,
                                             std::uint32_t seed);
