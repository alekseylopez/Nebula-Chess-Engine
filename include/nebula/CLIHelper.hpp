#ifndef NEBULA_CLIHELPER_HPP
#define NEBULA_CLIHELPER_HPP

namespace nebula
{

enum class ReturnCode { Good, Help, Error };

enum class InputMode { PlayerInput, Auto };

// modifies references only if valid input, in which case ReturnCode is Good
ReturnCode opts(int argc, char* argv[], InputMode& mode, int& depth, int& length);

}

#endif