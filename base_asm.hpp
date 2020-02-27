#ifndef BASE_ASM_HPP_INCLUDED
#define BASE_ASM_HPP_INCLUDED

#include <optional>
#include <string_view>

inline
constexpr std::optional<int> get_register_assembly_value_from_name(std::string_view in)
{
    if(in == "a")
        return 0;

    if(in == "b")
        return 1;

    if(in == "c")
        return 2;

    if(in == "x")
        return 3;

    if(in == "y")
        return 4;

    if(in == "z")
        return 5;

    if(in == "i")
        return 6;

    if(in == "j")
        return 7;

    return std::nullopt;
}

inline
constexpr uint32_t construct_type_a(uint32_t o, uint32_t a, uint32_t b)
{
    return o | (b << 5) | (a << 10);
}

inline
constexpr uint32_t construct_type_b(uint32_t o, uint32_t a)
{
    return (o << 5) | (a << 10);
}

namespace arg_pos
{
    enum type
    {
        A,
        B,
    };
}

inline
constexpr std::optional<uint32_t> decode_value(std::string_view in, arg_pos::type apos, std::optional<uint32_t>& out)
{
    {
        auto reg_opt = get_register_assembly_value_from_name(in);

        if(reg_opt.has_value())
            return reg_opt;
    }

    if(is_address(in))
    {
        auto extracted = extract_address_contents(in);

        auto extracted_reg_opt = get_register_assembly_value_from_name(extracted);

        if(extracted_reg_opt.has_value())
        {
            return 0x08 + extracted_reg_opt.value();
        }

        if(extracted == "--sp" || extracted == "sp++")
            return 0x18;

        if(extracted == "sp")
            return 0x19;

        if(is_constant(extracted))
        {
            out = get_constant(extracted);
            return 0x1e;
        }
    }

    if(in == "push" || in == "pop")
        return 0x18;

    if(in == "sp")
        return 0x1b;

    if(in == "pc")
        return 0x1c;

    if(in == "ex")
        return 0x1d;

    if(is_constant(in))
    {
        auto val = get_constant(in);

        if(apos == arg_pos::A)
        {
            if(val == -1)
                return 0x20;

            if(val >= 0 && val <= 30)
                return 0x21 + val;
            else
            {
                out = val;
                return 0x1f;
            }
        }
        else
        {
            out = val;
            return 0x1f;
        }
    }

    return std::nullopt;
}

#endif // BASE_ASM_HPP_INCLUDED
