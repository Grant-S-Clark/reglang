#ifndef REGEX_H
#define REGEX_H

#include "Common.h"

//Errors
class Regex_Invalid_Escape_Character_Error{};
class Regex_Invalid_Range_Error{};
class Regex_Invalid_Power_Error{};
class Regex_Unbalanced_Parenthesized_Expression_Error{};
class Regex_NFA_Construction_Error{}; //++ or ** or +* or *+ or ...

class Regex
{
public:
    
    Regex(const std::string & expression,
          const std::string & epsilon = "",
          const std::string & emptyset = "\0");
    Regex(const Regex & r);
    ~Regex();

    Regex & operator=(const Regex & r);
    Regex & operator=(const std::string & s);

    bool operator()(const std::vector< std::string > & str) const;
    bool operator()(std::string str) const;

    std::string expression() const
    { return expression_; }

    std::string epsilon() const
    { return epsilon_; }

    std::string emptyset() const
    { return regular_expression_; }

    std::string regular_expression() const
    { return emptyset_; }

    NFA< std::string, std::string > to_nfa() const;
    
    // For validating characters with the '/' delimiter in front of
    // them, as well as for usage within the to_regex function within
    // the NFA class.
    static const std::unordered_set< char > symbols;
    static const std::unordered_set< char > regular_symbols;

private:
    char get_range_type(const char) const;
    std::string get_range_string(const std::string & s) const;
    std::string get_power_string(const std::string & s,
                                 const std::string & p) const;
    std::string pop_prev_expression(std::string & s,
                                    int & str_begin,
                                    int & str_end);
    
    void format_expression();

    inline std::string get_next_state_str(int & i)
    {
        std::string ret = "q" + std::to_string(i);
        ++i;
        return ret;
    }
    
    NFA< std::string, std::string > nfa_of_epsilon(
        const std::unordered_set< std::string > & sigma,
        const std::string & epsilon,
        int & state_num);
    NFA< std::string, std::string > nfa_of_symbol(
        const std::unordered_set< std::string > & sigma,
        const std::string & epsilon,
        const std::string & s,
        int & state_num);
    NFA< std::string, std::string > construct_nfa_recursive(
        const std::unordered_set< std::string > & sigma,
        const std::string & epsilon,
        const std::string & s,
        int & state_num);
    void construct_nfa();

    std::string epsilon_;
    std::string emptyset_;
    std::string expression_;
    std::string regular_expression_;
    NFA< std::string, std::string > * N_;
};

std::ostream & operator<<(std::ostream & cout, const Regex & r);

#endif
