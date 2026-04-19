//
// Created by Amaan Mahammad on 4/18/2026.
//

#include "Oasis/Polynomial.hpp"

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

namespace Oasis {
namespace {

    auto CopyOrThrow(const Expression* expression) -> std::unique_ptr<Expression>
    {
        if (expression == nullptr) {
            throw std::invalid_argument("Polynomial requires a non-null expression");
        }

        return expression->Copy();
    }

} // namespace

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
        i--;
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
    return std::make_unique<Polynomial>(*this);
}

auto Polynomial::Equals(const Expression& other) const -> bool
{
    if (const auto* otherPolynomial = dynamic_cast<const Polynomial*>(&other); otherPolynomial != nullptr) {
        return expr_->Equals(otherPolynomial->GetExpression());
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
        return expr_->StructurallyEquivalent(otherPolynomial->GetExpression());
    }

    return expr_->StructurallyEquivalent(other);
}

auto Polynomial::Substitute(const Expression& var, const Expression& val) -> std::unique_ptr<Expression>
{
    return std::make_unique<Polynomial>(expr_->Substitute(var, val));
}

auto Polynomial::GetExpression() const -> std::unique_ptr<Expression>
{
    // return *expr_;
    return expr_->Copy();
}

auto Polynomial::all_coeffs1(const std::unique_ptr<Oasis::Expression>& es) -> std::vector<std::unique_ptr<Oasis::Expression>>
{
    using namespace Oasis;
    // Oasis::InFixSerializer ser;

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
        coefficents.push_back(negCoefficents[i - 1]->Simplify());
    }
    for (const std::unique_ptr<Expression>& i : posCoefficents) {
        coefficents.push_back(i->Simplify());
    }

    std::reverse(coefficents.begin(), coefficents.end());
    return coefficents;
}


auto Polynomial::AcceptInternal(Visitor& visitor) const -> any
{
    return expr_->AcceptAny(visitor);
}

} // Oasis
