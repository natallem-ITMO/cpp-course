#pragma once

#include <memory>
#include "control_block.h"
#include "shared_ptr.h"

template<typename T>
struct shared_ptr;

template<typename T>
struct weak_ptr {

    weak_ptr() noexcept = default;

    weak_ptr(const weak_ptr &r) noexcept: ptr(r.ptr), c_block(r.c_block) {
        add_if_not_empty();
    };

    template<class Y>
    weak_ptr(const weak_ptr<Y> &r) noexcept: ptr(r.ptr), c_block(r.c_block) {
        add_if_not_empty();
    };

    template<class Y>
    weak_ptr(const shared_ptr<Y> &r) noexcept:ptr(r.ptr), c_block(r.c_block) {
        add_if_not_empty();
    }

    weak_ptr &operator=(const weak_ptr &r) noexcept {
        return copy_assignment_operator(r);
    }

    template<class Y>
    weak_ptr &operator=(const weak_ptr<Y> &r) noexcept {
        return copy_assignment_operator(r);
    }

    template<class Y>
    weak_ptr &operator=(const shared_ptr<Y> &r) noexcept {
        weak_ptr<T>(r).swap(*this);
        return *this;
    }

    weak_ptr(weak_ptr &&r) noexcept {
        swap(r);
    }

    template<class Y>
    weak_ptr(weak_ptr<Y> &&r) noexcept {
        swap(r);
    }

    weak_ptr &operator=(weak_ptr &&r) {
        return move_assignment_operator(std::forward<weak_ptr>(r));
    }

    template<typename Y>
    weak_ptr &operator=(weak_ptr<Y> &&r) {
        return move_assignment_operator(std::forward<weak_ptr<Y>>(r));
    }

    shared_ptr<T> lock() const {
        return (expired()) ? shared_ptr<T>() : shared_ptr<T>(*this);
    }

    long use_count() const noexcept {
        return (c_block != nullptr) ? c_block->ref_count() : 0;
    }

    size_t expired() const noexcept {
        return !(c_block != nullptr && c_block->ref_count() != 0);
    }

    virtual ~weak_ptr() {
        release_if_not_empty();
        if (c_block != nullptr && c_block->can_delete()) {
            delete c_block;
        }
    }

    void swap(weak_ptr &r) noexcept {
        std::swap(r.ptr, ptr);
        std::swap(c_block, r.c_block);
    }

private:
    T *ptr = nullptr;
    control_block *c_block = nullptr;

    template<typename U>
    weak_ptr<T> &copy_assignment_operator(weak_ptr<U> const &r) {
        if (this != &r) {
            weak_ptr<T>(r).swap(*this);
        }
        return *this;
    }

    template<typename U>
    weak_ptr<T> &move_assignment_operator(weak_ptr<U> &&r) {
        if (this != &r) {
            weak_ptr<T>(std::move(r)).swap(*this);
        }
        return *this;
    }

    void add_if_not_empty() {
        if (c_block != nullptr) {
            c_block->add_weak();
        }
    }

    void release_if_not_empty() {
        if (c_block != nullptr) {
            c_block->release_weak();
        }
    }

    template<typename U>
    friend
    struct shared_ptr;

};
