#pragma once

struct control_block {
    control_block() noexcept = default;

    void release_ref() noexcept {
        --weak_counter;
        --ref_counter;
        if (ref_counter ==  0) {
            delete_object();
        }
    }

    void release_weak() noexcept {
        --weak_counter;
    }

    void add_ref() noexcept {
        ++ref_counter;
        ++weak_counter;
    }

    void add_weak() noexcept {
        ++weak_counter;
    }

    size_t ref_count() const noexcept {
        return ref_counter;
    }

    virtual ~control_block() = default;

    bool can_delete() const {
        return (weak_counter == 0);
    }


protected:

    virtual void delete_object() noexcept = 0;

private:
    size_t ref_counter = 0;
    size_t weak_counter = 0;
};

template<typename T, typename D>
struct regular_control_block final : control_block, D {
    explicit regular_control_block(T *ptr, D && deleter) : D(std::move(deleter)), ptr(ptr)
    {}

    void delete_object() noexcept override {
        D::operator()(ptr);
    }

    virtual ~regular_control_block() = default;

private:
    T *ptr;
};

template<typename T>
struct emplace_control_block final : control_block {

    template<typename ... Args>
    explicit emplace_control_block(Args &&... args) {
        new(&stg) T(std::forward<Args>(args)...);
    }

    ~emplace_control_block() override = default;

    void delete_object() noexcept override {
        getPtr()->~T();
    }

    T *getPtr() {
        return reinterpret_cast<T *>(&stg);
    }

private:
    typename std::aligned_storage<sizeof(T), alignof(T)>::type stg;
};
