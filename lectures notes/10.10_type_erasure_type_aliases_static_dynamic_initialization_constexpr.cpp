#include <iostream>
#include <algorithm>
#include <functional>
#include <vector>
#include <utility>
#include <any>

/* Any может хранить в себе объекты, которые могут быть копируемы
 * Храним объекты любого размера, поэтому заранее память выделить нельзя. Только динамически.
 */

void ex1() {
    std::any a = 42;
    int v = std::any_cast<int &>(a);
    a = std::string("hello");
    std::string s = std::any_cast<std::string &>(a);
    /* Как такое реализовать? */
}

struct any {

    any() : obj(nullptr) {}

    ~any() {
        delete obj;
    }

    struct base_concept {
        virtual ~base_concept() {}

        virtual base_concept *clone() = 0;
    };


    template<typename T>
    struct model : base_concept {
        model(T value)
                : value(std::move(value)) {}

        /* Когда override'им виртуальнаую фукнцию, для return типа есть спец. правила:
         *
         * если в базовом классе функция возвращает ссылку или указатель на базовый класс, а в производном классе -
         * на произоводный, то пишем в функции в производном и базовом разные возвращаемые типы, но функция все равно
         * заоверрайдит.
         */
        base_concept */* or model<T>* */ clone() override {
            return new model<T>(value);
        }

        T value;
    };

    template<typename T>
    any(T val)
            :obj(new model<T>(std::move(val))) {}

    any(any const &other)
            : obj(other.obj ? other.obj->clone() : nullptr) {};

    template<typename T>
    friend T *any_cast(any *a);

private:
    base_concept *obj;
};

template<typename T>
T *any_cast(any *a) {
    if (!a)
        return nullptr;
    /*if (any::model<T> *m = dynamic_cast<any::model<T> *>(a->obj)) {
        return &m->value;
    }*/  /* OR  */
    if (typeid(*a->obj) == typeid(any::model<T>)) {
        return &static_cast<any::model<T> *>(a->obj)->value;
    }
    return nullptr;
}

void ex2() {
    any a = 32;
    int *v = any_cast<int>(&a);
    std::cout << *v << '\n';
}

/* Про RTTI
 * Допустим, хотим чтобы any компилился с ключом -fno-rtti (LANG=en_US g++ -std=c++17 -g -fno-rtti 14.cpp)
 * Почему хотим так компилить?
 * Для каждого класса с вирт. функций компилятор генерит инфу о runtime информации, которая помогает в
 *  typeid,
 *  dynamic_cast - достает typeid, смотрит, какие есть базы, и ходя по базам делает касты
 * Фичи используются редко, но классы с виртуальными функциями используются повсеместно. Платим везде, а пользы мало.
 * У людей была база, а потом появился RTTI, и размер программ стал больше, хотя им не пользуются.
 * RTTI - runtime type information
 * Информация о типах, которая доступна в процессе работы программы. К ней можно обращаться по typeid(..)
 *
 * Как сделать any_cast без RTTI?
 */



namespace ex3 {
    template<typename T>
    void noop() {}

    struct any {

        any() : obj(nullptr) {}

        ~any() {
            delete obj;
        }

        struct base_concept {
            typedef void(*noop_fn_t)();

            virtual ~base_concept() {}

            virtual base_concept *clone() = 0;

            virtual noop_fn_t get_type() = 0;
        };


        template<typename T>
        struct model : base_concept {
            model(T value)
                    : value(std::move(value)) {}

            base_concept *clone() override {
                return new model<T>(value);
            }

            noop_fn_t get_type() override {
                return &noop<T>;
            }

            T value;
        };

        template<typename T>
        any(T val)
                :obj(new model<T>(std::move(val))) {}

        any(any const &other)
                : obj(other.obj ? other.obj->clone() : nullptr) {};

        template<typename T>
        friend T *any_cast(any *a);

    private:
        base_concept *obj;
    };

    template<typename T>
    T *any_cast(any *a) {

        if (a->obj->get_type() == &noop<T>) {
            return &static_cast<any::model<T> *>(a->obj)->value;
        }
        return nullptr;
    }


    void main() {
        ex3::any t = 342;
        std::cout << *ex3::any_cast<int>(&t) << "\n";
    }

}
/*
   * В данном случае у нас функция - идентификатор, в RTTI структуры с какой то доп.инфой
   *
   * Можем завести глобальную переменную для типа.
   */
namespace ex4 {
    template<typename T>
    char tag;

    struct any {

        any() : obj(nullptr) {}

        ~any() {
            delete obj;
        }

        struct base_concept {

            using tag_t = char *;
            virtual ~base_concept() {}

            virtual base_concept *clone() = 0;

            virtual tag_t get_type() = 0;
        };


        template<typename T>
        struct model : base_concept {
            model(T value)
                    : value(std::move(value)) {}

            base_concept *clone() override {
                return new model<T>(value);
            }

            char *get_type() override {
                return &tag<T>;
            }

            T value;
        };

        template<typename T>
        any(T val)
                :obj(new model<T>(std::move(val))) {}

        any(any const &other)
                : obj(other.obj ? other.obj->clone() : nullptr) {};

        template<typename T>
        friend T *any_cast(any *a);

    private:
        base_concept *obj;
    };

    template<typename T>
    T *any_cast(any *a) {
        std::cout << "hi\n";

        if (a->obj->get_type() == &tag<T>) {
            return &static_cast<any::model<T> *>(a->obj)->value;
        }
        return nullptr;
    }


    void main() {
        any t = 342;
        std::cout << *any_cast<int>(&t) << "\n";
        std::cout << any_cast<std::string>(&t) << "\n";
    }
}


/*
 * Почему у итераторов листа и вектора нет общего базового класса? Двигаем вперед назад, разыменовываем, сравниваем. Но
 * с другой стороны они работают по разному.
 * std::vector<int>::iterator it1;
 * std::list<int>::iterator it1;
 * Интерфейс итераторов нельзя выразить через виртуальные функции.
 *      В стандартной библиотеке написан оператор стравнения для итераторов листа(допустим).
 *      Такую штуку нельзя выразить обычной виртуальной функцией. Вопросы: если делаем такую функцию в интерфейсе, что
 *      она будет принимать, как отрезать лишние типы. Непонятно.
 *
 * Замедленее существенное, т.к. пришлось бы использовать ссылки или указатели
 *      Обычно работаем с итераторами по значению. Но если у нас базовые классы, по значению работать нельзя. Надо
 *      использовать ссылки, а это замедление существенное. Дополнительный уровень индерекшена, потому что ссылаемся на
 *      итератор, который ссылается на ноду. Еще важно как создаем указатель, вдруг мы создаем их динамически в хипе?
 *      И если создаем в хипе, то еще замедленние изза времени на аллоцирование памяти, удаление.
 *
 *      Почему надо было бы использовать ссылки?(см ex5)
 *
 * Что делать тогда с int*(указатель на массив). Но они ни от чего не наследуются. Не могут отнаследоваться от общего
 *      интерфейса, т.к. они встрены в язык.
 *
 * Если надо сделать функцию, которая бы работала с происзвольными итераторами, делаем ее шаблонной по типу итератора.
 * (ex6)
 */
namespace ex5 {
    struct base {
        virtual void foo() {
            std::cout << "hello base\n";
        }
    };

    struct derived : base {
        void foo() override {
            std::cout << "hello derived\n";
        }
    };

    void main() {
        derived d;
        base b = d;
        b.foo();//base call
        base &b_ref = d;
        b_ref.foo();//derived call}
    }
    /*Идея, на которой основан полиморфизм:
     * Статический тип какой то впрограмме, а динамический тип разный. И чтобы они отличались, мы вынуждены использовать
     * ссылки и указатели. И если мы используем просто объекты, то у них статический и динамеский тип совпадает всегда.
     */

}

namespace ex6 {
    template<typename InputIterator, typename OutputIterator>
    void copy(InputIterator first, InputIterator last, OutputIterator out) {
        while (first != last) {
            *out++ = *first++;
        }
    }
}

/*
 * но что то такое есть => any_biderectional_iterator<int> i;
 *
 */
namespace ex7 {
    void pseudocode() {
        /*std::vector<int> v;
        std::list<int> l;
        int* p;
        any_bidirectional_itertor,int> i = v.begin();
        ++i;
        i = l.begin();
        i = p;
        работает так же, как any. И когда, допустим надо разыменовать, уходить через виртуальную функцию.
        Использование такого рода типов позволяют нам использовать или не использовать полиморфизм без замедления.

         */
    }
}

/*
 * type erasure -
 *      1) Широкое определение
 *          Берем разные типы с одним интерфейсом и приводим их к одному типу с этим интерфейсом
 *              Например хотим обернуть все что угодно, что имеет push_back функции. Любой набор функций по сути.
 *      2) Прием программирования как с any (concept and model)
 * Бонусы type_erasure(про прием)
 *
 * 1) сам класс без вирт.функций, он сам по себе быстрый
 *
 * 2) сохранение value семантики
 *      Например обернули std::funсtion, и дальше он таскает объект, данные по программе, которые нужны. Если бы мы
 *      пытались изображать это при помощи наследования(базовый класс с виртуальными ф-циями, реализуемыми в
 *      производных классах, то возникали бы вопросы, а когда и как удалять, кто ответсвенен за удаление?
 *
 *      Value semantics - наш тип ведет себя, как будто это обычное значение.
 *      (т.е. имеет значение только значение объекта, например int, std::string(ведет себя как int), в отличие от char *
 *      где нам надо помнить об удалении и т.д.) (only its value counts, not its identity == указатель this ). Т.е. в
 *      int например нам не важен адрес числа. И в string тоже.
 *.
 * 3) использование наследования требует, чтобы в момент когда мы пишем класс, мы знаем все интерфейсы которые надо
 *      реализовать. Бывают случае, когда мы заранее не знаем о том, что можем реализовать какой то интерфейс.
 *          std::fstream a;
 *          std::stringstream b;
 *      Заранее подумали, что хотят сделать общий интерфейс.
 *      Но сам по себе класс обертка над файлом самодостаточный, и класс форматирования строк тоже.
 *      Взгляд в будущее не всегда возможен.
 *      Часто бывают, что базовые классы должны угадывать, как хотят использовать наш класс.
 *      А с type erasure мы можем по ходу дела выделять функционал.
 *
 *      Однако не всегда мы выбраем правильные имена для функций. Есть механизмы, которые позволяют приспособить наш
 *      класс к нужному интерфейсу. Кастомно реализовать нужную функцию.
 */

/* type alias позволяет делать шаблонные typedef'ы
* до этого приходилось делать шаблонные структуры, а уже в них typedef.
*/
template<typename ReturnType>
using func_t = ReturnType (*)();

using void_ptr = void *;

/*using declaration*/
struct base {
    void foo();

    void bar();

};

struct derived : private base {
    using base::foo;
};

/*Using - одно ключевое слово, которое используется в 3 разны смыслах.
 *      using declaration
 *          берем другую декларацию и ее в этом скоупе тоже объявляем.
 *          используется в private наследовании. (см ex)
 *      using directive
 *              using namespace std;
 *          позводляет вытащить все что внутри namespace;
 *          and in C++20 can be
 *              using enum
 *          чтобы вытащить из него все енумераторы.
 *      type alias
 *          еще есть похожая штука для namespace'ов.
 *              namespace fs = std::filesystem;
 *
 *
 */


namespace ex8 {
    struct base {
        void foo();

        void bar();
    };

    struct derived : private base {
        using base::foo; // теперь снаружи видна эта функция из приватного наследования.
    };
}
using namespace std;

using value_t = int; // type alias
//namespace fs = std::filesystem; //namespace alias

int main() {
    ex5::main();
}
