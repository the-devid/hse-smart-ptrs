#pragma once

#include <type_traits>
#include "shared.h"
#include "sw_fwd.h"  // Forward declaration

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() noexcept : block_(nullptr), observer_(nullptr) {
    }

    template <typename U>
    WeakPtr(const WeakPtr<U>& other) noexcept : block_(other.block_), observer_(other.observer_) {
        if (block_) {
            block_->IncrementWeakRefCounter();
        }
    }
    WeakPtr(const WeakPtr& other) noexcept : block_(other.block_), observer_(other.observer_) {
        if (block_) {
            block_->IncrementWeakRefCounter();
        }
    }

    template <typename U>
    WeakPtr(WeakPtr<U>&& other) noexcept : block_(other.block_), observer_(other.observer_) {
        other.block_ = nullptr;
        other.observer_ = nullptr;
    }
    WeakPtr(WeakPtr&& other) noexcept : block_(other.block_), observer_(other.observer_) {
        other.block_ = nullptr;
        other.observer_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) noexcept : block_(other.block_), observer_(other.observer_) {
        if (block_) {
            block_->IncrementWeakRefCounter();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    template <typename U>
    WeakPtr& operator=(WeakPtr<U> other) noexcept {
        if (block_) {
            block_->DecrementWeakRefCounter();
        }
        block_ = other.block_;
        observer_ = other.observer_;
        if (block_) {
            block_->IncrementWeakRefCounter();
        }
        return *this;
    }
    WeakPtr& operator=(WeakPtr other) noexcept {
        if (block_) {
            block_->DecrementWeakRefCounter();
        }
        block_ = other.block_;
        observer_ = other.observer_;
        if (block_) {
            block_->IncrementWeakRefCounter();
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        if (block_) {
            block_->DecrementWeakRefCounter();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        *this = WeakPtr<T>();
    }
    void Swap(WeakPtr& other) {
        std::swap(block_, other.block_);
        std::swap(observer_, other.observer_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        return block_ ? block_->GetRefCount() : 0;
    }
    bool Expired() const {
        return UseCount() == 0;
    }
    SharedPtr<T> Lock() const {
        return Expired() ? SharedPtr<T>() : SharedPtr<T>(*this);
    }

private:
    ControlBlockBase* block_;
    T* observer_;

    template <typename Y>
    friend class SharedPtr;
    template <typename Y>
    friend class WeakPtr;
};
