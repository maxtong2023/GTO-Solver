#include "deck.hpp"

#include <algorithm>
#include <random>

namespace {

std::string rank_string(Rank r) {
  switch (r) {
    case Rank::Ace:
      return "Ace";
    case Rank::Two:
      return "Two";
    case Rank::Three:
      return "Three";
    case Rank::Four:
      return "Four";
    case Rank::Five:
      return "Five";
    case Rank::Six:
      return "Six";
    case Rank::Seven:
      return "Seven";
    case Rank::Eight:
      return "Eight";
    case Rank::Nine:
      return "Nine";
    case Rank::Ten:
      return "Ten";
    case Rank::Jack:
      return "Jack";
    case Rank::Queen:
      return "Queen";
    case Rank::King:
      return "King";
  }
  return "?";
}

std::string suit_string(Suit s) {
  switch (s) {
    case Suit::Clubs:
      return "Clubs";
    case Suit::Diamonds:
      return "Diamonds";
    case Suit::Hearts:
      return "Hearts";
    case Suit::Spades:
      return "Spades";
  }
  return "?";
}

}  // namespace

std::string format_card(const Card& c) {
  return rank_string(c.rank) + " of " + suit_string(c.suit);
}
//optimize by just hardcoding the 52 cards in a vector
void Deck::refill_ordered() {
  cards_.clear();
  cards_.reserve(52);
  for (std::uint8_t s = 0; s < 4; ++s) {
    for (std::uint8_t r = 0; r < 13; ++r) {
      cards_.push_back(Card{static_cast<Rank>(r), static_cast<Suit>(s)});
    }
  }
}



Deck::Deck() { refill_ordered(); }

void Deck::shuffle(std::mt19937& gen) {
  refill_ordered();
  std::shuffle(cards_.begin(), cards_.end(), gen);
}


//alternative shuffler that just returns the 7 required cards.
void Deck::yates_shuffle(std::mt19937& gen) {
  refill_ordered();
  for (std::size_t i = 0; i < 7; ++i){
    std::uniform_int_distribution<std::size_t> dist(i, cards_.size() - 1);
    std::size_t j = dist(gen);
    std::swap(cards_[i], cards_[j]);
  }

}

std::optional<std::pair<Card, Card>> Deck::deal() {
  if (cards_.size() < 2) {
    return std::nullopt;
  }

  const Card first = cards_.back();
  cards_.pop_back();
  const Card second = cards_.back();
  cards_.pop_back();
  return std::make_pair(first, second);
}

std::size_t Deck::remaining() const { return cards_.size(); }
