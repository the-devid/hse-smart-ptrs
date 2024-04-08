#pragma once

#include <cstddef>  // for std::nullptr_t
#include <utility>  // for std::exchange / std::swap

class SimpleCounter {
public:
    size_t IncRef() {
        return ++count_;
    }
    size_t DecRef() {
        return --count_;
    }
    size_t RefCount() const {
        return count_;
    }

private:
    size_t count_ = 0;
};

struct DefaultDelete {
    template <typename T>
    static void Destroy(T* object) {
        delete object;
    }
};

template <typename Derived, typename Counter, typename Deleter>
class RefCounted {
public:
    // Increase reference counter.
    void IncRef() {
        counter_.IncRef();
    }

    // Decrease reference counter.
    // Destroy object using Deleter when the last instance dies.
    void DecRef() {
        if (counter_.DecRef() == 0) {
            Deleter::Destroy(static_cast<Derived*>(this));
        }
    }

    // Get current counter value (the number of strong references).
    size_t RefCount() const {
        return counter_.RefCount();
    }

    RefCounted() {
    }
    // Lots of boilerplate to avoid UB.
    RefCounted([[maybe_unused]] const RefCounted& other) {
    }
    RefCounted([[maybe_unused]] const RefCounted&& other) {
    }
    RefCounted& operator=([[maybe_unused]] const RefCounted& other) {
        return *this;
    }
    RefCounted& operator=([[maybe_unused]] RefCounted&& other) {
        return *this;
    }
    // virtual ~RefCounted() = default;

private:
    Counter counter_;
};

template <typename Derived, typename D = DefaultDelete>
using SimpleRefCounted = RefCounted<Derived, SimpleCounter, D>;

template <typename T>
class IntrusivePtr {
    template <typename Y>
    friend class IntrusivePtr;

public:
    // Constructors
    IntrusivePtr() : observer_(nullptr) {
    }
    IntrusivePtr(std::nullptr_t) : observer_(nullptr) {
    }
    IntrusivePtr(T* ptr) : observer_(ptr) {
        if (observer_) {
            observer_->IncRef();
        }
    }

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y>& other) : observer_(other.observer_) {
        if (observer_) {
            observer_->IncRef();
        }
    }

    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y>&& other) : observer_(other.observer_) {
        other.observer_ = nullptr;
    }

    IntrusivePtr(const IntrusivePtr& other) : observer_(other.observer_) {
        if (observer_) {
            observer_->IncRef();
        }
    }
    IntrusivePtr(IntrusivePtr&& other) : observer_(other.observer_) {
        other.observer_ = nullptr;
    }

    // `operator=`-s
    // IntrusivePtr& operator=(const IntrusivePtr& other);
    // IntrusivePtr& operator=(IntrusivePtr&& other);
    // Copy-and-swap operator=
    IntrusivePtr& operator=(IntrusivePtr other) {
        this->Swap(other);  // |this| placed here to show that we intend Swap(*this, other)
        return *this;
    }

    // Destructor
    ~IntrusivePtr() {
        if (observer_) {
            observer_->DecRef();
        }
    }

    // Modifiers
    void Reset() {
        if (observer_) {
            observer_->DecRef();
        }
        observer_ = nullptr;
    }
    void Reset(T* ptr) {
        if (observer_) {
            observer_->DecRef();
        }
        observer_ = ptr;
        if (observer_) {
            observer_->IncRef();
        }
    }
    void Swap(IntrusivePtr& other) {
        std::swap(observer_, other.observer_);
    }

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
        return observer_ ? observer_->RefCount() : 0;
    }
    explicit operator bool() const {
        return observer_ != nullptr;
    }

private:
    T* observer_ = nullptr;
};

template <typename T, typename... Args>
IntrusivePtr<T> MakeIntrusive(Args&&... args) {
    auto obj = new T(std::forward<Args>(args)...);
    return IntrusivePtr(obj);
}
