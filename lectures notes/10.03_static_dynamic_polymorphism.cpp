#include <iostream>
#include <algorithm>
#include <functional>
#include <vector>
/* Анонимные функции */
/*Пример, почему анонимные функции сделаны именно так*/

template<typename T>
struct less {
    less() {}

    bool operator()(T const &a, T const &b) const {
        return a < b;
    }
};

bool int_less(int a, int b) {
    return a < b;
}

void ex1() {
    std::vector<int> v(10, 0);
    std::sort(v.begin(), v.end(), std::less<int>()); // possible less impl. наверху
    // статический полиморфизм работает быстрее, т.к. инлайнится
    std::sort(v.begin(), v.end(), &int_less); // работает медленнее, т.к. динамический полиморфизм
}


/* 2 способа задать функцию*/

typedef bool (*comparator_t)(int, int);

void sort(int &first, int &second, comparator_t comp) {
    std::cout << "hell1\n";
}

template<typename Comparator>
void sort(int &first, int &second, Comparator comp) {
    std::cout << "hell2\n";
}

void ex2() {
    /* Для случая когда много компараторов
     *
     * При указателе на функцию компаратора функция sort будет одна
     *
     *
     * При типовой перегрузке с шаблоном для каждого компаратора будет создаваться своя версия
     * Компилятор будет генерить функции
     * Но внутри функции sort мы вызывает функцию comp из структуры, компилятор может инлайнить.
     *
     * В случае же указателя, не понятно как инлайнить, любая функция может быть вызвана
     * */

    int buf[] = {1, 4, 3, 2};
    std::cout << *buf << *(buf + 3);
    sort(*buf, *(buf + 1), &int_less);
}

/*
 * правда ли, что все методы инлайн? Если мы определяем метод внутри класса, а не снаружи, то да.
 * Не все что инлайн, инлайнится компилятором
 *
 * Почему инлайн лучше, чем выхов фнукиции по указателю? Ну в асм коде у нас не надо будет закидывтаь аргументы на стек,
 * прыгать на метку нашей функции, возвращать объект, а можно будет сразу же в коде сделать сравнение. Меньше асм. кода
 *
 * Но мы выиграли в оптимизации инлайнинга, а если его нет, то мы проиграем по размеру сгенерированного кода
 *  (меньше в кэше)
 *
 * Хотим сделать так, чтобы компилятор сам понимал как инлайнить и когда.
 * Один из способов мог бы быть - заинлайнить вызов sort в мэйне, тогда будет понятно какую функцию компаратора вызываем
 * по указателю, и ее тоже можно было бы заинлайнить
 */


/*
 * flto ключ оптимизациии для компилятора делает следующее. Когда мы компилим код в объектный файл, не можем инлайнить
 * функции, которые находятся в другой единице трансляции. Ключ говорить скомпилить в некоторый промежуточный код все
 * объектные файлы, а потом линковщих их слинкует в один и соптимизирует как надо. У оптимизатора убдет полный вид
 * программы как она выглядит
 *
 * -О3 работает быстро на бенчмарке оба способа так как там так же включается ключ -fipa-cp-clone, который оптимизирует
 * функции под коснтанттный аргумент, который ей передается https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
 * Но такая оптимизация не всегда хороша, т.к. код раздувается(делаем копии функций под каждый тип), меньше умещается в
 * кэш, рабоатет все медленнее
 *
 * Девиртуализация - у нас есть указатель на функцию, или виртуальная фукнция, и вместо того, чтобы вызывать ее через
 * указаетль или по таблице, вызываем ее непосредственно.
 * Подробнее: http://hubicka.blogspot.com/2014/01/devirtualization-in-c-part-1.html
 *
 * В теории компилятор может даже оптимизировать как то так
 *
 */

void sort1(int *p1, int *p2, bool (*comp)(int, int)) {
    /*...*/
    if (comp == int_less) {
        /* inline int_less comparator...*/
    } else {
        /*call real comparator...*/
    }
    /*Если бы мы делалаи if в теле цикла, то смогли бы вынести проверку из цикла, тогда она была бы одна для всего цикла
     * и  ъорошая оптимизация
     *
     * -fdevirtualize-speculatively
        Attempt to convert calls to virtual functions to speculative direct calls. Based on the analysis of the type inheritance
     graph, determine for a given call the set of likely targets. If the set is small, preferably of size 1, change the
     call into a conditional deciding between direct and indirect calls. The speculative calls enable more optimizations,
     such as inlining. When they seem useless after further optimization, they are converted back into original form.

     Особенно хорошо оптимизацию применять с pgo (profile-guided optimization) - когда компилируем программу со всякими
        счетчиками, прогоняем на репрезентативных данных и потом уже компилируем программу, оптимизируя по сделанному
        анализу. См -fprofile-generate -fprofile-use
     */
}

/*
 * Статический полиморфизм лучше, т.к. позволяет отлавливать больше ошибок на этапе компиляции.
 * Но если действие зависит от того, что вводит ползьватель, динамический полиморфизм необходим.
 */

/*
 * Задача. Хотим компарить вектор по хитрому.
 * до С++11
 */

struct mod_10_comparator {
    bool operator()(int a, int b) const {
        return (a % 10) < (b % 10);
    }
};

struct mod_comparator {
    mod_comparator(int mod) : mod(mod) {}

    bool operator()(int a, int b) const {
        return (a % mod) < (b % mod);
    }

    void bar() {
        [this]() {
            this->mod = 32;
        };
    }

private :
    int mod;
};

void ex3() {
    std::vector<int> v(10, 0);
    std::sort(v.begin(), v.end(), mod_10_comparator());
    /*
     * Нужно каждый раз заводить новый класс. Надо часто прыгать по коду. Т.к. прыгаем на структуры, чтобы смотреть на
     * компаратор.
     *
     * Теперь же в С++11
     */
    std::sort(v.begin(), v.end(), [](int a, int b) /* -> bool */ {
        return a % 10 < b % 10;
    }); // Оператор генерит анонимную структуру с оператором () с нужными аргументами и типом повзвращаемого значения как auto
    // В терминах типов языка для одинаковых лямбд генерится 2 разные анон. структуры, но компилятор сможет их соптимизировать

    std::sort(v.begin(), v.end(), mod_comparator(10));

    /*
     * Общий синтаксис
     * [closure(захват значений)] (аргументы) {тело фукнции}
     * Захват двух типов - как ссылка или как копирование
     */

    int x, y, z, t;
    []() {};
    [x, &y, z]() {};

    [=, &y]() {
        x;
        z;
        t;
    };//захват по значению всех, и по ссылке у
    [&, x, z]() {
        t = 23;
    };
    /*Мы не можем захватывать глобальные переменные, они и так доступны*/
    /*
     * Смотреть, во что разворачиваются лямбды можно тут
     * https://cppinsights.io/
     */
    /*Мы можем захватывать this, захватывается как обычный указатель, даже если пишем [&]*/
}

/*
 * Может ли анонимная функция быть шаблонной?
 * У чего параметр шаблона мы хотим?
 *
 */
template<typename T>
struct less_comparator {
    bool operator()(T const &t, T const &tt) const {
        return t < tt;
    }
};

struct polymorhic_less {
    template<typename T>
    bool operator()(T const &t, T const &tt) const {
        return t < tt;
    }
};

/* Оба тип отличаются тем, в какой момент мы захотим фиксировать тип сравниваемого значения
 */

void ex4() {
    std::vector<int> v(10, 0);
    std::sort(v.begin(), v.end(), less_comparator<int>());
    std::sort(v.begin(), v.end(), polymorhic_less());
}

//не сработало бы для 1го компаратора
//сработало бы для 2го компаратора
template<typename Comparer>
void sort(Comparer comp) {
    comp(1, 2);
    comp(1., 2.);
}

//делать лямбду шаблонной бессмыленно, т.к. мы делаем ее один разд для конкретного тпиа и вызваем один раз.
// шаблонные фукнции делаем если хотим вызвать несколько разынх типов.

//Начиная С++20 существует лямбды с оператором () шаблонным
void ex5() {
    auto id = [] < typename T > (T
    a ) { return a; };
    int val = id(43);
}

void foo(auto a) // неявно создался доп.шаблонный параметр, удобная сокращенная форма записи того что ниже
{}

template<typename T>
void foo1(T a) {}

void foo2(auto a, auto b) {

}

/*эквиванлентно*/
template<typename T, typename U>
void foo(T a, U b) {

}

/*если нужны параметры одного типа, то запись через auto не позволяет это сделать
 *  делаем как и раньше template <...>
 *  auto можно применять и в анон. функциях
 *  */

void ex6() {
    auto id = [](auto a) { return a; };
    int val = id(43);

    /*auto id создает структуру какую то, можно поговорить про ее свойства.
     * есть оператор ()
     * тип умеет копироваться и муваться, но нет обычного присваивания
     *
     * */
//     id = id; // error
//        decltype(id) a; // до С++20 дефолтный конструтор запрещен. С С++20 разрешили только если лямбда ничего не
//        захватывает
/*
 * обычно используем саму лямбду и по проге таскаем
 *
 * а можно сделать static bool operator() и передавать в функцию less::operator() и имеет ли смысл?
 * В этом случае мы лишаемся возможности передачи параметра в фукнция operator()(...) как это было с mod_comparator.
 * А если сделать такой класс со статическим методом шаблонным для такого рода переменных? Они должны быть
 * константами времени компиляции.
 * А если сделать статик поле? Это глобальная переменная. Можно никогда не заводить объектов и все хранить в глобальных
 * перемнных, но в этом есть свои минусы.
 *
 * Про пустые структурки, которые мы передаем в параметры, как например
template<typename Comparer>
void sort(Comparer comp) {
}
 *  Оказывается, при вызове никакого копирования одного байта структуры не происходит, у нее нет полей. По сути нам нужен
 *      только паддинг (отступ на вызов оператора из структуры), и его не надо копировать.
 *
 *  Если функция без захвата, то мы можем взять от нее указатель на функцию, в каком то смысле взяв адрес оператора()
 *
 * Для чего нужны лямбда функции с шаблонами?
 */
}

/*
* Для чего то, чему хотим передавать разные типы(например аутпут)
*/

template<typename PrintFunc>
void myfunc(PrintFunc print_func) {
    print_func("hello");
    print_func(42);
}

/*
 * а конверсия в указатель на функцию всегда происходит для пустых лямбд?
 * Нет, только там где надо. Если справа указатель на функция, а слева лябмда написана(делаем присваивание)
 */

struct my_lambda {
    typedef void (*func_ptr_t)();

    my_lambda &operator=(my_lambda const &) = delete;

    void operator()() {}

    operator func_ptr_t() const {
        std::cout << "hello";
        return nullptr;
    }
};

void ex7() {
    my_lambda b, a;
//    a = b; // CE delted operator=
    void (*c)();
    c = b; // ok. call operator конверсии
    +b; // тут тоже приводятся к указателю
}


/*std::function*/

void foo(bool flag) {
    std::function<void(int, int)> func;
    if (flag) {
        func = [](int, int) {};
    } else {
        func = [](int, int) {};
    }
}

struct timer {
    timer(std::function<void()> on_tick)
            : on_tick(on_tick) {}

    std::function<void()> on_tick;
};

/*
 * function -  полиморфный враппер над фукнциональными объектами
 * Что за шаблоны с ()?
 */

template<typename T>
struct mytype {
};
mytype<void (*)()> a; // указатель на функцию
mytype<void()> b; //  функция (передали тип функции)
/*что с const? только для мемберов. К самим функциям т.к. они не копируемы и не присваемы, конст к ним
 * квалификатор не применяется*/
struct mystruct {
    void myfunc(int a)  {}
};

mytype<int (mystruct::*)() const> c; // указатель на функцию в классе

/*
 * В function можно передать
 *      лямбду
 *      указатель на функцию
 *      объект с оператором() (func = struct_with_less();)
 */

/*
 * как создать свой std::function?
 * пусть делаем только для функций типа void (int,int)
 */

template<typename T>
void give_deleter(void *obj) {
    delete static_cast<T *>(obj);
}

//template<typename T>
//void give_invoker(void *obj, int a, int b) {
//    (*static_cast<T *>(obj))(a, b);
//}

template<typename T, typename Ret, typename  ... Args>
Ret give_invoker(void *obj, Args ... args) {
    return (*static_cast<T *>(obj))(args ...);
}


/*
struct function {
    typedef void (*deleter_t)(void *);

    typedef void (*invoker_t)(void *, int, int);

    function() {};

    function(function const & other) = delete; // тоже можно запросто сделать
    function & operator=(function const & other) = delete;// но можно запросто сделать

    template<typename T>
    function(T obj):
        obj(new T(obj)),
        deleter(&give_deleter<T>),
        invoker(&give_invoker<T>){}

    ~function() {
        deleter(obj);
    }

    void operator()(int a, int b) {
        return invoker(obj,a,b);
    }

private:
    void *obj; // хранит захваченные переменные
    deleter_t deleter;
    invoker_t invoker;
};
*/
template<typename T>
struct function;

template<typename Ret, typename ... Args>
struct function<Ret (Args...)> {
    typedef void (*deleter_t)(void *);

    typedef Ret (*invoker_t)(void *, Args...);

    function() {};

//    function(function const & other) = delete; // тоже можно запросто сделать
//    function & operator=(function const & other) = delete;// но можно запросто сделать

    template<typename T>
    function(T obj):
            obj(new T(obj)),
            deleter(&give_deleter<T>),
            invoker(&give_invoker<T, Ret, Args ...>) {}

    ~function() {
        deleter(obj);
    }

    Ret operator()(Args ... args) {
        return invoker(obj, args ...);
    }

private:
    void *obj; // хранит захваченные переменные
    deleter_t deleter;
    invoker_t invoker;
};


void bar(bool flag) {
    function<void (int,int)> func;
    if (flag)
        func = ([](int, int) {});
    else
        func = (less<int>());
    func(32, 32);
}
/*мемберы и std::function
 * std::function принимает функции, которые вызваются по (). Но с мемберами все не так.
 * */

void ex8(){

    mystruct *p;
    void (mystruct:: * f)(int) = & mystruct::myfunc;
    /*cannot do smth like
     * f(p,32)
     * */
    (p->*f)(42);//->* special operator
    //C++11 появился mem_fn
    auto g = std::mem_fn(f);
    g(p,32);//now we can do this

    std::function<void (mystruct*, int)> f1 = std::mem_fn(&mystruct::myfunc);
    /* or */
    std::function<void (mystruct*, int)> f2 = [](mystruct * p, int a){
        p ->myfunc(a);
    };
    /*что лучше, лямбда или mem_fn лучше? лямбда читабельнее*/


    /*немного о шаблонах. Почему определение и объявление шаблонной фукнции должны быть в одном файле?
     * В машинный код все файлы превращаются отдельно. Файл с определением шаблонного класса будет пустым.
     * При линковке произойдет ошибка.
     * Если хотим зафорсить в файле с определением явно инстанцировать и пофортить функцию таких типов.
     *
     * template <typename T>
     * void f(T t)
     * {}
     *
     *
     * template void f<int>(int);
     *
     */

}

int main() {
    ex7();
}
