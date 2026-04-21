#pragma once

#include <cstdint>
#include <random>
#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class Suit : std::uint8_t { Clubs, Diamonds, Hearts, Spades };

enum class Rank : std::uint8_t {
  Ace,
  Two,
  Three,
  Four,
  Five,
  Six,
  Seven,
  Eight,
  Nine,
  Ten,
  Jack,
  Queen,
  King
};

struct Card {
  Rank rank;
  Suit suit;
};

class Deck {
 public:
  Deck();
  void shuffle(std::mt19937& gen);
  void yates_shuffle(std::mt19937& gen);
  std::optional<std::pair<Card, Card>> deal();
  std::size_t remaining() const;

 private:
  void refill_ordered();
  std::vector<Card> cards_;
};

std::string format_card(const Card& c);
