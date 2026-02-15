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

std::string my_fun(Oasis::InFixSerializer& ser, const std::unique_ptr<Oasis::Expression>& ex)
{
    auto expression_stringified = ex->Accept(ser);
    if (expression_stringified.has_value()) return std::move(expression_stringified.value());
    return "";
}

auto my_fun2(std::unique_ptr<Oasis::Expression> es) -> std::unique_ptr<Oasis::Expression> {
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
        std::println("exponent: {}, coefficient: {}", exponent, my_fun(ser, coefficent));
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
        std::println("{}*x^{}", my_fun(ser, coefficents[i]), i);
    }

    return corresponding_coeff;
}

long long gcf1(long long a, long long b)
{
    if (b > a) {
        std::swap(a, b);
    }
    while (1) {
        a %= b;
        if (a == 0) {
            return b;
        }
        b %= a;
        if (b == 0) {
            return a;
        }
    }
}

auto pos_to_sop(const std::unique_ptr<Oasis::Expression>& ex)
{
    if (ex->Is<Oasis::Multiply<>>()) {
        return true;
    }
    return false;
}

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
    auto numerator = Subtract(Multiply(x, Real{2}), Real{-1});



    auto repeated_zero_quadratic = Add(Real{16}, Add(Multiply(Real{20}, x), Add(Multiply(Real{8}, Multiply(x, x)), Multiply(Multiply(x, x), x))));
    auto repeated_zero_quad_simpl = repeated_zero_quadratic.Accept(simplify_visitor).value();
    std::println("complete quadratic: {}", repeated_zero_quad_simpl->Accept(serializer).value());
    auto repeated_zero_quad_zeroes = repeated_zero_quad_simpl->FindZeros();
    for (auto& zer : repeated_zero_quad_zeroes) {
        std::println("repeated zero?: {}\n\n\n", zer->Accept(serializer).value());
    }


    // ax^2+bx+c
    // double a = 1;
    // double b = -1;
    // double c = -6;
    double a = 3;
    double b = 4;
    double c = -4;
    auto denominator = Add(Real{c}, Add(Multiply(Real{b}, x), Multiply(Real{a}, Multiply(x, x))));
    auto denominator2 = Multiply {x, denominator};
    // auto& denominator2 = denominator;


    // example to show matthew on why simplify is broken

    // auto simpleFrac = Add { Real{2}, Real{3} };
    // auto simpleFrac = Add { Divide { Real{4}, Real{9}}, Real{4}};
    // auto simpleFrac = Exponent { Divide{Real{2}, Real{3}}, Real{2} };
    // auto simpleFrac = Divide { Divide{Real{40}, Real{9}}, Divide{Real{16}, Real{3}}};
    auto simpleFrac = Multiply { Divide{Real{40}, Real{9}}, Divide{Real{3}, Real{16}}};
    // auto simpleFrac = Divide {
    //     Subtract {
    //         Real{-4},
    //         Exponent{
    //             Real{64},
    //             Divide {
    //                 Real{1},
    //                 Real{2}
    //             }
    //         }
    //     },
    //     Real{6}
    // };
    // auto simpleFrac = Add { Divide { Real{4}, Real{9}}, Real{4}};
    // auto simpleFrac = Multiply { Real{2}, Add{ x, Real{3}}};
    auto flattened = std::vector<std::unique_ptr<Expression>>();
    simpleFrac.Flatten(flattened);
    auto simplifiedFrac = std::move(simpleFrac.Accept(simplify_visitor)).value();
    // auto simplifiedFrac = simpleFrac2.Generalize();
    auto simplifiedFracStr = simplifiedFrac->Accept(serializer);
    std::println("Simplified fraction: {}", simplifiedFracStr.value());
    for (auto& itm : flattened) {
        std::println("itm (part of simpleFrac): {}", itm->Accept(serializer).value());
    }


    // cubic polynomial
    // a_1 * x^3 + a_2 * x^2 + a_3 * x + a_4
    // std::unique_ptr<Expression> a1 = std::make_unique<Real>(3);
    // std::unique_ptr<Expression> a2 = std::make_unique<Real>(4);
    // std::unique_ptr<Expression> a3 = std::make_unique<Real>(-4);
    // std::unique_ptr<Expression> a4 = std::make_unique<Real>(0);
    // std::unique_ptr<Expression> x1 = std::make_unique<Variable>(x);
    // auto denominator2 =  a1*x1*x1*x1 + a2*x1*x1 + a3*x1 + a4;
    // auto denominator2 =  a4 + a3*x1 + a2*x1*x1 + a1*x1*x1*x1;




    // auto denominator_simplified = denominator2.Simplify();
    auto denominator_simplified = std::move(denominator2.Accept(simplify_visitor)).value();
    auto denominator_zeroes = denominator_simplified->FindZeros();


    auto vis = denominator2.Accept(simplify_visitor);
    auto vis2 = std::move(vis).value().get();
    auto vis3 = vis2->Accept(serializer);
    if (vis3.has_value())
        std::println("Expression has {} zeros: {}", denominator_zeroes.size(), vis3.value());

    auto coefficient = my_fun2(denominator.Simplify());
    std::println("c: {}", my_fun(serializer, coefficient));

    std::vector<std::unique_ptr<Oasis::Expression>> expressions;
    std::vector<std::unique_ptr<Divide<Real>>> actual_zeroes_better;
    auto actual_zeroes = std::vector<Oasis::Divide<Real>>();
    bool flag = false;
    for (auto& expr : denominator_zeroes) {
        auto root = Oasis::RecursiveCast<Oasis::Divide<Oasis::Expression>>(*expr);
        // auto simplified_s = expr->Simplify();
        auto simplified_s = std::move(expr->Accept(simplify_visitor)).value();
        Subtract sum1exp {x, *simplified_s};
        auto sum1 = sum1exp.Copy();

        if (root != nullptr) {
            auto num_simplified = root->GetMostSigOp().Simplify();
            auto den_simplified = root->GetLeastSigOp().Simplify();

            if (num_simplified->Is<Oasis::Real>() && den_simplified->Is<Oasis::Real>()) {
                auto simplifiedReal = dynamic_cast<Oasis::Real&>(*num_simplified).GetValue();
                auto simplifiedReal2 = dynamic_cast<Oasis::Real&>(*den_simplified).GetValue();

                auto n = std::llround(simplifiedReal);
                auto d = std::llround(simplifiedReal2);
                auto gcf_v = gcf1(n, d);
                // std::println("x = {}/{}",n / gcf_v, d / gcf_v);
                actual_zeroes.emplace_back( Real{ static_cast<double>(n / gcf_v) }, Real{ static_cast<double>(d / gcf_v) });

                if (coefficient->Is<Real>() && !flag) {
                    auto si1 = dynamic_cast<Oasis::Real&>(*coefficient);
                    auto si = si1.GetValue();
                    auto casted = std::llround(si);

                    if (abs(si - casted) < 0.001) {
                        if (d % casted == 0) {
                            sum1 = Subtract(Multiply{si1, x}, Multiply{si1, *simplified_s}).Copy();
                            flag = true;
                        }
                    }
                }
            } else {
                auto simplifiedReal = dynamic_cast<Oasis::Real&>(*num_simplified);
                auto simplifiedReal2 = dynamic_cast<Oasis::Real&>(*den_simplified);
                actual_zeroes.emplace_back(simplifiedReal, simplifiedReal2);
            }
        } else {
            // ex. if the zero is x=0, which can't be casted into a divide operation
            if (expr->Is<Oasis::Real>()) {
                auto expr_real_cast = dynamic_cast<Oasis::Real&>(*expr);
                actual_zeroes.emplace_back(expr_real_cast, Real{1});
            }

        }


        expressions.emplace_back(sum1->Simplify());
        auto res1 = simplified_s->Accept(serializer);

        auto res2 = expr->Accept(serializer);
        if (res1.has_value() && res2.has_value()) {
            std::println("denominator zero: {} == {}", res2.value(), res1.value());
        }

        // TODO: remove the earlier logic for simplifying the zeroes
        if (simplified_s->Is<Oasis::Real>()) {
            auto expr_real_cast = dynamic_cast<Oasis::Real&>(*simplified_s);
            actual_zeroes_better.emplace_back(std::make_unique<Divide<Real>>(expr_real_cast, Real{1}));
        } else {
            actual_zeroes_better.emplace_back(RecursiveCast<Divide<Real>>(*simplified_s));
        }
    }

    // i don't need this with the new changes to SimplifyVisitor.cpp (for Exponent, Divide)
    /*
    for (auto& actual_zero : actual_zeroes) {
        std::println("x = {}/{}", actual_zero.GetMostSigOp().GetValue(), actual_zero.GetLeastSigOp().GetValue());
    }*/



    auto expressions_pfd = std::vector<std::unique_ptr<Oasis::Expression>>();
    expressions_pfd.reserve(expressions.size());
    // create partial fraction expressions e.g. from x(x+2)(3x-2) -> A*x(x+2) + B*(3x-2)x + C*(3x-2)(x+2)

    // start at 'A', increment for each unique item in partial fraction expressions
    char var_name = 'A';
    for (int i = 0; i < expressions.size(); i++, var_name++) {
        // auto ex = Oasis::Variable{std::string{var_name}};
        std::vector<std::unique_ptr<Oasis::Expression>> current_vec;
        current_vec.emplace_back(std::make_unique<Variable>(std::string{var_name}));

        for (int j = 0; j < expressions.size(); j++) {
            if (j != i) {
                // expressions_pfd.emplace_back(expressions[j]->Copy());
                current_vec.emplace_back(expressions[j]->Copy());
            }
        }
        expressions_pfd.emplace_back(Oasis::BuildFromVector<Oasis::Multiply>(current_vec));
    }
    auto expression_pfd = Oasis::BuildFromVector<Oasis::Add>(expressions_pfd);
    std::println("expression_pfd: {}", expression_pfd->Accept(serializer).value());

    // combine factors back together to yield original, factorized expression
    auto expression = Oasis::BuildFromVector<Oasis::Multiply>(expressions);



    auto expression_stringified = expression->Accept(serializer);
    if (expression_stringified.has_value()) {}
        std::println("Final Expression: {}", expression_stringified.value());


    std::map<Variable, Divide<Real>> coefs;
#define print_expr(expr) std::println(#expr ": {}", expr->Accept(serializer).value())


    // assume numerator is x^2+4
    auto numerator_t = Add { Multiply { x, x }, Real { 4 } };

    /*
    auto sub1_x0_lhs = numerator_t.Substitute(x, Real{0});
    auto sub1_x0_rhs = expression_pfd->Substitute(x, Real{0});

    auto sub1_x2_lhs = numerator_t.Substitute(x, Divide{Real{2}, Real{3}});
    auto sub1_x2_rhs = expression_pfd->Substitute(x, Divide{Real{2}, Real{3}});
    auto divd1 = Divide { *sub1_x2_rhs, Variable{"A"} };
    auto divd1_s = divd1.Accept(simplify_visitor).value();
    auto divd = Divide { *sub1_x2_lhs, *divd1_s };
    auto divd_s = divd.Accept(simplify_visitor).value();
    auto divd_s2 = divd_s->Accept(simplify_visitor).value();

    print_expr(divd_s2);
    print_expr(sub1_x2_lhs);
    print_expr(sub1_x2_rhs); */

    std::println("\n\n\n");


    char var = 'A';
    std::vector<std::unique_ptr<Expression>> final_pfd;
    int i = 0;
    for (auto& ex : actual_zeroes_better) {
        print_expr(ex);
        auto& v = expressions[i];
        print_expr(v);

        auto sub_lhs = numerator_t.Substitute(x, *ex);
        auto sub_rhs = expression_pfd->Substitute(x, *ex);
        auto divd1a = Divide { *sub_rhs, Variable{std::string{ var }} };
        auto divd1a_s = divd1a.Accept(simplify_visitor).value();
        auto divda = Divide { *sub_lhs, *divd1a_s };
        auto divda_s = divda.Accept(simplify_visitor).value();
        auto divda_s2 = divda_s->Accept(simplify_visitor).value();
        auto divda_s2_eval = divda_s2->Accept(eval_visitor).value();

        std::println("x = {} => {} = {}", ex->Accept(serializer).value(), var, divda_s2->Accept(serializer).value());
        final_pfd.emplace_back(std::make_unique<Divide<Expression>>(*divda_s2_eval, *v));

        var++;
        i++;
    }

    auto final_pfd_result = Oasis::BuildFromVector<Add>(final_pfd);
    std::println("FINAL PFD RESULT: {}", final_pfd_result->Accept(serializer).value());

    return EXIT_SUCCESS;
}
