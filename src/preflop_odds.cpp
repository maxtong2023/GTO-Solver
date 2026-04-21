#include "preflop_odds.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <random>
#include <vector>

namespace {

constexpr std::array<int, 13> kPokerRankFromRankIndex = {
    14, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

inline int poker_rank_from_id(std::uint8_t id) {
  return kPokerRankFromRankIndex[id % 13];
}
inline int suit_index_from_id(std::uint8_t id) {
  return static_cast<int>(id / 13);
}

// Bit i corresponds to poker rank (i + 2), i.e. bit 0 = deuce … bit 12 = ace.
inline constexpr std::uint16_t rank_bit(int poker_rank) {
  return static_cast<std::uint16_t>(1u << (poker_rank - 2));
}

// Best straight high card (5–14) from a rank presence mask; 0 if none. Wheel = 5.
inline int highest_straight_high_mask(std::uint16_t present_ranks) {
  for (int high = 14; high >= 6; --high) {
    std::uint16_t need = 0;
    for (int i = 0; i < 5; ++i) {
      need |= rank_bit(high - i);
    }
    if ((present_ranks & need) == need) {
      return high;
    }
  }
  constexpr std::uint16_t kWheel =
      rank_bit(14) | rank_bit(2) | rank_bit(3) | rank_bit(4) | rank_bit(5);
  if ((present_ranks & kWheel) == kWheel) {
    return 5;
  }
  return 0;
}

struct SevenCardSummary {
  std::array<int, 15> rank_count{};
  std::array<int, 4> suit_count{};
};

SevenCardSummary summarize_seven(const std::array<std::uint8_t, 7>& cards) {
  SevenCardSummary s{};
  for (const std::uint8_t id : cards) {
    const int r = poker_rank_from_id(id);
    const int su = suit_index_from_id(id);
    ++s.rank_count[r];
    ++s.suit_count[su];
  }
  return s;
}

// Step 2: sanity-check the summary matches “7 cards total”.
void debug_check_seven_summary(const SevenCardSummary& s) {
  int rank_cards = 0;
  for (int r = 2; r <= 14; ++r) {
    rank_cards += s.rank_count[r];
  }
  int suit_cards = 0;
  for (int su = 0; su < 4; ++su) {
    suit_cards += s.suit_count[su];
  }
  assert(rank_cards == 7);
  assert(suit_cards == 7);
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

struct SevenFacts {
  std::uint16_t rank_mask = 0;
  int straight_high = 0;
  int flush_suit = -1;
  int sf_high = 0;
  int max_same_rank = 0;
};

SevenFacts seven_facts_from_summary(const SevenCardSummary& s,
                                    const std::array<std::uint8_t, 7>& cards) {
  SevenFacts f{};
  for (int r = 2; r <= 14; ++r) {
    if (s.rank_count[r] > f.max_same_rank) {
      f.max_same_rank = s.rank_count[r];
    }
  }

  std::array<std::uint16_t, 4> suit_masks{};
  for (const std::uint8_t id : cards) {
    const int su = suit_index_from_id(id);
    const int r = poker_rank_from_id(id);
    const std::uint16_t b = rank_bit(r);
    suit_masks[static_cast<std::size_t>(su)] |= b;
    f.rank_mask |= b;
  }
  f.straight_high = highest_straight_high_mask(f.rank_mask);

  for (int su = 0; su < 4; ++su) {
    if (s.suit_count[su] >= 5) {
      f.flush_suit = su;
      break;
    }
  }

  if (f.flush_suit >= 0) {
    f.sf_high = highest_straight_high_mask(
        suit_masks[static_cast<std::size_t>(f.flush_suit)]);
  }
  return f;
}

HandValue evaluate_seven_direct(const SevenCardSummary& s, const SevenFacts& facts,
                                const std::array<std::uint8_t, 7>& cards) {
  if (facts.sf_high > 0) {
    HandValue hv;
    hv.category = 8;
    hv.tiebreakers[0] = facts.sf_high;
    return hv;
  }

  for (int r = 14; r >= 2; --r) {
    if (s.rank_count[r] < 4) {
      continue;
    }
    int kicker = 0;
    for (int k = 14; k >= 2; --k) {
      if (k != r && s.rank_count[k] > 0) {
        kicker = k;
        break;
      }
    }
    HandValue hv;
    hv.category = 7;
    hv.tiebreakers = {r, kicker, 0, 0, 0};
    return hv;
  }

  bool found_fh = false;
  HandValue best_fh{};
  for (int t = 14; t >= 2; --t) {
    if (s.rank_count[t] < 3) {
      continue;
    }
    for (int p = 14; p >= 2; --p) {
      if (p == t || s.rank_count[p] < 2) {
        continue;
      }
      HandValue hv;
      hv.category = 6;
      hv.tiebreakers = {t, p, 0, 0, 0};
      if (!found_fh || compare_hand_value(hv, best_fh) > 0) {
        best_fh = hv;
        found_fh = true;
      }
    }
  }
  if (found_fh) {
    return best_fh;
  }

  if (facts.flush_suit >= 0) {
    std::array<int, 7> flush_ranks{};
    int n = 0;
    for (const std::uint8_t id : cards) {
      if (suit_index_from_id(id) == facts.flush_suit) {
        flush_ranks[static_cast<std::size_t>(n++)] = poker_rank_from_id(id);
      }
    }
    std::sort(flush_ranks.begin(), flush_ranks.begin() + n, std::greater<int>());
    HandValue hv;
    hv.category = 5;
    for (int i = 0; i < 5; ++i) {
      hv.tiebreakers[i] = flush_ranks[static_cast<std::size_t>(i)];
    }
    return hv;
  }

  if (facts.straight_high > 0) {
    HandValue hv;
    hv.category = 4;
    hv.tiebreakers[0] = facts.straight_high;
    return hv;
  }

  for (int r = 14; r >= 2; --r) {
    if (s.rank_count[r] < 3) {
      continue;
    }
    std::array<int, 15> rc{};
    for (int i = 2; i <= 14; ++i) {
      rc[i] = s.rank_count[i];
    }
    rc[r] -= 3;
    HandValue hv;
    hv.category = 3;
    hv.tiebreakers[0] = r;
    int ki = 1;
    for (int k = 14; k >= 2 && ki < 3; --k) {
      while (rc[k] > 0 && ki < 3) {
        hv.tiebreakers[static_cast<std::size_t>(ki++)] = k;
        --rc[k];
      }
    }
    return hv;
  }

  std::vector<int> pair_ranks;
  pair_ranks.reserve(3);
  for (int r = 14; r >= 2; --r) {
    if (s.rank_count[r] >= 2) {
      pair_ranks.push_back(r);
    }
  }
  if (pair_ranks.size() >= 2) {
    const int pr0 = pair_ranks[0];
    const int pr1 = pair_ranks[1];
    std::array<int, 15> rc{};
    for (int i = 2; i <= 14; ++i) {
      rc[i] = s.rank_count[i];
    }
    rc[pr0] -= 2;
    rc[pr1] -= 2;
    int kicker = 0;
    for (int k = 14; k >= 2; --k) {
      if (rc[k] > 0) {
        kicker = k;
        break;
      }
    }
    HandValue hv;
    hv.category = 2;
    hv.tiebreakers = {pr0, pr1, kicker, 0, 0};
    return hv;
  }

  for (int r = 14; r >= 2; --r) {
    if (s.rank_count[r] < 2) {
      continue;
    }
    std::array<int, 15> rc{};
    for (int i = 2; i <= 14; ++i) {
      rc[i] = s.rank_count[i];
    }
    rc[r] -= 2;
    HandValue hv;
    hv.category = 1;
    hv.tiebreakers[0] = r;
    int ki = 1;
    for (int k = 14; k >= 2 && ki < 4; --k) {
      while (rc[k] > 0 && ki < 4) {
        hv.tiebreakers[static_cast<std::size_t>(ki++)] = k;
        --rc[k];
      }
    }
    return hv;
  }

  HandValue hv;
  hv.category = 0;
  int ti = 0;
  for (int k = 14; k >= 2 && ti < 5; --k) {
    int c = s.rank_count[k];
    while (c > 0 && ti < 5) {
      hv.tiebreakers[static_cast<std::size_t>(ti++)] = k;
      --c;
    }
  }
  return hv;
}

[[maybe_unused]] HandValue evaluate_five(const std::array<std::uint8_t, 5>& cards) {
  std::array<int, 15> rank_count{};
  std::array<int, 4> suit_count{};
  std::uint16_t rank_mask = 0;
  std::vector<int> ranks_desc;
  ranks_desc.reserve(5);

  for (const std::uint8_t& id : cards) {
    const int rv = poker_rank_from_id(id);
    ++rank_count[rv];
    rank_mask |= rank_bit(rv);
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

  const int straight_high = highest_straight_high_mask(rank_mask);

  if (flush) {
    std::uint16_t flush_mask = 0;
    for (const std::uint8_t& id : cards) {
      if (suit_index_from_id(id) == flush_suit) {
        flush_mask |= rank_bit(poker_rank_from_id(id));
      }
    }
    const int sf_high = highest_straight_high_mask(flush_mask);
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
  const SevenCardSummary summary = summarize_seven(cards);
  debug_check_seven_summary(summary);
  const SevenFacts facts = seven_facts_from_summary(summary, cards);

  const HandValue best = evaluate_seven_direct(summary, facts, cards);

#ifndef NDEBUG
  if (facts.flush_suit < 0) {
    assert(best.category != 5 && best.category != 8);
  }
  if (facts.straight_high == 0) {
    assert(best.category != 4 && best.category != 8);
  }
  if (facts.max_same_rank < 4) {
    assert(best.category != 7);
  }
  if (facts.max_same_rank < 3) {
    assert(best.category != 6 && best.category != 3);
  }
#endif
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
