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
#include <cmath>
#include <assert.h>
#include <vector>

///TODO: https://github.com/EqualizR/DEQOS/blob/master/AssemblerExtensions.txt
///https://github.com/ddevault/organic

struct assembler_settings
{
    bool no_packed_constants = false;
    uint16_t location = 0;
    std::vector<std::string> label_values_to_extract;
    std::vector<std::pair<uint16_t, std::string_view>> provided_symbol_definitions;
};

constexpr
bool should_prune(char c)
{
    return c == ' ' || c == '\n' || c == ',';
}

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

constexpr std::optional<int> get_register_immediate_encoding(std::string_view in)
{
    if(auto value_opt = get_register_assembly_value_from_name(in); value_opt.has_value())
    {
        ///1:1 mapping
        return value_opt.value();
    }

    if(iequal(in, "sp"))
        return 0x1b;

    if(iequal(in, "pc"))
        return 0x1c;

    if(iequal(in, "ex"))
        return 0x1d;

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

struct delayed_expression
{
    // root instruction
    uint16_t base_word = 0;
    // the additional word
    uint16_t extra_word = 0;
    arg_pos::type type;
    std::string_view expression = "";
    bool is_memory_reference = true;
};

struct symbol_table
{
    std::vector<label> usages;
    std::vector<label> definitions;
    std::vector<define> defines;
    std::vector<delayed_expression> expressions;
    uint16_t base_offset = 0;

    constexpr
    std::optional<uint16_t> get_symbol_definition(std::string_view name) const
    {
        for(int i=0; i < (int)definitions.size(); i++)
        {
            if(definitions[i].name == name)
                return definitions[i].offset + base_offset;
        }

        for(int i=0; i < (int)defines.size(); i++)
        {
            if(defines[i].name == name)
                return defines[i].value;
        }

        return std::nullopt;
    }
};

constexpr std::optional<std::string_view> consume_expression_token(std::string_view& in)
{
    while(in.size() > 0 && (in.front() == ' ' || in.front() == '\t')){in.remove_prefix(1);}

    if(in.size() == 0)
        return "";

    if(in.size() >= 3)
    {
        char start_tok = in[0];
        char end_tok = in[2];

        if(start_tok == end_tok && is_valid_string_delimiter(start_tok))
        {
            std::string_view data(in.begin(), in.begin() + 3);

            in.remove_prefix(3);

            return data;
        }
    }

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

        return std::nullopt;
    }
}

constexpr int get_operator_idx(std::string_view in)
{
    std::array supported_operators
    {
        "+", "-", "/", "|", "^", "&", "%", "*", "**",
    };

    for(int i=0; i < (int)supported_operators.size(); i++)
    {
        if(supported_operators[i] == in)
        {
            return i;
        }
    }

    return -1;
}

template<typename T>
constexpr
T exec_op(T one, T two, std::string_view op)
{
    if(op == "+")
        return one + two;

    if(op == "-")
        return one - two;

    if(op == "/")
    {
        if(two == 0)
            return 0;

        return one / two;
    }

    if(op == "|")
        return one | two;

    if(op == "^")
        return one ^ two;

    if(op == "&")
        return one & two;

    if(op == "%")
    {
        if(two == 0)
            return 0;

        return one % two;
    }

    if(op == "*")
        return one * two;

    if(op == "**")
        return std::pow(one, two);

    return 0;
}

template<typename T>
struct expression_result_with_width
{
    std::optional<std::string_view> op = std::nullopt;
    std::optional<std::string_view> which_register = std::nullopt;
    std::optional<T> word = std::nullopt;

    constexpr
    bool fully_resolved() const
    {
        return word.has_value() && !which_register.has_value();
    }
};

struct expression_result
{
    std::optional<std::string_view> op = std::nullopt;
    std::optional<std::string_view> which_register = std::nullopt;
    std::optional<uint16_t> word = std::nullopt;

    template<typename T>
    constexpr
    expression_result(const expression_result_with_width<T>& in)
    {
        op = in.op;
        which_register = in.which_register;

        if(in.word.has_value())
        {
            word = (uint16_t)in.word.value();
        }
    }

    constexpr
    expression_result()
    {

    }

    constexpr
    bool fully_resolved() const
    {
        return word.has_value() && !which_register.has_value();
    }
};

template<typename T>
constexpr
std::pair<std::optional<expression_result_with_width<T>>, int> resolve_expression(const symbol_table& sym, const std::vector<std::string_view>& stk, bool& should_delay, int idx)
{
    std::string_view found = stk[idx - 1];

    if(get_operator_idx(found) != -1)
    {
        expression_result_with_width<T> me;
        me.op = found;
        idx--;

        auto [right_exp_opt, idx1] = resolve_expression<T>(sym, stk, should_delay, idx);
        auto [left_exp_opt, idx2] = resolve_expression<T>(sym, stk, should_delay, idx1);

        idx = idx2;

        if(!left_exp_opt.has_value())
            return {std::nullopt, idx};

        if(!right_exp_opt.has_value())
            return {std::nullopt, idx};

        const expression_result_with_width<T>& left_exp = left_exp_opt.value();
        const expression_result_with_width<T>& right_exp = right_exp_opt.value();

        ///can't ever have two registers in an expression, even two of the same register
        if(!left_exp.fully_resolved() && !right_exp.fully_resolved())
            return {std::nullopt, idx};

        if(left_exp.fully_resolved() &&
           right_exp.fully_resolved())
        {
            me.word = exec_op(left_exp.word.value(), right_exp.word.value(), me.op.value());

            return {me, idx};
        }

        if(left_exp.which_register.has_value())
        {
            if(!right_exp.fully_resolved())
                return {std::nullopt, idx};

            if(left_exp.op.has_value())
            {
                if(left_exp.op.value() == "+" && me.op.value() == "+")
                {
                    me.word = exec_op(left_exp.word.value(), right_exp.word.value(), "+");
                    me.which_register = left_exp.which_register;

                    return {me, idx};
                }

                if(left_exp.op.value() == "+" && me.op.value() == "-")
                {
                    me.word = exec_op(left_exp.word.value(), right_exp.word.value(), "-");
                    me.which_register = left_exp.which_register;
                    me.op = "+";

                    return {me, idx};
                }

                return {std::nullopt, idx};
            }
            else if(me.op.value() == "+")
            {
                me.word = right_exp.word;
                me.which_register = left_exp.which_register;

                return {me, idx};
            }
            else if(me.op.value() == "-")
            {
                me.word = -right_exp.word.value();
                me.which_register = left_exp.which_register;
                me.op = "+";

                return {me, idx};
            }
            else
            {
                return {std::nullopt, idx};
            }
        }

        if(right_exp.which_register.has_value())
        {
            if(!left_exp.fully_resolved())
                return {std::nullopt, idx};

            if(right_exp.op.has_value())
            {
                if(right_exp.op.value() == "+" && me.op.value() == "+")
                {
                    me.word = exec_op(left_exp.word.value(), right_exp.word.value(), "+");
                    me.which_register = right_exp.which_register;

                    return {me, idx};
                }

                return {std::nullopt, idx};
            }
            else if(me.op.value() == "+")
            {
                me.word = left_exp.word;
                me.which_register = right_exp.which_register;

                return {me, idx};
            }
            else
            {
                return {std::nullopt, idx};
            }
        }

        return {std::nullopt, idx};
    }
    else
    {
        expression_result_with_width<T> res;

        std::string_view elem = stk[idx - 1];
        idx--;

        if(is_constant(elem))
        {
            res.word = get_constant_of<T>(elem);
            return {res, idx};
        }
        else
        {
            if(get_register_assembly_value_from_name(elem).has_value())
            {
                res.which_register = elem;

                return {res, idx};
            }

            if(iequal(elem, "sp"))
            {
                ///the sp register
                res.which_register = elem;
                return {res, idx};
            }

            auto val_opt = sym.get_symbol_definition(elem);

            if(val_opt.has_value())
            {
                res.word = val_opt.value();
                return {res, idx};
            }

            if(is_label_reference(elem))
                should_delay = true;

            return {std::nullopt, idx};
        }
    }
}

///shunting yard
constexpr
std::optional<expression_result> parse_expression(const symbol_table& sym, std::string_view expr, bool& should_delay)
{
    std::array precedence
    {
        4, 4, 3, 10, 9, 8, 3, 3, 2
    };

    std::array left_associative
    {
        1, 1, 1, 1, 1, 1, 1, 1, 0
    };

    static_assert(precedence.size() == left_associative.size());

    std::vector<std::string_view> operator_stack;

    auto get_precedence = [&](std::string_view in)
    {
        return precedence[get_operator_idx(in)];
    };

    auto is_left_associative = [&](std::string_view in)
    {
        return left_associative[get_operator_idx(in)];
    };

    std::vector<std::string_view> output_queue;

    while(expr.size() > 0)
    {
        auto consumed_opt = consume_expression_token(expr);

        if(!consumed_opt.has_value())
            return std::nullopt;

        std::string_view consumed = consumed_opt.value();

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

        if(is_constant(consumed) || is_label_reference(consumed))
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

                output_queue.push_back(operator_stack.back());

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

    /*for(auto i : output_queue)
    {
        std::cout << i << std::endl;
    }*/

    if(output_queue.size() == 0)
        return std::nullopt;

    auto [found, fin_idx] = resolve_expression<uint64_t>(sym, output_queue, should_delay, output_queue.size());

    if(fin_idx > 0)
        return std::nullopt;

    return found;
}

constexpr
uint16_t decode_pack_constant(uint16_t val, arg_pos::type apos, std::optional<int32_t>& out, assembler_settings& sett)
{
    if(sett.no_packed_constants)
    {
        out = std::optional<int32_t>{val};
        return 0x1f;
    }

    if(apos == arg_pos::A)
    {
        if(val == 0xFFFF)
            return 0x20;

        if(val >= 0 && val <= 30)
            return 0x21 + val;
        else
        {
            out = std::optional<int32_t>{val};
            return 0x1f;
        }
    }
    else
    {
        out = std::optional<int32_t>{val};
        return 0x1f;
    }
}

struct decode_result
{
    uint32_t val = 0;
    std::optional<int32_t> extra_word;
    std::optional<std::string_view> label;
    std::optional<std::string_view> expression;
    bool is_address = false;
};

// so
// create a table of MAX_WHATEVER long which contains byte -> label mapping
// then figure out a way to sub label pc value back in to instructions
// could insert all label references into an array of word values, and then insert all label definitions into an array of pc values
// then sub them in afterwards
inline
constexpr std::optional<decode_result> decode_value(std::string_view in, arg_pos::type apos, symbol_table& sym, assembler_settings& sett)
{
    decode_result res;

    auto set_val = [&](uint32_t val)
    {
        res.val = val;

        return res;
    };

    {
        auto reg_opt = get_register_assembly_value_from_name(in);

        ///cannot return reg_opt, because otherwise it isn't constexpr due to optional lacking constexpr move
        if(reg_opt.has_value())
            return set_val(reg_opt.value());
    }

    if(is_address(in))
    {
        res.is_address = true;

        auto extracted = extract_address_contents(in);

        auto extracted_reg_opt = get_register_assembly_value_from_name(extracted);

        if(extracted_reg_opt.has_value())
        {
            return set_val(0x08 + extracted_reg_opt.value());
        }

        if(iequal(extracted, "--sp") || iequal(extracted, "sp++"))
            return set_val(0x18);

        if(iequal(extracted, "sp"))
            return set_val(0x19);

        bool should_delay = false;
        auto expression_opt = parse_expression(sym, extracted, should_delay);

        if(expression_opt.has_value())
        {
            expression_result& eres = expression_opt.value();

            if(eres.fully_resolved())
            {
                res.extra_word = eres.word.value();
                return set_val(0x1e);
            }

            if(eres.which_register.has_value())
            {
                if(eres.op.has_value() && eres.op.value() != "+")
                    return std::nullopt;

                std::string_view reg = eres.which_register.value();

                if(eres.word.has_value() && eres.word.value() != 0)
                {
                    if(iequal(reg, "sp"))
                    {
                        res.extra_word = eres.word.value();
                        return set_val(0x1a);
                    }

                    auto reg_val_opt = get_register_assembly_value_from_name(reg);

                    if(reg_val_opt.has_value())
                    {
                        res.extra_word = eres.word.value();
                        return set_val(0x10 + reg_val_opt.value());
                    }
                }
                else
                {
                    if(iequal(reg, "sp"))
                        return set_val(0x19);

                    auto reg_val_opt = get_register_assembly_value_from_name(reg);

                    if(reg_val_opt.has_value())
                    {
                        return set_val(0x08 + reg_val_opt.value());
                    }
                }
            }
        }

        if(should_delay)
        {
            res.extra_word = 0;
            res.expression = extracted;
            return set_val(0x10); // placeholder
        }
    }

    if(iequal(in, "push") || iequal(in, "pop"))
        return set_val(0x18);

    if(iequal(in, "peek"))
        return set_val(0x19);

    if(iequal(in, "sp"))
        return set_val(0x1b);

    if(iequal(in, "pc"))
        return set_val(0x1c);

    if(iequal(in, "ex"))
        return set_val(0x1d);

    if(is_constant(in))
    {
        auto val = get_constant_of<uint16_t>(in);

        return set_val(decode_pack_constant(val, apos, res.extra_word, sett));
    }

    bool should_delay = false;
    auto expression_opt = parse_expression(sym, in, should_delay);

    if(expression_opt.has_value())
    {
        expression_result& eres = expression_opt.value();

        if(eres.word.has_value() && eres.which_register.has_value())
            return std::nullopt;

        if(eres.word.has_value())
            return set_val(decode_pack_constant(eres.word.value(), apos, res.extra_word, sett));

        if(eres.which_register.has_value())
        {
            auto decoded = get_register_immediate_encoding(eres.which_register.value());

            if(decoded.has_value())
                return set_val(decoded.value());
        }
    }

    if(should_delay)
    {
        res.extra_word = 0;
        res.expression = in;
        return set_val(0x1f); // next word (placeholder)
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
std::optional<error_info> add_opcode_with_prefix(symbol_table& sym, std::string_view& in, stack_vector<uint16_t, MEM_SIZE>& out, size_t& token_text_offset_start, size_t token_start, const stack_vector<uint16_t, MEM_SIZE>& source_to_line, assembler_settings& sett)
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

    auto consumed_name = consume_next(in, true);

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
        auto label_name = consume_next(in, true);

        if(peek_next(in, true) == ",")
            consume_next(in, true);

        auto label_value = consume_next(in, true);

        if(!is_constant(label_value))
        {
            err.msg = ".def value must be a constant";
            return err;
        }

        uint16_t val = get_constant_of<uint16_t>(label_value);

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
            auto value = consume_next(in, true);

            if(is_constant(value))
            {
                out.push_back(get_constant_of<uint16_t>(value));
            }
            else if(is_label_reference(value))
            {
                auto sym_opt = sym.get_symbol_definition(value);

                if(sym_opt.has_value())
                {
                    out.push_back(sym_opt.value());
                }
                else
                {
                    label use;
                    use.name = value;
                    use.offset = out.size();

                    sym.usages.push_back(use);

                    out.push_back(0);
                }
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
            else if(value == "?")
            {
                out.push_back(0);
            }
            else
            {
                err.msg = "Bad .dat, must be constant or label or definition or string";
                return err;
            }

            if(peek_next(in, true) == ",")
            {
                consume_next(in, true);

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
                auto val_b = consume_next(in, false);

                if(consume_next(in, true) != ",")
                {
                    err.msg = "Expected ,";
                    return err;
                }

                auto val_a = consume_next(in, false);

                auto decoded_b_opt = decode_value(val_b, arg_pos::B, sym, sett);
                auto decoded_a_opt = decode_value(val_a, arg_pos::A, sym, sett);

                if(!decoded_b_opt.has_value())
                {
                    err.msg = "first argument failed to decode";
                    return err;
                }

                if(!decoded_a_opt.has_value())
                {
                    err.msg = "second argument failed to decode";
                    return err;
                }

                auto decoded_b = decoded_b_opt.value();
                auto decoded_a = decoded_a_opt.value();

                auto instr = construct_type_a(code, decoded_a.val, decoded_b.val);

                uint16_t instruction_word = out.size();

                out.push_back(instr);

                if(decoded_a.extra_word.has_value())
                {
                    uint32_t promote_a = decoded_a.extra_word.value();

                    if(promote_a >= 65536)
                    {
                        err.msg = "second argument >= UINT_MAX or < INT_MIN";
                        return err;
                    }

                    if(decoded_a.label.has_value())
                    {
                        label l;
                        l.name = decoded_a.label.value();
                        l.offset = out.size();

                        sym.usages.push_back(l);
                    }

                    if(decoded_a.expression.has_value())
                    {
                        delayed_expression delayed;
                        delayed.base_word = instruction_word;
                        delayed.extra_word = out.size();
                        delayed.expression = decoded_a.expression.value();
                        delayed.type = arg_pos::A;
                        delayed.is_memory_reference = decoded_a.is_address;

                        sym.expressions.push_back(delayed);
                    }

                    out.push_back(promote_a);
                }

                if(decoded_b.extra_word.has_value())
                {
                    uint32_t promote_b = decoded_b.extra_word.value();

                    if(promote_b >= 65536)
                    {
                        err.msg = "first argument >= UINT_MAX or < INT_MIN";
                        return err;
                    }

                    if(decoded_b.label.has_value())
                    {
                        label l;
                        l.name = decoded_b.label.value();
                        l.offset = out.size();

                        sym.usages.push_back(l);
                    }

                    if(decoded_b.expression.has_value())
                    {
                        delayed_expression delayed;
                        delayed.base_word = instruction_word;
                        delayed.extra_word = out.size();
                        delayed.expression = decoded_b.expression.value();
                        delayed.type = arg_pos::B;
                        delayed.is_memory_reference = decoded_b.is_address;

                        sym.expressions.push_back(delayed);
                    }

                    out.push_back(promote_b);
                }

                return std::nullopt;
            }

            if(cls == 1)
            {
                auto val_a = consume_next(in, false);

                auto decoded_a_opt = decode_value(val_a, arg_pos::A, sym, sett);

                if(!decoded_a_opt.has_value())
                {
                    err.msg = "first argument failed to decode";
                    return err;
                }

                auto decoded_a = decoded_a_opt.value();

                auto instr = construct_type_b(code, decoded_a.val);

                uint16_t instruction_word = out.size();

                out.push_back(instr);

                if(decoded_a.extra_word.has_value())
                {
                    uint32_t promote_a = decoded_a.extra_word.value();

                    if(promote_a >= 65536)
                    {
                        err.msg = "first argument >= UINT_MAX or < INT_MIN";
                        return err;
                    }

                    if(decoded_a.label.has_value())
                    {
                        label l;
                        l.name = decoded_a.label.value();
                        l.offset = out.size();

                        sym.usages.push_back(l);
                    }

                    if(decoded_a.expression.has_value())
                    {
                        delayed_expression delayed;
                        delayed.base_word = instruction_word;
                        delayed.extra_word = out.size();
                        delayed.expression = decoded_a.expression.value();
                        delayed.type = arg_pos::A;
                        delayed.is_memory_reference = decoded_a.is_address;

                        sym.expressions.push_back(delayed);
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
std::pair<std::optional<return_info>, error_info> assemble(std::string_view text, assembler_settings sett = assembler_settings())
{
    return_info rinfo;
    symbol_table sym;
    sym.base_offset = sett.location;

    ///these are deliberately not affected by relocations
    for(auto [absolute_value, name] : sett.provided_symbol_definitions)
    {
        define d;
        d.name = name;
        d.value = absolute_value;

        sym.defines.push_back(d);
    }

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
    size_t last_line = 0;

    rinfo.source_line_to_pc.idx = line;

    while(text.size() > 0)
    {
        size_t offset = start_size - text.size();

        size_t token_offset = 0;

        auto error_opt = add_opcode_with_prefix(sym, text, rinfo.mem, token_offset, offset, source_to_line, sett);

        uint16_t source_character = offset + token_offset;

        for(size_t i = last_mem_size; i < rinfo.mem.size(); i++)
        {
            rinfo.translation_map.push_back(source_character);
            rinfo.pc_to_source_line.push_back(source_to_line[source_character]);
        }

        if(rinfo.pc_to_source_line.size() > 0)
        {
            for(size_t idx = last_line+1; idx <= rinfo.pc_to_source_line.back(); idx++)
            {
                rinfo.source_line_to_pc[idx] = last_mem_size;
            }
        }

        last_mem_size = rinfo.mem.size();

        if(rinfo.pc_to_source_line.size() > 0)
            last_line = rinfo.pc_to_source_line.back();

        if(error_opt.has_value())
        {
            return {std::nullopt, error_opt.value()};
        }
    }

    if(rinfo.pc_to_source_line.size() > 0)
    {
        size_t first_line = rinfo.pc_to_source_line[0];

        for(size_t idx = 0; idx <= first_line; idx++)
        {
            rinfo.source_line_to_pc[idx] = 0;
        }
    }

    //rinfo.source_line_to_pc[0] = 0;

    int last_val = rinfo.translation_map.size();

    if(last_val > 0)
    {
        int prev_val = last_val - 1;

        for(int i=rinfo.translation_map.size(); i < rinfo.translation_map.max_size; i++)
        {
            rinfo.translation_map[i] = rinfo.translation_map[prev_val];
        }
    }

    int last_pc_line = (int)rinfo.pc_to_source_line.size() - 1;

    if(last_pc_line >= 0)
    {
        for(int i=rinfo.pc_to_source_line.size(); i < rinfo.pc_to_source_line.max_size; i++)
        {
            rinfo.pc_to_source_line[i] = rinfo.pc_to_source_line[last_pc_line] + 1;
        }
    }

    /*for(int i=0; i < (int)rinfo.pc_to_source_line.size() - 1; i++)
    {
        int start_idx = rinfo.pc_to_source_line[i];
        int fin_idx = rinfo.pc_to_source_line[i + 1];

        for(int kk=start_idx; kk <= fin_idx; kk++)
        {
            rinfo.source_line_to_pc[kk] = i;
        }
    }*/

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
            err.line = rinfo.pc_to_source_line[j.offset];
            err.name_in_source = j.name;

            return {std::nullopt, err};
        }
    }

    for(delayed_expression& delayed : sym.expressions)
    {
        bool should_delay = false;
        auto value_opt = parse_expression(sym, delayed.expression, should_delay);

        error_info err;
        err.character = 0;
        err.line = rinfo.pc_to_source_line[delayed.base_word];
        err.name_in_source = delayed.expression;

        if(should_delay)
        {
            err.msg = "Expression contains undefined label";

            return {std::nullopt, err};
        }

        expression_result& res = value_opt.value();

        uint16_t& mem = rinfo.mem.svec[delayed.base_word];

        if(res.which_register.has_value())
        {
            if(!delayed.is_memory_reference && res.word.has_value())
            {
                err.msg = "Cannot execute reg + constant unless its in a memory reference";
                return {std::nullopt, err};
            }

            uint16_t extra_value = res.word.has_value() ? res.word.value() : 0;

            if(res.op.has_value() && res.op.value() != "+")
            {
                err.msg = "Expressions must boil down to [reg + constant], or solely reg or solely constant otherwise";

                return {std::nullopt, err};
            }

            uint16_t offset = 0;

            if(delayed.is_memory_reference)
            {
                auto reg_opt = get_register_assembly_value_from_name(res.which_register.value());

                if(reg_opt.has_value())
                {
                    offset = 0x10 + reg_opt.value();
                }
                else if(iequal(res.which_register.value(), "sp"))
                {
                    offset = 0x1a;
                }
                else
                {
                    err.msg = "Expression tried to use something which was not a register or a label";
                    return {std::nullopt, err};
                }
            }
            else
            {
                auto reg_opt = get_register_immediate_encoding(res.which_register.value());

                if(reg_opt.has_value())
                {
                    offset = reg_opt.value();
                }
                else
                {
                    err.msg = "Expression involving a register failed to decode, invalid register?";
                    return {std::nullopt, err};
                }
            }

            rinfo.mem.svec[delayed.extra_word] = extra_value;

            if(delayed.type == arg_pos::A)
            {
                mem = mem & 0b0000001111111111;
                mem = mem | (offset << 10);
            }

            if(delayed.type == arg_pos::B)
            {
                mem = mem & 0b1111110000011111;
                mem = mem | (offset << 5);
            }
        }
        else
        {
            uint16_t extra_value = res.word.has_value() ? res.word.value() : 0;

            if(res.word.has_value())
            {
                uint16_t offset = 0;

                if(delayed.is_memory_reference)
                {
                    offset = 0x1e;
                }
                else
                {
                    if(res.which_register.has_value())
                    {
                        err.msg = "Cannot have both a register and a constant as a literal";
                        return {std::nullopt, err};
                    }

                    offset = 0x1f;
                }

                rinfo.mem.svec[delayed.extra_word] = extra_value;

                if(delayed.type == arg_pos::A)
                {
                    mem = mem & 0b0000001111111111;
                    mem = mem | (offset << 10);
                }

                if(delayed.type == arg_pos::B)
                {
                    mem = mem & 0b1111110000011111;
                    mem = mem | (offset << 5);
                }
            }
            else
            {
                err.msg = "Something went wrong in delayed expression evaluation";
                return {std::nullopt, err};
            }
        }
    }

    ///relocate
    ///doing it down here because in the future, will need to be able to relocate eg the translation map
    if(sett.location != 0)
    {
        rinfo.mem.shift_contents_right(sett.location);
        rinfo.translation_map.shift_contents_right(sett.location);
        rinfo.pc_to_source_line.shift_contents_right(sett.location);

        for(uint16_t& val : rinfo.source_line_to_pc)
        {
            val += sett.location;
        }
    }

    for(std::string_view l : sett.label_values_to_extract)
    {
        auto val_opt = sym.get_symbol_definition(l);

        if(val_opt.has_value())
        {
            rinfo.requested_label_names.push_back({val_opt.value(), std::string(l)});
        }
    }

    return {rinfo, error_info()};
}

#endif // BASE_ASM_HPP_INCLUDED
