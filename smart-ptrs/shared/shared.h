#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <new>
#include <type_traits>

class ControlBlockBase {
public:
    virtual void IncrementRefCounter() = 0;
    virtual void DecrementRefCounter() = 0;
    virtual void IncrementWeakRefCounter() = 0;
    virtual void DecrementWeakRefCounter() = 0;
    virtual size_t GetRefCount() = 0;
    virtual ~ControlBlockBase() {
    }
};

class ESFTBase {};

template <typename Y>
class ControlBlockWithPtr : public ControlBlockBase {
public:
    ControlBlockWithPtr(Y* ptr) : ptr_(ptr), ref_counter_(0), weak_ref_counter_(0) {
    }

    virtual void IncrementRefCounter() override {
        ++ref_counter_;
        ++weak_ref_counter_;
    }
    virtual void DecrementRefCounter() override {
        --ref_counter_;
        --weak_ref_counter_;
        if (ref_counter_ == 0) {
            ++weak_ref_counter_;  // This prevents us from clearing memory "under legs" in ESFT case
            delete ptr_;
            --weak_ref_counter_;
        }
        if (weak_ref_counter_ == 0) {
            delete this;
        }
    }

    virtual void IncrementWeakRefCounter() override {
        ++weak_ref_counter_;
    }
    virtual void DecrementWeakRefCounter() override {
        --weak_ref_counter_;
        if (weak_ref_counter_ == 0) {
            delete this;
        }
    }

    virtual size_t GetRefCount() override {
        return ref_counter_;
    }

private:
    Y* ptr_;
    size_t ref_counter_;
    size_t weak_ref_counter_;
};

template <typename Y>
class ControlBlockOwning : public ControlBlockBase {
    template <typename... Args>
    ControlBlockOwning(Args&&... args) : ref_counter_(0), weak_ref_counter_(0) {
        new (&buffer_) Y(std::forward<Args>(args)...);
    }

    virtual void IncrementRefCounter() override {
        ++ref_counter_;
        ++weak_ref_counter_;
    }
    virtual void DecrementRefCounter() override {
        --ref_counter_;
        --weak_ref_counter_;
        if (ref_counter_ == 0) {
            ++weak_ref_counter_;  // This prevents us from clearing memory "under legs" in ESFT case
            reinterpret_cast<Y*>(&buffer_)->~Y();
            --weak_ref_counter_;
        }
        if (weak_ref_counter_ == 0) {
            delete this;
        }
    }

    virtual void IncrementWeakRefCounter() override {
        ++weak_ref_counter_;
    }
    virtual void DecrementWeakRefCounter() override {
        --weak_ref_counter_;
        if (weak_ref_counter_ == 0) {
            delete this;
        }
    }

    virtual size_t GetRefCount() override {
        return ref_counter_;
    }

private:
    std::aligned_storage_t<sizeof(Y), alignof(Y)> buffer_;
    size_t ref_counter_;
    size_t weak_ref_counter_;

    template <typename T, typename... Args>
    friend SharedPtr<T> MakeShared(Args&&... args);
};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() noexcept : block_(nullptr), observer_(nullptr) {
    }
    SharedPtr(std::nullptr_t) noexcept : SharedPtr() {
    }
    template <typename Y>
    explicit SharedPtr(Y* ptr) noexcept : block_(new ControlBlockWithPtr(ptr)), observer_(ptr) {
        if (block_) {
            block_->IncrementRefCounter();
            if constexpr (std::is_convertible_v<Y*, ESFTBase*>) {
                ptr->weak_this_ = WeakPtr(*this);
            }
        }
    }

    template <typename U>
    SharedPtr(const SharedPtr<U>& other) noexcept
        : block_(other.block_), observer_(other.observer_) {
        if (block_) {
            block_->IncrementRefCounter();
        }
    }
    SharedPtr(const SharedPtr& other) noexcept : block_(other.block_), observer_(other.observer_) {
        if (block_) {
            block_->IncrementRefCounter();
        }
    }

    template <typename U>
    SharedPtr(SharedPtr<U>&& other) noexcept : block_(other.block_), observer_(other.observer_) {
        other.block_ = nullptr;
        other.observer_ = nullptr;
    }
    SharedPtr(SharedPtr&& other) noexcept : block_(other.block_), observer_(other.observer_) {
        other.block_ = nullptr;
        other.observer_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) noexcept : block_(other.block_), observer_(ptr) {
        if (block_) {
            block_->IncrementRefCounter();
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.Expired()) {
            throw BadWeakPtr();
        }
        block_ = other.block_;
        observer_ = other.observer_;
        if (block_) {
            block_->IncrementRefCounter();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    // Fashioned operator=
    template <typename U>
    SharedPtr& operator=(SharedPtr<U> other) noexcept {
        if (block_) {
            block_->DecrementRefCounter();
        }
        block_ = other.block_;
        observer_ = other.observer_;
        if (block_) {
            block_->IncrementRefCounter();
        }
        return *this;
    }
    SharedPtr& operator=(SharedPtr other) noexcept {
        if (block_) {
            block_->DecrementRefCounter();
        }
        block_ = other.block_;
        observer_ = other.observer_;
        if (block_) {
            block_->IncrementRefCounter();
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        if (block_) {
            block_->DecrementRefCounter();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        *this = SharedPtr<T>(nullptr);
    }
    template <typename Y>
    void Reset(Y* ptr) {
        *this = SharedPtr<T>(ptr);
    }
    void Swap(SharedPtr& other) {
        std::swap(block_, other.block_);
        std::swap(observer_, other.observer_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return observer_;
    }
    T& operator*() const {
        return *observer_;
    }
    T* operator->() const {
        return observer_;
    }
    size_t UseCount() const {
        return block_ ? block_->GetRefCount() : 0;
    }
    explicit operator bool() const {
        return observer_ != nullptr;
    }

private:
    ControlBlockBase* block_;
    T* observer_;

    template <typename Y>
    friend class SharedPtr;

    template <typename Y>
    friend class WeakPtr;

    template <typename U, typename... Args>
    friend SharedPtr<U> MakeShared(Args&&... args);
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    SharedPtr<T> result;
    auto control_block = new ControlBlockOwning<T>(std::forward<Args>(args)...);
    result.block_ = control_block;
    result.observer_ = reinterpret_cast<T*>(&control_block->buffer_);
    result.block_->IncrementRefCounter();
    if constexpr (std::is_convertible_v<T*, ESFTBase*>) {
        result.observer_->weak_this_ = WeakPtr(result);
    }
    return result;
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis : public ESFTBase {
public:
    SharedPtr<T> SharedFromThis() {
        return SharedPtr(weak_this_);
    }
    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr(weak_this_);
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_this_;
    }
    WeakPtr<const T> WeakFromThis() const noexcept {
        return weak_this_;
    }

private:
    WeakPtr<T> weak_this_;

    template <typename>
    friend class SharedPtr;

    template <typename Y, typename... Args>
    friend SharedPtr<Y> MakeShared(Args&&... args);
};
