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

enum class T_Type {Ident, Number, Pluss, Minus, Multyplie, Divide, Fucntion, RBacket, LBacket};

std::ostream& operator<<(std::ostream& in, T_Type text)
{
    switch (text)
    {
    case T_Type::Ident: return in << "ТИП: Ідентифікатор"; break;
    case T_Type::Number: return in << "ТИП: Константа"; break;
    case T_Type::Pluss: return in << "ТИП: Оператор +"; break;
    case T_Type::Minus: return in << "ТИП: Оператор -"; break;
    case T_Type::Multyplie: return in << "ТИП: Оператор *"; break;
    case T_Type::Divide: return in << "ТИП: Оператор /"; break;
    case T_Type::Fucntion: return in << "ТИП: Функція"; break;
    case T_Type::RBacket: return in << "ТИП: Скобрка"; break;
    case T_Type::LBacket: return in << "ТИП: Скобрка"; break;
    default:return in << "НЕВІДОМИЙ ТИП";break;
    }
}


//Клас Токену
class Token
{
    private:
    std::string text;
    T_Type type;
    int pos;
    
    public:
    Token(std::string t, T_Type t_type, int i)
    {
        text = t;
        type = t_type;
        pos = i;
    }

    //Вивід токену
    friend std::ostream& operator<<(std::ostream& in, Token t)
    {   
        in << "Токен: " << t.text << " " << t.type << " на позиції: " << t.pos;
        return in;
    }

};


//Функція розбиття рядка на токени, неправильні тоцени заповнюють ветор error

std::vector<Token> lexemize (std::string input, std::vector<Token>& errors)
{
    std::vector<Token> tokens;

    int i = 0;

    while (i < input.size()) 
    {
        unsigned char c = input[i]; ////??????
        
        //Пропустити пробіли
        if (std::isspace(c)) 
        { 
            i++; 
            continue; 
        }

        //Перевірка на цілі та дробові числа
        if (std::isdigit(c) || c == '.') 
        {
            //Зберігаємо початкове значення і для пошуку цілого чилового токена
            int start = i;

            bool dot = false, bad = false;
            
            while (i < input.size() && (std::isdigit(input[i]) || input[i] == '.')) 
            {
                //шукаємо повторбванну точку
                if (input[i] == '.') 
                {
                    if (dot) bad = true;  // вторая точка: 1.2.3
                    dot = true;
                }
                i++;
            }

            std::string num = input.substr(start, i - start); //Зберігаємо числовий токен
            
            
            //Перевірка чилового токену на правильність, ЧИ є зайві точки та чи правильно вони поставленні
            if (num.front() == '.' || num.back() == '.') 
            {
                bad = true;
            }
            
            if (bad) 
            {
                errors.push_back({num, T_Type::Number, start});
            }
            else 
            {
                tokens.push_back({num, T_Type::Number, start});
            }

            continue;
        }

        //Первевірка на ім'я ідентифікатора чи функціїї
        if (std::isalpha(c) || c == '_') 
        {
            int start = i;
            while (i < input.size() && (std::isalnum(input[i]) || input[i] == '_')) 
            {
                i++;
            }
            
            tokens.push_back({input.substr(start, i - start),T_Type::Ident, start});
            continue;
        }

        //Перевірка на оператори
        T_Type t;

        switch(input[i])
        {
            case '+': t = T_Type::Pluss; break;
            case '-': t = T_Type::Minus; break;
            case '*': t = T_Type::Multyplie; break;
            case '/': t = T_Type::Divide; break;
            case '(': t = T_Type::LBacket; break;
            case ')': t = T_Type::RBacket; break;
            default:
                errors.push_back({std::string(1, (char)input[i]), T_Type::Ident, i});
                i++;
                continue;
            break;
        }
        tokens.push_back({std::string(1, (char)input[i]), T_Type::Ident, i});

        //Нове коло
        i++;

    }

    return tokens;
}





int main()
{   
    ///ВВедення рядка
    std::string input;
    std::cout << "Введіть рядок: ";

    std::getline(std::cin, input);

    //Token function prototupe

    std::vector<Token> errors;

    std::vector<Token> tokens = lexemize(input, errors);
    

    //Output of tokens
    
    std::cout<< "Good Tokens: " << std::endl;
 
    if (tokens.empty())
        std::cout << "NO one :(" << std::endl;
    else
        for (int i = 0; i < tokens.size(); i++)
            std::cout << tokens[i] << std::endl;

    std::cout<< "Bad Tokens: " << std::endl;

    if (errors.empty())
        std::cout << "Good job :)" << std::endl;
    else
        for (int i = 0; i < errors.size(); i++)
            std::cout << errors[i] << std::endl;

    return 0;
}