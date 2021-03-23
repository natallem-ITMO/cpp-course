#include <tuple>
#include <iostream>
#include <cassert>
#include <map>
#include <functional>
#include <type_traits>

namespace ex1 {/*
 * Есть hash_function
 * https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 *
FNV-1a hash

algorithm fnv-1a is
    hash := FNV_offset_basis

    for each byte_of_data to be hashed do
        hash := hash XOR byte_of_data
        hash := hash × FNV_prime // простое число

    return hash
 */

    uint32_t const offset_basis = 0x811c9dc5u;
    uint32_t const prime = 0x01000193u;

    uint32_t fnv1a(std::string const &str) {
        uint32_t result = offset_basis;
        for (char c : str) {
            /*
             * Для чего нужен static_cast?
             * Потому что по умолчаюнию в большинстве компиляторов char знаковый, поэтому он расширится с ведущими
             * единицами, что плохо.
             * если бы все char были бы беззнаковые, то все было бы ок.
             */
            result ^= static_cast<uint8_t>(c);
            result *= prime;
        }
        return result;
    }

    int main() {
        /*
         * Не очень хорошая сигнатура фукнции, т.к. не сможем передать вектор, например, а в нижнем коде будет лишняя
         * аллокация памяти(т.к. оборачиваем в string)
         */
        fnv1a("hello");
        /*
         * Если данные приходят из soket или file, то у нас может вообще не быть string или чего то такого, это
         * может быть просто буфер, который не обязательно завершается 0.
         * Поэтому на практике такие функции принимают:
         */
    }

}

namespace ex2 {

    uint32_t const offset_basis = 0x811c9dc5u;
    uint32_t const prime = 0x01000193u;

    /*
     * буфер, строка, вектор.data, vector.size - все можем передать.
     * т.е. мы можем предоставить для удобства пользователя перегрузки
     */
    uint32_t fnv1a(char const *data, size_t size) {
        uint32_t result = offset_basis;
        for (size_t i = 0; i != size; ++i) {
            result ^= static_cast<uint8_t>(data[i]);
            result *= prime;
        }
        return result;
    }

    uint32_t fnv1a(std::string const &str) {
        return fnv1a(str.data(), str.size());
    }

    int main() {
        /*
         * Но что нам передавать если мы просто хотим передать "hello", какой размер у этого?
         *
         * Можно было бы использовать forward_iterator от контейнера и сделать функцию шаблонной.
         *
         * Такого вида функции c-style и как правило данные лежать в памяти contiguous (by one chunk, последовательно)
         * Нигде не отражено, что data and size are connected. Чтобы выражать эту идею, используют string_view
         */

    }
    struct string_view
    {
        char const * data;
        size_t size;
    };

}

namespace ex3{/*
 * Есть hash_function
 * https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 *
FNV-1a hash

algorithm fnv-1a is
    hash := FNV_offset_basis

    for each byte_of_data to be hashed do
        hash := hash XOR byte_of_data
        hash := hash × FNV_prime // простое число

    return hash
 */

    uint32_t const offset_basis = 0x811c9dc5u;
    uint32_t const prime = 0x01000193u;

    /*
     * string_view умеет создваться от "hello" и от std::string, как минимум
     */
    uint32_t fnv1a(std::string_view str) {
        uint32_t result = offset_basis;
        for (size_t i = 0; i != str.size(); ++i) {
            result ^= static_cast<uint8_t>(str[i]);
            result *= prime;
        }
        return result;
    }
    /*
     * Пока мы держим string_view, наши данные должны существовать и мы не должны с ними что то делать.
std::string                               char const *
                        <-------
                        implicit

                        ------->
                        .c_str()

А вот c string_view наборот!

std::string                               string_view
                        <-------
                        explicit

                        ------->
                        implicit
Это стоит иметь в виду
*/

    /*
     * Если мы хотим такую штуку для произвольных типов - есть std::span
     * В каком то смысле есть аналогия
     * string <-> string view
     * vector/array <-> span
     */
}

/*
 * чтобы внутри bind сохранился аргумент как std::string мы делали:
 *  std::bind(print, std::string("hello");
 *
 * По аналогии с суффиксом u, l для чисел, в языке появился суффикс s
 * 32ul <=> unsigned long
 *
 * "hello"s <-> std::string
 * "hello"sv <-> std::string_view
 * before use s do:
 * using namespace std::literals
 */

void print(std::string_view str){
    std::cout << str;
}

int main(){
    using namespace std::literals;
    std::cout << (typeid("hello"s) == typeid(std::string("sdf")));
//    auto t = std::bind(print, "hello");
//t();
//    print(std::string("hello"));
//    print("hello");
}


