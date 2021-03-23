#pragma once

#include <type_traits>
#include <exception>

struct bad_function_call : std::exception {
    char const *what() const noexcept override {
        return "empty function call";
    }
};

template<typename T>
constexpr bool is_small_v = sizeof(T) <= sizeof(void *)
                            && (alignof(void *) % alignof(T) == 0)
                            && std::is_nothrow_move_constructible<T>::value;

template<typename R, typename... Args>
struct descriptor {
    using invoke_fn_t = R (*)(void *, Args &&...);
    using delete_obj_fn_t = void (*)(void *);
    using copy_constructor_fn_t = void (*)(void *dest, void const *srs);
    using move_constructor_fn_t = void (*)(void *dest, void *srs);
    using target_fn_t = void *(*)(void *);
    using const_target_fn_t = void const *(*)(void const *);

    invoke_fn_t invoke;
    delete_obj_fn_t delete_obj;
    copy_constructor_fn_t copy_constructor;
    move_constructor_fn_t move_constructor;
    target_fn_t target;
    const_target_fn_t const_target;
};

template<typename R, typename... Args>
descriptor<R, Args...> const *get_empty_descriptor() {
    static constexpr descriptor<R, Args...> table{
            [](void *, Args &&...) -> R { // invoke
                throw bad_function_call();
            },
            [](void *) { // delete_obj
            },
            [](void *, void const *) { //copy_constructor
            },
            [](void *, void *) { // move_constructor
            },
            [](void *) -> void * { // target
                return nullptr;
            },
            [](void const *) -> void const * { // const_target
                return nullptr;
            }
    };

    return &table;
}

template<typename T, bool isSmall>
struct object_traits;

template<typename T>
struct object_traits<T, true> {
    template<typename R, typename... Args>
    descriptor<R, Args...> const *get_type_descriptor() {
        static constexpr descriptor<R, Args...> table{
                [](void *obj, Args &&... args) -> R { // invoke
                    return (*static_cast<T *>(obj))(std::forward<Args>(args) ...);
                },
                [](void *obj) { // delete_obj
                    static_cast<T *>(obj)->~T();
                },
                [](void *dest, void const *src) { // copy_constructor
                    new(dest) T(*static_cast<T const *>(src));
                },
                [](void *dest, void *src) { // move_constructor
                    new(dest) T(std::move(*static_cast<T *>(src)));
                },
                [](void *obj) -> void * { // target
                    return obj;
                },
                [](void const *obj) -> void const * { // const_target
                    return obj;
                }
        };
        return &table;
    }
};

template<typename T>
struct object_traits<T, false> {
    template<typename R, typename... Args>
    descriptor<R, Args...> const *get_type_descriptor() const {
        static constexpr descriptor<R, Args...> table{
                [](void *obj, Args &&... args) -> R { // invoke
                    return (**(static_cast<T **>(obj)))(std::forward<Args>(args)...);
                },
                [](void *obj) { // delete_obj
                    delete *static_cast<T **>(obj);
                },
                [](void *dest, void const *src) { // copy_constructor
                    T *&ref_to_dest = *static_cast<T **>(dest);
                    ref_to_dest = new T(**static_cast<T *const *>(src));
                },
                [](void *dest, void *src) { // move constructor
                    T *&ref_to_dest = *static_cast<T **>(dest);
                    ref_to_dest = *static_cast<T **>(src);
                },
                [](void *obj) -> void * { // target
                    return *static_cast<T **>(obj);
                },
                [](void const *obj) -> void const * { // const_target
                    return *static_cast<T *const *>(obj);
                }
        };
        return &table;
    }
};

template<typename R, typename... Args>
struct storage {

    storage(const descriptor<R, Args...> *descriptor) : descr(descriptor) {}

    storage(storage const &other) {
        other.descr->copy_constructor(&obj, &other.obj);
        descr = other.descr;
    };

    storage(storage &&other) noexcept {
        move_initialize(std::move(other));
    }

    ~storage() {
        descr->delete_obj(&obj);
    }

    storage &operator=(storage &&rhs) noexcept {
        descr->delete_obj(&obj);
        move_initialize(std::move(rhs));
        return *this;
    };

    R invoke(Args &&... args) const {
        void const *const_void = &obj;
        return descr->invoke(const_cast<void *>(const_void), std::forward<Args>(args)...);
    }

    void swap(storage &other) noexcept {
        std::aligned_storage<sizeof(void *), alignof(void *)>::type temp;
        other.descr->move_constructor(&temp, &other.obj);
        descr->move_constructor(&other.obj, &obj);
        other.descr->move_constructor(&obj, &temp);
        std::swap(descr, other.descr);
    }

    void *void_target() noexcept {
        return descr->target(&obj);
    }

    void const *void_const_target() const noexcept {
        return descr->const_target(&obj);
    }

    std::aligned_storage<sizeof(void *), alignof(void *)>::type obj;
    descriptor<R, Args...> const *descr;

private :
    void move_initialize(storage &&other) {
        other.descr->move_constructor(&obj, &other.obj);
        descr = other.descr;
        other.descr = get_empty_descriptor<R, Args ...>();
    }
};

template<typename T>
struct function;

template<typename R, typename... Args>
struct function<R(Args...)> {
    function() noexcept: stg(get_empty_descriptor<R, Args ...>()) {};

    function(function const &other) = default;

    function(function &&other) noexcept = default;

    template<typename T>
    function(T val) : stg(object_traits<T, is_small_v<T>>().template get_type_descriptor<R, Args ...>()) {
        if constexpr (is_small_v<T>) {
            new(&stg.obj) T(std::move(val));
        } else {
            reinterpret_cast<T *&>(stg.obj) = new T(std::move(val));
        }
    }

    function &operator=(function const &rhs) {
        if (this != &rhs) {
            function(rhs).swap(*this);
        }
        return *this;
    };

    function &operator=(function &&rhs) noexcept {
        if (this != &rhs) {
            stg = std::move(rhs.stg);
        }
        return *this;
    };

    ~function() = default;

    void swap(function &other) noexcept {
        stg.swap(other.stg);
    }

    R operator()(Args... args) const {
        return stg.invoke(std::forward<Args>(args) ...);
    }

    explicit operator bool() const noexcept {
        return stg.descr != get_empty_descriptor<R, Args...>();
    }

    template<typename T>
    T *target() noexcept {
        if (stg.descr == object_traits<T, is_small_v<T>>().template get_type_descriptor<R, Args ...>()) {
            return static_cast<T *>(stg.void_target());
        } else {
            return nullptr;
        }
    }

    template<typename T>
    T const *target() const noexcept {
        if (stg.descr == object_traits<T, is_small_v<T>>().template get_type_descriptor<R, Args ...>()) {
            return static_cast<T const *>(stg.void_const_target());
        } else {
            return nullptr;
        }
    }

private :
    storage<R, Args...> stg;
};
