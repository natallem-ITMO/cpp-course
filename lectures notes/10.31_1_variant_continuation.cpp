#include <functional>
#include <variant>
#include <memory>
#include <iostream>


namespace ex1 {
    template<typename T>
    struct deferred_value {
        using compute_t = std::function<T()>;

        deferred_value(compute_t compute)
                : state(std::move(compute)) {}

        T get_value() const {
            /* Плохо, что get and index() разделены - потенциально это источник ошибки */
            if (state.index() == 0) {
                state = std::get<0>(state)();
            }
            return std::get<1>(state);
        }

    private:
        mutable std::variant<compute_t, T> state;
    };

    int main() {
        deferred_value<int> val([] {
            std::cout << "computing\n";
            return 42;
        });

        std::cout << val.get_value() << '\n';
        std::cout << val.get_value() << '\n';
        std::cout << val.get_value() << '\n';
        std::cout << val.get_value() << '\n';
    }
}

/*
 * Заменим функцию get and index() на хелпер, которая одновременно и проверяет, что объект присутсвует, и возращает его.
 *  Работает как dinamic_cast<>
 */
namespace ex2 {
    template<typename T>
    struct deferred_value {
        using compute_t = std::function<T()>;

        deferred_value(compute_t compute)
                : state(std::move(compute)) {}

        T get_value() const {
            if (compute_t *f = std::get_if<0>(&state)) {
                state = (*f)();
            }
            return std::get<1>(state);
        }

    private:
        mutable std::variant<compute_t, T> state;
    };

    int main() {
        deferred_value<int> val([] {
            std::cout << "computing\n";
            return 42;
        });

        std::cout << val.get_value() << '\n';
        std::cout << val.get_value() << '\n';
        std::cout << val.get_value() << '\n';
        std::cout << val.get_value() << '\n';
    }
}

/*
 * Хорошо было бы, если бы в variant можно было бы положить 2 одинаковых типа?
 *  std::variant<int, int, float> smth;
 * Минус этого подхода в том, что мы теряет возможность доставать объект не по индексу std::get<1>(smth), а по типу.
 */
namespace ex3 {
    template<typename ResultType, typename ErrorType>
    struct expected {
        std::variant<ResultType, ErrorType> value;
    };
    /*
     * Если бы variant хранил бы только значения разных типов, то был бы спецаильный случай при совпадении ResultType and
     *      ErrorType, когда бы наш шаблонный класс не раб отал бы.
     *
     *      Пришлось бы хранить какие то типы обертки.
     *
     *      Поэтому когда идетифицируем индексом, всегда будет работать.
     */
}

/*
 * Вдруг мы забили рассмотреть какую то алтернативу? Мы бы хотели написать код, в котором получали бы ворнинг или ошибку
 *  компиляции, если забыли рассмотреть альтернативу.
 *
 * Рассмотри что такое visitor.
 */
namespace ex4 {
    struct base {
        virtual void foo() = 0;

        virtual ~base() = default;
    };

    struct derived1 : base {
    };
    struct derived2 : base {
    };
    struct derived3 : base {
    };

    void foo(base &b) {
        /* b.foo();*/
        /*
         * Если мы хотим перебрать все варианты, то стоит в первую очередь сделать виртуальную функцию в базе.
         * Но минус такого подхода в том, что у нас foo не всегда должно относится к самим классам, возможно оно должно
         *  относится к функции. Виртуальная функция - засорение класса.
         *
         *  Например допустим у нас есть дерево разбора, в котором разные ноды. И пусть у нас есть функции разные,
         *      которые ходят по этим узлам и что то делают. Заводить для каждой такой функции функцию в классе ноды
         *      дерева не очень. Код разрастется, и будет куча не связанных функций в классе.
         */

        /*
         * Как альтернатива - dynamic_cast<>, но минус в том, что можем забыть какую то ветвь.
         */
        if (derived1 *d1 = dynamic_cast<derived1 *>(&b)) {
        }

        /*
         * Поэтому используют visitor
         */
    }

    struct base_visitor {
        virtual void visit1(derived1 &) = 0;

        virtual void visit1(derived2 &) = 0;

        virtual void visit1(derived3 &) = 0;

        virtual ~base_visitor() = default;
    };
}

namespace ex5 {
    struct derived1;
    struct derived2;
    struct derived3;

    struct base_visitor {

        virtual void visit(derived1 &) = 0;

        virtual void visit(derived2 &) = 0;

        virtual void visit(derived3 &) = 0;

        virtual ~base_visitor() = default;
    };

    struct base {
        virtual void accept(base_visitor &v) = 0;

        virtual ~base() = default;
    };

    struct derived1 : base {
        void accept(base_visitor &v) override {
            v.visit(*this);
        }
    };

    struct derived2 : base {
    };

    struct derived3 : base {
    };

    struct foo_visitor : base_visitor {
        void visit(derived1 &) override {
        };

        void visit(derived2 &) override {
        }

        void visit(derived3 &) override {
        }
    };


    void foo(base &b) {
        /*
         * Идея - вызывать некоторую функцию, передать ей операции - что делать на derived1/2/3. Т.е. мы ее
         *  вызвали, а дальше она нас вызвала.
         */
        foo_visitor v;
        b.accept(v);
    }
    /*
     * Плюс визитора и в том, что можно дописывать свичи(которые по сути целый класс foo_visitor),
     *      не имея доступа к исходной иерархии классов.
     */

    /*
     * Пример который может быть - все наши derived struct - Const, Multiplication, Addition
     * Ну и когда мы захотим делать какие то операции, по типу приведения подобных слагаемых и т.д., то понадобится
     * свич по классам.
     */
}

/* У variant есть визитор*/
namespace ex6 {
    template<typename T>
    struct deferred_value {
        using compute_t = std::function<T()>;

        struct state_visitor {
            void operator()(compute_t const &func) {
            }

            void operator()(T const &val) {
            }

            template<typename U>
            void operator()(U const &v) {

            }
        };


        deferred_value(compute_t compute)
                : state(std::move(compute)) {}

        T get_value() const {
            std::visit(state_visitor(), state);
            /* Если будут 2 одинаковых типа - то перейдут в одну функцию
             * Что хорошо - можем написать даже шаблонную функцию в классе визитора
             * Но что если код визитора тесно связан с функцией? а мы заводим целый отдельный класс.
             * Можно ли использовать лямбду вместо класса?
             * Есть несколько способов
             * Лямбду нельзя перегрузить по сути. Но можно сделать класс, у которого будет оператор (), такой, что он
             *   будет вызывать оператор() у лямбд. Т.е. это будет какой то библиотечный хелпер, который позволит
             *   перегружать лямбды.
             */
        }

    private:
        mutable std::variant<compute_t, T> state;
    };
}

namespace ex7 {


    template<typename ... Funcs>
    struct overloaded;

    template<typename Func0>
    struct overloaded<Func0> : Func0 {
        overloaded(Func0 &&f1) :
                Func0(std::forward<Func0>(f1)) {}

        using Func0::operator();
    };

    template<typename Func0, typename ... Funcs>
    struct overloaded<Func0, Funcs ...> : Func0, overloaded<Funcs...> {
        overloaded(Func0 &&f1, Funcs &&...funcs) :
                Func0(std::forward<Func0>(f1)),
                overloaded<Funcs...>(std::forward<Funcs>(funcs)...) {}

        using Func0::operator();
        using overloaded<Funcs...>::operator();
    };

    template<typename ... Funcs>
    overloaded<Funcs ...> overload(Funcs &&... funcs) {
        return overloaded<Funcs ...>(std::forward<Funcs>(funcs) ...);
    }

    template<typename T>
    struct deferred_value {
        using compute_t = std::function<T()>;

        deferred_value(compute_t compute)
                : state(std::move(compute)) {}

        T get_value() const {
            std::visit(overload( /*умеет принимать что угодно и передавать в нужную лямбду*/
                    [](compute_t const &) {
                        std::cout << "compute\n";
                    },
                    [](T const &) {
                        std::cout << "value\n";
                    }), state);
        }
    private:
        mutable std::variant<compute_t, T> state;
    };

    int main() {
        deferred_value<int> d([]() {
            std::cout << "creating\n";
            return 9;
        });
        d.get_value();

        /*   auto f = [](){
               std::cout << "fake\n";
           };

           overload(f);*/ // не работает, т.к. lvalue ссылка
    }
}

/*Вариант с наследованием проще*/
namespace ex8 {


    /*use std::remove_reference_t to use with & */
    template<typename ... Funcs>
    struct overloaded : std::remove_reference_t<Funcs> ... {
        overloaded(Funcs &&... fs) :  std::remove_reference_t<Funcs>(std::forward<Funcs>(fs)) ... {}

        using  std::remove_reference_t<Funcs>::operator() ...;
    };


    template<typename ... Funcs>
    overloaded<Funcs ...> overload(Funcs &&... funcs) {
        return overloaded<Funcs ...>(std::forward<Funcs>(funcs) ...);
    }

    template<typename T>
    struct deferred_value {
        using compute_t = std::function<T()>;

        deferred_value(compute_t compute)
                : state(std::move(compute)) {}

        T const &get_value() const {
            /* return std::visit(overload( *//*умеет принимать что угодно и передавать в нужную лямбду*//*
                    [this](compute_t const &compute) {
                        std::cout << "compute\n";
                        auto val = compute();
                        state = val;
                        return val;
                    },
                    [](T const &val) {
                        std::cout << "value\n";
                        return val;
                    }), state);*/
            /* но конкретно в это случае не стоит париться с визиторами, т.к. в реальном случае лучше возращать T const &
            */

            return std::visit(overload(
                    [this](compute_t const &compute) -> T const & {
                        std::cout << "compute\n";
                        auto val = compute();
                        state = val;
                        return std::get<1>(state);
                    },
                    [](T const &val) -> T const & {
                        std::cout << "value\n";
                        return val;
                    }), state);

        }

    private:
        mutable std::variant<compute_t, T> state;
    };

    int main() {
        deferred_value<int> d([]() {
            std::cout << "creating\n";
            return 9;
        });
        std::cout << d.get_value() << "\n";
        std::cout << d.get_value() << "\n";
        std::cout << d.get_value() << "\n";

        auto f = []() {
            std::cout << "fake\n";
        };

        auto t = overload(f); // use std::remove_reference
        t();

        /*но overload не работает для обычных функций(т.к. не можем отнаследоваться от функции) */
,
    }
}

/*Как variant взаимодействует с исключениями
 * Когда рассматривали реализацию optional, то поняли, что если его объект тривильный, то и наш optional тоже будет
 *  тривильным.
 * Давайте сделаем такой же трюк для variant.
 * Он имеет ровно такую же технику реализации - если все альтернативы тривиально разрушаемы, то и он тривильно разрушаем
 * Для variant это особенно важно.
 * Можно проверить код в compiler explorer
 *
#include <variant>
#include <utility>

struct foo{
    int a;
    double e;
};

using type = std::variant<int,double, char, foo>;

void copy(type& a, type&b){
    a = b;
}
 * Увидим, что пока мы тривиальны, то просто копируем все байты
copy(std::variant<int, double, char, foo>&, std::variant<int, double, char, foo>&):
        movdqu  xmm0, XMMWORD PTR [rsi]
        movups  XMMWORD PTR [rdi], xmm0
        movzx   eax, BYTE PTR [rsi+16]
        mov     BYTE PTR [rdi+16], al
        ret
 *
 * Но как только один из типов нетривиален*/
/*
std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__variant_idx_cookie (*)(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}&&, std::variant<int, double, char, foo> const&)>, std::integer_sequence<unsigned long, 0ul> >::__visit_invoke(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}, std::variant<int, double, char, foo> const):
        mov     rax, QWORD PTR [rdi]
        mov     edx, DWORD PTR [rsi]
        cmp     BYTE PTR [rax+16], 0
        mov     DWORD PTR [rax], edx
        jne     .L2
        ret
.L2:
        mov     BYTE PTR [rax+16], 0
        ret
std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__variant_idx_cookie (*)(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}&&, std::variant<int, double, char, foo> const&)>, std::integer_sequence<unsigned long, 18446744073709551615ul> >::__visit_invoke(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}, std::variant<int, double, char, foo> const):
        mov     rax, QWORD PTR [rdi]
        mov     BYTE PTR [rax+16], -1
        ret
std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__variant_idx_cookie (*)(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}&&, std::variant<int, double, char, foo> const&)>, std::integer_sequence<unsigned long, 1ul> >::__visit_invoke(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}, std::variant<int, double, char, foo> const):
        mov     rax, QWORD PTR [rdi]
        movsd   xmm0, QWORD PTR [rsi]
        cmp     BYTE PTR [rax+16], 1
        movsd   QWORD PTR [rax], xmm0
        je      .L9
        mov     BYTE PTR [rax+16], 1
        ret
.L9:
        ret
std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__variant_idx_cookie (*)(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}&&, std::variant<int, double, char, foo> const&)>, std::integer_sequence<unsigned long, 2ul> >::__visit_invoke(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}, std::variant<int, double, char, foo> const):
        mov     rax, QWORD PTR [rdi]
        cmp     BYTE PTR [rax+16], 2
        je      .L13
        mov     BYTE PTR [rax+16], -1
        movzx   edx, BYTE PTR [rsi]
        mov     BYTE PTR [rax+16], 2
        mov     BYTE PTR [rax], dl
        ret
.L13:
        movzx   edx, BYTE PTR [rsi]
        mov     BYTE PTR [rax], dl
        ret
std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__variant_idx_cookie (*)(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}&&, std::variant<int, double, char, foo> const&)>, std::integer_sequence<unsigned long, 3ul> >::__visit_invoke(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}, std::variant<int, double, char, foo> const):
        push    rbx
        mov     rbx, QWORD PTR [rdi]
        cmp     BYTE PTR [rbx+16], 3
        jne     .L15
        movdqu  xmm0, XMMWORD PTR [rsi]
        movups  XMMWORD PTR [rbx], xmm0
        pop     rbx
        ret
.L15:
        mov     BYTE PTR [rbx+16], -1
        mov     rdi, rbx
        call    foo::foo(foo const&)
        mov     BYTE PTR [rbx+16], 3
        pop     rbx
        ret
copy(std::variant<int, double, char, foo>&, std::variant<int, double, char, foo>&):
        sub     rsp, 24
        movsx   rax, BYTE PTR [rsi+16]
        mov     QWORD PTR [rsp+8], rdi
        lea     rdi, [rsp+8]
        call    [QWORD PTR std::__detail::__variant::__gen_vtable<std::__detail::__variant::__variant_idx_cookie, std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}&&, std::variant<int, double, char, foo> const&>::_S_vtable[8+rax*8]]
        add     rsp, 24
        ret
std::__detail::__variant::__gen_vtable<std::__detail::__variant::__variant_idx_cookie, std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}&&, std::variant<int, double, char, foo> const&>::_S_vtable:
        .quad   std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__variant_idx_cookie (*)(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}&&, std::variant<int, double, char, foo> const&)>, std::integer_sequence<unsigned long, 18446744073709551615ul> >::__visit_invoke(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}, std::variant<int, double, char, foo> const)
        .quad   std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__variant_idx_cookie (*)(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}&&, std::variant<int, double, char, foo> const&)>, std::integer_sequence<unsigned long, 0ul> >::__visit_invoke(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}, std::variant<int, double, char, foo> const)
        .quad   std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__variant_idx_cookie (*)(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}&&, std::variant<int, double, char, foo> const&)>, std::integer_sequence<unsigned long, 1ul> >::__visit_invoke(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}, std::variant<int, double, char, foo> const)
        .quad   std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__variant_idx_cookie (*)(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}&&, std::variant<int, double, char, foo> const&)>, std::integer_sequence<unsigned long, 2ul> >::__visit_invoke(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}, std::variant<int, double, char, foo> const)
        .quad   std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__variant_idx_cookie (*)(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}&&, std::variant<int, double, char, foo> const&)>, std::integer_sequence<unsigned long, 3ul> >::__visit_invoke(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo>::operator=(std::__detail::__variant::_Copy_assign_base<false, int, double, char, foo> const&)::{lambda(auto:1&&, auto:2)#1}, std::variant<int, double, char, foo> const)
 * */
/*
 * Надо посвичится по таблице в зависимости от того что внутри нас сейчас.
 */
namespace ex9 {
    /*
     * Как можно было бы реализовать.
     */
    template<typename A, typename B>
    struct variant {   /*
         * Вопрос,какую гарантию безопасности можем дать?
         * Скорее всего сначала нужно разрушить объект, потом скопировать.
         * Поэтому если операторы всех типов ноуфроу, то мы можем сделать его весь ноуфроу.
         * Но если хотя бы один бросает, то не понятно что делать. У нас может старый разрушиться, а новый не создался,
         * и возможно нарушился инвариант.
         * Если мы требуем свойство, что у нас вариант всегда одна из альтернатив.
         * И если у нас остался старый индекс, то можем в деструкторе второй раз разрушить уже разрушенный объект.
         * Создать какой то дополнительный индекс.
         *
         * Вариант реализовывали несколько раз до того как он попал в стандарт.
         * Каждый вариант разрешал вопросы.
         *  Один из вопросов - стоит ли разрешать делать variant такой, который не содержит никаких значений.
         *
         *  Плюс того, что у нас будет разрешено создавать пустой вариант - предоставляем weak гарантию безопасности для
         *  оператора присваивания.
         *
         *  Но мы ведь изначально боролись с тем, что вариант не может ничего содержать. Плюс это надо будет теперь
         *  проверять, а не пустой ли у нас variant.
         *
         * Можно привести аналогия с указателем - надо проверять, нулевой он или нет.
         * Можем либо проверять это, или нет. Часто люди считают по дефолту, что нельзя передать пустой указатель в
         *      функцию
         *
         * Вопрос не решен, давайте попробуем сделать другую реализацию, но такую, чтобы variant не мог быть пустым,
         *      а так же чтобы у оператора присваивания была хотябы weak гарантия, или strong.
         */
        variant &operator=(variant &&);


        std::aligned_storage_t<std::max(sizeof(A), sizeof(B)), std::max(alignof(A), alignof(B))> storage;
        //по хорошему надо для выравнивания использовать НОК
    };
}

namespace ex10 {
    /*
     * Идея - заведем 2 storage
     * Не просто index храним, но и какой storage мы используем(если успешно помували, то меняем сторадж на новый
     *
     * Минус - в 2 раза больше места занимаем.
     *
     * double buffered variant
     */
    template<typename A, typename B>
    struct variant {
        variant &operator=(variant &&);

        std::aligned_storage_t<std::max(sizeof(A), sizeof(B)), std::max(alignof(A), alignof(B))> storage1, storage2;
    };
}

namespace ex11 {
    template<typename A, typename B>
    struct variant {

        struct base_concept {
        };

        template<typename T>
        struct model : base_concept {
            T value;
        };

        /*
         *  Минус - всегда храним что то в динамической памяти.
         */
        variant &operator=(variant &&);

        std::aligned_storage_t<std::max(sizeof(A), sizeof(B)), std::max(alignof(A), alignof(B))> storage1, storage2;

        std::unique_ptr<base_concept> base;
    };
}

namespace ex12 {

    /*
     * Мы можем заводить второй сторадж в операторе присваивания, заполнять его, при успехе копировать побайтово в
     *  начальный. Нельзя - потому что мы можем например куда то передвать this, потому надо конструировать обеъкт на
     *  месте. Например:
     */
    /*
    struct mytype{
        mytype(signal& sig)
        {
            sig.connect([this]{});
        }
    };
    */
    /*
     * Как сделано в boost variant
     */



    template<typename A, typename B>
    struct variant {
        /*
         * По сути идея, что выделяем динамическую память если произошло исключение.
         */
        variant &operator=(variant &&other) {
            void *p = move_current_to_dynamic();
            try {
                construct_new(std::move(other));
            }
            catch (...) {
                current = p;
                /*
                 * Вопрос, а почему нельза обратно мувать в статический.
                 * https://youtu.be/hrUtXP1sZXk?list=PLd7QXkfmSY7YsYNecuoJLsurtqsMNyhNF&t=3178
                 * Вот тут вроде бы объясняется
                 */
                throw;
            }
            delete p;
        }

        union {
            A static_a;
            B static_b;
            A *dynamic_a;
            B *dynamic_b;
        } current;

        void *move_current_to_dynamic() {
            switch (index) {
                case 0: {
                    A *p = new A(std::move(current.a_static));
                    current.static_a.~A();
                    return p;
                }
                case 1: {
                    B *p = new B(std::move(current.b_static));
                    current.static_b.~B();
                    return p;
                }
                case 2: {
                    return current.dynamic_a;
                }
                case 3: {
                    return current.dynamic_b;
                }

            }

        };


        int index;
    };
}

/*
 * Каждая из реализаций имеет свои плюсы и минусы
 *   double buffer - boost variant 2
 *   backup storage (последнее) - variant 1
 *   выделенное неинициализированное состояние - std variant
 * std::variant сделан хитро, они постарались пустое значение максимально скрыть
 *
 * Empty value in std::variant:
 *      valueless_by_exception - можно узнать, пустые ли мы
 *      get<>() - просто ходим, если внутри нас ничего нету, индекс не совпадет, throws std::bad_variant_access
 *      visit - бросается исключение такое же, нам не надо добавлять альтернативу, что может быть пустой variant.
 *      дефолтный коснструктор у нас есть, только если у перого типа есть дефолтный конструктор(т.е. создаем дефолтный
 *          объект первого типа
 *
 * Ожидается, что если при присваивании бросилось исключение, то мы должны сами привестить в валидное состояние variant
 *
 * Единственные способ перейти в такое состояние - если во время присваивания бросилось исключение
 *
 *
 * Что если я хочу сделать в variant пустую структуру(аналог ничего)
 *  -Есть std::monostate
 *  std::variant<std::monostate, A, B> - дефолтный конструктор создать пустой объект - т.е. monostate
 *
 *
 *
 *
 *
 */

int main() {
    ex8::main();
}