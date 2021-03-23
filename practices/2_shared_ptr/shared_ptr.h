#pragma once

#include <memory>
#include "control_block.h"
#include "weak_ptr.h"

template<typename T>
struct weak_ptr;

template<typename T>
struct shared_ptr {

    /*Constructor*/

    shared_ptr() noexcept = default;

    shared_ptr(std::nullptr_t) noexcept: shared_ptr() {};

    template<typename Y>
    explicit shared_ptr(Y *ptr): shared_ptr(ptr, std::default_delete<Y>()) {};

    template<typename Y, typename D>
    shared_ptr(Y *ptr, D deleter) : ptr(ptr) {
        try {
            c_block = new regular_control_block<Y, D>(ptr, std::move(deleter));
            c_block->add_ref();
        }  catch (...) {
            deleter(ptr);
            throw;
        }
    }

    shared_ptr(shared_ptr const &r) {
        copy_constructor(r);
    };

    template<typename Y>
    shared_ptr(shared_ptr<Y> const &r) {
        copy_constructor(r);
    };

    template<class Y>
    explicit shared_ptr(weak_ptr<Y> const &r) {
        if (r.expired()) {
            throw std::bad_weak_ptr();
        }
        set_fields_and_add_ref(r.ptr, r.c_block);
    };

    shared_ptr(shared_ptr &&r) noexcept {
        swap(r);
    }

    template<class Y>
    shared_ptr(shared_ptr<Y> &&r) noexcept {
        swap(r);
    }

    template<typename Y>
    shared_ptr(shared_ptr<Y> const &sp, T *ptr): ptr(ptr), c_block(sp.c_block) {
        add_if_not_empty();
    };

    template<typename... Args>
    void initialize_make_shared(Args &&... args) {
        auto *emplace_c_block = new emplace_control_block<T>(std::forward<Args>(args)...);
        set_fields_and_add_ref(emplace_c_block->getPtr(), emplace_c_block);
    };

    /*Operator=*/

    shared_ptr &operator=(shared_ptr const &r) noexcept {
        return copy_assignment_operator(r);
    }

    template<typename Y>
    shared_ptr &operator=(shared_ptr<Y> const &r) noexcept {
        return copy_assignment_operator(r);
    }

    shared_ptr &operator=(shared_ptr &&r) noexcept {
        return move_assignment_operator(std::forward<shared_ptr>(r));
    }

    template<class Y>
    shared_ptr &operator=(shared_ptr<Y> &&r) noexcept {
        return move_assignment_operator(std::forward<shared_ptr<T>>(r));
    }

    /*Destructor*/

    ~shared_ptr() {
        release_if_not_empty();
        if (c_block != nullptr && c_block->can_delete()) {
            delete c_block;
        }
    }

    /*Modifiers*/

    void reset() noexcept {
        release_if_not_empty();
        ptr = nullptr;
        c_block = nullptr;
    }

    template<class Y>
    void reset(Y *r) {
        shared_ptr<T>(r).swap(*this);
    }

    template<class Y, class Deleter>
    void reset(Y *r, Deleter d) {
        shared_ptr<T>(r, std::move(d)).swap(*this);
    }

    void swap(shared_ptr &r) noexcept {
        std::swap(c_block, r.c_block);
        std::swap(ptr, r.ptr);
    }

    T *get() const noexcept {
        return ptr;
    }

    T &operator*() const noexcept {
        return *ptr;
    }

    T *operator->() const noexcept {
        return ptr;
    }

    size_t use_count() const noexcept {
        return (c_block != nullptr) ? c_block->ref_count() : 0;
    }

    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }

    bool unique() const noexcept {
        return use_count() == 1;
    }

    bool operator==(std::nullptr_t) const noexcept {
        return ptr == nullptr;
    }

private:
    T *ptr = nullptr;
    control_block *c_block = nullptr;

    template<typename U>
    void copy_constructor(shared_ptr<U> const &r) {
        set_fields_and_add_ref(r.ptr, r.c_block);
    }

    template<typename U>
    shared_ptr<T> &copy_assignment_operator(shared_ptr<U> const &r) {
        if (this != &r) {
            shared_ptr<T>(r).swap(*this);
        }
        return *this;
    }

    template<typename U>
    shared_ptr<T> &move_assignment_operator(shared_ptr<U> &&r) {
        if (this != &r) {
            shared_ptr<T>(std::move(r)).swap(*this);
        }
        return *this;
    }

    void add_if_not_empty() {
        if (c_block != nullptr) {
            c_block->add_ref();
        }
    }

    void release_if_not_empty() {
        if (c_block != nullptr) {
            c_block->release_ref();
        }
    }

    void set_fields_and_add_ref(T *new_ptr, control_block *new_c_block) {
        ptr = new_ptr;
        c_block = new_c_block;
        add_if_not_empty();
    }

    template<typename Y>
    friend
    struct weak_ptr;

    template<typename Y>
    friend
    struct shared_ptr;
};


template<class Y, class... Args>
shared_ptr<Y> make_shared(Args &&... args) {
    shared_ptr<Y> ptr;
    ptr.initialize_make_shared(std::forward<Args>(args)...);
    return ptr;
}


template<class T>
void swap(shared_ptr<T> &lhs, shared_ptr<T> &rhs) noexcept {
    lhs.swap(rhs);
}

template<class U, class V>
bool operator==(const shared_ptr<U> &lhs, const shared_ptr<V> &rhs) noexcept {
    return lhs.get() == rhs.get();
}

template<class U, class V>
bool operator!=(const shared_ptr<U> &lhs, const shared_ptr<V> &rhs) noexcept {
    return !(lhs == rhs);
}

template<class T>
bool operator==(std::nullptr_t, const shared_ptr<T> &rhs) noexcept {
    return rhs == nullptr;
}

template<class T>
bool operator!=(std::nullptr_t, const shared_ptr<T> &rhs) noexcept {
    return !(rhs == nullptr);
}

template<class T>
bool operator!=(const shared_ptr<T> &rhs, std::nullptr_t) noexcept {
    return !(rhs == nullptr);
}
