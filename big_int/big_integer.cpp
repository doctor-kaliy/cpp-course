#include "big_integer.h"

#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <iostream>
#include <cassert>

big_integer::big_integer(int32_t sign, std::vector<uint32_t> const& words) : words(words) {
    this->sign = sign;
}

big_integer::big_integer() {
    sign = 0;
}

big_integer::big_integer(big_integer const& other) : words(other.words) {
    sign = other.sign;
}

big_integer::big_integer(std::vector<uint32_t> const& other) : big_integer(1, other) {}

big_integer::big_integer(int a) {
    int32_t signum = 0;
    if (a != 0) {
        signum = a < 0? -1 : 1;
        words.push_back((uint64_t) signum * a);
    }
    sign = signum;
}

big_integer::big_integer(std::string const& str) : big_integer() {
    size_t len = str.size();
    if (len == 0) {
        throw std::runtime_error("Invalid string");
    }
    size_t ptr = 0;
    while (str[0] == ' ') {
        ++ptr;
    }
    int32_t signum = 1;
    if (str[ptr] == '-') {
        signum = -1;
        ++ptr;
    } else if (str[ptr] == '+') {
        ++ptr;
    }
    while (str[ptr] == '0') {
        ptr++;
    }
    big_integer ten(10);
    *this = 0;
    if (ptr == len) {
        signum = 0;
    } else {
        while (ptr < len) {
            if (str[ptr] < '0' || str[ptr] > '9') {
                throw std::runtime_error("Invalid string");
            }
            *this *= ten;
            *this += (int)(str[ptr] - '0');
            ++ptr;
        }
    }
    sign = signum;
}

size_t big_integer::size() const {
    return words.size();
}

static int32_t compare_abs(std::vector<uint32_t> const& words, std::vector<uint32_t> const& other_words) {
    if (words.size() == other_words.size()) {
        size_t ptr = 0;
        while (ptr < words.size() && words[ptr] == other_words[ptr]) {
            ptr++;
        }
        if (ptr == words.size()) {
            return 0;
        }
        return (words[ptr] < other_words[ptr])? -1 : 1;
    }
    return (words.size() < other_words.size())? -1 : 1;
}

static uint32_t get_word(std::vector<uint32_t> const& val, size_t n) {
    if (n >= val.size()) {
        return 0;
    }
    return val[val.size() - n - 1];
}

static void remove_zeroes(std::vector<uint32_t>& v) {
    if (v.empty()) {
        return;
    }
    size_t ptr = 0;
    while (ptr < v.size() && v[ptr] == 0) {
        ptr++;
    }
    if (ptr == v.size()) {
        v.resize(0);
        return;
    }
    v.resize(std::move(v.begin() + ptr, v.end(), v.begin()) - v.begin());
}

static void add_long(std::vector<uint32_t>& a, std::vector<uint32_t> const& b, size_t start) {
    int32_t carry = 0;
    uint64_t ss = (1Ull << 32ULL);
    for (size_t i = start; i < a.size(); ++i) {
        uint64_t swc = static_cast<uint64_t>(get_word(a, i)) + get_word(b, i - start) + carry;
        a[a.size() - i - 1] = (swc % ss);
        carry = swc / ss;
        if (i >= start + b.size() && carry == 0) {
            break;
        }
    }
}

static void subtract_long(std::vector<uint32_t>& a, std::vector<uint32_t> const& b, size_t start) {
    int32_t carry = 0;
    uint64_t ss = (1ULL << 32ULL);
    for (size_t i = start; i < a.size(); ++i) {
        uint64_t swc = static_cast<uint64_t>(get_word(a, i)) - get_word(b, i - start) + carry;
        a[a.size() - i - 1] = (swc % ss);
        carry = swc / ss;
        if (i >= start + b.size() && carry == 0) {
            break;
        }
    }
}

static std::vector<uint32_t> apply_add_long(std::vector<uint32_t> const& a, std::vector<uint32_t> const &b) {
    std::vector<uint32_t> res(std::max(a.size(), b.size()) + 1, 0);
    add_long(res, a, 0);
    add_long(res, b, 0);
    remove_zeroes(res);
    return res;
}

static std::vector<uint32_t> apply_subtract_long(std::vector<uint32_t> const& a, std::vector<uint32_t> const &b) {
    std::vector<uint32_t> res(std::max(a.size(), b.size()) + 4, 0);
    add_long(res, a, 0);
    subtract_long(res, b, 0);
    remove_zeroes(res);
    return res;
}

big_integer& big_integer::add_signed(int32_t rhs_sign, std::vector<uint32_t> const& rhs_words) {
    std::vector<uint32_t> _words = this->words;
    if (rhs_sign == 0) {
        return *this;
    } else if (sign == 0) {
        sign = rhs_sign;
        _words = rhs_words;
    } else if (sign == rhs_sign) {
        _words = apply_add_long(_words, rhs_words);
    } else {
        int32_t cmp = compare_abs(_words, rhs_words);
        if (cmp == 0) {
            _words.clear();
            sign = 0;
        } else {
            _words = cmp > 0 ? apply_subtract_long(_words, rhs_words) : apply_subtract_long(rhs_words, _words);
            sign = sign == cmp? 1 : -1;
        }
    }
    words = _words;
    assert(words.empty() && sign == 0 || !words.empty());
    return *this;
}

big_integer& big_integer::operator+=(big_integer const& rhs) {
    return add_signed(rhs.sign, rhs.words);
}

big_integer& big_integer::operator-=(big_integer const& rhs) {
    return add_signed(-rhs.sign, rhs.words);
}

big_integer& big_integer::operator*=(big_integer const& rhs) {
    if (sign != 0 && rhs.sign == 0) {
        *this = 0;
    }
    if (sign == 0) {
        return *this;
    }
    int32_t signum = sign * rhs.sign;
    std::vector<uint32_t> a(words);
    std::vector<uint32_t> b(rhs.words);
    a.insert(a.begin(), 0);
    b.insert(b.begin(), 0);
    *this = 0;
    std::vector<uint32_t> kal(a.size() + b.size() + 1, 0);
    for (size_t i = 0; i < b.size(); i++) {
        uint64_t carry = 0;
        for (size_t j = 0; j < a.size(); j++) {
            size_t index = i + j;
            carry += (uint64_t)b[b.size() - i - 1] * (j < a.size() ? a[a.size() - j - 1] : 0) + kal[kal.size() - index - 1];
            kal[kal.size() - index - 1] = (uint32_t)carry;
            carry >>= 32u;
        }
    }

    remove_zeroes(kal);
    this->words = kal;
    sign = signum;
    return *this;
}

// start of division block
bool big_integer::smaller(big_integer const &a, big_integer const &b, size_t index) {
    for (size_t i = 1; i <= a.size(); i++) {
        if (get_word(a.words, a.size() - i) != (index - i < b.size() ? get_word(b.words, index - i) : 0)) {
            return get_word(a.words, a.size() - i) > (index - i < b.size() ? get_word(b.words, index - i) : 0);
        }
    }
    return true;
}

void big_integer::difference(big_integer &a, big_integer const &b, size_t index) {
    if (a.size() < index) {
        return;
    }
    size_t start = a.size() - index;
    bool borrow = false;
    for (size_t i = 0; i < index; ++i) {
        uint32_t x = get_word(a.words, start + i);
        uint32_t y = (i < b.size() ? get_word(b.words, i) : 0);
        uint64_t c = (int64_t) x - y - borrow;
        borrow = (y + borrow > x);
        c &= UINT32_MAX;
        if (a.size() >= start + i + 1) {
            a.words[a.size() - start - i - 1] = (uint32_t) c;
        }
    }
}

big_integer big_integer::shortdiv(big_integer const &lhs, uint32_t rhs) {
    std::vector<uint32_t> tmp(lhs.size());
    uint64_t rest = 0;
    uint64_t x = 0;
    for (size_t i = 0; i < lhs.size(); i++) {
        x = (rest << 32) | lhs.words[i];
        tmp[i] = (uint32_t)(x / rhs);
        rest = x % rhs;
    }
    remove_zeroes(tmp);
    big_integer res(tmp);
    return res;
}

big_integer& big_integer::operator/=(big_integer const &one) {
    int32_t signum = this->sign * one.sign;
    big_integer a = *this;
    big_integer b = one;
    if (a.sign == -1) {
        a.sign = 1;
    }
    if (b.sign == -1) {
        b.sign = 1;
    }
    big_integer tmp;
    big_integer dq;
    a.sign = b.sign = 1;
    if (a < b) {
        *this = 0;
        return *this;
    }
    if (b.size() == 1) {
        tmp = shortdiv(a, b.words[0]);
        tmp.sign = signum;
        *this = tmp;
        return *this;
    }
    uint32_t f = (uint64_t(UINT32_MAX) + 1) / (uint64_t(b.words[0]) + 1);
    big_integer ff;
    ff.words.push_back(f);
    ff.sign = 1;
    a *= ff;
    b *= ff;
    a.words.insert(a.words.begin(), 0);
    size_t m = b.size() + 1;
    size_t n = a.size();
    tmp.words.resize(n - m + 1);
    uint32_t qt = 0;
    for (size_t i = m, j = tmp.size() - 1; i <= n; ++i, --j) {
        __uint128_t x = (((__uint128_t) get_word(a.words, a.size() - 1) << 64) +
                         ((__uint128_t) get_word(a.words, a.size() - 2) << 32) +
                         ((__uint128_t) get_word(a.words, a.size() - 3)));
        __uint128_t y = (((__uint128_t) b.words[0] << 32) +
                         (__uint128_t) b.words[1]);
        qt = std::min((uint32_t) (x / y), UINT32_MAX);
        big_integer qqt;
        if (qt != 0) {
            qqt.sign = 1;
            qqt.words.push_back(qt);
        }
        dq = b * qqt;
        if (!smaller(a, dq, m)) {
            qt--;
            dq -= b;
        }
        tmp.words[tmp.size() - j - 1] = qt;
        difference(a, dq, m);
        remove_zeroes(a.words);
    }
    remove_zeroes(tmp.words);
    *this = big_integer(signum, tmp.words);
    return *this;
}
// end of division block


big_integer& big_integer::operator%=(big_integer const& rhs) {
    *this -= (*this / rhs) * rhs;
    assert(sign == 0 || !words.empty());
    return *this;
}

static size_t not_zero_id(std::vector<uint32_t> const& v) {
    for (size_t i = v.size(); i > 0; --i) {
        if (v[i - 1] != 0) {
            return v.size() - i;
        }
    }
    return v.size();
}

uint32_t big_integer::get_signed(size_t id, size_t not_zero_pos) const {
    if (sign == 0) {
        return 0;
    }
    if (id > words.size()) {
        return sign == 1? 0 : -1;
    } else if (id == words.size()) {
        int32_t word = 0;
        return sign == 1? word : (id <= not_zero_pos? -word : ~word);
    } else {
        int32_t word = words[words.size() - id - 1];
        return sign == 1? word : (id <= not_zero_pos? -word : ~word);
    }
}

static big_integer get_value(std::vector<uint32_t>& value) {
    if (!value.empty() && value[0] >> 31u) {
        for (size_t i = 0; i < value.size(); ++i) {
            value[i] = ~value[i];
        }
        remove_zeroes(value);
        return big_integer(-1, value) - 1;
    }
    remove_zeroes(value);
    if (value.empty()) {
        return 0;
    }
    return big_integer(value);
}

big_integer& big_integer::operator&=(big_integer const& rhs) {
    std::vector<uint32_t> result(std::max(words.size(), rhs.words.size()) + 1);
    size_t pos1 = not_zero_id(words);
    size_t pos2 = not_zero_id(rhs.words);
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = get_signed(result.size() - i - 1, pos1)
                    & rhs.get_signed(result.size() - i - 1, pos2);
    }
    *this = get_value(result);
    return *this;
}

big_integer& big_integer::operator|=(big_integer const& rhs) {
    std::vector<uint32_t> result(std::max(words.size(), rhs.words.size()) + 1);
    size_t pos1 = not_zero_id(words);
    size_t pos2 = not_zero_id(rhs.words);
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = get_signed(result.size() - i - 1, pos1)
                    | rhs.get_signed(result.size() - i - 1, pos2);
    }
    *this = get_value(result);
    return *this;
}

big_integer& big_integer::operator^=(big_integer const& rhs) {
    std::vector<uint32_t> result(std::max(words.size(), rhs.words.size()) + 1);
    size_t pos1 = not_zero_id(words);
    size_t pos2 = not_zero_id(rhs.words);
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = get_signed(result.size() - i - 1, pos1)
                    ^ rhs.get_signed(result.size() - i - 1, pos2);
    }
    *this = get_value(result);
    return *this;
}

big_integer& big_integer::operator<<=(int rhs) {
    int big_shift = rhs / 32;
    int small_shift = rhs % 32;
    words.resize(big_shift + words.size(), 0);
    if (small_shift == 0) {
        return *this;
    }
    uint32_t ss = (1ULL << ((uint32_t)rhs % 32));
    assert(ss > 0);
    big_integer sss;
    sss.sign = 1;
    sss.words.push_back(ss);
    return (*this *= sss);
}

big_integer& big_integer::operator>>=(int rhs) {
    int big_shift = rhs / 32;
    int small_shift = rhs % 32;
    if (static_cast<size_t>(big_shift) >= words.size()) {
        *this = 0;
    } else {
        words.resize(words.size() - big_shift);
        std::vector<uint32_t> new_data(words);
        new_data.insert(new_data.begin(), 0);
        size_t pos = not_zero_id(words);
        for (size_t i = 0; i < new_data.size(); ++i) {
            new_data[i] = get_signed(new_data.size() - i - 1, pos);
        }
        uint64_t shifted = ((static_cast<int64_t>(new_data[0]) << 32) >> small_shift);
        new_data[0] = shifted >> 32;
        shifted <<= 32;
        for (size_t i = 1; i < new_data.size(); ++i) {
            shifted |= ((static_cast<uint64_t>(new_data[i]) << 32) >> small_shift);
            new_data[i] = shifted >> 32u;
            shifted <<= 32u;
        }
        *this = get_value(new_data);
    }
    return *this;
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    return big_integer(-this->sign, this->words);
}

big_integer big_integer::operator~() const {
    return -*this - 1;
}

big_integer& big_integer::operator++() {
    return (*this += 1);
}

big_integer big_integer::operator++(int) {
    big_integer r(*this);
    ++*this;
    return r;
}

big_integer& big_integer::operator--() {
    return (*this -= 1);
}

big_integer big_integer::operator--(int) {
    big_integer r = *this;
    --*this;
    return r;
}

big_integer operator+(big_integer a, big_integer const& b) {
    return a += b;
}

big_integer operator-(big_integer a, big_integer const& b) {
    return a -= b;
}

big_integer operator*(big_integer a, big_integer const& b) {
    return a *= b;
}

big_integer operator/(big_integer a, big_integer const& b) {
    return a /= b;
}

big_integer operator%(big_integer a, big_integer const& b) {
    return a %= b;
}

big_integer operator&(big_integer a, big_integer const& b) {
    return a &= b;
}

big_integer operator|(big_integer a, big_integer const& b) {
    return a |= b;
}

big_integer operator^(big_integer a, big_integer const& b) {
    return a ^= b;
}

big_integer operator<<(big_integer a, int b) {
    return a <<= b;
}

big_integer operator>>(big_integer a, int b) {
    return a >>= b;
}

bool operator==(big_integer const& a, big_integer const& b) {
    return a.sign == b.sign && a.words == b.words;
}

bool operator!=(big_integer const& a, big_integer const& b) {
    return !(a == b);
}

bool operator<(big_integer const& a, big_integer const& b) {
    if (a.sign == b.sign) {
        int32_t sign = a.sign;
        if (sign == 0) {
            return false;
        }
        return ((compare_abs(a.words, b.words) * sign) < 0);
    }
    return a.sign < b.sign;
}

bool operator>(big_integer const& a, big_integer const& b) {
    return !(a <= b);
}

bool operator<=(big_integer const& a, big_integer const& b) {
    return a < b || a == b;
}

bool operator>=(big_integer const& a, big_integer const& b) {
    return !(a < b);
}

std::string to_string(big_integer const& a) {
    std::string result;
    if (a.sign == 0) {
        return "0";
    }

    big_integer s(a);
    assert(a.words[0] != 0);
    s.sign = 1;
    while (s > 0) {
        big_integer temp(s % 10);
        if (temp == 0) {
            result.push_back('0');
        } else {
            assert(!temp.words.empty());
            result.push_back(temp.words[0] + '0');
        }
        s /= 10;
    }
    if (a.sign == -1) {
        result.push_back('-');
    }
    reverse(result.begin(), result.end());
    return result;
}

big_integer &big_integer::operator=(big_integer const &other) {
    big_integer tmp(other);
    words.swap(tmp.words);
    std::swap(sign, tmp.sign);
    return *this;
}

std::ostream& operator<<(std::ostream& s, big_integer const& a) {
    return s << to_string(a);
}
