#include <memory>
#include <optional>
#include <iostream>
/*
 * Optional
 */
namespace ex1 {
    struct deferred_value {
        /*умеет вычислять некоторое значение, которое может долго считаться
         * Хотим возвращать и вычислять если нет в кеше*/

        int get_value() const {
            if (!is_initialized) {
                cached_value = compute_value();
                is_initialized = true;

            }
            return cached_value;
        }

        int compute_value() const;

    private:
        mutable bool is_initialized = false;
        mutable int cached_value;
    };
}
/*
 * Заметим, что is_initialized and cached_value сильно связаны, но в типах это не отражено.
 * Т.е. если человек человек прочитает, не проверив флаг, то прочитает просто какое то значение, но в хипе это не
 * проверяется и ворнинга не напишет компилятор.
 *
 * В нашем случае пример простой очень, но могут быть сложнее структуры, гду связь не очевидна. Компилятор ничего не
 * не знает про свойства этих переменных и не может нам помочь.
 *
 * Инициализация дефолтным значением не решает проблемы с точки зрения логики программы
 *
 * Что же делать?
 * Можно бросать исключение
 * Можно в случае отладочной сборки вызывать ассерт
 * Можно сделать синтаксис, который бы показывал человеку что данные значения не инициализированы.
 *
 * Представим себе что тип внутри не default constructable, поэтому не сможем проинициализировать
 * А что если хранить указатель на объект?
 */

namespace ex2 {
    template<typename T>
    struct deffered_value {
        T get_value() const {
            if (!cached_value)
                cached_value = std::make_unique<T>(compute_value());
        }

        T compute_value() const;

    private:
        mutable std::unique_ptr<T> cached_value;
    };
    /*
     * Этот пример лучше, т.к. теперь мы храним 1 переменную, которая может быть нуллом если объекта нет.
     */
}


/*
 * Теперь сделаем со стореджем
 */
namespace ex3 {
    template<typename T>
    struct optional {

        optional() = default;

        optional(T value)
                : is_initialized(true) {
            new(&storage) T(std::move(value));
        }

        ~optional() {
            if (is_initialized) {
                reinterpret_cast<T &>(storage).~T();
            }
        }

    private:
        bool is_initialized = false;
        std::aligned_storage<sizeof(T), alignof(T)> storage;
    };
}
/*
 * Optional устроен таким образом, что когда изменяется is_initialized, то что то происходит со стореджем.
 *
 * Такой класс есть в std
 *        Optional:
 *        As opposed to other approaches, such as std::pair<T,bool>, optional handles
 *        expensive-to-construct objects well and is more readable, as the intent is expressed explicitly.
 *        Есть коснструктор для пустого опт, для копирования и для создания объекта внутри опт.
 *        template< class... Args >
            constexpr explicit optional( std::in_place_t, Args&&... args );
 *        Отличие такого конструктора от emplace - последний, меняет существующий, конструктор создает новый.
 *        std::in_place_t нужен для того, чтобы различать, переданные нам аргументы нужны в конструкторе самого объекта,
 *        или они для нашего конструктора
 *
 *        template < class U >
            optional( const optional<U>& other );
          Что то вроде конверсии(есть опт от int, хотим от long, например
 */


namespace ex4 {
    struct foo {
        int a, b, c;

        foo(int a, int b, int c) : a(a), b(b), c(c) {}
    };

    int main() {
        std::optional<foo> opt(std::in_place, 2, 3, 4);

        // обращение
        std::cout << opt->a << " " << opt->b << "\n";
        //проверка на пустоту
        if (opt)
            std::cout << "not empty\n";
        opt = std::optional<foo>();
        if (!opt)
            std::cout << "empty\n";
        return 0;
    }

    template<typename T>
    struct optional {

        optional() = default;

        optional(T value) :
                cached_value(value) {
        }

        T get_value() const {
            if (!cached_value)
                cached_value = compute_value();
            return *cached_value;
        }

        T compute_value() const {
        }

    private:
        mutable std::optional<T> cached_value;
    };

/*
 * Если в optional лежит T*, мы сможем обращаться к методам T через ->->?
 * Не совсем, скорее надо :
 * А что произойдёт при обращении к пустому Optional?
 */
    int main1() {
        std::optional<foo *> opt;
        (*opt)->a;
    }
    /*
     * А что произойдёт при обращении к пустому Optional?
     * Есть value которое бросает исключение, а есть * разыменование которое, которое не бросает исключение.
     * Лучше не бросать исключение, т.к. в режиме дебага или при нужных ключах компиляции программа будет абортится,
     * т.к. в стандартной реализации прописан ассерт, если мы пытаемся обращаться к ненашей помяти(разыменовываем
     * невалидный указатель)
     * Так же вылет ошибки не говорит о том, где произошла ошибка, а в случае аборта у нас сохраняется core dump
     * (состояние программы), плюс мы видим конкретное место.
     *
     * В std там где можно писать ассерт, его пишут.
     *
     * Исключение зачастую ломают программу.
    */

}

/* Обсудим trivial destructor */
namespace ex5 {
    /*Проблема в том, что в реализации optional из ex3 деструктор не тривиален. Хотя наш объект может иметь тривиальный
     * деструктор.
     *
     * Что такое class trivial destructable?
     * У int и подобных просто нет деструктора, поэтому если наша структура состоит из подобных полей, то ей не нужен
     * деструктор(т.у. он не просто пустой, а его тупа нет) - такие структуры - trivial destructable
     * tuple, pair of trivial classes - also trivial.
     */
    struct bool_int_pair {
        int a;
        bool b;
        /*no destructor at all*/
    };
    /*
     * Но если мы определим деструктор как пустой, то это уже не trivial destructable, т.к. у нас уже есть функция и она
     * будет вызвана, хоть она и пустая.
     *
     * implicitly destructable объекты, это те, у которых поля - не тривиально деструктабл, хотя явного деструктора нет.
     * Т.е. для таких классаов деструктор генерится компилятором.
     *
     *
     * Аналогично тривиальному деструктору бывают тревиальность дефолтного создавания, тревиальность дефолтного
     * копирования (копируем тупо по байтам)
     */

    struct my_type { // trivial constructable
        int a, b;
    };

    struct my_type_2 {
        int a, b;

        my_type_2(const my_type_2 &m) : a(m.a), b(m.b) {};
        /*
        * это уже не тривиал конструктабл, хотя вроде бы делает то же самое.
        */
    };

    /*about default constructor*/

    struct my_type_3 {
        int a, b;

        my_type_3(int a, int b) : a(a), b(b) {}; // подавили дефолтный конструктор
        my_type_3() = default; // этот конструктор будет тривиальным
    };

    /*
     * Почему тривиальность некоторых операций может быть полезна:
     *  Тривильано копируемые, разрушаемые - ведут себя подругому при передаче в функцию.
     *
     * Как же сделать такой деструктор у optional?
     *      SFINAE - substitution failure is not an error. Не можем использовать, т.к. у деструктора нет ни аргументов,
     *          ни возвращаемого типа, чтобы написать enable_if
     *      if constexpr - это надо будет писать внутри, а для этого должно быть тело и сам деструктор.
     *
     *      сделать всю структуру enable_if? продублировать структуру в общем. Да, решит проблему, но это надо учитывать
     *           каждый тривиал деструктор, тривиал коснтруктабл и т.д.
     *
     *      поиграться с наследованием. Хороший вариант про то, как сделать тривиал деструктабл или нет.
     */


    template<typename T>
    struct optional_storage {
        bool is_initialized = false;
        std::aligned_storage<sizeof(T), alignof(T)> storage;
    };

    template<typename T, bool isTriviallyDestructible>
    struct optional_base : optional_storage<T> {
        ~optional_base() {
            if (this->is_initialized) {
                reinterpret_cast<T &>(this->storage).~T();
            }
        }
    };


    //struct specialization
    template<typename T>
    struct optional_base<T, true> {
        // no destructor
    };

    template<typename T>
    struct optional : optional_base<T, std::is_trivially_destructible_v<T>> {

        optional() = default;

        optional(T value) {
            new(&this->storage) T(std::move(value));
        }

    };
    /*Но минус - для каждой тривиальной фукнции делаем базу. Поэтому в исходниках optional много кода*/
}

/*
 *
 * В optional можно хранить все, что мувается и разрушаться(указатели, инты умеют это делать)
 *
 * Подытоживая:
 *      если есть пара объект и бул - задумываемся, а не нужен ли optional.
 *          т.е. допустим у нас поле есть какое то не оптионал, по нему нельзя понять, что у него есть какое то значение
 *
 *          Когда все таки стоит пользоваться специальными значениями - когда важна память и производительность.
 *              Структура из int + bool = 8 bytes because of padding.
 *          Вектор optional не очень эффективен
 *
 * Казалось бы, если у нас optional от bool, то почему бы не закодировать значение как то = 0,1,2 - но так не стали
 *      делать, потому что optional возращаеть & на свой аргумент, и в таком случае надо было бы создавать какой то
 *      проксевый объет, что бы на него возвращать указатель(так делали с vector<bool>)
 */

int main() {
    ex4::main();
}
