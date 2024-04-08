#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t
#include <type_traits>

template <typename T>
struct MyDefaultDelete {
    MyDefaultDelete() = default;

    template <typename U>
    MyDefaultDelete([[maybe_unused]] const MyDefaultDelete<U>& oth) noexcept {
    }

    void operator()(T* ptr) const {
        delete ptr;
    }
};
template <typename T>
struct MyDefaultDelete<T[]> {
    MyDefaultDelete() = default;

    template <typename U>
    MyDefaultDelete([[maybe_unused]] const MyDefaultDelete<U[]>& oth) noexcept {
    }

    void operator()(T* ptr) const {
        delete[] ptr;
    }
};

// Primary template
template <typename T, typename Deleter = MyDefaultDelete<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : ptr_cp_(ptr, Deleter()) {
    }
    template <typename FreakDeleter>
    UniquePtr(T* ptr, FreakDeleter&& deleter) : ptr_cp_(ptr, std::forward<FreakDeleter>(deleter)) {
    }

    template <typename TBase, typename DeleterBase>
    UniquePtr(UniquePtr<TBase, DeleterBase>&& other) noexcept
        : ptr_cp_(std::move(other.ptr_cp_.GetFirst()), std::move(other.ptr_cp_.GetSecond())) {
        other.ptr_cp_.GetFirst() = nullptr;
    }

    UniquePtr(const UniquePtr&) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    template <typename TBase, typename DeleterBase>
    UniquePtr& operator=(UniquePtr<TBase, DeleterBase>&& other) noexcept {
        if (other.ptr_cp_.GetFirst() == this->ptr_cp_.GetFirst()) {
            return *this;
        }
        ptr_cp_.GetSecond()(ptr_cp_.GetFirst());

        ptr_cp_.GetFirst() = std::move(other.ptr_cp_.GetFirst());
        ptr_cp_.GetSecond() = std::move(other.ptr_cp_.GetSecond());

        other.ptr_cp_.GetFirst() = nullptr;
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        ptr_cp_.GetSecond()(ptr_cp_.GetFirst());
        ptr_cp_.GetFirst() = nullptr;
        return *this;
    }

    UniquePtr& operator=(const UniquePtr&) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        ptr_cp_.GetSecond()(ptr_cp_.GetFirst());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        auto result = ptr_cp_.GetFirst();
        ptr_cp_.GetFirst() = nullptr;
        return result;
    }
    void Reset(T* ptr = nullptr) {
        auto what_to_delete = ptr_cp_.GetFirst();
        ptr_cp_.GetFirst() = ptr;
        ptr_cp_.GetSecond()(what_to_delete);
    }
    void Swap(UniquePtr& other) {
        std::swap(ptr_cp_, other.ptr_cp_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_cp_.GetFirst();
    }
    Deleter& GetDeleter() {
        return ptr_cp_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return ptr_cp_.GetSecond();
    }
    explicit operator bool() const {
        return ptr_cp_.GetFirst() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference_t<T> operator*() const {
        return *ptr_cp_.GetFirst();
    }
    T* operator->() const {
        return ptr_cp_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> ptr_cp_;  // ptr in compressed pair.

    template <typename TBase, typename DeleterBase>
    friend class UniquePtr;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : ptr_cp_(ptr, Deleter()) {
    }
    template <typename FreakDeleter>
    UniquePtr(T* ptr, FreakDeleter&& deleter) : ptr_cp_(ptr, std::forward<FreakDeleter>(deleter)) {
    }

    template <typename TBase, typename DeleterBase>
    UniquePtr(UniquePtr<TBase, DeleterBase>&& other) noexcept
        : ptr_cp_(std::move(other.ptr_cp_.GetFirst()), std::move(other.ptr_cp_.GetSecond())) {
        other.ptr_cp_.GetFirst() = nullptr;
    }

    UniquePtr(const UniquePtr&) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    template <typename TBase, typename DeleterBase>
    UniquePtr& operator=(UniquePtr<TBase, DeleterBase>&& other) noexcept {
        if (&other == this) {
            return *this;
        }
        ptr_cp_.GetSecond()(ptr_cp_.GetFirst());

        ptr_cp_.GetFirst() = std::move(other.ptr_cp_.GetFirst());
        ptr_cp_.GetSecond() = std::move(other.ptr_cp_.GetSecond());

        other.ptr_cp_.GetFirst() = nullptr;
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        ptr_cp_.GetSecond()(ptr_cp_.GetFirst());
        ptr_cp_.GetFirst() = nullptr;
        return *this;
    }

    UniquePtr& operator=(const UniquePtr&) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        ptr_cp_.GetSecond()(ptr_cp_.GetFirst());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        auto result = ptr_cp_.GetFirst();
        ptr_cp_.GetFirst() = nullptr;
        return result;
    }
    void Reset(T* ptr = nullptr) {
        auto what_to_delete = ptr_cp_.GetFirst();
        ptr_cp_.GetFirst() = ptr;
        ptr_cp_.GetSecond()(what_to_delete);
    }
    void Swap(UniquePtr& other) {
        std::swap(ptr_cp_, other.ptr_cp_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_cp_.GetFirst();
    }
    Deleter& GetDeleter() {
        return ptr_cp_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return ptr_cp_.GetSecond();
    }
    explicit operator bool() const {
        return ptr_cp_.GetFirst() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference_t<T> operator*() const {
        return *ptr_cp_.GetFirst();
    }
    T* operator->() const {
        return ptr_cp_.GetFirst();
    }
    std::add_lvalue_reference_t<T> operator[](size_t index) const {
        return ptr_cp_.GetFirst()[index];
    }

private:
    CompressedPair<T*, Deleter> ptr_cp_;  // ptr in compressed pair.

    template <typename TBase, typename DeleterBase>
    friend class UniquePtr;
};
