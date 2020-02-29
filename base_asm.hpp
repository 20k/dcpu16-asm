#ifndef BASE_ASM_HPP_INCLUDED
#define BASE_ASM_HPP_INCLUDED

#include <optional>
#include <string_view>
#include <array>
#include <tuple>
#include "stack_vector.hpp"
#include "shared.hpp"

#define MEM_SIZE 0x10000

inline
constexpr std::optional<int> get_register_assembly_value_from_name(std::string_view in)
{
    if(iequal(in, "a"))
        return 0;

    if(iequal(in, "b"))
        return 1;

    if(iequal(in, "c"))
        return 2;

    if(iequal(in, "x"))
        return 3;

    if(iequal(in, "y"))
        return 4;

    if(iequal(in, "z"))
        return 5;

    if(iequal(in, "i"))
        return 6;

    if(iequal(in, "j"))
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

inline
constexpr uint32_t construct_type_c(uint32_t o)
{
    return (o << 10);
}

inline
constexpr std::optional<uint32_t> decode_value(std::string_view in, arg_pos::type apos, std::optional<uint32_t>& out)
{
    {
        auto reg_opt = get_register_assembly_value_from_name(in);

        ///cannot return reg_opt, because otherwise it isn't constexpr due to optional lacking constexpr move
        if(reg_opt.has_value())
            return reg_opt.value();
    }

    if(is_address(in))
    {
        auto extracted = extract_address_contents(in);

        auto extracted_reg_opt = get_register_assembly_value_from_name(extracted);

        if(extracted_reg_opt.has_value())
        {
            return 0x08 + extracted_reg_opt.value();
        }

        if(iequal(extracted, "--sp") || iequal(extracted, "sp++"))
            return 0x18;

        if(iequal(extracted, "sp"))
            return 0x19;

        // unimpl [register + next word]
        // unimpl [sp + next word]

        if(is_constant(extracted))
        {
            out = get_constant(extracted);
            return 0x1e;
        }
    }

    if(iequal(in, "push") || iequal(in, "pop"))
        return 0x18;

    if(iequal(in, "sp"))
        return 0x1b;

    if(iequal(in, "pc"))
        return 0x1c;

    if(iequal(in, "ex"))
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

struct opcode
{
    std::string_view view;
    int type;
    uint16_t code;
};

inline
constexpr
std::optional<std::string_view> add_opcode_with_prefix(std::string_view& in, stack_vector<uint32_t, 128>& out)
{
    opcode opcodes[] =
    {
        {"set", 0, 1},
        {"mov", 0, 1},
        {"add", 0, 2},
        {"sub", 0, 3},
        {"mul", 0, 4},
        {"mli", 0, 5},
        {"div", 0, 6},
        {"dvi", 0, 7},
        {"mod", 0, 8},
        {"mdi", 0, 9},
        {"and", 0, 0x0a},
        {"bor", 0, 0x0b},
        {"xor", 0, 0x0c},
        {"shr", 0, 0x0d},
        {"asr", 0, 0x0e},
        {"shl", 0, 0x0f},
        {"ifb", 0, 0x10},
        {"ifc", 0, 0x11},
        {"ife", 0, 0x12},
        {"ifn", 0, 0x13},
        {"ifg", 0, 0x14},
        {"ifa", 0, 0x15},
        {"ifl", 0, 0x16},
        {"ifu", 0, 0x17},
        {"adx", 0, 0x1a},
        {"sbx", 0, 0x1b},
        {"sti", 0, 0x1e},
        {"std", 0, 0x1f},

        {"jsr", 1, 0x01},
        {"int", 1, 0x08},
        {"iag", 1, 0x09},
        {"ias", 1, 0x0a},
        {"rfi", 1, 0x0b},
        {"iaq", 1, 0x0c},
        {"hwn", 1, 0x10},
        {"hwq", 1, 0x11},
        {"hwi", 1, 0x12},

        {"brk", 2, 0x0},
    };

    auto consumed_name = consume_next(in);

    if(consumed_name.size() == 0)
        return std::nullopt;

    for(auto [name, cls, code] : opcodes)
    {
        if(iequal(name, consumed_name))
        {
            if(cls == 0)
            {
                auto val_b = consume_next(in);
                auto val_a = consume_next(in);

                std::optional<uint32_t> extra_b;
                auto compiled_b = decode_value(val_b, arg_pos::B, extra_b);

                std::optional<uint32_t> extra_a;
                auto compiled_a = decode_value(val_a, arg_pos::A, extra_a);

                if(!compiled_b.has_value())
                    return "first argument failed to decode";

                if(!compiled_a.has_value())
                    return "second argument failed to decode";

                auto instr = construct_type_a(code, compiled_a.value(), compiled_b.value());

                out.push_back(instr);

                if(extra_a.has_value())
                {
                    out.push_back(extra_a.value());
                }

                if(extra_b.has_value())
                {
                    out.push_back(extra_b.value());
                }

                return std::nullopt;
            }

            if(cls == 1)
            {
                auto val_a = consume_next(in);

                std::optional<uint32_t> extra_a;
                auto compiled_a = decode_value(val_a, arg_pos::A, extra_a);

                if(!compiled_a.has_value())
                    return "first argument failed to decode";

                auto instr = construct_type_b(code, compiled_a.value());

                out.push_back(instr);

                if(extra_a.has_value())
                {
                    out.push_back(extra_a.value());
                }

                return std::nullopt;
            }

            if(cls == 2)
            {
                auto instr = construct_type_c(code);

                out.push_back(instr);

                return std::nullopt;
            }
        }
    }

    return "Error not command";
}

struct return_info
{
    std::array<uint16_t, MEM_SIZE> mem = {};
    uint16_t idx = 0;

    constexpr return_info(){}
};

inline
constexpr
std::pair<std::optional<return_info>, std::string_view> assemble(std::string_view text)
{
    return_info rinfo;

    while(text.size() > 0)
    {
        stack_vector<uint32_t, 128> svec;

        auto error_opt = add_opcode_with_prefix(text, svec);

        for(uint32_t val = 0; val < svec.idx; val++)
        {
            if(val + rinfo.idx >= MEM_SIZE)
                return {std::nullopt, "Memory overflow"};

            rinfo.mem[val + rinfo.idx] = svec.svec[val];
        }

        rinfo.idx += svec.idx;

        ///TODO, pass along error messages
        if(error_opt.has_value())
        {
            return {std::nullopt, error_opt.value()};
        }
    }

    return {rinfo, ""};
}

#endif // BASE_ASM_HPP_INCLUDED
