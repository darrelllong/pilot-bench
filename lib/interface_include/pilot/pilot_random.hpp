/*
 * pilot_random.hpp: a random number generator and normal variates
 *
 * Pilot needs random numbers for one thing: to make readings of which we
 * know the answer, so that we can measure how often its analysis is right.
 * The distributions of the C++ standard library give different numbers with
 * different compilers, so a measurement that uses them cannot be repeated.
 * The numbers from this file are the same everywhere.
 *
 * It has PCG64 seeded through SplitMix64, the uniform of 53 bits, the dense
 * uniform, and the ziggurat for the standard normal. It needs nothing but
 * the C++ standard library.
 *
 * Copyright (c) 2026, Darrell Long. All rights reserved.
 * The Pilot tool and library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License version 2.1 (not any other version) as published by the Free
 * Software Foundation.
 *
 * The Pilot tool and library is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program in file lgpl-2.1.txt; if not, see
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#ifndef LIB_INTERFACE_INCLUDE_PILOT_PILOT_RANDOM_HPP_
#define LIB_INTERFACE_INCLUDE_PILOT_PILOT_RANDOM_HPP_

// PCG64 has a state of 128 bits, for which we use the integer of 128 bits
// of GCC and Clang, and we use their function for counting leading zeros
#if !defined(__SIZEOF_INT128__) || !defined(__GNUC__)
#error "pilot_random.hpp needs GCC or Clang on a machine of 64 bits"
#endif

#include <cmath>
#include <cstdint>
#include <cstring>

namespace pilot {

/**
 * \brief One step of SplitMix64
 * \details G. L. Steele, D. Lea, and C. H. Flood, "Fast splittable
 * pseudorandom number generators," OOPSLA 2014.
 * @param[in,out] state the state, which is advanced
 * @return the output word
 */
inline uint64_t splitmix64(uint64_t *state) {
    *state += 0x9e3779b97f4a7c15ULL;
    uint64_t z = *state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

/**
 * \brief The ziggurat for the standard normal
 * \details G. Marsaglia and W. W. Tsang, "The Ziggurat Method for Generating
 * Random Variables," Journal of Statistical Software 5(8), 2000. The area
 * under the half-normal density f(x) = exp(-x^2 / 2) is cut into 256 pieces
 * of equal area v: a base piece that has the rectangle [0, r] x [0, f(r)]
 * and the tail beyond r, and above it rectangles [0, x_i] x [f(x_i),
 * f(x_{i+1})].
 *
 * The table is not copied from a publication. Equal areas mean
 *
 *   v = r f(r) + integral of f from r,  x_{i+1} = f^-1(f(x_i) + v / x_i),
 *
 * with x_1 = r. The recurrence has to end at x_256 = 0, which fixes r. We
 * find r by bisection.
 */
class ziggurat_normal_t {
public:
    enum { LAYERS = 256 };

    ziggurat_normal_t() {
        double lo = 1, hi = 8;
        // the residual falls with r, so it is positive at the low end
        while (hi - lo > 1e-15 * hi) {
            const double middle = 0.5 * (lo + hi);
            if (build(middle) > 0)
                lo = middle;
            else
                hi = middle;
        }
        build(0.5 * (lo + hi));
    }

    //! the table, which is derived once
    static const ziggurat_normal_t& instance(void) {
        static const ziggurat_normal_t table;
        return table;
    }

    //! x(0) is r and x(LAYERS - 1) is 0
    double x(size_t i) const { return x_[i]; }
    //! f(i) is the density at x(i)
    double f(size_t i) const { return f_[i]; }
    //! the area of every piece
    double area(void) const { return area_; }

    static double density(double x) { return std::exp(-0.5 * x * x); }
    //! the integral of the density from x
    static double tail_area(double x) {
        // sqrt(pi / 2) erfc(x / sqrt(2))
        return 1.2533141373155003 * std::erfc(x / 1.4142135623730951);
    }

    /**
     * \brief One standard normal variate
     * \details A word has the significand of a uniform, the index of a
     * piece, and a sign. A point in the inner rectangle of the piece is
     * returned at once. Otherwise the point is accepted if it is under the
     * density, and the part of the base piece that is beyond r is the tail.
     * @param rng a generator that has next_u64() and unit_f64_dense()
     */
    template <typename Rng>
    double sample(Rng &rng) const {
        const double uniform_scale = 1.0 / 9007199254740992.0;  // 2^-53
        while (true) {
            const uint64_t word = rng.next_u64();
            const double uniform = static_cast<double>(word & ((1ULL << 53) - 1)) * uniform_scale;
            const size_t layer = static_cast<size_t>((word >> 53) & (LAYERS - 1));
            const double sign = ((word >> 61) & 1) ? -1.0 : 1.0;
            if (0 == layer) {
                // The base piece. The uniform chooses between the rectangle
                // and the tail by their shares of the area of the piece.
                const double position = uniform * area_ / f_[0];
                if (position < x_[0])
                    return sign * position;
                return sign * draw_tail(rng);
            }
            // piece `layer` is [0, x[layer - 1]] x [f[layer - 1], f[layer]]
            const double x = uniform * x_[layer - 1];
            if (x < x_[layer])
                return sign * x;   // left of where the curve crosses
            const double lower = f_[layer - 1], upper = f_[layer];
            const double unit = static_cast<double>(rng.next_u64() >> 11) * uniform_scale;
            const double height = lower + unit * (upper - lower);
            if (height < density(x))
                return sign * x;
        }
    }

private:
    /**
     * Build the table for the base boundary r
     * @return f(x_LAYERS) - 1, which is 0 for the right r, and is positive
     * if r is too small
     */
    double build(double r) {
        area_ = r * density(r) + tail_area(r);
        for (size_t i = 0; i < LAYERS; ++i) {
            x_[i] = 0;
            f_[i] = 0;
        }
        x_[0] = r;
        f_[0] = density(r);
        // x[LAYERS - 1] is 0, where the density is 1
        for (size_t i = 1; i < LAYERS - 1; ++i) {
            const double y = f_[i - 1] + area_ / x_[i - 1];
            if (y >= 1.0 || std::isnan(y))
                return y - 1.0;   // the pieces are too tall
            x_[i] = std::sqrt(-2.0 * std::log(y));
            f_[i] = y;
        }
        x_[LAYERS - 1] = 0;
        f_[LAYERS - 1] = 1;
        return f_[LAYERS - 2] + area_ / x_[LAYERS - 2] - 1.0;
    }

    //! -ln U of a dense uniform U
    template <typename Rng>
    static double exponential(Rng &rng) {
        while (true) {
            const double u = rng.unit_f64_dense();
            if (u > 0)
                return -std::log(u);
        }
    }

    /**
     * The tail beyond r, by rejection: with e1 and e2 exponential, r + e1 / r
     * is accepted when 2 e2 > (e1 / r)^2
     */
    template <typename Rng>
    double draw_tail(Rng &rng) const {
        const double r = x_[0];
        while (true) {
            const double excess = exponential(rng) / r;
            const double height = exponential(rng);
            if (2.0 * height > excess * excess)
                return r + excess;
        }
    }

    double x_[LAYERS];
    double f_[LAYERS];
    double area_;
};

/**
 * \brief PCG64: a linear congruential generator of 128 bits with the XSL-RR
 * output permutation
 * \details M. E. O'Neill, "PCG: A Family of Simple Fast Space-Efficient
 * Statistically Good Algorithms for Random Number Generation," 2014. The
 * period is 2^128.
 */
class pcg64_t {
public:
    typedef unsigned __int128 u128;

    //! from 256 bits of seed material: the state and the stream
    pcg64_t(u128 state, u128 seq) : state_(0), inc_((seq << 1) | 1) {
        step();
        state_ += state;
        step();
    }

    //! from the SplitMix64 expansion of the seed
    explicit pcg64_t(uint64_t seed) : state_(0), inc_(0) {
        uint64_t s = seed;
        uint64_t w[4];
        for (int i = 0; i < 4; ++i)
            w[i] = splitmix64(&s);
        const u128 state = (static_cast<u128>(w[1]) << 64) | w[0];
        const u128 seq = (static_cast<u128>(w[3]) << 64) | w[2];
        inc_ = (seq << 1) | 1;
        step();
        state_ += state;
        step();
    }

    uint64_t next_u64(void) { return step(); }
    uint32_t next_u32(void) { return static_cast<uint32_t>(step() >> 32); }

    //! a uniform in [0, 1) that is a multiple of 2^-53, from one word
    double unit_f64(void) {
        return static_cast<double>(next_u64() >> 11) * (1.0 / 9007199254740992.0);
    }

    /**
     * \brief A uniform real in [0, 1) rounded down to a double
     * \details Every double x in [0, 1) occurs with the probability of the
     * gap between x and the next double. The real U has 2^-(e+1) <= U < 2^-e
     * with probability 2^-(e+1), so e is the number of leading zero bits of
     * a stream of uniform words, and the next 52 bits are the significand.
     */
    double unit_f64_dense(void) {
        const uint32_t zero_bits_to_zero = 1074;
        const uint32_t zero_bits_to_subnormal = 1022;
        const uint32_t fraction_bits = 52;
        uint32_t zeros = 0;
        uint64_t word = next_u64();
        while (0 == word) {
            zeros += 64;
            if (zeros >= zero_bits_to_zero)
                return 0.0;
            word = next_u64();
        }
        zeros += static_cast<uint32_t>(__builtin_clzll(word));
        if (zeros >= zero_bits_to_zero)
            return 0.0;
        const uint64_t significand = next_u64() >> (64 - fraction_bits);
        uint64_t bits;
        if (zeros < zero_bits_to_subnormal) {
            const uint64_t exponent = zero_bits_to_subnormal - zeros;
            bits = (exponent << fraction_bits) | significand;
        } else {
            const uint32_t top = zero_bits_to_zero - 1 - zeros;
            bits = (1ULL << top) | (significand >> (fraction_bits - top));
        }
        double result;
        std::memcpy(&result, &bits, sizeof(result));
        return result;
    }

    //! a standard normal variate, by the ziggurat
    double normal(void) {
        return ziggurat_normal_t::instance().sample(*this);
    }

    //! an exponential variate of mean 1, by inversion of the dense uniform
    double exponential(void) {
        while (true) {
            const double u = unit_f64_dense();
            if (u > 0)
                return -std::log(u);
        }
    }

private:
    uint64_t step(void) {
        // 47026247687942121848144207491837523525
        const u128 mult = (static_cast<u128>(2549297995355413924ULL) << 64) | 4865540595714422341ULL;
        state_ = state_ * mult + inc_;
        // XSL-RR: xor the two halves, then rotate right by the top 6 bits
        const uint64_t xsl = static_cast<uint64_t>(state_ >> 64) ^ static_cast<uint64_t>(state_);
        const unsigned rot = static_cast<unsigned>(state_ >> 122);
        return (xsl >> rot) | (xsl << ((64 - rot) & 63));
    }

    u128 state_;
    u128 inc_;
};

} // namespace pilot

#endif /* LIB_INTERFACE_INCLUDE_PILOT_PILOT_RANDOM_HPP_ */
