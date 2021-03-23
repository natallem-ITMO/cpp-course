#include <utility>
#include <iostream>
#include <iomanip>
#include<type_traits>
#include <memory>
/*
 * Variadic templates
 */

//Набор типов, разные типов (... U)
template<typename ... U>
struct tuple {
};
template<typename ...V>
struct tuple2 : V ... {
    using V::foo...;
};

/*example from cppref  using U::foo...*/
template<typename... Ts>
struct Overloader : Ts ... {
    using Ts::operator()...; // exposes operator() from every base
    template<typename T>
    void show(T t) {
        std::cout << "\n" << typeid(T).name() << " ";
        operator()(t);
    }
};

template<typename... T>
Overloader(T...) -> Overloader<T...>; // C++17 deduction guide, not needed in C++20

void mainExample() {
    auto o = Overloader{[](auto const &a) { std::cout << a; },
                        [](float f) { std::cout << std::setprecision(3) << f; }};
    o.show(4.234534);//d     4.23453
    o.show(4.234534f);//f    4.23
    o.show('d');//c d
}

template<typename ... U>
struct tuple1 : /*private*/ U ... {
};

void g(int, float, char);

struct agg {
    int a;
    float b;
    char c;
};

template<typename ... V, typename ...U>
void f(V ... args) {
    //V calls pack of types

    //можно ли использовать где то args without ...?
    sizeof...(args);//считает, сколько элементов в args(число элементов)

    //where can write ...?
    g(args...);

    agg a = {args ...};

    tuple<V ...> t; // tuple<inf, float, char> t;
    /*
     * 3 точки можно писать в тех местах, где мы пишем какие то списки
     */

    tuple<tuple2<V...>> t1; // tuple<typle2<int,float,char>>;
    tuple<tuple2<V>...> t2; // tuple<tuple2<int>, tuple2<float>, tuple2<char>>;

    tuple<tuple2<V, V> ...> t3; //tuple<tuple2<int,int>, tuple2<float,float>, tuple2<char,char>>
    // tuple<tuple2<U, V> ...> t4; // U, V - должны быть одного размера. Как такое предать? См ниже struct x

    tuple<tuple2<V, V...>...> t5; // tuple<tuple2<V, int, float, char> ...> и теперь расскрываем внешние
    // tuple<tuple2<int, int, float, char>,
    //       tuple2<float, int, float, char>,
    //       tuple2<char, int, float, char>>

}

template<typename ...U>
void g(U &&... args) {
    //not f(std::forward<U>(args...)) //
    f(std::forward<U>(args)...); // f(std::forward<int>(arg1), std::forward<float>(arg2), std::forward<char>(arg3))
}

//template <typename ... U, typename ...V> // CE - pack should be the last in class templates
template<typename  ...T>
struct x {
};

template<typename ... UArgs, typename... VArgs>
struct x<tuple<UArgs...>, tuple<VArgs...>> {
};

template<typename ... UArgs, typename... VArgs>
void x_func(tuple<UArgs...>, tuple<VArgs...>) {
    tuple<tuple2<UArgs, VArgs>...> f;
}

template<typename T, typename ... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

/*если бы хотели обращаться к i типу аргумента*/

template<size_t index, typename ... Ts>
struct at;

template<typename T0, typename ... Ts>
struct at<0, T0, Ts...> {
    typedef T0 type;
};

template<size_t index, typename T0, typename  ... Ts>
struct at<index, T0, Ts ...> {
    typedef typename at<index - 1, Ts ...>::type type;
};

void main4();

int main() {
//    tuple<int, float, char> xx;
//    x_func(tuple<int, float>(), tuple<char, double>());//tuple sizoof... same for 1 and 2 tuple
//    f<int, float, char>(1, 1.f, '1');
//    std::cout << typeid(at<2, int, double, float, char>::type).name() << '\n';
    main4();
}


int foo();

int &bar();

int &&baz();

//decletype(foo())  -> int
//decletype(bar())  -> int&
//decletype(baz())  -> int&&

// decltype(expr)     type       (prvalue) одно из немногих мест в языке, где важно pr/x value
// decltype(expr)     type&      (lvalue)
// decltype(expr)     type&&     (xvalue) - результат вызова фукнции, которые возвращают rvalue (&&int)

//decltype сохраняет ссылки, а auto отбрасывает

//decltype(var_name) - вернет тип этой переменной
//decltype(expr) - считается тип выражения

//а разве имя переменной это не выражение состоящее только из имени?
//нет.

struct x1 {
    int a;
};

int main2() {
    int a;
    a; // простое обращение к переменной. Допустимо
    //    x1::a; // Недопустимое обращение. Невалидное выржание
    //decltype(x1::a); // Допустимо в decltype.
    //В основном деклтайп используется для определения типа выражений, а не переменных

    decltype(a) b = 32;// деклтайп от имени -> int b = 32
//    decltype((a)) b1 = 4; // деклтайп от выражения (а) -> int & b1 = 4 -> CE
    decltype((a)) b1 = a;
}

// несколько перегрузок
struct xx{};

int fooo(int);

float fooo(float);

char fooo(char);

xx &&fooo(xx &&);

//xx fooo(xx);
/*
 * Хотим задать функция, которая бы принимала какие то аргументы для перегрузки foo и форвардила бы их в foo(...) и
 * возвращала бы результат
 */
template<typename T>
T &&declval(); // без && не работает с incomplete types

template<typename Arg0>
/*??? =>
 * delctype(fooo()) ->
 * delctype(fooo(std::forward<Arg0>(arg0))) cannot refer to arg0 ->
 * decltype(fooo(Arg0())) -> (создадим объект) НО: никто не гарантирует, что у Arg0 есть дефолтный конструктор
 * decltype(fooo(Arg0())) -> (создадим объект) НО: никто не гарантирует, что у Arg0 есть дефолтный конструктор
 *                                      +  Arg0() всегда prvalue
 * decltype(fooo(declvar<Arg0>())). Решение. Тело для declvar не надо, т.к. компилятор не генерирует машинный код для функций в decltype;
 *     нам важно знать только тип. Такая же ситуация без вычисления типа в char arr[sizeof(foo(42))] (внутри sizeof не вычисляется ничего, это unevoluated context)
 *     std::declval -> будет ошибкой эвалюировать вызов функции
 *В с++ 11:
 *
 * */auto foo_(Arg0 &&arg0) -> decltype(fooo(std::forward<Arg0>(arg0))) {
    return fooo(std::forward<Arg0>(arg0));
}

template<typename ... Args>
auto foo_Args(Args &&... args) -> decltype(fooo(std::forward<Args>(args)...)) { // trailing return type
    return fooo(std::forward<Args>(args)...);
}

// нужно писать выражение 2жды fooo(std::forward<Args>(args)...), неудобно, заменямем c C++14

template<typename ... Args>
auto foo_Args_2(Args &&... args) -> decltype(auto) { // trailing return type
    return fooo(std::forward<Args>(args)...);
}

template<typename ... Args>
decltype(auto) foo_Args_3(Args &&... args) { // trailing return type
    return fooo(std::forward<Args>(args)...);
}

//Можно ли использовать вместо decltype(auto) -> auto ? Нет.
// В нашем случае если бы написали авто, а fooo возвращала бы int &, то наша бы функция foo_ возвращала бы int.
//Правило вывода типа для авто такие же, как и при дедьюсе шаблоном. Дискардят ссылки и консты.

int main3() {
    auto c = bar(); // int c = bar (but bar is int &
}

/*Общее правило:
 * объявление локальной переменной -> auto
 * пишем return type -> decltype(auto)
 *
 * auto in return type(С++14)
 * */


auto f(int t) {
    if (t > 10) {
        return 1;
    } else {
//        return  3u;// CE - возвращаемые значения должны быть одного типа. Ничего не приводится никуда
        return 3;
    }
}

//#define NULL 0
/*
 * До с++11 для нулевого указателя использовался 0 или NULL
 * Числовая константа, которая является 0, может приводиться к указателю (но тольк компайл тайм константа)
 *
 */

void qux(int *p) {

}

template<typename... Args>
decltype(auto) qux_start(Args ... args) {
    return qux(std::forward<Args>(args)...); // cannot deduce
}

struct my_nullptr_t {
    //умеет приводиться ко всем указателям, поэтому сработает ив qux(nullptr), и в qux_start приведется после форвадинга
    template<typename T>
    operator T *() const {
        return 0;
    }
};

void main4() {
    qux(0);
    int a = 0;
//    qux_start(0);//problem -> Args = int -> cannot call qux
//    qux(a); a - not compile time const
    // решение -> внедрили в язык спец key word. Но мы можем обратиться к его типу как
    decltype(nullptr) ; //==nullptr_t

}

