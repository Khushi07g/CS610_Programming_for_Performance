#ifndef BASE_H
#define BASE_H
#include <cstdint>
#include <memory>
#include <cassert>
#include <iostream>
#include <string>
#include <utility>

extern uint64_t currentTime;

inline void __print(int x) {std::cerr << x;}
inline void __print(long x) {std::cerr << x;}
inline void __print(unsigned x) {std::cerr << x;}
inline void __print(unsigned long x) {std::cerr << x;}
inline void __print(unsigned long long x) {std::cerr << x;}
inline void __print(float x) {std::cerr << x;}
inline void __print(double x) {std::cerr << x;}
inline void __print(long double x) {std::cerr << x;}
inline void __print(char x) {std::cerr << '\'' << x << '\'';}
inline void __print(const char *x) {std::cerr << '"' << x << '"';}
inline void __print(const std::string &x) {std::cerr << '"' << x << '"';}
inline void __print(bool x) {std::cerr << (x ? "true" : "false");}

template<typename T, typename V>
void __print(const std::pair<T, V> &x) {std::cerr << '{'; __print(x.first); std::cerr << ','; __print(x.second); std::cerr << '}';}
template<typename T>
void __print(const T &x) {int f = 0; std::cerr << '{'; for (auto &i: x) std::cerr << (f++ ? "," : ""), __print(i); std::cerr << "}";}
inline void _print() {std::cerr << "]\n";}
template <typename T, typename... V>
void _print(T t, V... v) {__print(t); if (sizeof...(v)) std::cerr << ", "; _print(v...);}
#ifdef AMOGH
#define debug(x...) std::cerr << "[" << #x << "] = ["; _print(x)
#else
#define debug(x...)
#endif
#endif // BASE_H