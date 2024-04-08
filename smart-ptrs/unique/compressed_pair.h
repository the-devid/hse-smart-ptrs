#pragma once

#include <type_traits>
#include <utility>

template <typename T, int TOrder>
class CompressedPairElement;

template <typename T, int TOrder>
    requires std::is_empty_v<T> && (!std::is_final_v<T>)
class CompressedPairElement<T, TOrder> : public T {
public:
    CompressedPairElement() = default;

    template <typename U>
    CompressedPairElement(U&& oth) : T(std::forward<U>(oth)) {}

    T& GetValue() {
        return *this;
    }
    const T& GetValue() const {
        return *this;
    }
};
template <typename T, int TOrder>
class CompressedPairElement {
public:
    CompressedPairElement() = default;

    template <typename U>
    CompressedPairElement(U&& oth) : value_(std::forward<U>(oth)) {}

    T& GetValue() {
        return value_;
    }
    const T& GetValue() const {
        return value_;
    }

private:
    T value_ = T();
};

// Me think, why waste time write lot code, when few code do trick.
template <typename F, typename S>
class CompressedPair : public CompressedPairElement<F, 0>, public CompressedPairElement<S, 1> {
public:
    CompressedPair() = default;
    template <typename FU, typename SU>
    CompressedPair(FU&& first, SU&& second)
        : FirstBase(std::forward<FU>(first)),
          SecondBase(std::forward<SU>(second)) {}

    F& GetFirst() {
        return FirstBase::GetValue();
    }
    const F& GetFirst() const {
        return FirstBase::GetValue();
    }

    S& GetSecond() {
        return SecondBase::GetValue();
    };
    const S& GetSecond() const {
        return SecondBase::GetValue();
    };

private:
    using FirstBase = CompressedPairElement<F, 0>;
    using SecondBase = CompressedPairElement<S, 1>;
};
