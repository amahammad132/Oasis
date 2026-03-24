//
// Created by Levy Lin on 2/09/2024.
//

#include "Oasis/Integral.hpp"

#include "Oasis/Add.hpp"
// #include "Oasis/Log.hpp"
// #include "Oasis/Imaginary.hpp"

namespace Oasis {

Integral<Expression>::Integral(const Expression& integrand, const Expression& differential)
    : BinaryExpression(integrand, differential)
{
}

} // Oasis