//
// Created by Amaan Mahammad on 4/18/2026.
//

#include "Oasis/Polynomial.hpp"
#include "../io/include/Oasis/InFixSerializer.hpp"
#include <string>
#include <print>
#include "Oasis/BinaryExpression.hpp"
#include "Oasis/Exponent.hpp"
// #include "Oasis/Add.hpp"
#include "Oasis/Multiply.hpp"
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
#include "Oasis/Multiply.hpp"
#include <iostream>
#include <map>

#include "Oasis/DifferentiateVisitor.hpp"
#include "Oasis/Polynomial.hpp"
#include "Oasis/SimplifyVisitor.hpp"

#include <stdexcept>

#include "Oasis/Real.hpp"
#include "Oasis/Variable.hpp"

#include <random>

namespace Oasis {
namespace {

    auto CopyOrThrow(const Expression* expression) -> std::unique_ptr<Expression>
    {
        if (expression == nullptr) {
            throw std::invalid_argument("Polynomial requires a non-null expression");
        }

        return expression->Copy();
    }

    // auto print_expr_list(std::vector<std::unique_ptr<Expression>>& expr_list) -> std::string
    // {
    //     InFixSerializer serializer;
    //     std::string res;
    //     auto it = expr_list.begin();
    //     for (; std::next(it) != expr_list.end(); ++it) {
    //         std::format_to(std::back_inserter(res), "{}, ", (*it)->Accept(serializer).value());
    //     }
    //     std::format_to(std::back_inserter(res), "{}", (*it)->Accept(serializer).value());
    //     return res;
    // }

    auto print_expr_list(std::ranges::input_range auto&& expr_list) -> std::string
    {
        InFixSerializer serializer;
        std::string res;
        auto it = expr_list.begin();
        for (; std::next(it) != expr_list.end(); ++it) {
            std::format_to(std::back_inserter(res), "{}, ", (*it)->Accept(serializer).value());
        }
        std::format_to(std::back_inserter(res), "{}", (*it)->Accept(serializer).value());
        return res;
    }

} // namespace

Polynomial::Polynomial(std::unique_ptr<Expression>&& other)
    : expr_(std::move(other))
{
}

Polynomial::Polynomial(const Polynomial& other)
    : expr_(other.expr_ ? other.expr_->Copy() : nullptr)
{
}

/*
Polynomial::Polynomial(std::initializer_list<Oasis::Real> coeffs)
{
    // base case #1
    if (coeffs.size() == 0) {
        throw std::invalid_argument("Polynomial requires at least one coefficient");
    }

    // base case #2 (BuildFromVector needs minimum two terms) (input: [ a ], output: a)
    if (coeffs.size() == 1) {
        expr_ = coeffs.begin()->Generalize();
        return;
    }

    Oasis::Variable x {"x"};
    std::vector<std::unique_ptr<Expression>> coeffs_vec;
    int i = coeffs.size();
    for (auto& coeff : coeffs) {
        coeffs_vec.emplace_back(Oasis::Multiply{coeff, Oasis::Exponent{x, Real{ static_cast<double>(i - 1)}}}.Generalize());
        i++;
    }

    expr_ = Oasis::BuildFromVector<Oasis::Add>(coeffs_vec);
}*/

// template<std::ranges::range R>
Polynomial::Polynomial(std::span<Real>&& coeffs)
{
    // std::println("Constructing polynom from list:");
    // base case #1
    if (coeffs.empty()) {
        throw std::invalid_argument("Polynomial requires at least one coefficient");
    }

    // base case #2 (BuildFromVector needs minimum two terms) (input: [ a ], output: a)
    if (coeffs.size() == 1) {
        expr_ = Oasis::Real{coeffs.begin()->GetValue()}.Generalize();
        return;
    }

    Oasis::Variable x {"x"};
    std::vector<std::unique_ptr<Expression>> coeffs_vec;
    int i = coeffs.size();
    for (auto& coeff : coeffs) {
    // auto it = coeffs.begin();
    // constexpr auto EPSILON = std::numeric_limits<float>::epsilon();
    // while (std::abs(it->GetValue()) < EPSILON && it != coeffs.end()) {
    //     ++it;
    //     --i;
    // }
    // for (; it != coeffs.end(); ++it) {
        coeffs_vec.emplace_back(Oasis::Multiply{coeff, Oasis::Exponent{x, Real{ static_cast<double>(i - 1)}}}.Generalize());
        --i;
    }

    InFixSerializer serializer;
    SimplifyVisitor simplify_visitor {};
    expr_ = Oasis::BuildFromVector<Oasis::Add>(coeffs_vec)->Accept(simplify_visitor).value();
    // std::println("created polynomial from coeffs: {}", expr_->Accept(serializer).value());
}

Polynomial::Polynomial(std::span<std::unique_ptr<Expression>>&& coeffs)
{
    InFixSerializer serializera;
    // std::println("Constructing polynom from list (size={})", coeffs.size());

    // base case #1
    if (coeffs.empty()) {
        // throw std::invalid_argument("Polynomial requires at least one coefficient");
        expr_ = Real{0}.Generalize();
        return;
    }

    // // base case #2 (BuildFromVector needs minimum two terms) (input: [ a ], output: a)
    if (coeffs.size() == 1) {
        expr_ = (*coeffs.begin())->Generalize();
        return;
    }

    Oasis::Variable x {"x"};
    std::vector<std::unique_ptr<Expression>> coeffs_vec;


    InFixSerializer serializer;
    int i = coeffs.size();
    // auto it = coeffs.begin();
    // constexpr auto EPSILON = std::numeric_limits<float>::epsilon();
    // while (std::abs(RecursiveCast<Real>(**it)->GetValue()) < EPSILON && std::next(it) != coeffs.end()) {
    //     std::println("Ignoring empty coefficient at i={}: {} == {}", i, (*it)->Accept(serializer).value(), RecursiveCast<Real>(**it)->GetValue());
    //     ++it;
    //     --i;
    // }
    // std::println("it match?: {}", it != coeffs.end());
    for (auto& coeff : coeffs) {
    // for (; it != coeffs.end(); ++it) {
    //     std::println("Adding coeff {} at i={}", (*it)->Accept(serializer).value(), i);
    //     coeffs_vec.emplace_back(Oasis::Multiply{**it, Oasis::Exponent{x, Real{ static_cast<double>(i - 1)}}}.Generalize());
    //     std::println("finished adding.");


    // for (auto& coeff : coeffs) {
        coeffs_vec.emplace_back(Oasis::Multiply{*coeff, Oasis::Exponent{x, Real{ static_cast<double>(i - 1)}}}.Generalize());
        --i;
    }

    SimplifyVisitor simplify_visitor {};
    expr_ = Oasis::BuildFromVector<Oasis::Add>(coeffs_vec)->Accept(simplify_visitor).value();
}

Polynomial::Polynomial(std::initializer_list<int> coeffs)
{
    // base case #1
    if (coeffs.size() == 0) {
        throw std::invalid_argument("Polynomial requires at least one coefficient");
    }

    // base case #2 (BuildFromVector needs minimum two terms) (input: [ a ], output: a)
    if (coeffs.size() == 1) {
        expr_ = Oasis::Real{static_cast<double>(*coeffs.begin())}.Generalize();
        return;
    }

    Oasis::Variable x {"x"};
    std::vector<std::unique_ptr<Expression>> coeffs_vec;
    int i = coeffs.size();
    for (auto& coeff : coeffs) {
        coeffs_vec.emplace_back(Oasis::Multiply{Oasis::Real{ static_cast<double>(coeff)}, Oasis::Exponent{x, Real{ static_cast<double>(i - 1)}}}.Generalize());
        --i;
    }

    expr_ = Oasis::BuildFromVector<Oasis::Add>(coeffs_vec);
}

Polynomial::Polynomial(const Expression& expression)
    : expr_(expression.Copy())
{
}

Polynomial::Polynomial(const Expression* expression)
    : expr_(CopyOrThrow(expression))
{
}

Polynomial::Polynomial(std::unique_ptr<Expression> expression)
    : expr_(std::move(expression))
{
    if (expr_ == nullptr) {
        throw std::invalid_argument("Polynomial requires a non-null expression");
    }
}

auto Polynomial::operator=(const Polynomial& other) -> Polynomial&
{
    if (this == &other) {
        return *this;
    }

    expr_ = other.expr_ ? other.expr_->Copy() : nullptr;
    return *this;
}

auto Polynomial::Copy() const -> std::unique_ptr<Expression>
{
    return expr_->Copy();
}

auto Polynomial::Copy_P() const -> Polynomial
{
    return Polynomial {*this};
}

auto Polynomial::Equals(const Expression& other) const -> bool
{
    if (const auto* otherPolynomial = dynamic_cast<const Polynomial*>(&other); otherPolynomial != nullptr) {
        return expr_->Equals(*otherPolynomial->GetExpression());
    }

    return expr_->Equals(other);
}

auto Polynomial::Generalize() const -> std::unique_ptr<Expression>
{
    return expr_->Generalize();
}

auto Polynomial::StructurallyEquivalent(const Expression& other) const -> bool
{
    if (const auto* otherPolynomial = dynamic_cast<const Polynomial*>(&other); otherPolynomial != nullptr) {
        return expr_->StructurallyEquivalent(*otherPolynomial->GetExpression());
    }

    return expr_->StructurallyEquivalent(other);
}

auto Polynomial::Substitute(const Expression& var, const Expression& val) -> std::unique_ptr<Expression>
{
    // return std::make_unique<Polynomial>(expr_->Substitute(var, val));
    return expr_->Substitute(var, val);
}

auto Polynomial::GetExpression() const -> std::unique_ptr<Expression>
{
    // return *expr_;
    return expr_->Copy();
    // return std::forward(expr_);
}

void print_pair(const std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>& pair)
{
    Oasis::InFixSerializer serializer;
    auto& it1 = pair.first;
    auto& it2 = pair.second;
    std::println("quotient: {}, remainder: {}", it1->Accept(serializer).value(), it2->Accept(serializer).value());
}

void print_pair(const std::pair<Polynomial, Polynomial>& pair)
{
    Oasis::InFixSerializer serializer;
    auto& it1 = pair.first;
    auto& it2 = pair.second;
    std::println("quotient: {}, remainder: {}", it1.Accept(serializer).value(), it2.Accept(serializer).value());
}

void print_pair(const std::pair<std::vector<std::unique_ptr<Expression>>, std::vector<std::unique_ptr<Expression>>>& pair)
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

void print_pair(const std::pair<std::vector<Polynomial>, std::vector<Polynomial>>& pair)
{
    Oasis::InFixSerializer serializer;
    auto& it1 = pair.first;
    auto& it2 = pair.second;
    std::print("quotient: ");
    for (auto& itm : it1) {
        std::print("{} ", itm.Accept(serializer).value());
    }
    std::print(", remainder: ");
    for (auto& itm : it2) {
        std::print("{} ", itm.Accept(serializer).value());
    }
    std::println();
}

auto Polynomial::get_coefficients() const -> std::vector<std::unique_ptr<Oasis::Expression>>
{
    SimplifyVisitor simplify_visitor {};
    auto es = expr_->Accept(simplify_visitor).value();
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
        // std::println("exponent: {}, coefficient: {}", exponent, coefficent->Accept(ser).value());
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
        coefficents.push_back(negCoefficents[i - 1]->Accept(simplify_visitor).value());
    }
    for (const std::unique_ptr<Expression>& i : posCoefficents) {
        coefficents.push_back(i->Accept(simplify_visitor).value());
    }

    std::reverse(coefficents.begin(), coefficents.end());
    return coefficents;
}

/*
auto another_simpl(std::unique_ptr<Expression>& expr)
{
    if (auto addCase = RecursiveCast<Multiply<Multiply<Expression, Variable>>>(*expr); addCase != nullptr) {
        auto lhs = addCase->GetMostSigOp();
        auto rhs = addCase->GetLeastSigOp();

        auto coeff_mul = Multiply{ lhs.GetMostSigOp(), rhs.GetMostSigOp()};
        auto var_exp = Multiply{ lhs.GetLeastSigOp(), rhs.GetLeastSigOp() };
        // if (lhs.Is<Multiply>() && rhs.Is<Multiply>()) {
        //     // auto lhs_mul = RecursiveCast<Multiply<Expression>>(lhs);
        //     // auto rhs_mul = RecursiveCast<Multiply<Expression>>(rhs);
        //     if (lhs_mul->GetMostSigOp().Is<Variable>() && rhs_mul->GetLeastSigOp().Is<Variable>()) {
        //         auto var_lhs = RecursiveCast<Variable>(lhs_mul->GetMostSigOp());
        //         auto var_rhs = RecursiveCast<Variable>(rhs_mul->GetLeastSigOp());
        //         if (var_lhs->GetName() == var_rhs->GetName()) {
        //             auto new_lhs = lhs_mul->GetLeastSigOp();
        //             auto new_rhs = rhs_mul->GetMostSigOp();
        //         }
        //     }
        // }
    }
} */

auto Polynomial::expand() const -> Polynomial
{
    SimplifyVisitor simplify_visitor {};
    if (expr_->Is<Multiply>()) {
        if (auto addCase = RecursiveCast<Multiply<Expression>>(*expr_); addCase != nullptr) {
            Oasis::InFixSerializer serializer;
            std::vector<std::unique_ptr<Expression>> lhs, rhs;
            if (addCase->GetMostSigOp().Is<Add>()) {
                auto conv = RecursiveCast<Add<Expression>>(addCase->GetMostSigOp());
                conv->Flatten(lhs);
            } else {
                lhs.emplace_back(addCase->GetMostSigOp().Copy());
            }
            if (addCase->GetLeastSigOp().Is<Add>()) {
                auto conv = RecursiveCast<Add<Expression>>(addCase->GetLeastSigOp());
                conv->Flatten(rhs);
            } else {
                rhs.emplace_back(addCase->GetLeastSigOp().Copy());
            }


            // std::println("lhs # of items added: {}, rhs # of items added: {}", lhs.size(), rhs.size());
            std::vector<std::unique_ptr<Expression>> result;
            for (const auto& lhs_exp : lhs) {
                for (const auto& rhs_exp : rhs) {
                    // ex. (2x)*(2x) -> (2*2)*(x*x) -> 4*x^2
                    if (auto mulCaseLHS = RecursiveCast<Multiply<Expression, Variable>>(*lhs_exp),
                            mulCaseRHS = RecursiveCast<Multiply<Expression, Variable>>(*rhs_exp);
                            mulCaseLHS != nullptr && mulCaseRHS != nullptr) {

                        auto lhs_new = mulCaseLHS->GetMostSigOp().Copy() * mulCaseRHS->GetMostSigOp().Copy();
                        auto rhs_new = mulCaseLHS->GetLeastSigOp().Copy() * mulCaseRHS->GetLeastSigOp().Copy();

                        // std::println("lhs_new: {}, rhs_new: {}", lhs_new->Accept(serializer).value(), rhs_new->Accept(serializer).value());
                        result.emplace_back(lhs_new * rhs_new);
                    } else {
                        result.emplace_back(lhs_exp * rhs_exp);
                    }
                }
            }

            std::unique_ptr<Expression> combined = BuildFromVector<Add>(result);
            std::println("expanded polynom: {}", combined->Accept(serializer).value());
            return combined;
        }
    }
    if (expr_->Is<Exponent>()) {
        auto expCase = RecursiveCast<Exponent<Expression, Real>>(*expr_);
        if (expCase != nullptr) {
            std::unique_ptr<Expression> base = expCase->GetMostSigOp().Copy();
            std::unique_ptr<Expression> exponent = expCase->GetLeastSigOp().Copy();

            if (exponent->Is<Real>()) {
                auto exponent_double = RecursiveCast<Real>(*exponent)->GetValue();
                auto exponent_int = static_cast<int>(exponent_double);
                if (std::abs(exponent_double - exponent_int) < std::numeric_limits<float>::epsilon()) {
                    std::vector<std::unique_ptr<Expression>> result;
                    for (int i = 0; i < exponent_int; i++) {
                        result.emplace_back(base->Copy());
                    }
                    auto expanded = Oasis::BuildFromVector<Multiply>(result)->Generalize();
                    InFixSerializer serializer;

                    std::unique_ptr<Expression> res1 = Polynomial(*expanded).expand().Accept(simplify_visitor).value();
                    auto res2 = Polynomial(*res1);
                    std::println("{} -> {} -> {}", expr_->Accept(serializer).value(), expanded->Accept(serializer).value(), res2.Accept(serializer).value());
                    return res2;
                }
            }
        }
    }

    InFixSerializer serializer;
    auto expr_simpl = expr_->Accept(simplify_visitor).value();
    // std::println("expanding a non-multiply polynomial: {}", expr_simpl->Accept(serializer).value());
    return expr_simpl->Copy();
}

// TODO: make this function work for non-integer coefficients
int multi_gcd(const std::vector<std::unique_ptr<Expression>>& numbers) {
    if (numbers.empty()) return 0;

    int result = static_cast<int>(RecursiveCast<Real>(*numbers.front())->GetValue());
    for (auto it = std::next(numbers.begin()); it != numbers.end(); ++it) {
        result = std::gcd(result, static_cast<int>(RecursiveCast<Real>(**it)->GetValue()));
    }
    return result;
}

// get the primitive part of a polynomial (divide the polynomial by its content)
// Ex. given 6*x^2 + 4*x + 2 => content = gcd(6, 4, 2) = 2,
// so p = (6/2)x^2 + (4/2)x + (2/2) = 3x^2 + 2x + 1
// TODO: make this function work for non-integer coefficients
Polynomial Polynomial::primitive_part() const
{
    SimplifyVisitor simplify_visitor {};
    InFixSerializer serializer;
    // auto coeffs = all_coeffs(poly->Copy());
    auto coeffs = get_coefficients();
    std::println("PRIMITIVE_PART of {}: coeffs size: {}", expr_->Accept(serializer).value(), coeffs.size());
    std::vector<std::unique_ptr<Expression>> new_coeffs;
    new_coeffs.reserve(coeffs.size());

    // auto gcd = (Real { static_cast<double>(multi_gcd(coeffs)) }).Copy();
    int gcd = multi_gcd(coeffs);
    std::println("gcd val: {}", gcd);

    // prevent a divide by zero if the GCD is zero
    if (gcd == 0) return expr_->Copy();

    std::println("PRIMITIVE_PART: gcd = {}", gcd);
    for (std::unique_ptr<Expression>& coeff : coeffs) {
        // coeff = (coeff / gcd);
        // coeff = coeff->Accept(simplify_visitor);
        int val = static_cast<int>(RecursiveCast<Real>(*coeff)->GetValue()) / gcd;
        new_coeffs.emplace_back(Real{ static_cast<double>(val) }.Copy());
    }

    // return coeffs_to_polynomial(new_coeffs);
    return Polynomial { new_coeffs };
}

std::pair<std::vector<std::unique_ptr<Expression>>, std::vector<std::unique_ptr<Expression>>> synthetic_divide_list(std::vector<std::unique_ptr<Expression>>& dividend, std::vector<std::unique_ptr<Expression>>& divisor)
{
    // Fast polynomial division by using Extended Synthetic Division. Also works with non-monic polynomials.
    // dividend and divisor are both polynomials, which are here simply lists of coefficients. Eg: x^2 + 3x + 5 will be represented as [1, 3, 5]
    std::println("synthetic_divide_list([{}],\n\t[{}])", print_expr_list(dividend), print_expr_list(divisor));

    InFixSerializer serializer;
    SimplifyVisitor simplify_visitor {};
    std::vector<std::unique_ptr<Expression>> out {}; // Copy the dividend
    for (const auto& d : dividend) {
        out.emplace_back(d->Copy());
        // std::println("out val: {}", d->Accept(serializer).value());
    }

    // std::println("out: {}", out);
    auto normalizer = divisor[0]->Copy();
    for (int i = 0; i < static_cast<int>(dividend.size()-(divisor.size()-1)); i++) {
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

    std::vector<std::unique_ptr<Expression>> quotient {};
    std::vector<std::unique_ptr<Expression>> remainder {};

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

    // auto res3 = std::vector<std::vector<std::unique_ptr<Expression>>>();
    // res3.emplace_back(std::move(quotient));
    // res3.emplace_back(std::move(remainder));
    // return std::move(res3);

    return std::make_pair<std::vector<std::unique_ptr<Expression>>, std::vector<std::unique_ptr<Expression>>>(std::move(quotient), std::move(remainder));
}



std::pair<Polynomial, Polynomial> synthetic_divide(Polynomial& dividend, Polynomial& divisor)
{
    SimplifyVisitor simplify_visitor {};
    InFixSerializer serializer;
    std::println("synthetic_divide({}, {})", dividend.Accept(serializer).value(), divisor.Accept(serializer).value() );

    auto dividend_coeffs = dividend.expand().get_coefficients();
    auto divisor_coeffs = divisor.expand().get_coefficients();
    // auto dividend_coeffs = dividend.expand().Accept(simplify_visitor).value();

    // std::println("number of dividend coeffs: {}", dividend_coeffs.size());
    // std::println("number of divisor coeffs: {}", divisor_coeffs.size());

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
    std::print("print_pair(old_result): ");
    print_pair(old_result);

    // BUG:
    // if the dividend is 0, synthetic_divide_list() returns ([], [...]), so a polynomial cannot be constructed.
    // std::println("old result.first size: {}, old result.second size: {}", old_result.first.size(), old_result.second.size());

    // std::vector<std::unique_ptr<Expression>> first_polynom_coeffs;
    // for (auto& itm : old_result.first) {
    //     first_polynom_coeffs.emplace_back(itm->Copy());
    // }
    // std::println("copy complete");

    auto new_result_quotient = Polynomial(old_result.first);
    // std::println("new result quotient: {}", new_result_quotient.Accept(serializer).value());
    auto new_result_remainder = Polynomial(old_result.second);
    // std::unique_ptr<Expression> new_result_remainder = new_result_quotient->Copy();
    // std::println("size(new_result_quotient) = {}, size(new_result_remainder) = {}", ne)
    // std::println("new result remainder: {}", new_result_remainder.Accept(serializer).value());
    return std::make_pair<Polynomial, Polynomial>(new_result_quotient.Copy(), new_result_remainder.Copy());
    // return std::make_pair(coeffs_to_polynomial(old_result.first), coeffs_to_polynomial(old_result.second));
}

auto Polynomial::monic() const -> Polynomial
{
    SimplifyVisitor simplify_visitor {};
    auto coeffs = get_coefficients();
    std::vector<std::unique_ptr<Expression>> scaled_coeffs;
    for (const auto& coeff : coeffs) {
        scaled_coeffs.emplace_back((coeff / coeffs[0])->Accept(simplify_visitor).value());
    }
    auto monic_b = Polynomial(scaled_coeffs);
    return monic_b;
}

std::pair<Polynomial, Polynomial> synthetic_pseudo_divide(Polynomial& dividend, Polynomial& divisor)
{
    InFixSerializer ser;
    std::println("synthetic_pseudo_divide({}, {})", dividend.Accept(ser).value(), divisor.Accept(ser).value() );
    // if (divisor.Is<Real>() && RecursiveCast<Real>(*divisor.GetExpression())->GetValue() == 0) {
    //     throw std::runtime_error("Cannot divide by zero!");
    // }

    // if (dividend.Is<Real>()) {
    //     std::println("dividend is real");
    // }

    SimplifyVisitor simplify_visitor {};
    InFixSerializer serializer;
    auto dividend_coeffs = dividend.expand().get_coefficients();
    auto divisor_coeffs = divisor.expand().get_coefficients();
    auto lc = divisor_coeffs.front()->Generalize();
    auto dividend_degree = std::max(static_cast<int>(dividend_coeffs.size()) - 1, 0);
    auto divisor_degree = std::max(static_cast<int>(divisor_coeffs.size()) - 1, 0);
    auto degree_diff = dividend_degree - divisor_degree;
    auto scale_factor = Exponent {*lc, Real { static_cast<double>(degree_diff + 1) }};
    std::println("scale factor: {}", scale_factor.Accept(serializer).value());
    std::vector<std::unique_ptr<Expression>> dividend_coeffs_scaled;
    dividend_coeffs_scaled.reserve(dividend_coeffs.size());

    // scale all of the dividend coefficients such that the result of a synthetic
    // division only has integer coefficients (property of pseudo-division)
    for (auto& coeff : dividend_coeffs) {
        auto tmp = Multiply { *coeff, scale_factor };
        dividend_coeffs_scaled.emplace_back(tmp.Accept(simplify_visitor).value());
        // dividend_coeffs_scaled.emplace_back(coeff->Copy());
    }

    // std::println("number of dividend coeffs: {}", dividend_coeffs_scaled.size());
    // std::println("number of divisor coeffs: {}", divisor_coeffs.size());

    std::print("dividend coeffs of {}: ", dividend.Accept(serializer).value());
    for (auto& coeff : dividend_coeffs_scaled) {
        std::print("{} ", coeff->Accept(serializer).value());
    }
    std::println();
    std::print("divisor coeffs of {}: ", divisor.Accept(serializer).value());
    for (auto& coeff : divisor_coeffs) {
        std::print("{} ", coeff->Accept(serializer).value());
    }
    std::println();
    auto old_result = synthetic_divide_list(dividend_coeffs_scaled, divisor_coeffs);
    std::print("print_pair(old_result): ");
    print_pair(old_result);
    // std::println("old result.first size: {}, old result.second size: {}", old_result.first.size(), old_result.second.size());

    // std::vector<std::unique_ptr<Expression>> first_polynom_coeffs;
    // for (auto& itm : old_result.first) {
    //     first_polynom_coeffs.emplace_back(itm->Copy());
    // }
    // std::println("copy complete");

    auto new_result_quotient = Polynomial(old_result.first);
    std::println("new result quotient: {}", new_result_quotient.Accept(serializer).value());
    auto new_result_remainder = Polynomial(old_result.second);
    // std::unique_ptr<Expression> new_result_remainder = new_result_quotient->Copy();
    // std::println("size(new_result_quotient) = {}, size(new_result_remainder) = {}", ne)
    std::println("new result remainder: {}", new_result_remainder.Accept(serializer).value());
    return std::make_pair<Polynomial, Polynomial>(new_result_quotient.Copy(), new_result_remainder.Copy());
    // return std::make_pair(coeffs_to_polynomial(old_result.first), coeffs_to_polynomial(old_result.second));
}

auto poly_gcd_list(std::vector<std::unique_ptr<Expression>>& a, std::vector<std::unique_ptr<Expression>>& b) -> std::vector<std::unique_ptr<Expression>>
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

auto Polynomial::degree() const -> int
{
    return std::max(static_cast<int>(get_coefficients().size()) - 1, 0);
}

auto poly_gcd(Polynomial& a, Polynomial& b) -> Polynomial
{
    SimplifyVisitor simplify_visitor {};
    InFixSerializer serializer {};
    std::println("poly_gcd({}, {}) called!", a.Accept(serializer).value(), b.Accept(serializer).value());
    if (b.Is<Real>()) {
        std::println("B is real. ({})", RecursiveCast<Real>(*b.GetExpression())->GetValue());
        // if (std::abs(RecursiveCast<Real>(*b.GetExpression())->GetValue()) < std::numeric_limits<float>::epsilon()) {
        if (RecursiveCast<Real>(*b.GetExpression())->GetValue() == 0) {
            std::println("Base case reached1! Terminating!");
            // return std::move(a);
            return a.Copy();
        }
    }


    // std::println("a = {}, b = {}", a->Accept(serializer).value(), b->Accept(serializer).value());
    auto [fst, snd] = synthetic_pseudo_divide(a, b);
    std::println("synthetic pseudo divide complete.");

    // SimplifyVisitor simplify_visitor {};
    // auto res_second_simplified = res.second->Accept(simplify_visitor);
    auto rem = snd.primitive_part();
    auto rem_a = rem.GetExpression();
    std::println("primitive part complete.");

    // std::println("rem: <{}>, type: {}", rem.Accept(serializer).value(), rem.GetType() == ExpressionType::Add ? "Real" : "Polynomial" );
    // std::println("res.first: {}, res.second: {}, rem: {}", res.first->Accept(serializer).value(), res.second->Accept(serializer).value(), rem->Accept(serializer).value());
    if (rem_a->Is<Real>()) {
        std::println("res.second is real!");
        auto res_second_casted = RecursiveCast<Real>(*(rem.GetExpression()) );
        std::println("val of res.second: {}", res_second_casted->GetValue());

        if (RecursiveCast<Real>(*(rem.GetExpression()))->GetValue() == 0) {
            std::println("Base case reached! Terminating!");
            // Scale the resulting expression by the leading coefficient
            auto coeffs = b.get_coefficients();
            std::vector<std::unique_ptr<Expression>> scaled_coeffs;
            for (const auto& coeff : coeffs) {
                scaled_coeffs.emplace_back((coeff / coeffs[0])->Accept(simplify_visitor).value());
            }
            auto monic_b = Polynomial(scaled_coeffs);
            return monic_b;

            // below works, but doesn't return monic
            // return std::move(b);
        }
    } else {
        std::println("remainder is not real!");
    }
    // std::println("above works");

    auto rem_copy = Polynomial(rem);
    // std::unique_ptr<std::unique_ptr<Expression>> rem1;
    // auto rem1 = std::move(rem);
    // prevent infinite recursion
    if (rem.degree() >= b.degree()) {
        std::println("deg(rem) >= deg(b)");
        auto rem1 = synthetic_divide(rem_copy, b).second;
        std::println("about to recurse poly_gcd 1!");
        return poly_gcd(b, rem1);
    }

    std::println("about to recurse poly_gcd!");
    return poly_gcd(b, rem);
}

auto yuns(Polynomial& f)
{
    SimplifyVisitor simplify_visitor {};
    // InFixSerializer serializer {};
    auto x = Oasis::Variable{"x"};
    auto f_diff = Polynomial(f.Differentiate(x));

    auto a0 = poly_gcd(f, f_diff);
    auto b1 = synthetic_divide(f, a0).first;
    auto c1 = synthetic_divide(f_diff, a0).first;
    auto b1_diff = b1.Differentiate(x);
    // auto d1 = c1 - b1_diff;
    // auto d1 = Subtract<Expression, Expression>(*c1, *b1_diff).Accept(simplify_visitor).value();
    auto d1 = c1 - b1_diff;

    // std::println("a0: {}", a0->Accept(serializer).value());
    // std::println("b1: {}", b1->Accept(serializer).value());
    // std::println("diff(b1): {}", b1_diff->Accept(serializer).value());
    // std::println("c1: {}", c1->Accept(serializer).value());
    // std::println("d1: {}", d1->Accept(serializer).value());

    int i = 1;
    std::list<Polynomial> a, b, c, d;
    a.emplace_back(std::move(a0));
    b.emplace_back(b1.Copy_P());
    b.emplace_back(std::move(b1));
    c.emplace_back(c1.Copy_P());
    c.emplace_back(std::move(c1));
    d.emplace_back(d1.Copy_P());
    d.emplace_back(std::move(d1));

    while (!b.back().Is<Real>() && b.back() != Real{1}.Copy()) {
        // remove this later - iteration limit
        if (i > 20) break;

        // std::println("YUNS: calling poly_gcd(a, b) inside loop w/ a = {}, b = {}", b.back()->Copy()->Accept(serializer).value(), d.back()->Copy()->Accept(serializer).value());
        std::println("getting a_i");
        auto a_i = poly_gcd(b.back(), d.back());
        std::println("getting b_i");
        auto b_i = synthetic_divide(b.back(), a_i).first;
        std::println("getting c_i");
        auto c_i = synthetic_divide(d.back(), a_i).first;
        std::println("getting d_i");
        auto d_i = c_i - b_i.Differentiate(x);

        // std::println("a[{}]: {}", i, a_i->Accept(serializer).value());
        // std::println("b[{}]: {}", i+1, b_i->Accept(serializer).value());
        // std::println("c[{}]: {}", i+1, c_i->Accept(serializer).value());
        // std::println("d[{}]: {}", i+1, d_i->Accept(serializer).value());

        a.emplace_back(std::move(a_i));
        b.emplace_back(std::move(b_i));
        c.emplace_back(std::move(c_i));
        d.emplace_back(std::move(d_i));
        i++;
    }
    std::println("completed {} iterations of yun's algorithm!", i);
    return a;
}

// Polynomial factorization using Yun's Algorithm.
// Major limitation: only works on square-free polynomials (e.g. (x+2)*(x+3)^2*(x+5)^3) )
auto Polynomial::factor(Polynomial& f) -> std::list<Polynomial>
{
    InFixSerializer serializer {};
    auto val = f.expr_->Accept(serializer).value();
    std::println("val: {}", val);
    std::list<std::list<Polynomial>> all_yuns_runs;
    auto result_list_1 = yuns(f);
    all_yuns_runs.emplace_back(std::move(result_list_1));
    // std::unique_ptr<Expression>& val = result_list_1.front();
    // while (all_factors.back().front()->Is<Add>() && RecursiveCast<Real>(*(all_factors.back().back()))->GetValue() == 1) {
    while (all_yuns_runs.back().front().degree() > 1) {
        all_yuns_runs.emplace_back(std::move(yuns(all_yuns_runs.back().front())));
    }

    // InFixSerializer serializer;
    // int i = 0;
    // for (const auto& fact : all_yuns_runs) {
    //     std::println("yuns run {}:", i);
    //     for (const auto& expptr : fact) {
    //         std::println("    {}", expptr->Accept(serializer).value());
    //     }
    //     i++;
    // }
    // return all_factors;
    // std::println("val: {}", val->Accept(InFixSerializer{}).value());
    // while (val )
    std::list<Polynomial> res;
    for (auto it = all_yuns_runs.begin(); it != all_yuns_runs.end(); ++it) {
        // Only the last run of yuns algorithm provides a full list of factors.
        // Previous runs will contain a partially factored expression as the first list element
        if (std::next(it) == all_yuns_runs.end()) res.emplace_back(std::move(*it->begin()));
        for (auto it1 = std::next(it->begin()); it1 != it->end(); ++it1) {
            res.emplace_back(it1->Copy_P());
        }
    }

    return res;
}

auto Polynomial::factor_l(Polynomial& f) -> std::unique_ptr<Expression>
{
    auto factored = factor(f);
    std::vector<std::unique_ptr<Expression>> factors;
    std::transform(factored.begin(), factored.end(), std::back_inserter(factors), [](const auto& p) { return p.GetExpression(); });

    SimplifyVisitor simplify_visitor {};
    std::unique_ptr<Expression> recombined = Oasis::BuildFromVector<Multiply>(factors)->Accept(simplify_visitor).value();
    return recombined;
}

auto positive_rep(Polynomial& poly, unsigned int modulus)
{
    // coeffs = [int(c) % p for c in poly.all_coeffs()]
    // return Poly.from_list(coeffs, gens=poly.gens)
    auto coeffs = poly.get_coefficients();
    std::vector<std::unique_ptr<Expression>> coeffs_mod;
    for (const auto& coeff : coeffs) {
        coeffs_mod.emplace_back(Real{ static_cast<double>(static_cast<long>(RecursiveCast<Real>(*coeff)->GetValue()) % modulus) }.Copy());
    }
    return Polynomial(coeffs_mod);
}



auto random_poly_mod(int q, int n)
{
    // coeffs = [random.randrange(q) for _ in range(n)]
    // return Poly(coeffs, x, modulus=q)
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(42); // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(0, q-1);

    // // Use distrib to transform the random unsigned int
    // // generated by gen into an int in [1, 6]
    // for (int n = 0; n != 10; ++n)
    //     std::cout << distrib(gen) << ' ';
    // std::cout << '\n';
    // std::array<std::unique_ptr<Expression>, n> coeffs;
    std::vector<std::unique_ptr<Expression>> coeffs;
    coeffs.reserve(n);
    for (int i = 0; i < n; ++i) {
        coeffs.emplace_back(Real{ static_cast<double>(distrib(gen)) }.Copy());
    }
    return Polynomial(coeffs);
}

// fixes problem with negative modulo positive being negative
// ex. -2 % 7 = -2, but it should be 5
int mod(int a, int b) {
    return ((a % b) + b) % b;
}

// only works with integer coefficients
auto poly_mod(Polynomial& p, int modulus) -> Polynomial
{
    InFixSerializer serializer {};
    auto coeffs = p.get_coefficients();
    std::vector<std::unique_ptr<Expression>> coeffs_mod;
    for (const auto& coeff : coeffs) {
        auto t = RecursiveCast<Real>(*coeff)->GetValue();
        // std::println("coeff: {} == {}", t, static_cast<long>(t) % modulus);
        // coeffs_mod.emplace_back(Real{ static_cast<double>(static_cast<long>(RecursiveCast<Real>(*coeff)->GetValue()) % modulus) }.Copy());
        coeffs_mod.emplace_back(Real{ static_cast<double>(mod(static_cast<long>(t), modulus)) }.Copy());
    }
    return Polynomial {coeffs_mod};
}

auto poly_pow_mod(Polynomial& base, int exp, Polynomial& mod_poly, int modulus) -> Polynomial
{
    // Compute base^exp mod mod_poly using repeated squaring.
    Polynomial result = Polynomial{1};
    base = synthetic_divide(base, mod_poly).second;

    InFixSerializer ser {};
    while (exp > 0) {
        std::println("result: {}\nbase: {}\nexp: {}\n", result.Accept(ser).value(), base.Accept(ser).value(), exp);
        if (exp & 1) {
            auto tmp = result * base;
            auto tmp3 = tmp.expand();
            result = synthetic_divide(tmp3, mod_poly).second;
            result = poly_mod(result, modulus);
        }
        auto tmp2 = base * base;
        auto tmp4 = tmp2.expand();
        // auto mm = Polynomial { modulus };
        // auto tmp5 = synthetic_divide(tmp4, mm).second;
        auto tmp5 = poly_mod(tmp4, modulus);
        base = synthetic_divide(tmp5, mod_poly).second;
        base = poly_mod(base, modulus);
        std::println("before mod {}: {}\nafter mod {}: {}\nafter mod {}: {}", modulus, tmp4.Accept(ser).value(), modulus, tmp5.Accept(ser).value(), mod_poly.Accept(ser).value(), base.Accept(ser).value());
        exp >>= 1;
    }

    return result;
}

constexpr int IntPower(const int x, const int p)
{
    if (p == 0) return 1;
    if (p == 1) return x;

    int tmp = IntPower(x, p/2);
    if ((p % 2) == 0) return tmp * tmp;
    return x * tmp * tmp;
}

auto Polynomial::cantor_zassenhaus_equal_degree(Polynomial& f, int d, int q) -> std::vector<Polynomial>
{
    InFixSerializer serializer {};
    int n = f.degree();

    if (q % 2 == 0)
        throw std::runtime_error("Field order q must be odd.");

    int r = n / d;
    std::vector<Polynomial> factors = {f};
    auto exp = (IntPower(q, d) - 1) / 2;

    while (factors.size() < r) {
        auto h = random_poly_mod(q, n);

        auto g = poly_pow_mod(h, exp, f, q) - Polynomial {1};
        g = synthetic_divide(g, f).second;

        std::println("h = {}, g = {}, len(factors) = {}", h.Accept(serializer).value(), g.Accept(serializer).value(), factors.size());

        std::vector<Polynomial> new_factors;
        for (auto& u : factors) {
            if (u.degree() > d) {
                auto gu = poly_gcd(g, u);

                if (!(gu == Real{1}.Copy()) && gu.GetExpression() != u.GetExpression()) {
                    new_factors.emplace_back(gu.monic());
                    auto tmp = synthetic_divide(u, gu).first;
                    new_factors.emplace_back(tmp.monic());
                } else {
                    new_factors.emplace_back(u);
                }
            } else {
                new_factors.emplace_back(u);
            }
        }

        factors = new_factors;
    }

    // return factors
    std::vector<Polynomial> positive_rep;
    positive_rep.reserve(factors.size());

    for (auto& fac : factors) {
        positive_rep.push_back(poly_mod(fac, q));
    }
    return positive_rep;
}

auto Polynomial::AcceptInternal(Visitor& visitor) const -> any
{
    return expr_->AcceptAny(visitor);
}

} // Oasis
