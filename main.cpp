#include <iostream>

#include "common.hpp"

void tests()
{
    std::string str("SET X, 10\nSET Y, 1\nADD X, Y");
    std::string_view view = str;

    while(view.size() > 0)
    {
        auto found = consume_next(view);

        std::cout << "TOK " << found << std::endl;
    }

    constexpr std::string_view test(" hi");

    static_assert(trim_start(test, [](char c){return c == ' ';}) == "hi");
}

constexpr std::string_view fcheck(std::string_view in)
{
    return consume_next(in);
}

constexpr void constexpr_tests()
{
    static_assert(fcheck("SET X, 10") == "SET");
    static_assert(fcheck(" \n:tokeny \n") == ":tokeny");
    static_assert(fcheck(" 1234 , 5") == "1234");
}

int main()
{
    tests();
    constexpr_tests();

    return 0;
}
