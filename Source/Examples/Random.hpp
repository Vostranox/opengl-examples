#pragma once

#include <iostream>
#include <random>

namespace adh {
    class Random {
      public:
        template <typename T = std::int32_t>
        static auto Int(T min, T max) noexcept {
            return GetInstance().IntGenerator(min, max);
        }

        template <typename T = float>
        static auto Real(T min, T max) noexcept {
            return GetInstance().RealGenerator(min, max);
        }

      private:
        Random() noexcept : m_Generator{ std::random_device()() } {
        }

        static Random& GetInstance() noexcept {
            static Random instance{};
            return instance;
        }

        template <typename T>
        auto IntGenerator(T min, T max) noexcept {
            std::uniform_int_distribution<T> distrib{ min, max };
            return distrib(m_Generator);
        }

        template <typename T>
        auto RealGenerator(T min, T max) noexcept {
            std::uniform_real_distribution<T> distrib{ min, max };
            return distrib(m_Generator);
        }

      private:
        std::mt19937_64 m_Generator;
    };
} // namespace adh
