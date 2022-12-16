#ifndef COMMON_H
#define COMMON_H

//Include all commonly used headers
#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <cctype>
#include <utility>

#include "STL_Helper.h"

/*
  This file is used for forward declarations
  within the regular language classes.
*/

//DFA
template< typename S_t, typename Q_t >
class DFA;

class DFA_Invalid_Sigma_Character_Error{};
class DFA_To_NFA_Invalid_Epsilon_Error{};


//NFA
template< typename S_t, typename Q_t >
class NFA;

class NFA_Epsilon_Not_In_Sigma_Error{};
class NFA_Invalid_Sigma_Character_Error{};
class NFA_Invalid_State_Error{};
class NFA_Invalid_Kleene_Star_Initial_State_Error{};


//Regex
class Regex;

#endif
