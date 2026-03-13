//
// Created by Matthew McCall on 2/19/25.
//

#include "Oasis/Exponent.hpp"

#include <Oasis/Add.hpp>
#include <Oasis/Divide.hpp>
#include <Oasis/Real.hpp>
#include <Oasis/Subtract.hpp>
#include <Oasis/Variable.hpp>
#include <Oasis/SimplifyVisitor.hpp>
#include <Oasis/RecursiveCast.hpp>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <print>
#include <ranges>
#include <limits>
#include <numeric>

#include "Oasis/FromString.hpp"
#include "Oasis/InFixSerializer.hpp"
#include "Oasis/TeXSerializer.hpp"
#include "Oasis/Multiply.hpp"

#include <boost/callable_traits/return_type.hpp>
#include <fmt/color.h>
#include <isocline.h>
// #include <bits/std>
#include <Eigen/Dense>
#include <iostream>
#include <map>

template <typename FnT>
auto operator|(const std::string& str, FnT fn) -> boost::callable_traits::return_type_t<FnT>
// auto operator|(const std::string& str, FnT fn) -> std::invoke_result<FnT, std::string>
//
// auto operator|(const std::string& str, FnT fn) -> decltype(fn(std::declval<std::string>()))
{
    return fn(str);
}

// https://en.cppreference.com/w/cpp/string/byte/isspace#Notes
bool safe_isspace(const unsigned char ch) { return std::isspace(ch); }

auto trim_whitespace(const std::string& str) -> std::string
{
    return str
        | std::views::drop_while(safe_isspace)
        | std::views::reverse
        | std::views::drop_while(safe_isspace)
        | std::views::reverse
        | std::ranges::to<std::string>();
}

// Calling Oasis::FromInFix passed as template fails because defaulted parameters aren't represented in the type, so a wrapper is needed
auto Parse(const std::string& in) -> Oasis::FromInFixResult { return Oasis::FromInFix(in); };

// std::string my_fun(Oasis::InFixSerializer& ser, const std::unique_ptr<Oasis::Expression>& ex)
// {
//     auto expression_stringified = ex->Accept(ser);
//     if (expression_stringified.has_value()) return std::move(expression_stringified.value());
//     return "";
// }

/*
auto highest_order_coefficient(std::unique_ptr<Oasis::Expression> es) -> std::unique_ptr<Oasis::Expression> {
    using namespace Oasis;
    Oasis::InFixSerializer ser;

    std::string varName = "";
    std::vector<std::unique_ptr<Expression>> termsE;
    if (auto addCase = RecursiveCast<Add<Expression>>(*es); addCase != nullptr) {
        addCase->Flatten(termsE);
    } else {
        termsE.push_back(es->Copy());
    }
    std::vector<std::unique_ptr<Expression>> posCoefficents;
    std::vector<std::unique_ptr<Expression>> negCoefficents;

    long max_floored_exponent = 0;
    std::unique_ptr<Expression> corresponding_coeff;
    for (const auto& i : termsE) {
        std::unique_ptr<Expression> coefficent;
        std::string variableName;
        double exponent;
        if (auto variableCase = RecursiveCast<Variable>(*i); variableCase != nullptr) {
            coefficent = Real(1).Copy();
            variableName = variableCase->GetName();
            exponent = 1;
        } else if (auto expCase = RecursiveCast<Exponent<Variable, Real>>(*i); expCase != nullptr) {
            coefficent = Real(1).Copy();
            variableName = expCase->GetMostSigOp().GetName();
            exponent = expCase->GetLeastSigOp().GetValue();
        } else if (auto prodCase = RecursiveCast<Multiply<Expression, Variable>>(*i); prodCase != nullptr) {
            coefficent = prodCase->GetMostSigOp().Copy();
            variableName = prodCase->GetLeastSigOp().GetName();
            exponent = 1;
        } else if (auto prodExpCase = RecursiveCast<Multiply<Expression, Exponent<Variable, Real>>>(*i); prodExpCase != nullptr) {
            coefficent = prodExpCase->GetMostSigOp().Copy();
            variableName = prodExpCase->GetLeastSigOp().GetMostSigOp().GetName();
            exponent = prodExpCase->GetLeastSigOp().GetLeastSigOp().GetValue();
        } else if (auto divCase = RecursiveCast<Divide<Expression, Variable>>(*i); divCase != nullptr) {
            coefficent = divCase->GetMostSigOp().Copy();
            variableName = divCase->GetLeastSigOp().GetName();
            exponent = -1;
        } else if (auto divExpCase = RecursiveCast<Divide<Expression, Exponent<Variable, Real>>>(*i); divExpCase != nullptr) {
            coefficent = divExpCase->GetMostSigOp().Copy();
            variableName = divExpCase->GetLeastSigOp().GetMostSigOp().GetName();
            exponent = -divExpCase->GetLeastSigOp().GetLeastSigOp().GetValue();
        } else {
            coefficent = i->Copy();
            variableName = varName;
            exponent = 0;
        }
        if (varName.empty()) {
            varName = variableName;
        }
        if (exponent != std::round(exponent) || varName != variableName) {
            return {};
        }
        std::println("exponent: {}, coefficient: {}", exponent, coefficent->Accept(ser).value());
        long flooredExponent = std::lround(exponent);
        if (flooredExponent > max_floored_exponent) {
            max_floored_exponent = flooredExponent;
            corresponding_coeff = coefficent->Copy();
        }
        if (exponent >= 0) {
            while (posCoefficents.size() <= exponent) {
                posCoefficents.push_back(Real(0).Copy());
            }
            posCoefficents[flooredExponent] = Add<Expression>(*coefficent, *posCoefficents[flooredExponent]).Copy();
        } else {
            exponent *= -1;
            while (negCoefficents.size() <= exponent) {
                negCoefficents.push_back(Real(0).Copy());
            }
            negCoefficents[flooredExponent] = Add<Expression>(*coefficent, *negCoefficents[flooredExponent]).Copy();
        }
    }

    while (negCoefficents.size() > 0 && RecursiveCast<Real>(*negCoefficents.back()) != nullptr && RecursiveCast<Real>(*negCoefficents.back())->GetValue() == 0) {
        negCoefficents.pop_back();
    }
    while (posCoefficents.size() > 0 && RecursiveCast<Real>(*posCoefficents.back()) != nullptr && RecursiveCast<Real>(*posCoefficents.back())->GetValue() == 0) {
        posCoefficents.pop_back();
    }
    std::vector<std::unique_ptr<Expression>> coefficents;
    for (size_t i = negCoefficents.size(); i > 1; i--) {
        coefficents.push_back(negCoefficents[i - 1]->Simplify());
    }
    for (const std::unique_ptr<Expression>& i : posCoefficents) {
        coefficents.push_back(i->Simplify());
    }


    std::println("{} coefficients.", coefficents.size());
    for (int i = 0; i < coefficents.size(); i++) {
        std::println("{}*x^{}", coefficents[i]->Accept(ser).value(), i);
    }

    return corresponding_coeff;
} */

auto all_coeffs(std::unique_ptr<Oasis::Expression> es) -> std::vector<std::unique_ptr<Oasis::Expression>> {
    using namespace Oasis;
    Oasis::InFixSerializer ser;

    std::string varName = "";
    std::vector<std::unique_ptr<Expression>> termsE;
    if (auto addCase = RecursiveCast<Add<Expression>>(*es); addCase != nullptr) {
        addCase->Flatten(termsE);
    } else {
        termsE.push_back(es->Copy());
    }
    std::vector<std::unique_ptr<Expression>> posCoefficents;
    std::vector<std::unique_ptr<Expression>> negCoefficents;

    long max_floored_exponent = 0;
    std::unique_ptr<Expression> corresponding_coeff;
    for (const auto& i : termsE) {
        std::unique_ptr<Expression> coefficent;
        std::string variableName;
        double exponent;
        if (auto variableCase = RecursiveCast<Variable>(*i); variableCase != nullptr) {
            coefficent = Real(1).Copy();
            variableName = variableCase->GetName();
            exponent = 1;
        } else if (auto expCase = RecursiveCast<Exponent<Variable, Real>>(*i); expCase != nullptr) {
            coefficent = Real(1).Copy();
            variableName = expCase->GetMostSigOp().GetName();
            exponent = expCase->GetLeastSigOp().GetValue();
        } else if (auto prodCase = RecursiveCast<Multiply<Expression, Variable>>(*i); prodCase != nullptr) {
            coefficent = prodCase->GetMostSigOp().Copy();
            variableName = prodCase->GetLeastSigOp().GetName();
            exponent = 1;
        } else if (auto prodExpCase = RecursiveCast<Multiply<Expression, Exponent<Variable, Real>>>(*i); prodExpCase != nullptr) {
            coefficent = prodExpCase->GetMostSigOp().Copy();
            variableName = prodExpCase->GetLeastSigOp().GetMostSigOp().GetName();
            exponent = prodExpCase->GetLeastSigOp().GetLeastSigOp().GetValue();
        } else if (auto divCase = RecursiveCast<Divide<Expression, Variable>>(*i); divCase != nullptr) {
            coefficent = divCase->GetMostSigOp().Copy();
            variableName = divCase->GetLeastSigOp().GetName();
            exponent = -1;
        } else if (auto divExpCase = RecursiveCast<Divide<Expression, Exponent<Variable, Real>>>(*i); divExpCase != nullptr) {
            coefficent = divExpCase->GetMostSigOp().Copy();
            variableName = divExpCase->GetLeastSigOp().GetMostSigOp().GetName();
            exponent = -divExpCase->GetLeastSigOp().GetLeastSigOp().GetValue();
        } else {
            coefficent = i->Copy();
            variableName = varName;
            exponent = 0;
        }
        if (varName.empty()) {
            varName = variableName;
        }
        if (exponent != std::round(exponent) || varName != variableName) {
            return {};
        }
        std::println("exponent: {}, coefficient: {}", exponent, coefficent->Accept(ser).value());
        long flooredExponent = std::lround(exponent);
        if (flooredExponent > max_floored_exponent) {
            max_floored_exponent = flooredExponent;
            corresponding_coeff = coefficent->Copy();
        }
        if (exponent >= 0) {
            while (posCoefficents.size() <= exponent) {
                posCoefficents.push_back(Real(0).Copy());
            }
            posCoefficents[flooredExponent] = Add<Expression>(*coefficent, *posCoefficents[flooredExponent]).Copy();
        } else {
            exponent *= -1;
            while (negCoefficents.size() <= exponent) {
                negCoefficents.push_back(Real(0).Copy());
            }
            negCoefficents[flooredExponent] = Add<Expression>(*coefficent, *negCoefficents[flooredExponent]).Copy();
        }
    }

    while (negCoefficents.size() > 0 && RecursiveCast<Real>(*negCoefficents.back()) != nullptr && RecursiveCast<Real>(*negCoefficents.back())->GetValue() == 0) {
        negCoefficents.pop_back();
    }
    while (posCoefficents.size() > 0 && RecursiveCast<Real>(*posCoefficents.back()) != nullptr && RecursiveCast<Real>(*posCoefficents.back())->GetValue() == 0) {
        posCoefficents.pop_back();
    }
    std::vector<std::unique_ptr<Expression>> coefficents;
    for (size_t i = negCoefficents.size(); i > 1; i--) {
        coefficents.push_back(negCoefficents[i - 1]->Simplify());
    }
    for (const std::unique_ptr<Expression>& i : posCoefficents) {
        coefficents.push_back(i->Simplify());
    }

    std::reverse(coefficents.begin(), coefficents.end());
    return coefficents;


    // std::println("{} coefficients.", coefficents.size());
    // for (int i = 0; i < coefficents.size(); i++) {
    //     std::println("{}*x^{}", coefficents[i]->Accept(ser).value(), i);
    // }
    //
    // return corresponding_coeff;
}

// long long gcf1(long long a, long long b)
// {
//     if (b > a) {
//         std::swap(a, b);
//     }
//     while (1) {
//         a %= b;
//         if (a == 0) {
//             return b;
//         }
//         b %= a;
//         if (b == 0) {
//             return a;
//         }
//     }
// }

/*
bool exactly_representable_double(intmax_t a, intmax_t b) {
    if (b == 0) return false;
    if (a == 0) return true;

    using F = double;
    static_assert(std::numeric_limits<F>::radix == 2,
                  "Requires binary floating point");

    // reduce fraction
    long long g = std::gcd(a, b);
    uint64_t num = std::llabs(a / g);
    uint64_t den = std::llabs(b / g);

    // denominator must be a power of two
    if ((den & (den - 1)) != 0) return false;

    // compute k where den = 2^k
    int k = 0;
    while ((den >> k) != 1) ++k;

    // find highest set bit of numerator
    int msb = -1;
    for (uint64_t t = num; t; t >>= 1) ++msb;

    int exponent = msb - k;

    // exponent bounds
    int max_exp = std::numeric_limits<F>::max_exponent - 1;
    int min_exp = std::numeric_limits<F>::min_exponent
                  - std::numeric_limits<F>::digits;

    if (exponent > max_exp) return false;
    if (exponent < min_exp) return false;

    // precision check (53-bit significand)
    int extra_bits = msb - (std::numeric_limits<F>::digits - 1);
    if (extra_bits > 0) {
        uint64_t mask = (1ULL << extra_bits) - 1;
        if (num & mask) return false;
    }

    return true;
} */


auto expand_polynomial3(const std::unique_ptr<Oasis::Expression>& polynom)
{
    using namespace Oasis;

    if (polynom->Is<Multiply>()) {
        if (auto addCase = RecursiveCast<Multiply<Expression>>(*polynom); addCase != nullptr) {
            std::vector<std::unique_ptr<Expression>> lhs, rhs;
            if (addCase->GetMostSigOp().Is<Add>()) {
                auto conv = RecursiveCast<Add<Expression>>(addCase->GetMostSigOp());
                conv->Flatten(lhs);
                // std::println("most sig op: {}", conv->Accept(serializer).value());
            } else {
                lhs.emplace_back(addCase->GetMostSigOp().Copy());
            }
            if (addCase->GetLeastSigOp().Is<Add>()) {
                auto conv = RecursiveCast<Add<Expression>>(addCase->GetLeastSigOp());
                conv->Flatten(rhs);
                // std::println("least sig op: {}", conv->Accept(serializer).value());
            } else {
                rhs.emplace_back(addCase->GetLeastSigOp().Copy());
            }


            std::println("lhs # of items added: {}, rhs # of items added: {}", lhs.size(), rhs.size());
            std::vector<std::unique_ptr<Expression>> result;
            for (const auto& lhs_exp : lhs) {
                for (const auto& rhs_exp : rhs) {
                    // result.emplace_back(Multiply {*lhs_exp, *rhs_exp}.Copy());
                    result.emplace_back(lhs_exp * rhs_exp);
                }
            }

            std::unique_ptr<Expression> combined = BuildFromVector<Add>(result);
            // std::println("expanded polynom: {}", combined->Accept(serializer).value());
            return combined;
        }
    }

    return polynom->Copy();
}
//
// auto pfd(std::unique_ptr<Oasis::Expression> numerator, std::unique_ptr<Oasis::Expression> denominator) -> std::unique_ptr<Oasis::Expression>
// {
//     using namespace Oasis;
//
//     Oasis::Variable x {"x"};
//
//     Oasis::InFixSerializer serializer;
//
//     auto opts = SimplifyOpts { .simplifyFractions = true };
//     auto opts_eval = SimplifyOpts { .simplifyFractions = false };
//     Oasis::SimplifyVisitor simplify_visitor { opts };
//     Oasis::SimplifyVisitor eval_visitor { opts_eval };
//
//
//
//     auto denominator_simplified = std::move(denominator->Accept(simplify_visitor)).value();
//     auto denominator_zeroes = denominator_simplified->FindZeros();
//
//
//     auto vis = denominator->Accept(simplify_visitor);
//     auto vis2 = std::move(vis).value().get();
//     auto vis3 = vis2->Accept(serializer);
//     if (vis3.has_value())
//         std::println("Expression has {} zeros: {}", denominator_zeroes.size(), vis3.value());
//
//     auto coefficient = highest_order_coefficient(expand_polynomial(denominator_simplified->Copy()));
//     std::println("highest order coefficient: {}", coefficient->Accept(serializer).value());
//
//     std::vector<std::unique_ptr<Oasis::Expression>> expressions;
//     std::vector<std::unique_ptr<Divide<Real>>> all_zeros;
//
//
//     auto actual_zeroes = std::vector<Oasis::Divide<Real>>();
//     bool flag = false;
//     for (auto& expr : denominator_zeroes) {
//         auto root = Oasis::RecursiveCast<Oasis::Divide<Oasis::Expression>>(*expr);
//         auto simplified_s = std::move(expr->Accept(simplify_visitor)).value();
//         Subtract sum1exp {x, *simplified_s};
//         auto sum1 = sum1exp.Copy();
//
//
//         expressions.emplace_back(sum1->Simplify());
//         auto res1 = simplified_s->Accept(serializer);
//
//         auto res2 = expr->Accept(serializer);
//         if (res1.has_value() && res2.has_value()) {
//             std::println("denominator zero: {} == {}", res2.value(), res1.value());
//         }
//
//         if (simplified_s->Is<Oasis::Real>()) {
//             auto expr_real_cast = dynamic_cast<Oasis::Real&>(*simplified_s);
//             all_zeros.emplace_back(std::make_unique<Divide<Real>>(expr_real_cast, Real{1}));
//         } else {
//             all_zeros.emplace_back(RecursiveCast<Divide<Real>>(*simplified_s));
//         }
//     }
//
//     auto expressions_pfd = std::vector<std::unique_ptr<Oasis::Expression>>();
//     expressions_pfd.reserve(expressions.size());
//     // create partial fraction expressions e.g. from x(x+2)(3x-2) -> A*x(x+2) + B*(3x-2)x + C*(3x-2)(x+2)
//
//     // start at 'A', increment for each unique item in partial fraction expressions
//     char var_name = 'A';
//     for (int i = 0; i < expressions.size(); i++, var_name++) {
//         // auto ex = Oasis::Variable{std::string{var_name}};
//         std::vector<std::unique_ptr<Oasis::Expression>> current_vec;
//         current_vec.emplace_back(std::make_unique<Variable>(std::string{var_name}));
//
//         for (int j = 0; j < expressions.size(); j++) {
//             if (j != i) {
//                 // expressions_pfd.emplace_back(expressions[j]->Copy());
//                 current_vec.emplace_back(expressions[j]->Copy());
//             }
//         }
//         expressions_pfd.emplace_back(Oasis::BuildFromVector<Oasis::Multiply>(current_vec));
//     }
//     auto expression_pfd = Oasis::BuildFromVector<Oasis::Add>(expressions_pfd);
//     std::println("expression_pfd: {}", expression_pfd->Accept(serializer).value());
//
//     // combine factors back together to yield original, factorized expression
//     auto expression = Oasis::BuildFromVector<Oasis::Multiply>(expressions);
//
//
//
//     auto expression_stringified = expression->Accept(serializer);
//     if (expression_stringified.has_value()) {}
//         std::println("Final Expression: {}", expression_stringified.value());
//
//
//     std::map<Variable, Divide<Real>> coefs;
// #define print_expr(expr) std::println(#expr ": {}", expr->Accept(serializer).value())
//
//
//
//
//     std::println("\n\n\n");
//
//
//     char var = 'A';
//     std::vector<std::unique_ptr<Expression>> final_pfd;
//     int i = 0;
//     for (auto& zero : all_zeros) {
//         print_expr(zero);
//         auto& v = expressions[i];
//         print_expr(v);
//
//         auto sub_lhs = numerator->Substitute(x, *zero);
//         auto sub_rhs = expression_pfd->Substitute(x, *zero);
//
//         std::println("lhs ==> {} = {} <== rhs", sub_lhs->Accept(serializer).value(), sub_rhs->Accept(serializer).value());
//         auto divd1a = Divide { *sub_rhs, Variable{std::string{ var }} };
//         auto divd1a_s = divd1a.Accept(simplify_visitor).value();
//         auto divda = Divide { *sub_lhs, *divd1a_s };
//         auto divda_s = divda.Accept(simplify_visitor).value();
//         auto divda_s2 = divda_s->Accept(simplify_visitor).value();
//
//         /*
//          * TODO: instead of evaluating the expression at this point (which may introduce error),
//          * use simplify visitor (after improving fraction simplification)
//          */
//         auto divda_s2_eval = divda_s2->Accept(eval_visitor).value();
//
//         std::println("x = {} => {} = {}", zero->Accept(serializer).value(), var, divda_s2->Accept(serializer).value());
//         final_pfd.emplace_back(std::make_unique<Divide<Expression>>(*divda_s2_eval, *v));
//
//         var++;
//         i++;
//     }
//
//     auto final_pfd_result = Oasis::BuildFromVector<Add>(final_pfd);
//     std::println("FINAL PFD RESULT: {}", final_pfd_result->Accept(serializer).value());
//     return final_pfd_result;
// }

using std::vector, std::unique_ptr, std::println;
using namespace Oasis;
typedef Divide<Real, Real> Fraction;
typedef unique_ptr<Expression> ExpressionPtr;

void print_pair(const std::pair<ExpressionPtr, ExpressionPtr>& pair)
{
    Oasis::InFixSerializer serializer;
    auto& it1 = pair.first;
    auto& it2 = pair.second;
    std::println("quotient: {}, remainder: {}", it1->Accept(serializer).value(), it2->Accept(serializer).value());
}

void print_pair(const std::pair<vector<ExpressionPtr>, vector<ExpressionPtr>>& pair)
{
    Oasis::InFixSerializer serializer;
    auto& it1 = pair.first;
    auto& it2 = pair.second;
    std::print("quotient: ");
    for (auto& itm : it1) {
        std::print("{} ", itm->Accept(serializer).value());
    }
    std::print(", remainder: ");
    for (auto& itm : it2) {
        std::print("{} ", itm->Accept(serializer).value());
    }
    std::println();
}

std::pair<vector<unique_ptr<Expression>>, vector<unique_ptr<Expression>>> synthetic_divide_list(vector<ExpressionPtr>& dividend, vector<ExpressionPtr>& divisor)
{
    // Fast polynomial division by using Extended Synthetic Division. Also works with non-monic polynomials.
    // dividend and divisor are both polynomials, which are here simply lists of coefficients. Eg: x^2 + 3x + 5 will be represented as [1, 3, 5]

    SimplifyVisitor simplify_visitor {};
    std::vector<ExpressionPtr> out {}; // Copy the dividend
    for (const auto& d : dividend) {
        out.emplace_back(d->Copy());
    }

    // std::println("out: {}", out);
    auto normalizer = divisor[0]->Copy();
    for (int i = 0; i < (dividend.size())-((divisor.size())-1); i++) {
        // Divide aval { Real { out[i]->GetMostSigOp().GetValue() * normalizer.GetLeastSigOp().GetValue() },  Real { out[i]->GetLeastSigOp().GetValue() * normalizer.GetMostSigOp().GetValue() } }; // for general polynomial division (when polynomials are non-monic),
        out[i] = out[i] / normalizer;
        // we need to normalize by dividing the coefficient with the divisor's first coefficient
        auto coef = out[i]->Copy();
        if (coef != Real{0}.Copy()) {
            // useless to multiply if coef is 0
            for (int j = 1; j < (divisor.size()); j++) {
                // in synthetic division, we always skip the first coefficient of the divisor,
                // because it is only used to normalize the dividend coefficients
                out[i + j] = out[i + j] - divisor[j] * coef;
            }
        }
    }

    // The resulting out contains both the quotient and the remainder, the remainder being the size of the divisor (the remainder
    // has necessarily the same degree as the divisor since it is what we couldn't divide from the dividend), so we compute the index
    // where this separation is, and return the quotient and remainder.

    std::vector<ExpressionPtr> quotient {};
    std::vector<ExpressionPtr> remainder {};

    int sep = out.size() - (divisor.size() - 1);
    for (int i = 0; i < sep; i++) {
        quotient.emplace_back(out[i]->Accept(simplify_visitor).value());
        // auto out_i_copy = out[i]->Copy();
        // auto tmp1 = out_i_copy->Accept(simplify_visitor).value();
        // quotient.emplace_back(tmp1.get());
        // std::println("adding a quotient.");
    }
    for (int i = sep; i < out.size(); ++i) {
        remainder.emplace_back(out[i]->Accept(simplify_visitor).value());
        // auto tmp1 = out[i]->Accept(simplify_visitor).value();
        // remainder.emplace_back(tmp1.get());
        // std::println("adding a remainder.");
    }
    std::println("done adding items: {}, {}", quotient.size(), remainder.size());

    // auto res3 = std::vector<std::vector<ExpressionPtr>>();
    // res3.emplace_back(std::move(quotient));
    // res3.emplace_back(std::move(remainder));
    // return std::move(res3);

    return std::make_pair<vector<ExpressionPtr>, vector<ExpressionPtr>>(std::move(quotient), std::move(remainder));
}

// ex. if given [2, 5, -3, 6], return 2x^3+5x^2-3x+6
auto coeffs_to_polynomial(const std::vector<ExpressionPtr>& coeffs) -> unique_ptr<Expression>
{
    // base case #1 (input: [], output: 0)
    if (coeffs.empty()) return Real{0}.Copy();

    // base case #2 (BuildFromVector needs minimum two terms) (input: [ a ], output: a)
    if (coeffs.size() == 1) return coeffs[0]->Copy();


    auto x = Oasis::Variable {"x"};
    std::vector<ExpressionPtr> terms;
    for (size_t i = 0; i < coeffs.size(); i++) {
        terms.emplace_back(coeffs[i] * (Exponent{x, Real{static_cast<double>(coeffs.size() - 1 - i)}}).Copy());
    }
    return Oasis::BuildFromVector<Add>(terms);
}

std::pair<unique_ptr<Expression>, unique_ptr<Expression>> synthetic_divide(ExpressionPtr& dividend, ExpressionPtr& divisor)
{
    SimplifyVisitor simplify_visitor {};
    InFixSerializer serializer;
    auto dividend_coeffs = all_coeffs(expand_polynomial3(dividend)->Accept(simplify_visitor).value());
    auto divisor_coeffs = all_coeffs(expand_polynomial3(divisor)->Accept(simplify_visitor).value());

    std::println("number of dividend coeffs: {}", dividend_coeffs.size());
    std::println("number of divisor coeffs: {}", divisor_coeffs.size());

    std::print("dividend coeffs: ");
    for (auto& coeff : dividend_coeffs) {
        std::print("{} ", coeff->Accept(serializer).value());
    }
    std::println();
    std::print("divisor coeffs: ");
    for (auto& coeff : divisor_coeffs) {
        std::print("{} ", coeff->Accept(serializer).value());
    }
    std::println();
    auto old_result = synthetic_divide_list(dividend_coeffs, divisor_coeffs);
    std::println("running print_pair(old_result)");
    print_pair(old_result);
    // std::println("old result.first size: {}, old result.second size: {}", old_result.first.size(), old_result.second.size());

    // vector<ExpressionPtr> first_polynom_coeffs;
    // for (auto& itm : old_result.first) {
    //     first_polynom_coeffs.emplace_back(itm->Copy());
    // }
    // std::println("copy complete");

    unique_ptr<Expression> new_result_quotient = coeffs_to_polynomial(old_result.first);
    std::println("new result quotient: {}", new_result_quotient->Accept(serializer).value());
    unique_ptr<Expression> new_result_remainder = coeffs_to_polynomial(old_result.second);
    // unique_ptr<Expression> new_result_remainder = new_result_quotient->Copy();
    // std::println("size(new_result_quotient) = {}, size(new_result_remainder) = {}", ne)
    std::println("new result remainder: {}", new_result_remainder->Accept(serializer).value());
    return std::make_pair<ExpressionPtr, ExpressionPtr>(new_result_quotient->Copy(), new_result_remainder->Copy());
    // return std::make_pair(coeffs_to_polynomial(old_result.first), coeffs_to_polynomial(old_result.second));
}

auto poly_gcd_list(vector<ExpressionPtr>& a, vector<ExpressionPtr>& b) -> vector<unique_ptr<Expression>>
{
    if (b.size() == 1 && b[0]->Is<Real>() && RecursiveCast<Real>(*b[0])->GetValue() == 0) {
        return std::move(a);
    }

    auto res = synthetic_divide_list(a, b);
    if (res.second.size() == 1 && res.second[0]->Is<Real>() && RecursiveCast<Real>(*(res.second.front()))->GetValue() == 0) {
        std::println("Base case reached! Terminating!");
        // Scale the resulting expression by the leading coefficient
        auto lc = b[0]->Copy();
        for (auto& b_element : b)
            b_element = b_element / lc;
        return std::move(b);
    }
    return std::move(poly_gcd_list(b, res.second));
}

auto poly_gcd(ExpressionPtr& a, ExpressionPtr& b) -> ExpressionPtr
{
    std::println("another poly_gcd iteration!");
    if (b->Is<Real>() && RecursiveCast<Real>(*b)->GetValue() == 0) {
        // return std::move(a);
        return a->Copy();
    }

    InFixSerializer serializer {};
    std::println("a = {}, b = {}", a->Accept(serializer).value(), b->Accept(serializer).value());
    auto res = synthetic_divide(a, b);
    std::println("res.first: {}, res.second: {}", res.first->Accept(serializer).value(), res.second->Accept(serializer).value());
    // SimplifyVisitor simplify_visitor {};
    // auto res_second_simplified = res.second->Accept(simplify_visitor);
    if (res.second->Is<Real>()) {
        std::println("res.second is real!");
        auto res_second_casted = RecursiveCast<Real>(*(res.second));
        std::println("val of res.second: {}", res_second_casted->GetValue());
        if (RecursiveCast<Real>(*(res.second))->GetValue() == 0) {
            std::println("Base case reached! Terminating!");
            // Scale the resulting expression by the leading coefficient
            // auto coeffs = all_coeffs(b->Copy());
            // std::vector<ExpressionPtr> scaled_coeffs;
            // for (const auto& coeff : coeffs) {
            //     scaled_coeffs.emplace_back((coeff / coeffs[0])->Simplify());
            // }
            // auto monic_b = coeffs_to_polynomial(scaled_coeffs);
            // return std::move(b);
            return b->Copy();
        }
    }
    // return std::move(poly_gcd(b, res.second));
    return poly_gcd(b, res.second);
}



// auto yuns(ExpressionPtr& f)
// {
//     SimplifyVisitor simplify_visitor {};
//     auto x = Oasis::Variable{"x"};
//     auto f_diff = f->Differentiate(x);
//
//     auto expanded_coeffs_f = all_coeffs(expand_polynomial3(f)->Accept(simplify_visitor).value());
//     auto expanded_coeffs_fdiff = all_coeffs(expand_polynomial3(f_diff)->Accept(simplify_visitor).value());
//
//     auto a0 = poly_gcd_list(expanded_coeffs_f, expanded_coeffs_fdiff);
//     auto b1_division_results = synthetic_divide_list(expanded_coeffs_f, a0);
//     auto b1 = std::move(b1_division_results.front());
//     // recombine the coefficients to make an expression
//     auto combined_b1 = coeffs_to_polynomial(b1)->Generalize();
//
//     auto c1_division_results = synthetic_divide_list(expanded_coeffs_fdiff, a0);
//     auto c1 = std::move(c1_division_results.front());
//     auto combined_c1 = coeffs_to_polynom
//     auto d1 = c1 + combined_b1->Differentiate(x);
// }

auto yuns(ExpressionPtr& f)
{
    SimplifyVisitor simplify_visitor {};
    auto x = Oasis::Variable{"x"};
    auto f_diff = f->Differentiate(x);

    auto a0 = poly_gcd(f, f_diff);
    auto b1 = synthetic_divide(f, a0).first;
    auto c1 = synthetic_divide(f_diff, a0).first;
    auto d1 = c1 - b1->Differentiate(x);

    int i = 1;
    std::list<ExpressionPtr> a, b, c, d;
    a.emplace_back(std::move(a0));
    b.emplace_back(std::move(b1));
    b.emplace_back(std::move(b1));
    c.emplace_back(std::move(c1));
    c.emplace_back(std::move(c1));
    d.emplace_back(std::move(d1));
    d.emplace_back(std::move(d1));

    while (b.back() != Real{1}.Copy()) {
        // auto a_i = poly_gcd()
    }
}






int main(int argc, char** argv)
{
    using namespace Oasis;

    Oasis::InFixSerializer serializer;

    auto opts = SimplifyOpts { .simplifyFractions = true };
    auto opts_eval = SimplifyOpts { .simplifyFractions = false };
    Oasis::SimplifyVisitor simplify_visitor { opts };
    Oasis::SimplifyVisitor eval_visitor { opts_eval };
    constexpr auto err_style = fg(fmt::color::indian_red);
    constexpr auto success_style = fg(fmt::color::green);

    Oasis::Variable x {"x"};
    // auto numerator = Subtract(Multiply(x, Real{2}), Real{-1});

    // BELOW WORKS
    // auto repeated_zero_quadratic = Add(Real{16}, Add(Multiply(Real{20}, x), Add(Multiply(Real{8}, Multiply(x, x)), Multiply(Multiply(x, x), x))));
    // auto repeated_zero_quad_simpl = repeated_zero_quadratic.Accept(simplify_visitor).value();
    // std::println("complete quadratic: {}", repeated_zero_quad_simpl->Accept(serializer).value());
    // auto repeated_zero_quad_zeroes = repeated_zero_quad_simpl->FindZeros();
    // for (auto& zer : repeated_zero_quad_zeroes) {
    //     std::println("repeated zero?: {}\n\n\n", zer->Accept(serializer).value());
    // }
    //
    //
    // // example to show matthew on why simplify is broken
    //
    // // auto simpleFrac = Add { Real{2}, Real{3} };
    // // auto simpleFrac = Add { Divide { Real{4}, Real{9}}, Real{4}};
    // // auto simpleFrac = Exponent { Divide{Real{2}, Real{3}}, Real{2} };
    // // auto simpleFrac = Divide { Divide{Real{40}, Real{9}}, Divide{Real{16}, Real{3}}};
    // auto simpleFrac = Multiply { Divide{Real{40}, Real{9}}, Divide{Real{3}, Real{16}}};
    // // auto simpleFrac = Divide {
    // //     Subtract {
    // //         Real{-4},
    // //         Exponent{
    // //             Real{64},
    // //             Divide {
    // //                 Real{1},
    // //                 Real{2}
    // //             }
    // //         }
    // //     },
    // //     Real{6}
    // // };
    // // auto simpleFrac = Add { Divide { Real{4}, Real{9}}, Real{4}};
    // // auto simpleFrac = Multiply { Real{2}, Add{ x, Real{3}}};
    // auto flattened = std::vector<std::unique_ptr<Expression>>();
    // simpleFrac.Flatten(flattened);
    // auto simplifiedFrac = std::move(simpleFrac.Accept(simplify_visitor)).value();
    // // auto simplifiedFrac = simpleFrac2.Generalize();
    // auto simplifiedFracStr = simplifiedFrac->Accept(serializer);
    // std::println("Simplified fraction: {}", simplifiedFracStr.value());
    // for (auto& itm : flattened) {
    //     std::println("itm (part of simpleFrac): {}", itm->Accept(serializer).value());
    // }
    //
    //
    // // cubic polynomial
    // // a_1 * x^3 + a_2 * x^2 + a_3 * x + a_4
    // // std::unique_ptr<Expression> a1 = std::make_unique<Real>(3);
    // // std::unique_ptr<Expression> a2 = std::make_unique<Real>(4);
    // // std::unique_ptr<Expression> a3 = std::make_unique<Real>(-4);
    // // std::unique_ptr<Expression> a4 = std::make_unique<Real>(0);
    // // std::unique_ptr<Expression> x1 = std::make_unique<Variable>(x);
    // // auto denominator2 =  a1*x1*x1*x1 + a2*x1*x1 + a3*x1 + a4;
    // // auto denominator2 =  a4 + a3*x1 + a2*x1*x1 + a1*x1*x1*x1;
    //
    //
    //
    // // (3x+11)/(x^2-x-6)
    // auto numerator1 = Add(Multiply(x, Real{3}), Real{11});
    //
    //
    // // TODO: Fix coeffs() function for this case (Subtract and Add)
    // // auto denominator1 = Subtract { Multiply {x, x}, Add { x, Real{6}} };
    //
    // auto denominator1 = Add { Multiply {x, x}, Add { Multiply {Real{-1}, x}, Real{-6}} };
    // auto exp1 = Divide{ numerator1, denominator1 };
    // std::println("exp1: {}", exp1.Accept(serializer).value());
    //
    // // (x^2+4)/x(3x^2+4x-4)
    // auto numerator2 = Add { Multiply { x, x }, Real { 4 } };
    // // denominator is of the form ax^2+bx+c
    // double a = 3, b = 4, c = -4;
    // auto denominator2 = Multiply {x, Add(Real{c}, Add(Multiply(Real{b}, x), Multiply(Real{a}, Multiply(x, x))))};
    // // auto denominator2 = Multiply {x, denominator};
    // auto exp2 = Divide{ numerator2, denominator2 };
    //
    // // vector<ExpressionPtr> coeffs = all_coeffs(expand_polynomial3(denominator1.Copy())->Accept(simplify_visitor).value());
    // // for (const auto& coeff : coeffs) {
    // //     std::println("coeff: {}", coeff->Accept(serializer).value());
    // // }
    //
    // auto cop1 = denominator1.Copy();
    // auto cop2 = numerator1.Copy();
    // auto sres = synthetic_divide(cop1, cop2);
    //
    //
    //
    // // auto res = pfd(numerator1.Generalize(), denominator1.Generalize());
    // // auto res = exp2.PartialFractionDecomp();
    //
    // // auto arg1 = std::vector<float> {1, 5, 6};
    // // auto arg2 = std::vector<float> {1, -1};
    //
    //
    //
    // // Synthetic Division Test
    // auto arg11 = Real{1};
    // auto arg12 = Real{-2};
    // auto arg13 = Real{-3};
    // auto arg1 = std::vector<std::unique_ptr<Expression>>();
    // arg1.emplace_back(std::make_unique<Real>(arg11));
    // arg1.emplace_back(std::make_unique<Real>(arg12));
    // arg1.emplace_back(std::make_unique<Real>(arg13));
    //
    // auto arg21 = Real{3};
    // auto arg22 = Real{-7};
    // auto arg2 = std::vector<std::unique_ptr<Expression>>();
    // arg2.emplace_back(std::make_unique<Real>(arg21));
    // arg2.emplace_back(std::make_unique<Real>(arg22));
    //
    // auto re = synthetic_divide_list(arg1, arg2);
    // for (const auto& expr : re.first) {
    //     std::print("expr: {}, ", expr->Accept(simplify_visitor).value()->Accept(serializer).value());
    // }
    // std::println();
    // for (const auto& expr : re.first) {
    //     std::print("expr: {}, ", expr->Accept(serializer).value());
    // }
    //
    //
    //
    //
    // // Polynomial GCD Test
    vector<ExpressionPtr> vp;
    vp.emplace_back(Real{1}.Copy());
    vp.emplace_back(Real{-3}.Copy());
    vp.emplace_back(Real{-2}.Copy());
    vp.emplace_back(Real{7}.Copy());
    vp.emplace_back(Real{-3}.Copy());

    auto vp_combined = coeffs_to_polynomial(vp)->Copy();
    // std::println("res polynom: {}", vp_combined->Accept(serializer).value());


    vector<ExpressionPtr> vp2;
    vp2.emplace_back(Real{1}.Copy());
    vp2.emplace_back(Real{-2}.Copy());
    vp2.emplace_back(Real{-3}.Copy());
    auto vp2_combined = coeffs_to_polynomial(vp2)->Copy();

    // auto gcds = poly_gcd_list(vp, vp2);
    // std::println("GCD vector size: {} means the degree is {}", gcds.size(), gcds.size() - 1);
    // std::println("GCD Terms:");
    // for (const auto& term : gcds) {
    //     std::print("{} ", term->Accept(serializer).value());
    // }
    // std::println();

    // auto divided = synthetic_divide(vp_combined, vp2_combined);
    // auto divided = synthetic_divide_list(vp, vp2);
    // print_pair(divided);
    // ABOVE WORKS

    auto gcd_using_poly_gcd = poly_gcd(vp_combined, vp2_combined);
    std::println("gcd using poly gcd: {}", gcd_using_poly_gcd->Accept(serializer).value());

    // auto x_expr = x.Copy();
    // auto vp3 = (x_expr * x_expr) + (Real{-1}.Copy() * x_expr) + Real{-6}.Copy();
    // auto vp4 = (Real{3}.Copy() * x_expr) + Real{11}.Copy();


    // BELOW WORKS
    // vector<ExpressionPtr> vp3;
    // vp3.emplace_back(Real{1}.Copy());
    // vp3.emplace_back(Real{-1}.Copy());
    // vp3.emplace_back(Real{-6}.Copy());
    //
    // auto vp3_combined = coeffs_to_polynomial(vp3)->Copy();
    // std::println("res polynom: {}", vp3_combined->Accept(serializer).value());
    //
    //
    // vector<ExpressionPtr> vp4;
    // vp4.emplace_back(Real{3}.Copy());
    // vp4.emplace_back(Real{11}.Copy());
    // auto vp4_combined = coeffs_to_polynomial(vp4)->Copy();
    //
    // auto synthetic_res = synthetic_divide(vp4_combined, vp3_combined);
    // print_pair(synthetic_res);
    // ABOVE WORKS





    return EXIT_SUCCESS;
}