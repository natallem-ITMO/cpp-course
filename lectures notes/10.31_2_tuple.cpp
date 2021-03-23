#include <tuple>
#include <iostream>
#include <cassert>
#include <map>
#include <functional>
#include <type_traits>

namespace ex1 {
    std::pair<std::string, int> p("abs", 32);
/*
    В стандартной библиотеке это применяется
        в map - там у нас пары ключ значение
        в некоторых операциях типа insert
        когда надо из функции вернуть несколько значение(как альтернатива передачи ссылки для значения)

    Пара - в какой то степени struct для ленивых.
    По какой то причине не захотели написать
    */
    struct insert_result {
        std::map<int, char>::iterator position;
        bool success;
    };

    /*
     * В какой то степени pair and struct взаимно заменяемы, в случае если мы можем дать имена мемберам, то лучше
     *  использовать структуру, иначе если нормальные имена нельзя дать, то делаем tuple - обобщение пары на n элементов
     */



    int main() {
        /*
         * хотим что то типа
         */

        auto t = std::bind(printf, std::string("hello world"));
    }

    /*
     * Как можно было бы написать bind
     */

    void print(std::string &s) {
        std::cout << s;
    }

    template<typename F, typename Arg>
    struct bound {
        template<typename F1, typename Arg1>
        bound(F1 &&f, Arg1 &&arg)
                : f(std::forward<F1>(f)),
                  arg(std::forward<Arg1>(arg)) {}

        auto operator()() {
            return f(arg);
        }

    private:
        F f;
        Arg arg;
    };

    template<typename F, typename Arg>
    auto bind(F &&f, Arg &&arg) {
        /*срезаем reference и делаем template constuctor for bound struct because of
         * bound<func, int&>
         * bound<func, int>
         * would be different classes, but shouldn't be different classes
         */
//        return bound<F, Arg>(std::remove_reference_t<F>(func),
//                             std::remove_reference_t<Arg>(arg));
        /*
         * При такой реализации при вызове нашей функции от функций или массивов надо писать &print,
         * но мы можем это сделать автоматом, используя decay
         *
         * Т е массивы кастуются к arrya *,  function -> &function , lvalue -> rvalue without reference
         */
        return bound<std::decay_t<F>, std::decay_t<Arg>>(std::forward<F>(f), std::forward<Arg>(arg));

        /*
         * decay - функция
         * https://en.cppreference.com/w/cpp/types/decay
         * По сути, если к нам какая то штука приходит, и мы хотим ее долго хранить, то используем это
         *
         */
    }

    namespace
    part1 {
        int main() {
            auto t = bind(&print, std::string("hello"));
            t();
        }
    }

}

namespace ex2 {
    /*
     * Хотим bind with varargs
     */
    /*  template<typename F, typename ... Arg>
      struct bound {
          template<typename F1, typename ... Arg1>
          bound(F1 &&f, Arg1 &&... arg)
                  : f(std::forward<F1>(f)),
                    arg(std::forward<Arg1>(arg) ...) {}

          auto operator()() {
              return f(arg ...);
          }

      private:
          F f;
          Arg ... arg;
          *//* проблема - объявлять member используя ... нельзя
         * Традиционная рекомендация, если надо хранить внутри - tuple
         *//*
    };

    template<typename F, typename ... Args>
    auto bind(F &&f, Args &&...arg) {
        return bound<std::decay_t<F>, std::decay_t<Args>...>(std::forward<F>(f), std::forward<Args>(arg)...);
    }*/
}

namespace ex3 {
    /*
     * Хотим bind with varargs
     */
    template<typename F, typename ... Args>
    struct bound {
        template<typename F1, typename ... Arg1>
        bound(F1 &&f, Arg1 &&... arg)
                : f(std::forward<F1>(f)),
                  args(std::forward<Arg1>(arg) ...) {}

        auto operator()() {
            return std::apply(f, args);//in tuple already has function that unpack
        }

    private:
        F f;
        std::tuple<Args...> args;
    };

    template<typename F, typename ... Args>
    auto bind1(F &&f, Args &&...arg) {
        return bound<std::decay_t<F>, std::decay_t<Args>...>(std::forward<F>(f), std::forward<Args>(arg)...);
    }

    void print(std::string &s, int a) {
        std::cout << s << a;
    }

    int main() {
        auto t = bind1(print, std::string("hello "), 42);
        t();
    }
    /*
     * Про то, как реализован tuple?
     */
   /* struct tuple: A, tuple<Vs ...>
    {

    };*/
}

namespace ex4{
    /*
   * Про tuple_element
   */
    int main(){
        using tuple = std::tuple<int, float, std::string>;
        using str = std::tuple_element<2, tuple>;
        static_assert(std::is_same<str::type, std::string>::value);
    }
}


int main() {
//    ex1::part1::main();
//    print(std::string("sdf"));
    ex3::main();
}


