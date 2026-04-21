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

using CardId = std::uint8_t;

inline CardId make_card_id(Rank r, Suit s) {
  return static_cast<CardId>(static_cast<unsigned>(s) * 13U + static_cast<unsigned>(r));
}

inline CardId make_card_id(const Card& c){
  return make_card_id(c.rank, c.suit);

}
inline Card card_from_id(CardId id){
  return Card{static_cast<Rank>(id % 13), static_cast<Suit>(id / 13)};
}

class Deck {
 public:
  Deck();
  void shuffle(std::mt19937& gen);
  void yates_shuffle(std::mt19937& gen);
  std::optional<std::pair<Card, Card>> deal();
  std::size_t remaining() const;

 private:
  void refill_ordered();
  std::vector<CardId> cards_;
};

std::string format_card(const Card& c);
