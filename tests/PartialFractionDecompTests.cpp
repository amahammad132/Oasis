//
// Created by Amaan Mahammad on 2/19/25.
//
#include "catch2/catch_test_macros.hpp"

#include "Oasis/Add.hpp"
#include "Oasis/Divide.hpp"
#include "Oasis/Exponent.hpp"
#include "Oasis/Expression.hpp"
#include "Oasis/Imaginary.hpp"
#include "Oasis/Multiply.hpp"
#include "Oasis/Real.hpp"
#include "Oasis/RecursiveCast.hpp"
#include "Oasis/Subtract.hpp"
#include "Oasis/Variable.hpp"

#include <cmath>
#include <set>
#include <tuple>
#include <vector>

inline Oasis::SimplifyVisitor simplifyVisitor{};

// TEST_CASE("Two-Term Partial Fraction Decomposition", "[Divide][Polynomial]")
// {
//
//     // (3x+11)/(x^2-x-6)
//     Oasis::Divide div {
//         Oasis::Add {
//             Oasis::Real { 11 },
//             Oasis::Multiply {
//                 Oasis::Variable { "x" },
//                 Oasis::Real { 3 } }},
//         Oasis::Subtract {
//             Oasis::Exponent {
//                 Oasis::Variable { "x" },
//                 Oasis::Real { 2 } },
//             Oasis::Add { Oasis::Variable{ "x" }, Oasis::Real { 6 } },
//         }
//     };
//
//     auto decomposed = div.PartialFractionDecomp();
//
//     REQUIRE(Oasis::Add {
//         Oasis::Divide { Oasis::Real{ 4 }, Oasis::Add{ Oasis::Variable { "x" }, Oasis::Real{-3}} },
//         Oasis::Divide { Oasis::Real{ -1 }, Oasis::Add{ Oasis::Variable { "x" }, Oasis::Real{2}} }
//     }.Equals(*decomposed));
// }