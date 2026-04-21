#include "deck.hpp"

#include <algorithm>
#include <iostream>
#include <random>
#include <string>

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

std::string card_string(const Card& c) {
  return rank_string(c.rank) + " of " + suit_string(c.suit);
}

}  // namespace

Deck::Deck() {
  cards_.reserve(52);
  for (std::uint8_t s = 0; s < 4; ++s) {
    for (std::uint8_t r = 0; r < 13; ++r) {
      cards_.push_back(Card{static_cast<Rank>(r), static_cast<Suit>(s)});
    }
  }
}

void Deck::shuffle() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::shuffle(cards_.begin(), cards_.end(), gen);
}

void Deck::deal() {
  if (cards_.size() < 2) {
    std::cout << "Not enough cards to deal.\n";
    return;
  }

  const Card first = cards_[cards_.size() - 1];
  const Card second = cards_[cards_.size() - 2];
  cards_.pop_back();
  cards_.pop_back();

  std::cout << card_string(first) << ", " << card_string(second) << '\n';
}
