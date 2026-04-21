#include "preflop_odds.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <random>
#include <vector>
#include <cstdint>

namespace {
  constexpr std::array<int, 13> kPokerRankFromRankIndex = {
    14, 2,3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13
  };

  inline int poker_rank_from_id(std::uint8_t id){
    return kPokerRankFromRankIndex[id % 13];
  }
  inline int suit_index_from_id(std::uint8_t id){
    return static_cast<int>(id / 13);
  }


void partial_fisher_yates_prefix(std::vector<uint8_t>& cards, std::size_t k,
                                 std::mt19937& gen) {
  const std::size_t n = cards.size();
  if (n == 0 || k == 0) {
    return;
  }
  const std::size_t kk = std::min(k, n);
  for (std::size_t i = 0; i < kk; ++i) {
    std::uniform_int_distribution<std::size_t> dist(i, n - 1);
    std::swap(cards[i], cards[dist(gen)]);
  }
}

int rank_to_value(Rank rank) {
  switch (rank) {
    case Rank::Ace:
      return 14;
    case Rank::Two:
      return 2;
    case Rank::Three:
      return 3;
    case Rank::Four:
      return 4;
    case Rank::Five:
      return 5;
    case Rank::Six:
      return 6;
    case Rank::Seven:
      return 7;
    case Rank::Eight:
      return 8;
    case Rank::Nine:
      return 9;
    case Rank::Ten:
      return 10;
    case Rank::Jack:
      return 11;
    case Rank::Queen:
      return 12;
    case Rank::King:
      return 13;
  }
  return 0;
}

int suit_to_index(Suit suit) {
  switch (suit) {
    case Suit::Clubs:
      return 0;
    case Suit::Diamonds:
      return 1;
    case Suit::Hearts:
      return 2;
    case Suit::Spades:
      return 3;
  }
  return 0;
}

struct HandValue {
  int category = 0;
  std::array<int, 5> tiebreakers{};
};

int compare_hand_value(const HandValue& a, const HandValue& b) {
  if (a.category != b.category) {
    return a.category < b.category ? -1 : 1;
  }
  for (std::size_t i = 0; i < a.tiebreakers.size(); ++i) {
    if (a.tiebreakers[i] != b.tiebreakers[i]) {
      return a.tiebreakers[i] < b.tiebreakers[i] ? -1 : 1;
    }
  }
  return 0;
}

int highest_straight_high(std::array<bool, 15> present) {
  if (present[14]) {
    present[1] = true;  // Wheel: A-2-3-4-5
  }
  for (int high = 14; high >= 5; --high) {
    if (present[high] && present[high - 1] && present[high - 2] &&
        present[high - 3] && present[high - 4]) {
      return high;
    }
  }
  return 0;
}

HandValue evaluate_five(const std::array<std::uint8_t, 5>& cards) {
  std::array<int, 15> rank_count{};
  std::array<int, 4> suit_count{};
  std::array<bool, 15> rank_present{};
  std::vector<int> ranks_desc;
  ranks_desc.reserve(5);

  for (const std::uint8_t& id : cards) {
    const int rv = poker_rank_from_id(id);
    ++rank_count[rv];
    rank_present[rv] = true;
    ++suit_count[suit_index_from_id(id)];
    ranks_desc.push_back(rv);
  }
  std::sort(ranks_desc.begin(), ranks_desc.end(), std::greater<int>());

  bool flush = false;
  int flush_suit = -1;
  for (int s = 0; s < 4; ++s) {
    if (suit_count[s] == 5) {
      flush = true;
      flush_suit = s;
      break;
    }
  }

  const int straight_high = highest_straight_high(rank_present);

  if (flush) {
    std::array<bool, 15> flush_present{};
    for (const std::uint8_t& id : cards) {
      if (suit_index_from_id(id) == flush_suit) {
        flush_present[poker_rank_from_id(id)] = true;
      }
    }
    const int sf_high = highest_straight_high(flush_present);
    if (sf_high > 0) {
      HandValue hv;
      hv.category = 8;  // Straight flush
      hv.tiebreakers[0] = sf_high;
      return hv;
    }
  }

  int four_rank = 0;
  int three_rank = 0;
  std::vector<int> pair_ranks;
  pair_ranks.reserve(2);

  for (int r = 14; r >= 2; --r) {
    if (rank_count[r] == 4) {
      four_rank = r;
    } else if (rank_count[r] == 3) {
      if (three_rank == 0) {
        three_rank = r;
      }
    } else if (rank_count[r] == 2) {
      pair_ranks.push_back(r);
    }
  }

  if (four_rank > 0) {
    int kicker = 0;
    for (int r = 14; r >= 2; --r) {
      if (r != four_rank && rank_count[r] > 0) {
        kicker = r;
        break;
      }
    }
    HandValue hv;
    hv.category = 7;
    hv.tiebreakers = {four_rank, kicker, 0, 0, 0};
    return hv;
  }

  if (three_rank > 0 && !pair_ranks.empty()) {
    HandValue hv;
    hv.category = 6;
    hv.tiebreakers = {three_rank, pair_ranks[0], 0, 0, 0};
    return hv;
  }

  if (flush) {
    HandValue hv;
    hv.category = 5;
    for (std::size_t i = 0; i < ranks_desc.size(); ++i) {
      hv.tiebreakers[i] = ranks_desc[i];
    }
    return hv;
  }

  if (straight_high > 0) {
    HandValue hv;
    hv.category = 4;
    hv.tiebreakers[0] = straight_high;
    return hv;
  }

  if (three_rank > 0) {
    std::vector<int> kickers;
    for (int r = 14; r >= 2; --r) {
      if (r != three_rank && rank_count[r] > 0) {
        kickers.push_back(r);
      }
    }
    HandValue hv;
    hv.category = 3;
    hv.tiebreakers = {three_rank, kickers[0], kickers[1], 0, 0};
    return hv;
  }

  if (pair_ranks.size() >= 2) {
    int kicker = 0;
    for (int r = 14; r >= 2; --r) {
      if (rank_count[r] == 1) {
        kicker = r;
        break;
      }
    }
    HandValue hv;
    hv.category = 2;
    hv.tiebreakers = {pair_ranks[0], pair_ranks[1], kicker, 0, 0};
    return hv;
  }

  if (pair_ranks.size() == 1) {
    std::vector<int> kickers;
    for (int r = 14; r >= 2; --r) {
      if (rank_count[r] == 1) {
        kickers.push_back(r);
      }
    }
    HandValue hv;
    hv.category = 1;
    hv.tiebreakers = {pair_ranks[0], kickers[0], kickers[1], kickers[2], 0};
    return hv;
  }

  HandValue hv;
  hv.category = 0;
  for (std::size_t i = 0; i < ranks_desc.size(); ++i) {
    hv.tiebreakers[i] = ranks_desc[i];
  }
  return hv;
}

HandValue evaluate_seven(const std::array<std::uint8_t, 7>& cards) {
  HandValue best;
  bool initialized = false;
  for (int a = 0; a < 7; ++a) {
    for (int b = a + 1; b < 7; ++b) {
      for (int c = b + 1; c < 7; ++c) {
        for (int d = c + 1; d < 7; ++d) {
          for (int e = d + 1; e < 7; ++e) {
            std::array<std::uint8_t, 5> five = {
                cards[a], cards[b], cards[c], cards[d], cards[e]};
            const HandValue hv = evaluate_five(five);
            if (!initialized || compare_hand_value(hv, best) > 0) {
              best = hv;
              initialized = true;
            }
          }
        }
      }
    }
  }
  return best;
}

bool card_ids_equal(std::uint8_t a, std::uint8_t b) {
  return a == b;
}

}  // namespace

bool parse_card_code(const std::string& code, Card* out_card) {
  if (out_card == nullptr) {
    return false;
  }
  if (code.size() < 2 || code.size() > 3) {
    return false;
  }

  const char suit_ch = static_cast<char>(std::tolower(code.back()));
  Suit suit = Suit::Clubs;
  switch (suit_ch) {
    case 'c':
      suit = Suit::Clubs;
      break;
    case 'd':
      suit = Suit::Diamonds;
      break;
    case 'h':
      suit = Suit::Hearts;
      break;
    case 's':
      suit = Suit::Spades;
      break;
    default:
      return false;
  }

  const std::string rank_str = code.substr(0, code.size() - 1);
  Rank rank = Rank::Ace;
  if (rank_str == "A" || rank_str == "a") {
    rank = Rank::Ace;
  } else if (rank_str == "K" || rank_str == "k") {
    rank = Rank::King;
  } else if (rank_str == "Q" || rank_str == "q") {
    rank = Rank::Queen;
  } else if (rank_str == "J" || rank_str == "j") {
    rank = Rank::Jack;
  } else if (rank_str == "T" || rank_str == "t" || rank_str == "10") {
    rank = Rank::Ten;
  } else if (rank_str == "9") {
    rank = Rank::Nine;
  } else if (rank_str == "8") {
    rank = Rank::Eight;
  } else if (rank_str == "7") {
    rank = Rank::Seven;
  } else if (rank_str == "6") {
    rank = Rank::Six;
  } else if (rank_str == "5") {
    rank = Rank::Five;
  } else if (rank_str == "4") {
    rank = Rank::Four;
  } else if (rank_str == "3") {
    rank = Rank::Three;
  } else if (rank_str == "2") {
    rank = Rank::Two;
  } else {
    return false;
  }

  *out_card = Card{rank, suit};
  return true;
}

PreflopOddsResult estimate_preflop_vs_random(const Card& hero_a,
                                             const Card& hero_b,
                                             int trials,
                                             std::uint32_t seed) {

  PreflopOddsResult result;
  result.trials = trials;
  const std::uint8_t hero_a_id = make_card_id(hero_a);
  const std::uint8_t hero_b_id = make_card_id(hero_b);
  if (trials <= 0 || card_ids_equal(hero_a_id, hero_b_id)) {
    return result;
  }

  

  std::vector<std::uint8_t> deck_template;
  deck_template.reserve(50);
  for (std::uint8_t s = 0; s < 4; ++s) {
    for (std::uint8_t r = 0; r < 13; ++r) {
      const std::uint8_t c = make_card_id(static_cast<Rank>(r), static_cast<Suit>(s));
      if (!card_ids_equal(c, hero_a_id) && !card_ids_equal(c, hero_b_id)) {
        deck_template.push_back(c);
      }
    }
  }

  std::mt19937 rng(seed);
  std::vector<std::uint8_t> deck;
  deck.reserve(deck_template.size());

  for (int t = 0; t < trials; ++t) {
    deck = deck_template;
    partial_fisher_yates_prefix(deck, 7, rng);

    const std::uint8_t villain_a = deck[0];
    const std::uint8_t villain_b = deck[1];
    const std::uint8_t board0 = deck[2];
    const std::uint8_t board1 = deck[3];
    const std::uint8_t board2 = deck[4];
    const std::uint8_t board3 = deck[5];
    const std::uint8_t board4 = deck[6];

    const std::array<std::uint8_t, 7> hero_seven = {hero_a_id,  hero_b_id,  board0, board1,
                                            board2, board3, board4};
    const std::array<std::uint8_t, 7> villain_seven = {
        villain_a, villain_b, board0, board1, board2, board3, board4};

    const HandValue hero_value = evaluate_seven(hero_seven);
    const HandValue villain_value = evaluate_seven(villain_seven);

    const int cmp = compare_hand_value(hero_value, villain_value);
    if (cmp > 0) {
      ++result.wins;
    } else if (cmp < 0) {
      ++result.losses;
    } else {
      ++result.ties;
    }
  }

  return result;
}
