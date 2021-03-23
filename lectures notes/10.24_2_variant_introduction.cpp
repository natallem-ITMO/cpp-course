#include <functional>
#include <variant>


namespace ex1{
    /*
     * Хотим передавать снаржи фукнцию вычисления, т.е. откладываем вычисление некторого занчения - аналог lazy.
     */
    template <typename T>
    struct deferred_value
    {
        using compute_t = std::function<T()>;
        deferred_value(compute_t compute)
        :compute(std::move(compute))
        {}

        T get_value() const {
            if (!cached_value)
            {
                cached_value = compute();
                compute = {}; // assigment empty constructor, т.к. после того как посчитали, нам функция больше не нужна

            }
            return *cached_value;
        }

    private:
        mutable std::function <T ()> compute;
        mutable std::optional<T> cached_value;
        /*
         * т.е. по сути, мы либо храним значение, либо храним функцию.
         * В этом смысле такой инвариант нигде не обозначен, только читая код, можно это понять.
         *
         * Т е у нас есть несколько состояний - и мы храним либо то, либо то, либо то.
         * можно было бы даже использовать юнион
         * Но в std есть variant!
         */
    };

    struct A;
    struct B;
    struct C;

    int main(){
        std::variant<A,B,C> v(A); //  храним либо А либо B либо C
    }
}
namespace ex2{
    /*template <typename T>
    struct deferred_value
    {
        using compute_t = std::function<T()>;
        deferred_value(std::function<T()> compute)
        :compute(std::move(compute))
        {}
        T get_value() const {
            if (!cached_value)
            {
                cached_value = compute();
                compute = {}; // assigment empty constructor, т.к. после того как посчитали, нам функция больше не нужна

            }
            return *cached_value;
        }

    private:
        std::variant<compute_t, T> state;
    };*/

    /*
     * Какие есть операции?
     * index (позволяет получить что у нас внутри)
     * get (позволяет обратиться по индексу)
     * visit (мы ее вызываем - она нас вызывает)
     * variant - число + union из n альтернатив
     */
}

/*
 * string view позволяет объединят char const * and size_t size
 * tuple - что то в роде структоры, но которой не обязательно давать название.
 */