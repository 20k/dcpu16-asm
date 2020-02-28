#include <iostream>

#include "common.hpp"
#include "base_asm.hpp"

void tests()
{
    std::string_view view = "SET X, 10\nSET Y, 1\nADD X, Y";

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

    static_assert(is_constant("-0x1234"));
    static_assert(is_constant("0x1234"));
    static_assert(is_constant("1234"));
    static_assert(is_constant("-1234"));

    static_assert(!is_constant("0xjasdf"));
    static_assert(!is_constant("potato"));
    static_assert(!is_constant("-1234cat"));

    static_assert(get_constant("0x1234") == 0x1234);
    static_assert(get_constant("-0x1234") == -0x1234);
    static_assert(get_constant("-1234") == -1234);
    static_assert(get_constant("1234") == 1234);

    //std::optional<uint32_t> out;
    //constexpr auto val = decode_value("x", arg_pos::B, out);

    //auto [v1, vextra] = decode_value("x", arg_pos::B);

    //constexpr auto val = assemble("set x, 10\n");


    constexpr auto fval = assemble("set x, 10\nadd x, 1").value();

    static_assert(fval[0] == 0b1010110001100001);
    static_assert(fval[1] == 0b1000100001100010);

    constexpr auto fval2 = assemble("SET X, 10\nADD X, 1").value();

    static_assert(fval2[0] == 0b1010110001100001);
    static_assert(fval2[1] == 0b1000100001100010);
}

int main(int argc, char* argv[])
{
    /*if(argc == 1)
    {
        tests();
        constexpr_tests();
    }*/

    if(argc <= 1)
    {
        printf("Usage: dcpu-16-asm.exe ./source [./out]");
        return 0;
    }

    for(int i=1; i < argc; i++)
    {
        std::string_view view(argv[i]);
    }

    return 0;
}
