#pragma once

#include <cstdint>
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
  void shuffle();
  void deal();

 private:
  std::vector<Card> cards_;
};
