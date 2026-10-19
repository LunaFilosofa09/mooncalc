#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "value.hpp"

class BigInt {
public:
    BigInt(unsigned long long v = 0) {
        if (v == 0) limbs_.push_back(0);
        while (v) {
            limbs_.push_back((std::uint32_t)(v % kBase));
            v /= kBase;
        }
    }

    BigInt& mulSmall(std::uint32_t m) {
        unsigned long long carry = 0;
        for (auto& limb : limbs_) {
            unsigned long long cur = (unsigned long long)limb * m + carry;
            limb = (std::uint32_t)(cur % kBase);
            carry = cur / kBase;
        }
        while (carry) {
            limbs_.push_back((std::uint32_t)(carry % kBase));
            carry /= kBase;
        }
        trim();
        return *this;
    }

    BigInt& add(const BigInt& o) {
        if (limbs_.size() < o.limbs_.size()) limbs_.resize(o.limbs_.size(), 0);
        unsigned long long carry = 0;
        for (size_t i = 0; i < limbs_.size(); ++i) {
            unsigned long long cur = limbs_[i] + carry + (i < o.limbs_.size() ? o.limbs_[i] : 0);
            limbs_[i] = (std::uint32_t)(cur % kBase);
            carry = cur / kBase;
        }
        if (carry) limbs_.push_back((std::uint32_t)carry);
        return *this;
    }

    std::string str() const {
        std::string s = std::to_string(limbs_.back());
        for (size_t i = limbs_.size() - 1; i-- > 0;) {
            std::string part = std::to_string(limbs_[i]);
            s += std::string(9 - part.size(), '0') + part;
        }
        return s;
    }

    static BigInt factorial(unsigned n) {
        BigInt r(1);
        for (unsigned i = 2; i <= n; ++i) r.mulSmall(i);
        return r;
    }

    static BigInt power(unsigned base, unsigned exp) {
        BigInt r(1);
        for (unsigned i = 0; i < exp; ++i) r.mulSmall(base);
        return r;
    }

    static BigInt fibonacci(unsigned n) {
        BigInt a(0), b(1);
        for (unsigned i = 0; i < n; ++i) {
            BigInt next = a;
            next.add(b);
            a = b;
            b = next;
        }
        return a;
    }

private:
    static constexpr std::uint32_t kBase = 1000000000;
    std::vector<std::uint32_t> limbs_;

    void trim() {
        while (limbs_.size() > 1 && limbs_.back() == 0) limbs_.pop_back();
    }
};
