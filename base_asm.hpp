#ifndef BASE_ASM_HPP_INCLUDED
#define BASE_ASM_HPP_INCLUDED

#include <optional>
#include <string_view>
#include <array>
#include <tuple>
#include "stack_vector.hpp"
#include "shared.hpp"
#include "util.hpp"
#include "base_asm_fwd.hpp"
#include <iostream>

///TODO: https://github.com/EqualizR/DEQOS/blob/master/AssemblerExtensions.txt
///https://github.com/ddevault/organic

constexpr
bool should_prune(char c)
{
    return c == ' ' || c == '\n' || c == ',';
}

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

struct label
{
    uint16_t offset = 0;
    std::string_view name = "";
};

struct define
{
    uint16_t value = 0;
    std::string_view name = "";
};

struct symbol_table
{
    stack_vector<label, 1024> usages;
    stack_vector<label, 1024> definitions;
    stack_vector<define, 1024> defines;

    constexpr
    std::optional<uint16_t> get_symbol_definition(std::string_view name)
    {
        for(int i=0; i < (int)definitions.size(); i++)
        {
            if(definitions[i].name == name)
                return definitions[i].offset;
        }

        for(int i=0; i < (int)defines.size(); i++)
        {
            if(defines[i].name == name)
                return defines[i].value;
        }

        return std::nullopt;
    }
};

struct expression_result
{
    uint16_t which_register = 0;
    uint16_t word = 0;
};

constexpr std::string_view consume_expression_token(std::string_view& in)
{
    while(in.size() > 0 && (in.front() == ' ' || in.front() == '\t')){in.remove_prefix(1);}

    if(in.size() == 0)
        return "";

    if(in.starts_with("0x"))
    {
        int fin = 2;

        for(fin = 2; fin < (int)in.size(); fin++)
        {
            if(!is_hex_digit(in[fin]))
                break;
        }

        std::string_view data(in.begin(), in.begin() + fin);

        in.remove_prefix(fin);

        return data;
    }
    else if(in.starts_with("0b"))
    {
        int fin = 2;

        for(fin = 2; fin < (int)in.size(); fin++)
        {
            if(!is_binary_digit(in[fin]))
                break;
        }

        std::string_view data(in.begin(), in.begin() + fin);

        in.remove_prefix(fin);

        return data;
    }
    else if(is_digit(in[0]))
    {
        int fin = 0;

        for(fin = 0; fin < (int)in.size(); fin++)
        {
            if(!is_digit(in[fin]))
                break;
        }

        std::string_view data(in.begin(), in.begin() + fin);

        in.remove_prefix(fin);

        return data;
    }
    else
    {
        if(in[0] == ')' || in[0] == '(' || in[0] == '+' || in[0] == '-' || in[0] == '/' ||
           in[0] == '|' || in[0] == '^' || in[0] == '&' ||
           in[0] == '%')
        {
            std::string_view ret = in.substr(0, 1);

            in.remove_prefix(1);

            return ret;
        }

        if(in[0] == '*')
        {
            if(in.size() >= 2)
            {
                if(in[1] == '*')
                {
                    in.remove_prefix(2);
                    return "**";
                }
            }

            in.remove_prefix(1);

            return "*";
        }

        if(isalnum_c(in[0]))
        {
            int fin = 0;

            for(fin=0; fin < (int)in.size(); fin++)
            {
                if(!isalnum_c(in[fin]))
                    break;
            }

            std::string_view ret = in.substr(0, fin);
            in.remove_prefix(fin);

            return ret;
        }

        return "No match";
    }
}

///shunting yard
constexpr std::optional<expression_result> parse_expression(symbol_table& sym, std::string_view expr)
{
    std::array supported_operators
    {
        "+", "-", "/", "|", "^", "&", "%", "*", "**",
    };

    std::array precedence
    {
        4, 4, 3, 10, 9, 8, 3, 3, 2
    };

    std::array left_associative
    {
        1, 1, 1, 1, 1, 1, 1, 1, 0
    };

    static_assert(precedence.size() == supported_operators.size());

    stack_vector<std::string_view, 512> operator_stack;
    expression_result res;

    auto get_operator_idx = [&](std::string_view in)
    {
        for(int i=0; i < (int)supported_operators.size(); i++)
        {
            if(supported_operators[i] == in)
            {
                return i;
            }
        }

        return -1;
    };

    auto get_precedence = [&](std::string_view in)
    {
        return precedence[get_operator_idx(in)];
    };

    auto is_left_associative = [&](std::string_view in)
    {
        return left_associative[get_operator_idx(in)];
    };

    stack_vector<std::string_view, 512> output_queue;

    while(expr.size() > 0)
    {
        std::string_view consumed = consume_expression_token(expr);

        if(consumed.size() == 0)
            break;

        bool all_whitespace = true;

        for(auto i : consumed)
        {
            if(!(i == ' ' || i == '\t'))
                all_whitespace = false;
        }

        if(all_whitespace)
            continue;

        if(is_constant(consumed))
        {
            output_queue.push_back(consumed);
        }
        else if(get_operator_idx(consumed) != -1)
        {
            while(operator_stack.size() > 0 && (get_operator_idx(operator_stack.back()) != -1) &&
                  operator_stack.back() != "(" &&
                  (
                    (get_precedence(operator_stack.back()) < get_precedence(consumed)) ||
                        ((get_precedence(operator_stack.back()) == get_precedence(consumed)) && is_left_associative(consumed))))
            {
                output_queue.push_back(operator_stack.back());
                operator_stack.pop_back();
            }

            operator_stack.push_back(consumed);
        }
        else if(consumed == "(")
        {
            operator_stack.push_back("(");
        }
        else if(consumed == ")")
        {
            bool found = false;

            while(operator_stack.size() > 0)
            {
                if(operator_stack.back() == "(")
                {
                    found = true;
                    operator_stack.pop_back();
                    break;
                }

                auto val = operator_stack.back();

                output_queue.push_back(val);

                operator_stack.pop_back();
            }

            // error! mismatched parentheses
            if(!found)
                return std::nullopt;
        }
    }

    while(operator_stack.size() > 0)
    {
        // mismatched parentheses
        if(operator_stack.back() == ")" || operator_stack.back() == "(")
            return std::nullopt;

        output_queue.push_back(operator_stack.back());
        operator_stack.pop_back();
    }

    for(auto i : output_queue)
    {
        std::cout << i << std::endl;
    }

    return res;
}

// so
// create a table of MAX_WHATEVER long which contains byte -> label mapping
// then figure out a way to sub label pc value back in to instructions
// could insert all label references into an array of word values, and then insert all label definitions into an array of pc values
// then sub them in afterwards
inline
constexpr std::optional<uint32_t> decode_value(std::string_view in, arg_pos::type apos, std::optional<int32_t>& out, std::optional<std::string_view>& is_label, symbol_table& sym)
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

        if(is_label_reference(extracted))
        {
            auto test_val_opt = sym.get_symbol_definition(extracted);

            if(test_val_opt.has_value())
            {
                out = test_val_opt.value();
                return 0x1e;
            }

            out = 0;
            is_label = extracted;
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

    if(is_label_reference(in))
    {
        auto test_val_opt = sym.get_symbol_definition(in);

        if(test_val_opt.has_value())
        {
            out = test_val_opt.value();
            return 0x1f;
        }

        out = 0;
        is_label = in;
        return 0x1f;
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
std::optional<error_info> add_opcode_with_prefix(symbol_table& sym, std::string_view& in, stack_vector<uint16_t, MEM_SIZE>& out, size_t& token_text_offset_start, size_t token_start, const stack_vector<uint16_t, MEM_SIZE>& source_to_line)
{
    error_info err;

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
        {"snd", 0, 0x1c}, ///extension for multiprocessor. sends a value on a channel
        {"rcv", 0, 0x1d}, ///extension for multiprocessor. receives a value on a channels
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
        {"ifw", 1, 0x1a}, ///extension for multiprocessor. only executes next instruction if the channel is waiting to write a value
        {"ifr", 1, 0x1b}, ///extension for multiprocessor. only executes next instruction if the channel is waiting to read a value

        {"brk", 2, 0x0},
        // could have an instruction that swaps modes into extended alt proposal mode
    };

    size_t old_size = in.size();

    auto consumed_name = consume_next(in);

    size_t num_removed = old_size - (in.size() + consumed_name.size());

    token_text_offset_start = num_removed;

    err.name_in_source = consumed_name;
    err.character = token_text_offset_start + token_start;
    err.line = source_to_line[err.character];

    if(consumed_name.size() == 0)
        return std::nullopt;

    if(is_label_definition(consumed_name))
    {
        if(consumed_name.starts_with(':'))
            consumed_name.remove_prefix(1);
        if(consumed_name.ends_with(':'))
            consumed_name.remove_suffix(1);

        auto trimmed = trim_start(consumed_name, [](char c) {return c == ' ' || c == '\n' || c == ','; });

        label l;
        l.name = trimmed;
        l.offset = out.size();

        sym.definitions.push_back(l);
        return std::nullopt;
    }

    if(iequal(".def", consumed_name) || iequal("def", consumed_name))
    {
        auto label_name = consume_next(in);
        auto label_value = consume_next(in);

        if(!is_constant(label_value))
        {
            err.msg = ".def value must be a constant";
            return err;
        }

        uint16_t val = get_constant(label_value);

        define d;
        d.name = label_name;
        d.value = val;

        sym.defines.push_back(d);

        return std::nullopt;
    }

    if(iequal(".dat", consumed_name) || iequal("dat", consumed_name))
    {
        bool looping = true;

        while(looping)
        {
            auto value = consume_next(in);

            if(is_label_reference(value) && sym.get_symbol_definition(value).has_value())
            {
                out.push_back(sym.get_symbol_definition(value).value());
            }
            else if(is_constant(value))
            {
                out.push_back(get_constant(value));
            }
            else if(is_string(value))
            {
                value.remove_prefix(1);
                value.remove_suffix(1);

                ///need to parse string constants
                for(int idx = 0; idx < (int)value.size(); idx++)
                {
                    char c = value[idx];

                    if(c == '\\')
                    {
                        if(idx == (int)value.size() - 1)
                        {
                            err.msg = "Invalid unterminated escape sequence \\ at end of token";
                            return err;
                        }

                        char next = value[idx + 1];

                        bool valid_escape_sequence = true;

                        if(next == 't')
                            c = '\t';

                        else if(next == 'n')
                            c = '\n';

                        else if(next == '\\')
                            c = '\\';

                        else if(next == '\'')
                            c = '\'';

                        else if(next == '\"')
                            c = '\"';

                        else if(next == 'r')
                            c = '\r';

                        else if(next == 'b')
                            c = '\b';

                        else if(next == 'f')
                            c = '\f';

                        else if(next == 'v')
                            c = '\v';

                        else if(next == '0')
                            c = '\0';

                        else if(next == 'a')
                            c = '\a';

                        else if(next == 'e')
                            c = '\e';

                        else if(next == '?')
                            c = '?';
                        else
                            valid_escape_sequence = false;

                        if(!valid_escape_sequence)
                        {
                            err.msg = "Invalid escape sequence";
                            err.name_in_source = std::string_view(value.begin() + idx, value.begin() + idx + 2);
                            return err;
                        }

                        idx++;
                    }

                    uint16_t wide = (uint8_t)c;

                    out.push_back(wide);
                }
            }
            else
            {
                err.msg = "Bad .dat, must be constant or label or definition or string";
                return err;
            }

            if(peek_next(in) == ",")
            {
                consume_next(in);

                looping = true;
            }
            else
            {
                looping = false;
            }
        }

        return std::nullopt;
    }

    for(auto [name, cls, code] : opcodes)
    {
        if(iequal(name, consumed_name))
        {
            if(cls == 0)
            {
                auto val_b = consume_next(in);

                if(consume_next(in) != ",")
                {
                    err.msg = "Expected ,";
                    return err;
                }

                auto val_a = consume_next(in);

                std::optional<int32_t> extra_b;
                std::optional<std::string_view> is_label_b;
                auto compiled_b = decode_value(val_b, arg_pos::B, extra_b, is_label_b, sym);

                std::optional<int32_t> extra_a;
                std::optional<std::string_view> is_label_a;
                auto compiled_a = decode_value(val_a, arg_pos::A, extra_a, is_label_a, sym);

                if(!compiled_b.has_value())
                {
                    err.msg = "first argument failed to decode";
                    return err;
                }

                if(!compiled_a.has_value())
                {
                    err.msg = "second argument failed to decode";
                    return err;
                }

                auto instr = construct_type_a(code, compiled_a.value(), compiled_b.value());

                out.push_back(instr);

                if(extra_a.has_value())
                {
                    uint32_t promote_a = extra_a.value();

                    if(promote_a >= 65536)
                    {
                        err.msg = "second argument >= UINT_MAX or < INT_MIN";
                        return err;
                    }

                    if(is_label_a.has_value())
                    {
                        label l;
                        l.name = is_label_a.value();
                        l.offset = out.size();

                        sym.usages.push_back(l);
                    }

                    out.push_back(promote_a);
                }

                if(extra_b.has_value())
                {
                    uint32_t promote_b = extra_b.value();

                    if(promote_b >= 65536)
                    {
                        err.msg = "first argument >= UINT_MAX or < INT_MIN";
                        return err;
                    }

                    if(is_label_b.has_value())
                    {
                        label l;
                        l.name = is_label_b.value();
                        l.offset = out.size();

                        sym.usages.push_back(l);
                    }

                    out.push_back(promote_b);
                }

                return std::nullopt;
            }

            if(cls == 1)
            {
                auto val_a = consume_next(in);

                std::optional<int32_t> extra_a;
                std::optional<std::string_view> is_label_a;
                auto compiled_a = decode_value(val_a, arg_pos::A, extra_a, is_label_a, sym);

                if(!compiled_a.has_value())
                {
                    err.msg = "first argument failed to decode";
                    return err;
                }

                auto instr = construct_type_b(code, compiled_a.value());

                out.push_back(instr);

                if(extra_a.has_value())
                {
                    uint32_t promote_a = extra_a.value();

                    if(promote_a >= 65536)
                    {
                        err.msg = "first argument >= UINT_MAX or < INT_MIN";
                        return err;
                    }

                    if(is_label_a.has_value())
                    {
                        label l;
                        l.name = is_label_a.value();
                        l.offset = out.size();

                        sym.usages.push_back(l);
                    }

                    out.push_back(promote_a);
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

    err.msg = "Not command or label";
    return err;
}

constexpr
std::pair<std::optional<return_info>, error_info> assemble(std::string_view text)
{
    return_info rinfo;
    symbol_table sym;

    stack_vector<uint16_t, MEM_SIZE> source_to_line;

    int line = 0;
    for(int idx = 0; idx < (int)text.size(); idx++)
    {
        source_to_line.push_back((uint16_t)line);

        if(text[idx] == '\n')
        {
            line++;
        }
    }

    size_t start_size = text.size();
    size_t last_mem_size = 0;

    while(text.size() > 0)
    {
        size_t offset = start_size - text.size();

        size_t token_offset = 0;

        auto error_opt = add_opcode_with_prefix(sym, text, rinfo.mem, token_offset, offset, source_to_line);

        for(size_t i = last_mem_size; i < rinfo.mem.size(); i++)
        {
            //rinfo.translation_map[i] = offset;
            rinfo.translation_map.push_back(offset + token_offset);
        }

        last_mem_size = rinfo.mem.size();

        if(error_opt.has_value())
        {
            return {std::nullopt, error_opt.value()};
        }
    }

    int last_val = rinfo.translation_map.size();

    if(last_val > 0)
    {
        int prev_val = last_val - 1;

        for(int i=rinfo.translation_map.size(); i < rinfo.translation_map.max_size; i++)
        {
            rinfo.translation_map[i] = rinfo.translation_map[prev_val];
        }
    }

    for(int idx = 0; idx < (int)rinfo.translation_map.size(); idx++)
    {
        uint16_t source_character = rinfo.translation_map[idx];

        rinfo.pc_to_source_line.push_back(source_to_line[source_character]);
    }

    int last_pc_line = (int)rinfo.pc_to_source_line.size() - 1;

    if(last_pc_line >= 0)
    {
        for(int i=rinfo.pc_to_source_line.size(); i < rinfo.pc_to_source_line.max_size; i++)
        {
            rinfo.pc_to_source_line[i] = rinfo.pc_to_source_line[last_pc_line] + 1;
        }
    }

    /*if(sym.definitions.size() > 0 && sym.usages.size() > 0)
    {
        if(!std::is_constant_evaluated())
        {
            std::sort(sym.definitions.begin(), sym.definitions.end(), [](const label& l1, const label& l2) constexpr {return l1.name < l2.name;});
            std::sort(sym.usages.begin(), sym.usages.end(), [](const label& l1, const label& l2) constexpr {return l1.name < l2.name;});
        }
        else
        {

        }
    }*/

    for(label& j : sym.usages)
    {
        bool found = false;

        for(label& i : sym.definitions)
        {
            if(j.name != i.name)
                continue;

            rinfo.mem.svec[j.offset] = i.offset;

            found = true;
            break;
        }

        if(!found)
        {
            error_info err;
            err.msg = "Label used with no definition";
            err.character = 0;
            err.line = 0;
            err.name_in_source = j.name;

            return {std::nullopt, err};
        }
    }

    return {rinfo, error_info()};
}

#endif // BASE_ASM_HPP_INCLUDED
