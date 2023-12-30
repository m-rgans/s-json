#include <string>
#include <istream>
#include <sstream>

#include <iostream>

std::string get_next_token(std::istream& stream) {
    char c;
    std::stringstream token;

    // track to next token
    while(std::isspace(c = stream.get())){}

    //get everything until colon or comma or } or ]

    
    return token.str();
}



int main() {
    std::string a = get_next_token(std::cin);
    std::cout << a << '\n';
    return 0;
}