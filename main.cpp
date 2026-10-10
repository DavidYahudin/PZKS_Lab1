#include <iostream>
#include <string>
#include <vector>

//Token Type identefier
//Ident     - ідентифікатор
//Number    - константа
//Pluss     - +
//Minuss    - -
//Multyplie - *
//Divide    - /
//RBacket   - (
//LBacket   - )

enum class T_Type {Ident, Number, Pluss, Minuss, Multyplie, Divide, RBacket, LBacket};


//Токен
struct Token
{
    std::string text;
    T_Type type;
    int pos;
};



int main()
{
    ///ВВедення рядка
    std::string input;
    std::cout << "Input your line: ";

    std::getline(std::cin, input);

    return 0;
}