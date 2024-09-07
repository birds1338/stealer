#pragma once
#include <utility>

template <int N> struct RandomGenerator {
private:
  static constexpr unsigned a = 16807;
  static constexpr unsigned m = 2147483647;
  static constexpr unsigned s = RandomGenerator<N - 1>::value;
  static constexpr unsigned lo = a * (s & 0xFFFF);
  static constexpr unsigned hi = a * (s >> 16);
  static constexpr unsigned lo2 = lo + ((hi & 0x7FFF) << 16);
  static constexpr unsigned hi2 = hi >> 15;
  static constexpr unsigned lo3 = lo2 + hi;

public:
  static constexpr unsigned value = lo3 > m ? lo3 - m : lo3;
};

template <> struct RandomGenerator<0> {
  static constexpr unsigned value = __COUNTER__ + __TIME__[0] + __TIME__[1] + __TIME__[3] + __TIME__[4] + __TIME__[6] + __TIME__[7];
};

template <int N> struct RandomChar {
  static const char value = static_cast<char>(1 + RandomGenerator<N + 1>::value % 0x7e);
};

template <size_t N, int K> struct XorString {
private:
  const char key;
  char encrypted[N + 1];
  constexpr char enc(char c, int i) const { return c ^ (key + __TIME__[i % 8]) % 128; }
  char dec(char c, int i) const { return c ^ (key + __TIME__[i % 8]) % 128; }

public:
  template <size_t... Is> constexpr __forceinline XorString(const char *str, std::index_sequence<Is...>) : key(RandomChar<K>::value), encrypted{enc(str[Is], Is)...} {}

  __forceinline auto decrypt() {
    for (size_t i = 0; i < N; ++i) {
      encrypted[i] = dec(encrypted[i], i);
    }
    encrypted[N] = '\0';
    return encrypted;
  }
};

#define xor(str) XorString<sizeof(str) - 1, (__COUNTER__ + __LINE__) % 128>((str), std::make_index_sequence<sizeof(str) - 1>()).decrypt()
