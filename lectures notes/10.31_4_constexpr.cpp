#include <tuple>
#include <iostream>
#include <cassert>
#include <map>
#include <functional>
#include <string_view>
#include <type_traits>
#include <variant>


/*
 * Статическая и динамическая типизация
 */

int f() {
    int k;
    std::cin >> k;
    return k;
}

int a = 42; // статическая инициализация (компилятор сразу в памяти программы положил константу)
int b = f(); /* динамическая инициализация (компилятор зарезервировал место под переменную и потом вызовет фукнцию, куда
запишет результат, при старте программы программы вызвается функция*/

/*
 * Динамическая инициализация отработает до входа в main, деструктор при динамической инициализацией отработает после
 * выхода из main
 *
 * Динамическая инициализация плоха потому что
 *      если у нас много таких переменный, то даже для того чтобы просто запустить
 *          такую программу, понадобилось бы время.
 *
 *      если при инициализации одной глобальной переменной мы обращаемся к другой, которая еще не инициализирована, то
 *          у нас вообще ничего не инициализируется
 *
 *          стандарт гарантирует что в одной единице трансляции у нас инициализация происходит сверху вниз,
 *              но между разными единицами трансляции никакого порядка нет.
 *
 *          аналогично и при разрушении, если им надо обращаться к друг другу, могут возникнуть сложности.
 *
 *
 *      у нас инициализация может бросать исключения - программа будет аварийно завершена, и до мейна мы не дойдем.
 *          т.е. ошибки, которые происходят в функциях, инициализирующих глобальные переменные, нельзя отловить.
 *
 * Предпочтительнее статическая инициализация, для того чтобы не тратить лишнее время при старте программы.
 * Пример - в реализации std::function были какие то глобальные переменные, которые надо было инициализировать, и
 *  получается, сколько у нас function наинстанцировалось, так долго у нас программа и стартует(если инициализация
 *  динамическая). Если же инициализация статическая, то у нас просто будут какие то таблицы(это влияет только на время
 *  динамической загрузки, не существенно, по сути не влияет на время старта)
 */


namespace ex0 {
    template<typename T>
    T const &max(T const &a, T const &b) {
        return a < b ? b : a;
    }

    template<typename A, typename B>
    struct variant {
        char stg[max(sizeof(A), sizeof(B))];
        /*
         * max - потенциально какая то фукнция, которая возращает результат в рантайме (даже если просто return 42)
         *
         */
    };

}

/*
 * Как бы пришлось реализовывать это в С++03
 */
namespace ex1 {
    template<size_t a, size_t b>
    struct max {
        static size_t const value = a < b ? b : a;
    };


    template<typename A, typename B>
    struct variant {
        char stg[max<sizeof(A), sizeof(B)>::value];
    };
}

/*
 * Теперь есть constexpr in C++11
 * Для достаточно небольшого подмножества функций разрешили вызывать в компайл тайме
 *  Если все аргументы, переданные в функцию компайл тайм константы, то функцию вычисляется в компайл тайме
 *
 * Используется в размерах массивов, в темлейтах
 *
 * Ограничения на функции в C++11 было дикое
 *      в функции должен был только один ретерн(т.е. реализовывали через рекурсию)
 *
 * В последующих стандартах constexpr расширяли.
 */
namespace ex2 {
    template<typename T>
    constexpr T const &max(T const &a, T const &b) {
        return a < b ? b : a;
    }

    template<typename A, typename B>
    struct variant {
        char stg[max(sizeof(A), sizeof(B))];
    };
}

/*
 * Раз у нас constexpr - то мы не можем модифицировать объекты.
 *      Но это не так, единственное что это значит - что мы вычисляем что то в компайл тайме
 *
 * constexpr with function -- функция может быть вычислена в компайл тайме
 * constexpr with variable -- обязано быть посчитано в компайл тайме
 */
namespace ex3 {
    int f() {
        int k;
        std::cin >> k;
        return k;
    }

    int t() {
        return 32;
    }

    /*constexpr*/ int b = f(); // cannot be constexpr
    const int a = 23;
    /*constexpr*/ int g = t(); // cannot be constexpr

    /*
     * const - говорит только о том, что переменную нельзя менять, но про время инициализации ничего не гарантируется.
     *
     * const для глобальных переменных:
     *      1) переменную нельзя менять
     *      2) теперь сама переменная может использоваться в compile time выражениях
     */
    template<int T>
    struct x {
    };

    int main() {
        x<a>(); // a - compile time const т.к. инициализатор статический и она константа - условие для того чтобы быть
        // compile time const. Поэтому может использоваться в качестве шаблона.
        // x<b>(); //cannot, because b is not compile time constant value
    }
    /*
     * const - гарантирует, то переменная compile time constant
     */
    /*
     *     3) const для global variables имплаит то, что они static (то, что у них внутренняя линковка)
     *         По предположению Вани, const предполагался как замена define, а define пишется в хедерах
     */
}
namespace ex4 {
    /*заменим все constexpr, по сути ничего не поменяется, но получили гарантию, что они compile time constant*/
    constexpr uint32_t offset_basis = 0x811c9dc5u;
    constexpr uint32_t prime = 0x01000193u;

    constexpr uint32_t fnv1a(std::string_view str) {
        uint32_t result = offset_basis;
        for (size_t i = 0; i != str.size(); ++i) {
            result ^= static_cast<uint8_t>(str[i]);
            result *= prime;
        }
        return result;
    }

    constexpr uint32_t val = fnv1a("Hello");

    /*
     * Если мы не написали constexpr, то компилятор мог либо увидеть constexpr фукнцию и честно посчитать, а мог взять
     *  обычную функцию, соптимизировать и получить результат. Мы это не можем проверить.
     */

    template<uint32_t hash>
    void foo() {}

    int main() {
        foo<fnv1a("hello")>();
        /*
         * Но мы можем детектить как const_expr и более сложные фукнции. Например если бы мы завели класс accumulator
         */
    }

    struct accumulator {
        constexpr accumulator() : value(offset_basis) {};

        constexpr void operator()(uint8_t octet) {
            value ^= octet;
            value *= prime;
        }

        constexpr uint32_t get_value() const {
            return value;
        }

    private :
        uint32_t value;
    };

    int main2() {}

}

namespace ex5 {
    constexpr uint32_t offset_basis = 0x811c9dc5u;
    constexpr uint32_t prime = 0x01000193u;

    struct accumulator {
        constexpr accumulator() : value(offset_basis) {};

        constexpr void operator()(uint8_t octet) {
            value ^= octet;
            value *= prime;
        }

        constexpr uint32_t get_value() const {
            return value;
        }

    private :
        uint32_t value;
    };

    constexpr uint32_t fnv1a(std::string_view str) {
        accumulator acc;
        for (size_t i = 0; i != str.size(); ++i) {
            acc(static_cast<uint8_t>(str[i]));
        }
        return acc.get_value();
    }

    constexpr uint32_t val = fnv1a("Hello");


    template<uint32_t hash>
    void foo() {}

    int main() {
        /*
         * Даже так компилятор понимает и вычисляет,хотя мы создаем объект и как то его внутри меняем.
         *
         * Если же у нас происходит ошибка(например в функции fnv1a выйдем за границы строки, то компилятор об этом
         * сообщит. По сути внутри него будто интерпретатор языка.
         *
         * Все, что как то зависит от окружения(файлы и т.д.) мы не можем делать.
         * Все, что связано с языком - можно!
         *
         * Начиная с С++20 можно даже аллоцировать и освобождать память!
         * Но например пока нельзя бросать и ловить исключения в функциях.
         * Метки нельзя. Зато циклы и if можно.
         *
         * Компиляторы не все могут поддерживать например тоже - это все к тому, что конкретные правила перечислять
         *  смысла нет
         *
         *  Off-top но switch and goto practically the same thing
         */
        int value = 1;
        switch (value) {
            if (false) {
                while (false) {
                    if (false) {
                        while (false) {
                            case 1:
                                std::cout << "hello\n";
                        }
                    }
                }
            }
        }
//        foo<fnv1a("hello")>();
    }
}

/*
 * Проблема с variant. Если бы мы использовали aligned_storage, то не могли бы сделать variant constexpr, потому что
 *      placement new and reinterpret_cast is not supported by constexpr. Для этого можно использовать union
 */
namespace ex6 {
    template<typename A, typename B>
    union storage_t {
        A a;
        B b;
    };

    template<typename A, typename B>

    struct variant {

        variant(A a) : index(0), stg(std::move(a)) {
        }

        A *get_first() {
            return stg.a;
        }

    private:
        storage_t<A, B> stg;
        size_t index;
    };

}

struct foo {
    int a;

    constexpr foo(int a) : a(a) {}

    foo &operator=(foo const &f) {
        a = f.a;
        std::cout << "operator =";
        return *this;
    }
};

constexpr std::variant<int, float, foo> v1(foo{3});
constexpr std::variant<int, float, foo> v2 = v1;

/*
 * По поводу constexpr рукописных таблиц в реализации function - они позволяют соптимизировать код, иначе у нас бы
 * простоянно проверялся бы if(initalize) когда бы мы обращались к функции с static полем.
 */

int main() {
    ex5::main();
}


