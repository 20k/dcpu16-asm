#include "util.hpp"
#include "base_asm.hpp"
#include <string>
#include <assert.h>

inline
std::string read_file(const std::string& file)
{
    FILE* f = fopen(file.c_str(), "r");

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if(fsize == 0)
    {
        fclose(f);
        return "";
    }

    std::string ret;
    ret.resize(fsize);

    fread(&ret[0], 1, fsize, f);

    return ret;
}

inline
void write_all_bin(const std::string& fname, std::string_view str)
{
    FILE* f = fopen(fname.c_str(), "wb");

    if(str.size() > 0)
    {
        fwrite(&str[0], str.size(), 1, f);
    }

    fclose(f);
}

void print_sv(std::string_view in)
{
    for(auto i : in)
    {
        putchar(i);
    }

    printf("\n");
}

void tests()
{
    {
        std::string_view view = "SET X, 10\nSET Y, 1\nADD X, Y";

        while(view.size() > 0)
        {
            auto found = consume_next(view, true);

            printf("TOK ");

            for(auto& i : found)
                printf("%c", i);

            printf("\n");
        }

        constexpr std::string_view test(" hi");

        static_assert(trim_start(test, [](char c){return c == ' ';}) == "hi");
    }

    {
        std::string_view test = "SET X, 65539";
        auto [binary_opt, err] = assemble(test);

        assert(!binary_opt.has_value());
    }

    {
        std::string_view test = "SET X, [hello]\n:hello\nSET Y, 53\nSET Z hello";
        auto [binary_opt, err] = assemble(test);

        assert(binary_opt.has_value());
    }
}

constexpr std::string_view fcheck(std::string_view in)
{
    return consume_next(in, true);
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

    static_assert(get_constant_of<uint16_t>("0x1234") == 0x1234);
    static_assert(get_constant_of<uint16_t>("-0x1234") == (uint16_t)-0x1234);
    static_assert(get_constant_of<uint16_t>("-1234") == (uint16_t)-1234);
    static_assert(get_constant_of<uint16_t>("1234") == 1234);

    constexpr auto result = assemble("SET X, 10");

    static_assert(result.first.has_value());

    //std::optional<uint32_t> out;
    //constexpr auto val = decode_value("x", arg_pos::B, out);

    //auto [v1, vextra] = decode_value("x", arg_pos::B);

    //constexpr auto val = assemble("set x, 10\n");


    /*constexpr auto fval = assemble("set x, 10\nadd x, 1").first.value();

    static_assert(fval.mem.svec[0] == 0b1010110001100001);
    static_assert(fval.mem.svec[1] == 0b1000100001100010);

    constexpr auto fval2 = assemble("SET X, 10\nADD X, 1").first.value();

    static_assert(fval2.mem.svec[0] == 0b1010110001100001);
    static_assert(fval2.mem.svec[1] == 0b1000100001100010);*/
}

int main(int argc, char* argv[])
{
    /*if(argc == 1)
    {
        tests();
        constexpr_tests();
    }*/

    constexpr_tests();

    if(argc <= 1)
    {
        printf("Usage: dcpu16-asm.exe ./source [./out] [-fselftest]");
        return 0;
    }

    if(iequal(argv[1], "-fselftest"))
    {
        tests();
        printf("Self tests passed");
        return 0;
    }

    std::string file = read_file(argv[1]);

    auto [data_opt, err] = assemble(file);

    if(!data_opt.has_value())
    {
        printf("Could not assemble. Err: ");
        print_sv(err.msg);
        printf("Name: ");
        print_sv(err.name_in_source);
        printf("Character: %i", err.character);
        printf("Line %i\n", err.line);

        return 1;
    }

    std::string_view write((char*)&data_opt.value().mem.svec[0], data_opt.value().mem.idx * sizeof(uint16_t) / sizeof(char));

    if(argc == 2)
    {
        std::string out_name = std::string(argv[1]) + ".asm";

        write_all_bin(out_name, write);

        return 0;
    }

    if(argc == 3)
    {
        std::string out_name = std::string(argv[2]);

        write_all_bin(out_name, write);

        return 0;
    }

    /*for(int i=1; i < argc - 1; i++)
    {
        std::string_view view(argv[i]);
        std::string_view next(argv[i + 1]);

        if(view.starts_with("-f"))
        {
            std::cout << "Argument not recognised " << view << std::endl;
            continue;
        }

        if(view.starts_with("-o"))
        {

        }
    }*/

    return 0;
}
