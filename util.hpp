#ifndef COMMON_HPP_INCLUDED
#define COMMON_HPP_INCLUDED

#include <string_view>
#include <cctype>

template<typename T>
inline
constexpr std::string_view trim_start(std::string_view view, const T& matches)
{
    if(view.size() == 0)
        return view;

    int i=0;

    for(; i < (int)view.size() && matches(view[i]); i++)
    {

    }

    view.remove_prefix(i);

    return view;
}

template<typename T>
inline
constexpr std::string_view trim_end(std::string_view view, const T& matches)
{
    if(view.size() == 0)
        return view;

    int fin = (int)view.size();

    for(; (fin - 1) >= 0 && matches(view[fin - 1]); fin--)
    {

    }

    view.remove_suffix((int)view.size() - fin);

    return view;
}

inline
constexpr std::string_view consume_next(std::string_view& in)
{
    if(in.size() == 0)
        return in;

    int start_index = 0;
    std::string_view data = in;
    bool in_comment = false;

    for(; start_index < (int)data.size(); start_index++)
    {
        char cchar = data[start_index];

        if(!in_comment)
        {
            if(cchar == ' ' || cchar == '\n' || cchar == ',')
                continue;
        }
        else
        {
            if(cchar == '\n')
            {
                in_comment = false;
            }

            continue;
        }

        if(cchar == ';')
        {
            in_comment = true;
            continue;
        }

        break;
    }

    if(start_index == (int)data.size())
    {
        data.remove_prefix(start_index);
        in = data;
        return data;
    }

    data.remove_prefix(start_index);

    int word_end = 0;

    for(; word_end < (int)data.size(); word_end++)
    {
        char cchar = data[word_end];

        if(cchar == ' ' || cchar == '\n' || cchar == ',' || cchar == ';')
            break;
    }

    auto suffix = data;

    data.remove_suffix(data.size() - word_end);
    suffix.remove_prefix(word_end);

    in = suffix;

    return data;
}

inline
constexpr char ascii_to_lower(char c)
{
    if(c >= 'A' && c <= 'Z')
    {
        return (c - 'A') + 'a';
    }

    return c;
}

/*inline
std::string to_lower(std::string_view in)
{
    std::string ret;
    ret.resize(in.size());

    for(int i=0; i < (int)in.size(); i++)
    {
        ret[i] = ascii_to_lower(in[i]);
    }

    return ret;
}*/

inline
constexpr bool iequal(std::string_view in1, std::string_view in2)
{
    if(in1.size() != in2.size())
        return false;

    for(size_t i = 0; i < in1.size(); i++)
    {
        if(ascii_to_lower(in1[i]) != ascii_to_lower(in2[i]))
            return false;
    }

    return true;
}

inline
constexpr bool is_label_definition(std::string_view in)
{
    return in.starts_with(':') || in.ends_with(':');
}

inline
constexpr bool is_hex_digit(char in)
{
    return
    (in >= '0' && in <= '9') ||
    in == 'a' || in == 'A' ||
    in == 'b' || in == 'B' ||
    in == 'c' || in == 'C' ||
    in == 'd' || in == 'D' ||
    in == 'e' || in == 'E' ||
    in == 'f' || in == 'F';
}

inline
constexpr bool is_digit(char in)
{
    return in >= '0' && in <= '9';
}

inline
constexpr bool is_constant(std::string_view in)
{
    if(in.size() == 0)
        return false;

    if(in.starts_with('-'))
    {
        in.remove_prefix(1);
    }

    if(in.starts_with("0x"))
    {
        for(int i=2; i < (int)in.size(); i++)
        {
            if(!is_hex_digit(in[i]))
                return false;
        }

        return true;
    }

    for(int i=0; i < (int)in.size(); i++)
    {
        if(!is_digit(in[i]))
            return false;
    }

    return true;
}

inline
constexpr int get_digit(char in)
{
    if(!is_digit(in))
        return 0;

    return in - '0';
}

inline
constexpr int get_hex_digit(char in)
{
    if(!is_hex_digit(in))
        return 0;

    if(in >= '0' && in <= '9')
        return in - '0';

    if(in >= 'a' && in <= 'f')
        return 10 + (in - 'a');

    if(in >= 'A' && in <= 'F')
        return 10 + (in - 'A');

    return 0;
}

inline
constexpr uint32_t positive_stoi_cxper(std::string_view in, int radix)
{
    if(in.size() == 0)
        return 0;

    uint32_t value = 0;
    uint32_t base = 1;

    for(int i=(int)in.size() - 1; i >= 0; i--)
    {
        if(radix == 10)
        {
            if(!is_digit(in[i]))
            {
                return value;
            }

            value += base * get_digit(in[i]);
        }

        if(radix == 16)
        {
            if(!is_hex_digit(in[i]))
            {
                return value;
            }

            value += base * get_hex_digit(in[i]);
        }

        base *= radix;
    }

    return value;
}

inline
constexpr uint16_t get_constant(std::string_view in)
{
    if(in.size() == 0)
        return 0;

    bool is_neg = false;

    if(in.starts_with('-'))
    {
        is_neg = true;
        in.remove_prefix(1);
    }

    uint16_t n = 0;

    if(in.starts_with("0x"))
    {
        n = positive_stoi_cxper(in, 16);
    }
    else
    {
        n = positive_stoi_cxper(in, 10);
    }

    if(is_neg)
        return -(int16_t)n;
    else
        return n;
}

inline
constexpr bool isalnum_c(char in)
{
    if(in >= 'a' && in <= 'z')
        return true;

    if(in >= 'A' && in <= 'Z')
        return true;

    if(in >= '0' && in <= '9')
        return true;

    if(in == '_')
        return true;

    return false;
}

inline
constexpr bool is_label_reference(std::string_view in)
{
    for(auto& i : in)
    {
        if(!isalnum_c(i))
            return false;
    }

    return true;
}

inline
constexpr bool is_address(std::string_view in)
{
    return in.starts_with("[") && in.ends_with("]");
}

inline
constexpr std::string_view extract_address_contents(std::string_view in)
{
    if(!is_address(in))
        return "";

    in.remove_prefix(1);
    in.remove_suffix(1);

    return in;
}

#endif // COMMON_HPP_INCLUDED
