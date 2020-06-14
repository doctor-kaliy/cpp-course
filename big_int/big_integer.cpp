#include "big_integer.h"

#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <functional>

big_integer::big_integer(int32_t sign, std::vector<uint32_t> const& words) : data(words) {
    this->sign = sign;
}

big_integer::big_integer() {
    sign = 0;
}

big_integer::big_integer(big_integer const& other) : data(other.data) {
    sign = other.sign;
}

big_integer::big_integer(std::vector<uint32_t> const& other) : big_integer(1, other) {}

big_integer::big_integer(int a) {
    int32_t signum = 0;
    if (a != 0) {
        signum = a < 0? -1 : 1;
        data.push_back(signum * a);
    }
    sign = signum;
}

big_integer::big_integer(uint32_t a) {
    int32_t signum = 0;
    if (a != 0) {
        signum = 1;
        data.push_back(a);
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
    *this = 0;
    if (ptr == len) {
        signum = 0;
    } else {
        while (ptr < len) {
            if (str[ptr] < '0' || str[ptr] > '9') {
                throw std::runtime_error("Invalid string");
            }
            *this *= 10;
            *this += static_cast<int>(str[ptr] - '0');
            ++ptr;
        }
    }
    sign = signum;
}

size_t big_integer::size() const {
    return data.size();
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

uint64_t add(uint32_t a, uint32_t b) {
    return static_cast<uint64_t>(a) + b;
}

uint64_t sub(uint32_t a, uint32_t b) {
    return static_cast<uint64_t>(a) - b;
}

static void apply_arithmetic(std::vector<uint32_t>& a, std::vector<uint32_t> const& b,
                             size_t start, std::function<uint64_t(uint32_t, uint32_t)> op) {
    int32_t carry = 0;
    uint64_t ss = (1Ull << 32ULL);
    for (size_t i = start; i < a.size(); ++i) {
        uint64_t swc = op(get_word(a, i), get_word(b, i - start)) + carry;
        a[a.size() - i - 1] = (swc % ss);
        carry = swc / ss;
    }
}

static void add_long(std::vector<uint32_t>& a, std::vector<uint32_t> const& b, size_t start) {
    apply_arithmetic(a, b, start, add);
}

static void subtract_long(std::vector<uint32_t>& a, std::vector<uint32_t> const& b, size_t start) {
    apply_arithmetic(a, b, start, sub);
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
    std::vector<uint32_t> _words = this->data;
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
    data = _words;
    return *this;
}

big_integer& big_integer::operator+=(big_integer const& rhs) {
    return add_signed(rhs.sign, rhs.data);
}

big_integer& big_integer::operator-=(big_integer const& rhs) {
    return add_signed(-rhs.sign, rhs.data);
}

big_integer& big_integer::operator*=(big_integer const& rhs) {
    if (sign != 0 && rhs.sign == 0) {
        *this = 0;
    }
    if (sign == 0) {
        return *this;
    }
    int32_t signum = sign * rhs.sign;
    std::vector<uint32_t> a(data);
    std::vector<uint32_t> b(rhs.data);
    a.insert(a.begin(), 0);
    b.insert(b.begin(), 0);
    std::vector<uint32_t> tmp(a.size() + b.size() + 1, 0);
    for (size_t i = 0; i < b.size(); i++) {
        uint64_t carry = 0;
        for (size_t j = 0; j < a.size(); j++) {
            size_t index = i + j;
            carry += static_cast<uint64_t>(b[b.size() - i - 1]) * (j < a.size() ? a[a.size() - j - 1] : 0) + tmp[tmp.size() - index - 1];
            tmp[tmp.size() - index - 1] = carry;
            carry >>= 32u;
        }
    }
    remove_zeroes(tmp);
    return (*this = big_integer(signum, tmp));
}

bool big_integer::smaller(big_integer const &a, big_integer const &b, size_t index) {
    for (size_t i = 1; i <= a.size(); i++) {
        if (get_word(a.data, a.size() - i) != (index - i < b.size() ? get_word(b.data, index - i) : 0)) {
            return get_word(a.data, a.size() - i) > (index - i < b.size() ? get_word(b.data, index - i) : 0);
        }
    }
    return true;
}

void big_integer::difference(big_integer &a, big_integer const &b, size_t index) {
    if (a.size() < index) {
        return;
    }
    size_t start = a.size() - index;
    uint32_t borrow = 0;
    for (size_t i = 0; i < index; ++i) {
        uint32_t x = get_word(a.data, start + i);
        uint32_t y = (i < b.size() ? get_word(b.data, i) : 0);
        uint64_t c = static_cast<int64_t>(x) - y - borrow;
        borrow = (y + borrow > x);
        c &= UINT32_MAX;
        if (a.size() >= start + i + 1) {
            a.data[a.size() - start - i - 1] = c;
        }
    }
}

big_integer big_integer::shortdiv(big_integer const &lhs, uint32_t rhs) {
    std::vector<uint32_t> tmp(lhs.size());
    uint64_t rest = 0;
    uint64_t x = 0;
    for (size_t i = 0; i < lhs.size(); i++) {
        x = (rest << 32U) | lhs.data[i];
        tmp[i] = static_cast<uint32_t>(x / rhs);
        rest = x % rhs;
    }
    remove_zeroes(tmp);
    return big_integer(tmp);
}

big_integer& big_integer::operator/=(big_integer const &other) {
    int32_t signum = this->sign * other.sign;
    big_integer divident = *this;
    big_integer divisor = other;
    divident.sign = divisor.sign = 1;
    if (divident < divisor) {
        return (*this = 0);
    }
    if (divisor.size() == 1) {
        *this = shortdiv(divident, divisor.data[0]);
        sign = signum;
        return *this;
    }
    uint32_t f = (static_cast<uint64_t>(UINT32_MAX) + 1)
                 / (static_cast<uint64_t>(divisor.data[0]) + 1);
    divident *= f;
    divisor *= f;
    divident.data.insert(divident.data.begin(), 0);
    size_t m = divisor.size() + 1;
    size_t n = divident.size();
    data.resize(n - m + 1);
    for (size_t i = m, j = size() - 1; i <= n; ++i, --j) {
        __uint128_t x = (((__uint128_t) get_word(divident.data, divident.size() - 1) << 64U) +
                         ((__uint128_t) get_word(divident.data, divident.size() - 2) << 32U) +
                         ((__uint128_t) get_word(divident.data, divident.size() - 3)));
        __uint128_t y = (((__uint128_t) divisor.data[0] << 32U) +
                         (__uint128_t) divisor.data[1]);
        uint32_t qt = std::min(static_cast<uint32_t>(x / y), UINT32_MAX);
        big_integer dq = divisor * qt;
        if (!smaller(divident, dq, m)) {
            qt--;
            dq -= divisor;
        }
        data[size() - j - 1] = qt;
        difference(divident, dq, m);
        remove_zeroes(divident.data);
    }
    remove_zeroes(data);
    sign = signum;
    return *this;
}

big_integer& big_integer::operator%=(big_integer const& rhs) {
    *this -= (*this / rhs) * rhs;
    return *this;
}

static size_t not_zero_id(std::vector<uint32_t> const& value) {
    for (size_t i = value.size(); i > 0; --i) {
        if (value[i - 1] != 0) {
            return value.size() - i;
        }
    }
    return value.size();
}

uint32_t big_integer::get_signed(size_t id, size_t not_zero_pos) const {
    if (sign == 0) {
        return 0;
    }
    if (id > data.size()) {
        return sign == 1? 0 : -1;
    } else if (id == data.size()) {
        uint32_t word = 0;
        return sign == 1? word : (id <= not_zero_pos? -word : ~word);
    } else {
        uint32_t word = data[data.size() - id - 1];
        return sign == 1? word : (id <= not_zero_pos? -word : ~word);
    }
}

static big_integer get_value(std::vector<uint32_t>& value) {
    if (!value.empty() && value[0] >> 31u) {
        for (unsigned int & i : value) {
            i = ~i;
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


big_integer& big_integer::bit_operation(big_integer const& rhs,
        std::function<uint32_t(uint32_t, uint32_t)> const& op) {
    std::vector<uint32_t> result(std::max(data.size(), rhs.data.size()) + 1);
    size_t pos1 = not_zero_id(data);
    size_t pos2 = not_zero_id(rhs.data);
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = op(get_signed(result.size() - i - 1, pos1),
                       rhs.get_signed(result.size() - i - 1, pos2));
    }
    *this = get_value(result);
    return *this;
}

uint32_t bit_and(uint32_t a, uint32_t b) {
    return a & b;
}

uint32_t bit_or(uint32_t a, uint32_t b) {
    return a | b;
}

uint32_t bit_xor(uint32_t a, uint32_t b) {
    return a ^ b;
}

big_integer& big_integer::operator&=(big_integer const& rhs) {
    return bit_operation(rhs, bit_and);
}

big_integer& big_integer::operator|=(big_integer const& rhs) {
    return bit_operation(rhs, bit_or);
}

big_integer& big_integer::operator^=(big_integer const& rhs) {
    return bit_operation(rhs, bit_xor);
}

big_integer& big_integer::operator<<=(int rhs) {
    if (rhs < 0) {
        return *this >>= (-rhs);
    }
    size_t big_shift = rhs / 32;
    uint32_t small_shift = rhs % 32;
    data.resize(big_shift + data.size(), 0);
    if (small_shift == 0) {
        return *this;
    }
    return (*this *= static_cast<uint32_t>(1ULL << small_shift));
}

big_integer& big_integer::operator>>=(int rhs) {
    if (rhs < 0) {
        return *this <<= (-rhs);
    }
    size_t big_shift = rhs / 32;
    uint32_t small_shift = rhs % 32;
    if (big_shift >= data.size()) {
        return (*this = 0);
    }
    size_t pos = not_zero_id(data);
    data.resize(data.size() - big_shift);
    data.insert(data.begin(), 0);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = get_signed(data.size() - i - 1, pos);
    }
    uint64_t shifted = ((static_cast<int64_t>(data[0]) << 32LL) >> small_shift);
    data[0] = shifted >> 32U;
    shifted <<= 32U;
    for (size_t i = 1; i < data.size(); ++i) {
        shifted |= ((static_cast<uint64_t>(data[i]) << 32U) >> small_shift);
        data[i] = shifted >> 32U;
        shifted <<= 32U;
    }
    return (*this = get_value(data));
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    return big_integer(-this->sign, this->data);
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
    return a.sign == b.sign && a.data == b.data;
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
        return ((compare_abs(a.data, b.data) * sign) < 0);
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
    s.sign = 1;
    while (s > 0) {
        big_integer temp(s % 10);
        if (temp == 0) {
            result.push_back('0');
        } else {
            result.push_back(temp.data[0] + '0');
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
    data.swap(tmp.data);
    std::swap(sign, tmp.sign);
    return *this;
}

std::ostream& operator<<(std::ostream& s, big_integer const& a) {
    return s << to_string(a);
}
