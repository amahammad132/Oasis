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
auto modInversePrime(const long long a, const long long m) -> long long;
auto poly_gcd_galois(Polynomial& a_orig, Polynomial& b_orig, int q) -> Polynomial;
auto auto_choose_prime_for_hensel(Polynomial& f, int start, int stop) -> unsigned int;
auto distinct_degree_factor(Polynomial& f, int q) -> std::vector<std::pair<Polynomial, int>>;

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

    struct RationalCoefficient {
        long long numerator {0};
        long long denominator {1};
    };

    auto normalize_rational(RationalCoefficient value) -> RationalCoefficient
    {
        if (value.denominator == 0) {
            throw std::runtime_error("Polynomial coefficient has zero denominator.");
        }

        if (value.denominator < 0) {
            value.numerator *= -1;
            value.denominator *= -1;
        }

        const auto gcd_value = std::gcd(std::llabs(value.numerator), std::llabs(value.denominator));
        if (gcd_value == 0) {
            return {0, 1};
        }

        value.numerator /= gcd_value;
        value.denominator /= gcd_value;
        return value;
    }

    auto positive_mod(const long long value, const long long modulus) -> long long
    {
        if (modulus <= 0) {
            throw std::invalid_argument("Modulus must be positive.");
        }

        auto result = value % modulus;
        if (result < 0) {
            result += modulus;
        }
        return result;
    }

    auto trim_leading_zeros(std::vector<long long> coeffs) -> std::vector<long long>
    {
        auto first_non_zero = std::find_if(coeffs.begin(), coeffs.end(), [](const long long coefficient) {
            return coefficient != 0;
        });

        if (first_non_zero == coeffs.end()) {
            return {0};
        }

        coeffs.erase(coeffs.begin(), first_non_zero);
        return coeffs;
    }

    auto is_zero_polynomial(const std::vector<long long>& coeffs) -> bool
    {
        return std::all_of(coeffs.begin(), coeffs.end(), [](const long long coefficient) {
            return coefficient == 0;
        });
    }

    auto make_coefficient_expression(const RationalCoefficient coefficient) -> std::unique_ptr<Expression>
    {
        const auto normalized = normalize_rational(coefficient);
        if (normalized.denominator == 1) {
            return Real { static_cast<double>(normalized.numerator) }.Copy();
        }

        return Divide { Real { static_cast<double>(normalized.numerator) }, Real { static_cast<double>(normalized.denominator) } }.Copy();
    }

    auto rational_from_double(const double value, const long long max_denominator = 1'000'000) -> RationalCoefficient
    {
        if (!std::isfinite(value)) {
            throw std::runtime_error("Expected a finite polynomial coefficient.");
        }

        const auto nearest_integer = std::llround(value);
        if (std::abs(value - static_cast<double>(nearest_integer)) < 1e-12) {
            return {nearest_integer, 1};
        }

        const auto sign = value < 0 ? -1LL : 1LL;
        auto x = std::abs(value);

        long long h_prev_prev = 0;
        long long h_prev = 1;
        long long k_prev_prev = 1;
        long long k_prev = 0;

        for (int iteration = 0; iteration < 64; ++iteration) {
            const auto integer_part = static_cast<long long>(std::floor(x));
            const auto h = integer_part * h_prev + h_prev_prev;
            const auto k = integer_part * k_prev + k_prev_prev;

            if (k > max_denominator) {
                break;
            }

            const auto approximation = static_cast<double>(h) / static_cast<double>(k);
            if (std::abs(approximation - std::abs(value)) < 1e-12) {
                return normalize_rational({sign * h, k});
            }

            const auto fractional_part = x - static_cast<double>(integer_part);
            if (fractional_part < 1e-15) {
                return normalize_rational({sign * h, k});
            }

            h_prev_prev = h_prev;
            h_prev = h;
            k_prev_prev = k_prev;
            k_prev = k;
            x = 1.0 / fractional_part;
        }

        throw std::runtime_error("Could not recover a rational coefficient from floating-point data.");
    }

    auto build_polynomial_from_integer_coefficients(const std::vector<long long>& coefficients) -> Polynomial
    {
        const auto trimmed = trim_leading_zeros(coefficients);
        std::vector<std::unique_ptr<Expression>> expressions;
        expressions.reserve(trimmed.size());
        for (const auto coefficient : trimmed) {
            expressions.emplace_back(Real { static_cast<double>(coefficient) }.Copy());
        }
        return Polynomial { expressions };
    }

    auto build_polynomial_from_rational_coefficients(const std::vector<RationalCoefficient>& coefficients) -> Polynomial
    {
        std::vector<std::unique_ptr<Expression>> expressions;
        expressions.reserve(coefficients.size());
        for (const auto coefficient : coefficients) {
            expressions.emplace_back(make_coefficient_expression(coefficient));
        }
        return Polynomial { expressions };
    }

    auto extract_rational_coefficient(const Expression& expression) -> RationalCoefficient
    {
        if (auto rational_value = RecursiveCast<Divide<Real>>(expression); rational_value != nullptr) {
            return normalize_rational({
                std::llround(rational_value->GetMostSigOp().GetValue()),
                std::llround(rational_value->GetLeastSigOp().GetValue())
            });
        }

        if (auto real_value = RecursiveCast<Real>(expression); real_value != nullptr) {
            return rational_from_double(real_value->GetValue());
        }

        SimplifyVisitor simplify_visitor {};
        const auto simplified = expression.Accept(simplify_visitor).value();

        if (auto rational_value = RecursiveCast<Divide<Real>>(*simplified); rational_value != nullptr) {
            return normalize_rational({
                std::llround(rational_value->GetMostSigOp().GetValue()),
                std::llround(rational_value->GetLeastSigOp().GetValue())
            });
        }

        if (auto real_value = RecursiveCast<Real>(*simplified); real_value != nullptr) {
            return rational_from_double(real_value->GetValue());
        }

        throw std::runtime_error("Expected a polynomial with rational coefficients.");
    }

    auto rational_coefficients(const Polynomial& polynomial) -> std::vector<RationalCoefficient>
    {
        const auto coefficients = polynomial.get_coefficients();
        std::vector<RationalCoefficient> result;
        result.reserve(coefficients.size());
        for (const auto& coefficient : coefficients) {
            result.push_back(extract_rational_coefficient(*coefficient));
        }
        return result;
    }

    auto integer_coefficients(const Polynomial& polynomial) -> std::vector<long long>
    {
        const auto coefficients = rational_coefficients(polynomial);
        std::vector<long long> result;
        result.reserve(coefficients.size());
        for (const auto coefficient : coefficients) {
            if (coefficient.denominator != 1) {
                throw std::runtime_error("Expected integer polynomial coefficients.");
            }
            result.push_back(coefficient.numerator);
        }
        return trim_leading_zeros(result);
    }

    auto scale_polynomial_by_ground(const Polynomial& polynomial, const long long factor) -> Polynomial
    {
        auto coefficients = integer_coefficients(polynomial);
        for (auto& coefficient : coefficients) {
            coefficient *= factor;
        }
        return build_polynomial_from_integer_coefficients(coefficients);
    }

    auto divide_polynomial_by_ground(const Polynomial& polynomial, const long long divisor) -> Polynomial
    {
        if (divisor == 0) {
            throw std::invalid_argument("Cannot divide polynomial by zero.");
        }

        auto coefficients = integer_coefficients(polynomial);
        for (auto& coefficient : coefficients) {
            if (coefficient % divisor != 0) {
                throw std::runtime_error("Polynomial coefficients are not divisible by the requested integer.");
            }
            coefficient /= divisor;
        }
        return build_polynomial_from_integer_coefficients(coefficients);
    }

    auto scale_polynomial_by_rational(const Polynomial& polynomial, const long long numerator, const long long denominator) -> Polynomial
    {
        const auto normalized_scalar = normalize_rational({numerator, denominator});
        const auto coefficients = rational_coefficients(polynomial);

        std::vector<RationalCoefficient> scaled;
        scaled.reserve(coefficients.size());
        for (const auto coefficient : coefficients) {
            scaled.push_back(normalize_rational({
                coefficient.numerator * normalized_scalar.numerator,
                coefficient.denominator * normalized_scalar.denominator
            }));
        }
        return build_polynomial_from_rational_coefficients(scaled);
    }

    auto polynomials_have_same_integer_coefficients(const Polynomial& lhs, const Polynomial& rhs) -> bool
    {
        return integer_coefficients(lhs) == integer_coefficients(rhs);
    }

    auto polynomial_is_zero(const Polynomial& polynomial) -> bool
    {
        return is_zero_polynomial(integer_coefficients(polynomial));
    }

    auto polynomial_is_one_mod_p(const Polynomial& polynomial, const int modulus) -> bool
    {
        const auto coefficients = integer_coefficients(mod_poly_positive(polynomial, modulus));
        return coefficients.size() == 1 && positive_mod(coefficients.front(), modulus) == 1;
    }

    auto monic_mod_prime_coefficients(std::vector<long long> coefficients, const int modulus) -> std::vector<long long>
    {
        coefficients = trim_leading_zeros(std::move(coefficients));
        if (is_zero_polynomial(coefficients)) {
            return coefficients;
        }

        const auto leading = positive_mod(coefficients.front(), modulus);
        const auto inverse = positive_mod(modInversePrime(leading, modulus), modulus);
        for (auto& coefficient : coefficients) {
            coefficient = positive_mod(coefficient * inverse, modulus);
        }
        return trim_leading_zeros(std::move(coefficients));
    }

    auto add_coefficients_mod(const std::vector<long long>& lhs, const std::vector<long long>& rhs, const int modulus) -> std::vector<long long>
    {
        const auto max_size = std::max(lhs.size(), rhs.size());
        std::vector<long long> result(max_size, 0);

        const auto lhs_offset = static_cast<int>(max_size - lhs.size());
        const auto rhs_offset = static_cast<int>(max_size - rhs.size());

        for (size_t i = 0; i < max_size; ++i) {
            long long coefficient = 0;
            if (i >= static_cast<size_t>(lhs_offset)) {
                coefficient += lhs[i - lhs_offset];
            }
            if (i >= static_cast<size_t>(rhs_offset)) {
                coefficient += rhs[i - rhs_offset];
            }
            result[i] = positive_mod(coefficient, modulus);
        }

        return trim_leading_zeros(std::move(result));
    }

    auto subtract_coefficients_mod(const std::vector<long long>& lhs, const std::vector<long long>& rhs, const int modulus) -> std::vector<long long>
    {
        const auto max_size = std::max(lhs.size(), rhs.size());
        std::vector<long long> result(max_size, 0);

        const auto lhs_offset = static_cast<int>(max_size - lhs.size());
        const auto rhs_offset = static_cast<int>(max_size - rhs.size());

        for (size_t i = 0; i < max_size; ++i) {
            long long coefficient = 0;
            if (i >= static_cast<size_t>(lhs_offset)) {
                coefficient += lhs[i - lhs_offset];
            }
            if (i >= static_cast<size_t>(rhs_offset)) {
                coefficient -= rhs[i - rhs_offset];
            }
            result[i] = positive_mod(coefficient, modulus);
        }

        return trim_leading_zeros(std::move(result));
    }

    auto multiply_coefficients_mod(const std::vector<long long>& lhs, const std::vector<long long>& rhs, const int modulus) -> std::vector<long long>
    {
        if (is_zero_polynomial(lhs) || is_zero_polynomial(rhs)) {
            return {0};
        }

        std::vector<long long> result(lhs.size() + rhs.size() - 1, 0);
        for (size_t i = 0; i < lhs.size(); ++i) {
            for (size_t j = 0; j < rhs.size(); ++j) {
                result[i + j] = positive_mod(result[i + j] + lhs[i] * rhs[j], modulus);
            }
        }

        return trim_leading_zeros(std::move(result));
    }

    auto divide_coefficients_mod(std::vector<long long> dividend, const std::vector<long long>& divisor, const int modulus)
        -> std::pair<std::vector<long long>, std::vector<long long>>
    {
        dividend = trim_leading_zeros(std::move(dividend));
        const auto trimmed_divisor = trim_leading_zeros(divisor);

        if (is_zero_polynomial(trimmed_divisor)) {
            throw std::invalid_argument("Cannot divide by the zero polynomial modulo p.");
        }

        if (is_zero_polynomial(dividend) || dividend.size() < trimmed_divisor.size()) {
            return {{0}, dividend};
        }

        std::vector<long long> quotient(dividend.size() - trimmed_divisor.size() + 1, 0);
        auto remainder = dividend;
        const auto divisor_inverse = positive_mod(modInversePrime(positive_mod(trimmed_divisor.front(), modulus), modulus), modulus);

        while (!is_zero_polynomial(remainder) && remainder.size() >= trimmed_divisor.size()) {
            const auto shift = static_cast<int>(remainder.size() - trimmed_divisor.size());
            const auto factor = positive_mod(remainder.front() * divisor_inverse, modulus);
            const auto quotient_index = static_cast<int>(quotient.size() - 1 - shift);
            quotient[quotient_index] = factor;

            for (size_t i = 0; i < trimmed_divisor.size(); ++i) {
                remainder[i] = positive_mod(remainder[i] - factor * trimmed_divisor[i], modulus);
            }
            remainder = trim_leading_zeros(std::move(remainder));
        }

        return {trim_leading_zeros(std::move(quotient)), trim_leading_zeros(std::move(remainder))};
    }

    auto coefficients_modulo_positive(const Polynomial& polynomial, const int modulus) -> std::vector<long long>
    {
        auto coefficients = integer_coefficients(polynomial);
        for (auto& coefficient : coefficients) {
            coefficient = positive_mod(coefficient, modulus);
        }
        return trim_leading_zeros(std::move(coefficients));
    }

    auto monic_modulo_polynomial(const Polynomial& polynomial, const int modulus) -> Polynomial
    {
        return build_polynomial_from_integer_coefficients(monic_mod_prime_coefficients(coefficients_modulo_positive(polynomial, modulus), modulus));
    }

    auto remainder_modulo_polynomial(const Polynomial& dividend, const Polynomial& divisor, const int modulus) -> Polynomial
    {
        const auto remainder = divide_coefficients_mod(
            coefficients_modulo_positive(dividend, modulus),
            coefficients_modulo_positive(divisor, modulus),
            modulus
        ).second;
        return build_polynomial_from_integer_coefficients(remainder);
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

            // fallback if vector is too small for elements to be added together
            // it's probably better to make a helper function for BuildFromVector<T>() that
            // works with edge cases (empty input and size 1 input), or change BuildFromVector
            // (if there are no side effects to that change)
            if (result.size() == 1) {
                auto rp = result[0]->Accept(simplify_visitor).value();
                return Polynomial(*rp);
            }
            if (result.empty()) {
                auto expr_simpl = expr_->Accept(simplify_visitor).value();
                return expr_simpl->Copy();
            }
            std::unique_ptr<Expression> combined = BuildFromVector<Add>(result);
            // std::println("expanded polynom: {}", combined->Accept(serializer).value());
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
                if (exponent_int > 1 && std::abs(exponent_double - exponent_int) < std::numeric_limits<float>::epsilon()) {
                    std::vector<std::unique_ptr<Expression>> result;
                    for (int i = 0; i < exponent_int; i++) {
                        result.emplace_back(base->Copy());
                    }
                    // std::println("result.size() = {}", result.size());
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

    // int result = static_cast<int>(RecursiveCast<Real>(*numbers.front())->GetValue());
    // for (auto it = std::next(numbers.begin()); it != numbers.end(); ++it) {
    //     result = std::gcd(result, static_cast<int>(RecursiveCast<Real>(**it)->GetValue()));
    // }
    // return result;

    double result_a = RecursiveCast<Real>(*numbers.front())->GetValue();
    auto result = static_cast<long long>(result_a);
    std::println("first number: {} -> {}", result_a, result);
    for (auto it = std::next(numbers.begin()); it != numbers.end(); ++it) {
        auto va = RecursiveCast<Real>(**it)->GetValue();
        auto vb =  static_cast<long long>(va);
        std::println("double to long {} -> {}", va, vb);
        result = std::gcd(result, vb);
    }
    return static_cast<int>(result);
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
    std::println("PRIMITIVE_PART of {}: coeffs size: {}, coeffs: {}", expr_->Accept(serializer).value(), coeffs.size(), print_expr_list(coeffs));
    std::vector<std::unique_ptr<Expression>> new_coeffs;
    new_coeffs.reserve(coeffs.size());

    // auto gcd = (Real { static_cast<double>(multi_gcd(coeffs)) }).Copy();
    int gcd = multi_gcd(coeffs);
    std::println("gcd val: {}", gcd);

    // prevent a divide by zero if the GCD is zero
    if (gcd == 0) return expr_->Copy();

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
        std::println("out added: {}", d->Accept(serializer).value());
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
    std::println("sep: {}", sep);
    for (int i = 0; i < sep; i++) {
        quotient.emplace_back(out[i]->Accept(simplify_visitor).value());
        // auto out_i_copy = out[i]->Copy();
        // auto tmp1 = out_i_copy->Accept(simplify_visitor).value();
        // quotient.emplace_back(tmp1.get());
        // std::println("adding a quotient.");
    }
    for (int i = std::max(0, sep); i < out.size(); ++i) {
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
    std::println("synthetic_didvide({}, {})", dividend.Accept(serializer).value(), divisor.Accept(serializer).value() );

    auto ex_1 = dividend.expand();
    std::println("ex_1: {}", ex_1.Accept(serializer).value());
    auto ex_2 = ex_1.get_coefficients();
    std::println("ex_2: {}", print_expr_list(ex_2));

    auto dividend_coeffs = dividend.expand().get_coefficients();
    std::println("dividend_coeffs: {}", print_expr_list(dividend_coeffs));
    auto divisor_coeffs = divisor.expand().get_coefficients();
    std::println("divisor_coeffs: {}", print_expr_list(divisor_coeffs));
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

    // auto new_result_quotient = old_result.first.empty()
    //     ? build_polynomial_from_integer_coefficients({0})
    //     : Polynomial(old_result.first);
    auto new_result_quotient = Polynomial(old_result.first);
    // std::println("new result quotient: {}", new_result_quotient.Accept(serializer).value());
    // auto new_result_remainder = old_result.second.empty()
    //     ? build_polynomial_from_integer_coefficients({0})
    //     : Polynomial(old_result.second);
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

// returns base^exp (mod m)
long long power(long long base, long long exp, long long m) {
    long long res = 1;
    base %= m;
    while (exp > 0) {
        if (exp % 2 == 1) res = (res * base) % m;
        base = (base * base) % m;
        exp /= 2;
    }
    return res;
}

// constexpr int IntPower(const int x, const int p)

template<typename T, typename U>
constexpr T IntPower(const T x, const U p)
{
    if (p == 0) return 1;
    if (p == 1) return x;

    int tmp = IntPower(x, p/2);
    if ((p % 2) == 0) return tmp * tmp;
    return x * tmp * tmp;
}

// fixes problem with negative modulo positive being negative
// ex. -2 % 7 = -2, but it should be 5
// template<typename T, typename U>
// requires std::integral<T> && std::integral<U>
// T mod2(T a, U b) {
//     return ((a % b) + b) % b;
// }
//
// template<typename T, typename U>
// requires std::integral<T> && std::integral<U>
// T mod1(T a, U b) {
//     auto mod_1 = ((a % b) + b) % b;
//     auto mod_2 = a % b;
//
//     std::println("mod1({}, {}) => {}, {}", a, b, mod_1, mod_2);
//     if (std::abs(mod_1) < std::abs(mod_2)) return mod_1;
//     return mod_2;
// }

// returns a value such that a * modInversePrime(a, m) = 1 (mod m)
long long modInversePrimeOld(const long long a, long long m) {
    auto tmp = power(a, m - 2, m);
    std::println("modInversePrime({}, {}) => {}", a, m, tmp);
    return tmp;
}

template<typename T, typename U>
requires std::integral<T> && std::integral<U>
T mod(T a, U b) {
    T rem = a % b;
    // Calculate the alternative remainder by moving toward the next/previous multiple
    T alt_rem = (rem > 0) ? (rem - std::abs(b)) : (rem + std::abs(b));

    // Return the one with the smaller absolute value
    if (std::abs(alt_rem) < std::abs(rem)) {
        return alt_rem;
    }
    return rem;
}

// returns a value such that a * modInversePrime(a, m) = 1 (mod m)
long long modInversePrime(const long long a, const long long m) {
    auto tmp = mod(IntPower(a, m - 2), m);
    std::println("modInversePrime({}, {}) => {}", a, m, tmp);
    return tmp;
}

auto polynomial_content(const Polynomial& polynomial) -> long long
{
    const auto coefficients = integer_coefficients(polynomial);
    long long content = 0;
    for (const auto coefficient : coefficients) {
        content = std::gcd(content, std::llabs(coefficient));
    }
    return content;
}

auto primitive_part_with_content(const Polynomial& polynomial) -> std::pair<long long, Polynomial>
{
    const auto content = polynomial_content(polynomial);
    if (content == 0) {
        return {0, polynomial.Copy_P()};
    }
    return {content, divide_polynomial_by_ground(polynomial, content)};
}

auto clear_denominators(const Polynomial& polynomial) -> std::pair<long long, Polynomial>
{
    const auto coefficients = rational_coefficients(polynomial);
    long long denominator_lcm = 1;
    for (const auto coefficient : coefficients) {
        denominator_lcm = std::lcm(denominator_lcm, coefficient.denominator);
    }

    std::vector<long long> cleared;
    cleared.reserve(coefficients.size());
    for (const auto coefficient : coefficients) {
        cleared.push_back(coefficient.numerator * (denominator_lcm / coefficient.denominator));
    }

    return {denominator_lcm, build_polynomial_from_integer_coefficients(cleared)};
}

auto poly_mod(const Polynomial& p, int modulus) -> Polynomial
{
    InFixSerializer serializer;
    std::println("poly_mod({}, {})", p.Accept(serializer).value(), modulus);
    auto coeffs = p.get_coefficients();
    std::vector<std::unique_ptr<Expression>> coeffs_mod;
    coeffs_mod.reserve(coeffs.size());
    for (const auto& coeff : coeffs) {
        std::println("coeff in poly_mod: {}", coeff->Accept(serializer).value());
        const auto value = std::llround(RecursiveCast<Real>(*coeff)->GetValue());
        coeffs_mod.emplace_back(Real { static_cast<double>(mod(value, modulus)) }.Copy());
    }
    return Polynomial { coeffs_mod };
}

auto poly_mod(Polynomial& p, int modulus) -> Polynomial
{
    return poly_mod(static_cast<const Polynomial&>(p), modulus);
}

auto poly_mod_orig(Polynomial& p, int modulus) -> Polynomial
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

auto lift_factor_from_fp_to_zz(const Polynomial& g_fp, int p) -> Polynomial
{
    return build_polynomial_from_integer_coefficients(coefficients_modulo_positive(g_fp, p));
}

auto mod_poly_positive(const Polynomial& g, int modulus) -> Polynomial
{
    return build_polynomial_from_integer_coefficients(coefficients_modulo_positive(g, modulus));
}

auto extended_gcd_poly_mod_p(const Polynomial& a, const Polynomial& b, int p) -> std::tuple<Polynomial, Polynomial, Polynomial>
{
    auto A = coefficients_modulo_positive(a, p);
    auto B = coefficients_modulo_positive(b, p);

    std::vector<long long> s0 {1}, s1 {0};
    std::vector<long long> t0 {0}, t1 {1};

    while (!is_zero_polynomial(B)) {
        const auto [q, r] = divide_coefficients_mod(A, B, p);
        A = B;
        B = r;

        const auto next_s = subtract_coefficients_mod(s0, multiply_coefficients_mod(q, s1, p), p);
        const auto next_t = subtract_coefficients_mod(t0, multiply_coefficients_mod(q, t1, p), p);
        s0 = s1;
        s1 = next_s;
        t0 = t1;
        t1 = next_t;
    }

    if (is_zero_polynomial(A)) {
        throw std::runtime_error("Unexpected zero gcd in extended polynomial gcd modulo p.");
    }

    const auto inverse_lc = positive_mod(modInversePrime(positive_mod(A.front(), p), p), p);
    for (auto& coefficient : s0) {
        coefficient = positive_mod(coefficient * inverse_lc, p);
    }
    for (auto& coefficient : t0) {
        coefficient = positive_mod(coefficient * inverse_lc, p);
    }
    for (auto& coefficient : A) {
        coefficient = positive_mod(coefficient * inverse_lc, p);
    }

    return {
        build_polynomial_from_integer_coefficients(trim_leading_zeros(std::move(s0))),
        build_polynomial_from_integer_coefficients(trim_leading_zeros(std::move(t0))),
        build_polynomial_from_integer_coefficients(trim_leading_zeros(std::move(A)))
    };
}

auto multiply_polynomials(const std::vector<Polynomial>& polys, std::optional<int> modulus) -> Polynomial
{
    Polynomial result {1};
    for (const auto& polynomial : polys) {
        result = (result * polynomial).expand();
        if (modulus.has_value()) {
            result = mod_poly_positive(result, *modulus);
        }
    }
    return result;
}

auto hensel_lift_pair(const Polynomial& f_int, const Polynomial& a0, const Polynomial& b0, int p, int k_target)
    -> std::pair<Polynomial, Polynomial>
{
    auto a = lift_factor_from_fp_to_zz(monic_modulo_polynomial(a0, p), p);
    auto b = lift_factor_from_fp_to_zz(monic_modulo_polynomial(b0, p), p);

    auto [s, t, g] = extended_gcd_poly_mod_p(a, b, p);
    if (g.degree() != 0 || !polynomial_is_one_mod_p(g, p)) {
        throw std::runtime_error("Factors are not coprime modulo p; cannot Hensel lift.");
    }

    s = lift_factor_from_fp_to_zz(s, p);
    t = lift_factor_from_fp_to_zz(t, p);

    long long m = p;
    for (int current_k = 1; current_k < k_target; ++current_k) {
        auto product = (a * b).expand();
        auto diff = (f_int - product).expand();
        auto diff_mod = mod_poly_positive(diff, static_cast<int>(m * p));
        auto e = divide_polynomial_by_ground(diff_mod, m);

        const auto e_fp = mod_poly_positive(e, p);
        const auto da_fp = remainder_modulo_polynomial(
            multiply_polynomials({mod_poly_positive(t, p), e_fp}, p),
            mod_poly_positive(a, p),
            p
        );
        const auto db_fp = remainder_modulo_polynomial(
            multiply_polynomials({mod_poly_positive(s, p), e_fp}, p),
            mod_poly_positive(b, p),
            p
        );

        const auto da = scale_polynomial_by_ground(lift_factor_from_fp_to_zz(da_fp, p), m);
        const auto db = scale_polynomial_by_ground(lift_factor_from_fp_to_zz(db_fp, p), m);

        a = mod_poly_positive((a + da).expand(), static_cast<int>(m * p));
        b = mod_poly_positive((b + db).expand(), static_cast<int>(m * p));
        m *= p;
    }

    return {a, b};
}

auto hensel_lift_list(const Polynomial& f_int, const std::vector<Polynomial>& fs_mod_p, int p, int k_target) -> std::vector<Polynomial>
{
    if (fs_mod_p.empty()) {
        return {};
    }

    const auto modulus = static_cast<int>(IntPower(p, k_target));
    if (fs_mod_p.size() == 1) {
        return {mod_poly_positive(f_int, modulus)};
    }

    std::vector<Polynomial> lifted;
    lifted.reserve(fs_mod_p.size());

    auto target = f_int.Copy_P();
    std::vector<Polynomial> remaining = fs_mod_p;

    while (remaining.size() > 1) {
        const auto a0 = monic_modulo_polynomial(remaining.front(), p);
        const std::vector<Polynomial> tail(remaining.begin() + 1, remaining.end());
        const auto b0 = monic_modulo_polynomial(multiply_polynomials(tail, p), p);
        auto [A, B] = hensel_lift_pair(target, a0, b0, p, k_target);
        lifted.push_back(mod_poly_positive(A, modulus));
        target = mod_poly_positive(B, modulus);
        remaining.erase(remaining.begin());
    }

    lifted.push_back(mod_poly_positive(target, modulus));
    return lifted;
}

auto center_polynomial_coefficients(const Polynomial& g, int modulus) -> Polynomial
{
    auto coefficients = coefficients_modulo_positive(g, modulus);
    for (auto& coefficient : coefficients) {
        if (coefficient > modulus / 2) {
            coefficient -= modulus;
        }
    }
    return build_polynomial_from_integer_coefficients(coefficients);
}

auto lift_factors_to_Q(const Polynomial& f_over_Q, const std::vector<Polynomial>& factors_over_fp, int p, std::optional<int> k)
    -> std::vector<Polynomial>
{
    const auto [denominator, f_over_z] = clear_denominators(f_over_Q);
    const auto [content, f_primitive] = primitive_part_with_content(f_over_z);

    if (f_primitive.degree() <= 0) {
        return {f_over_Q.Copy_P()};
    }

    if (factors_over_fp.empty()) {
        throw std::invalid_argument("Need at least one modular factor to lift.");
    }

    std::vector<Polynomial> modular_factors;
    modular_factors.reserve(factors_over_fp.size());
    for (const auto& factor : factors_over_fp) {
        modular_factors.push_back(monic_modulo_polynomial(factor, p));
    }

    const auto modular_product = monic_modulo_polynomial(multiply_polynomials(modular_factors, p), p);
    const auto primitive_mod_p = monic_modulo_polynomial(f_primitive, p);
    if (!polynomials_have_same_integer_coefficients(modular_product, primitive_mod_p)) {
        throw std::invalid_argument("Provided factors do not multiply to f mod p.");
    }

    if (!k.has_value()) {
        const auto coefficients = integer_coefficients(f_primitive);
        long long max_abs_coefficient = 0;
        for (const auto coefficient : coefficients) {
            max_abs_coefficient = std::max(max_abs_coefficient, std::llabs(coefficient));
        }

        const auto bound = std::max<long long>(2, (1LL << f_primitive.degree()) * max_abs_coefficient);
        long long pk = p;
        int lift_exponent = 1;
        while (pk <= 2 * bound) {
            pk *= p;
            ++lift_exponent;
        }
        k = lift_exponent;
    }

    auto lifted = hensel_lift_list(f_primitive, modular_factors, p, *k);
    const auto modulus = static_cast<int>(IntPower(p, *k));
    for (auto& factor : lifted) {
        factor = center_polynomial_coefficients(factor, modulus);
    }

    std::vector<Polynomial> exact_integer_factors;
    exact_integer_factors.reserve(lifted.size());
    auto remaining = f_primitive.Copy_P();
    for (const auto& factor : lifted) {
        auto [factor_content, primitive_factor] = primitive_part_with_content(factor);
        if (factor_content == 0) {
            primitive_factor = factor.Copy_P();
        }
        const auto normalized_factor = primitive_factor.monic();

        auto remaining_copy = remaining.Copy_P();
        auto normalized_copy = normalized_factor.Copy_P();
        auto [quotient, remainder] = synthetic_divide(remaining_copy, normalized_copy);
        if (!polynomial_is_zero(remainder)) {
            exact_integer_factors.push_back(factor);
            continue;
        }

        exact_integer_factors.push_back(normalized_factor);
        remaining = quotient.monic();
    }

    if (exact_integer_factors.empty()) {
        exact_integer_factors = std::move(lifted);
    }

    exact_integer_factors.front() = scale_polynomial_by_rational(exact_integer_factors.front(), content, denominator);
    return exact_integer_factors;
}

// find the monic form over the finite-field of order q (GF(q)), where q is a prime number
// assuming polynomial has integer coefficients
auto Polynomial::monic(int q) const -> Polynomial
{
    SimplifyVisitor simplify_visitor {};
    auto coeffs = get_coefficients();
    auto mod_inv = modInversePrime(static_cast<long>(RecursiveCast<Real>(*coeffs.front())->GetValue()), q);
    std::vector<std::unique_ptr<Expression>> scaled_coeffs;
    for (const auto& coeff : coeffs) {
        auto coeff_int = static_cast<int>(RecursiveCast<Real>(*coeff)->GetValue());
        auto new_coeff = mod(mod_inv * coeff_int, q);
        std::println("coeff {} => ({}*{})%{} => {}", coeff_int, mod_inv, coeff_int, q, new_coeff);
        scaled_coeffs.emplace_back(Real{static_cast<double>(new_coeff)}.Accept(simplify_visitor).value());
    }
    return Polynomial(scaled_coeffs);
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

    // auto new_result_quotient = old_result.first.empty()
    //     ? build_polynomial_from_integer_coefficients({0})
    //     : Polynomial(old_result.first);
    auto new_result_quotient = Polynomial(old_result.first);
    std::println("new result quotient: {}", new_result_quotient.Accept(serializer).value());
    // auto new_result_remainder = old_result.second.empty()
    //     ? build_polynomial_from_integer_coefficients({0})
    //     : Polynomial(old_result.second);
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

auto yuns(Polynomial& f) -> std::list<Polynomial>
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
        auto c_i = Polynomial {0};
        if (!polynomial_is_zero(d.back())) {
            c_i = synthetic_divide(d.back(), a_i).first;
        }
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

/*
auto yuns_factor_only(Polynomial& f) -> std::list<Polynomial>
{
    InFixSerializer serializer {};
    auto val = f.Accept(serializer).value();
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
} */

auto factor_mod_p(const Polynomial& f_over_Q, int p) -> std::vector<Polynomial>
{
    auto [denominator, f_over_z] = clear_denominators(f_over_Q);
    static_cast<void>(denominator);

    auto f_fp = poly_mod(f_over_z, p);
    if (polynomial_is_zero(f_fp)) {
        return {f_fp};
    }
    if (f_fp.degree() <= 0) {
        return {f_fp.monic(p)};
    }

    Variable x {"x"};
    auto f_fp_prime = f_fp.Differentiate(x);
    if (!(poly_gcd_galois(f_fp, f_fp_prime, p) == Real {1}.Copy())) {
        throw std::runtime_error(std::format("f mod {} is not squarefree; choose a different p.", p));
    }

    f_fp = f_fp.monic(p);
    auto blocks = distinct_degree_factor(f_fp, p);
    std::println("DDF complete!");

    InFixSerializer serializer {};
    std::vector<Polynomial> irreducibles;
    for (auto& [block, degree] : blocks) {
        auto block_monic = block.monic(p);
        std::println("running cantor zassenhaus on {} with degree {}, p = {}", block_monic.Accept(serializer).value(), degree, p);
        auto equal_degree_factors = Polynomial::cantor_zassenhaus_equal_degree(block_monic, degree, p);
        std::println("cantor zassenhaus iteration complete!");
        for (auto& factor : equal_degree_factors) {
            irreducibles.emplace_back(factor.monic(p));
        }
    }

    return irreducibles;
}

auto factor_over_fp_and_lift_to_Q(const Polynomial& f_over_Q, int start, int stop, std::optional<int> k)
    -> std::tuple<int, std::vector<Polynomial>, std::vector<Polynomial>>
{
    auto mutable_copy = f_over_Q.Copy_P();
    const auto p = static_cast<int>(auto_choose_prime_for_hensel(mutable_copy, start, stop));
    std::println("p: {}", p);
    auto factors_mod_p = factor_mod_p(f_over_Q, p);
    auto factors_over_Q = lift_factors_to_Q(f_over_Q, factors_mod_p, p, k);
    return {p, std::move(factors_mod_p), std::move(factors_over_Q)};
}

auto factor_squarefree_over_Q(const Polynomial& f_over_Q, int start, int stop, std::optional<int> k) -> std::vector<Polynomial>
{
    if (f_over_Q.degree() <= 1) {
        return {f_over_Q.Copy_P()};
    }

    auto [p, factors_mod_p, factors_over_Q] = factor_over_fp_and_lift_to_Q(f_over_Q, start, stop, k);
    static_cast<void>(p);
    static_cast<void>(factors_mod_p);
    return factors_over_Q;
}

// Polynomial factorization using Yun's Algorithm for squarefree decomposition,
// then distinct-degree factorization + Cantor-Zassenhaus + Hensel lifting for
// each squarefree block.
auto Polynomial::factor(Polynomial& f) -> std::list<Polynomial>
{
    InFixSerializer serializer {};
    auto squarefree_factors = yuns(f);
    std::println("yuns complete ({} squarefree factors)!", squarefree_factors.size() - 1);
    std::list<Polynomial> fully_factored;

    int i = 1;
    for (auto it = std::next(squarefree_factors.begin()); it != squarefree_factors.end(); ++it, ++i) {
        Polynomial& factor = *it;

        auto exponent = Real{ static_cast<double>(i) };
        if (factor.degree() <= 1) {
            fully_factored.emplace_back(Exponent{ *factor.GetExpression(), exponent });
            continue;
        }

        std::println("factor squarefree over Q: {}", factor.Accept(serializer).value());
        const auto lifted_factors = factor_squarefree_over_Q(factor);
        if (lifted_factors.empty()) {
            fully_factored.emplace_back(Exponent{ *factor.GetExpression(), exponent });
            continue;
        }

        for (const auto& lifted_factor : lifted_factors) {
            if (i == 1) {
                fully_factored.emplace_back(lifted_factor.Copy_P());
                continue;
            }

            fully_factored.emplace_back(Exponent{ *lifted_factor.GetExpression(), exponent });
        }
    }

    return fully_factored;
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

// Mock random device for testing w/ deterministic numbers
class MockRandomDevice {
public:
    using result_type = unsigned int;

    // Allows setting a sequence of numbers to return
    MockRandomDevice(std::vector<unsigned int> sequence) : seq(sequence) {}

    result_type operator()() {
        if (idx >= seq.size()) return 0; // Or wrap around
        return seq[idx++];
    }

private:
    std::vector<unsigned int> seq;
    size_t idx = 0;
};

template <typename RandomGen>
int get_random_choice(RandomGen& gen, int q) {
    return gen() % q;
}

auto random_poly_mod(int q, int n)
{
    // coeffs = [random.randrange(q) for _ in range(n)]
    // return Poly(coeffs, x, modulus=q)
    std::random_device rd;  // a seed source for the random number engine

    // test case 1 (initially did not factor degree-2 equal degree polynomial)
    // static std::mt19937 gen(15429); // mersenne_twister_engine seeded with rd()

    // test case 2
    static std::mt19937 gen(7116); // mersenne_twister_engine seeded with rd()
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
    std::println("q={}, n={}, coeffs=[{}]", q, n, print_expr_list(coeffs));
    return Polynomial(coeffs);
}

auto random_poly_mod_mock(int q, int n)
{

    static MockRandomDevice mock_dev({4, 0, 3, 1, 4, 5});
    // assert(get_random_choice(mock_dev) == 5 % 10);
    // // Use distrib to transform the random unsigned int
    // // generated by gen into an int in [1, 6]
    // for (int n = 0; n != 10; ++n)
    //     std::cout << distrib(gen) << ' ';
    // std::cout << '\n';
    // std::array<std::unique_ptr<Expression>, n> coeffs;
    std::vector<std::unique_ptr<Expression>> coeffs;
    coeffs.reserve(n);
    for (int i = 0; i < n; ++i) {
        coeffs.emplace_back(Real{ static_cast<double>(get_random_choice(mock_dev, q)) }.Copy());
    }
    std::println("q={}, n={}, coeffs=[{}]", q, n, print_expr_list(coeffs));
    return Polynomial(coeffs);
}

// find the polynomial greatest common divisor of a and b, over the finite field/galois field q
auto poly_gcd_galois(Polynomial& a_orig, Polynomial& b_orig, const int q) -> Polynomial
{
    auto a = poly_mod(a_orig, q);
    auto b = poly_mod(b_orig, q);
    // auto b = b_orig;

    SimplifyVisitor simplify_visitor {};
    InFixSerializer serializer {};
    std::println("poly_gcd_galois({}, {}) called!", a.Accept(serializer).value(), b.Accept(serializer).value());
    if (b.Is<Real>()) {
        std::println("B is real. ({})", RecursiveCast<Real>(*b.GetExpression())->GetValue());
        // if (std::abs(RecursiveCast<Real>(*b.GetExpression())->GetValue()) < std::numeric_limits<float>::epsilon()) {
        if (RecursiveCast<Real>(*b.GetExpression())->GetValue() == 0) {
            std::println("Base case reached1! Terminating!");
            // return std::move(a);
            // auto u = poly_mod(a, q);
            // return a.Copy();
            // return u;
            return a.monic(q);
        }
    }


    // std::println("a = {}, b = {}", a->Accept(serializer).value(), b->Accept(serializer).value());
    auto rem = synthetic_pseudo_divide(a, b).second;
    std::println("synthetic pseudo divide complete.");

    std::println("rem: {}", rem.Accept(serializer).value());
    rem = poly_mod(rem, q);

    std::println("rem: {}", rem.Accept(serializer).value());
    rem = rem.primitive_part();

    std::println("rem3: {}", rem.Accept(serializer).value());

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
            auto monic_b = b.monic(q);
            return monic_b;
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
        rem1 = poly_mod(rem1, q);
        std::println("about to recurse poly_gcd_galois 1!");
        return poly_gcd(b, rem1);
    }

    std::println("about to recurse poly_gcd_galois!");
    return poly_gcd_galois(b, rem, q);
}

auto poly_pow_mod(Polynomial base, int exp, Polynomial& mod_poly, int modulus) -> Polynomial
{
    InFixSerializer ser {};
    std::println("poly_pow_mod(base={}, exp={}, mod_poly={}, modulus={})", base.Accept(ser).value(), exp, mod_poly.Accept(ser).value(), modulus);
    // Compute base^exp % mod_poly using repeated squaring.
    Polynomial result = Polynomial{1};
    base = synthetic_divide(base, mod_poly).second;
    std::println("synthetic divide success!");


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
        std::println("base now: {}\n", base.Accept(ser).value());
        // std::println("before mod {}: {}\nafter mod {}: {}\nafter mod {}: {}", modulus, tmp4.Accept(ser).value(), modulus, tmp5.Accept(ser).value(), mod_poly.Accept(ser).value(), base.Accept(ser).value());
        exp >>= 1;
    }

    return result;
}

// auto prime_iter(int start, int stop)
// {
//     for (int i = start; i < stop; i++) {}
//     unsigned int p = boost::math::prime(99);
// }


// get the leading coefficient of a polynomial
auto Polynomial::LC() const -> std::unique_ptr<Expression>
{
    auto coeffs = get_coefficients();
    return std::move(coeffs.front());
}

// pick a good prime number for factoring a polynomial
// and using hensel lifting to return the polynomial to standard form
auto auto_choose_prime_for_hensel(Polynomial& f, int start, int stop) -> unsigned int
{
    InFixSerializer serializer {};
    std::println("auto_choose_prime_for_hensel({}, {}, {}) called!", start, stop, f.Accept(serializer).value());
    auto f_content = f.primitive_part();

    if (f_content.degree() <= 1)
        throw std::runtime_error("Need degree >= 2 to choose a Hensel prime.");

    const auto lc = static_cast<int>(RecursiveCast<Real>(*f_content.LC())->GetValue());

    const std::array<unsigned int, 100> primes = {
        2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
        73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
        179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
        283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
        419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541
    };

    int i = 0;
    while (primes[i] < start) i++;
    while (primes[i] <= stop) {
        auto p = primes[i];
        if (lc % p == 0)
            continue;

        auto f_fp = f_content;
        f_fp = poly_mod(f_fp, p);
        if (f_fp.degree() <= 0)
            continue;
        Variable x {"x"};
        auto f_fp_prime = f_fp.Differentiate(x);
        if (poly_gcd_galois(f_fp, f_fp_prime, p) == Real{1}.Copy())
            return p;
        i++;
    }

    throw std::runtime_error("No suitable prime found. Increase the stopping value.");
}

// factors a monic square-free polynomial in the Galois field/finite field F_q
// returns all pairs of factors (g, d) such that
//      f has an irreducible factor of degree d
//      g is the product of all monic irreducible factors of f of degree d
// ex. (x^3+8x^2+22x+21) = (x+3)(x^2+5x+7) -> x+3, x^2+5x+7
auto distinct_degree_factor(Polynomial& f, int q) -> std::vector<std::pair<Polynomial, int>>
{
    Variable x {"x"};
    InFixSerializer serializer {};

    int i = 1;
    std::vector<std::pair<Polynomial, int>> S;
    S.reserve(f.degree());
    // std::println("S size: {}", S.size());
    auto f_star = f;
    while (f_star.degree() >= 2*i) {
        Polynomial exp = Subtract{ Exponent{x, Real{static_cast<double>(IntPower(q, i))}}, x}.Copy();
        auto g = poly_gcd_galois(f_star, exp, q);
        if (g.GetExpression() != Real{1}.Copy()) {
            // std::println("g: {}", g.Accept(serializer).value());
            S.emplace_back(std::move(g), i);

            f_star = synthetic_divide(f_star, g).first;
            f_star = poly_mod(f_star, q);
        }
        i++;
    }
    if (f_star != Real{1}.Copy()) {
        S.emplace_back(std::move(f_star), f_star.degree());
        // std::println("S size: {}", S.size());
    }
    if (S.empty()) {
        // std::println("S is empty! emplacing again.");
        S.emplace_back(std::make_pair<Polynomial, int>(f.Copy(), 1));
        // std::println("S size: {}", S.size());
    }
    // std::println("S size: {}", S.size());
    return S;
}

auto Polynomial::cantor_zassenhaus_equal_degree(Polynomial& f, int d, int q) -> std::vector<Polynomial>
{
    InFixSerializer serializer {};

    auto f_q = poly_mod(f, q);
    int n = f.degree();

    if (q % 2 == 0)
        throw std::runtime_error("Field order q must be odd.");

    int r = n / d;
    std::vector<Polynomial> factors = {f_q};
    auto exp = (IntPower(q, d) - 1) / 2;

    while (factors.size() < r) {
        std::println("random_poly_mod({}, {})", q, n);
        auto h = random_poly_mod_mock(q, n);
        h = poly_mod(h, q);

        auto g = poly_pow_mod(h, exp, f_q, q) - Polynomial {1};
        std::println("g before mod q: {}", g.Accept(serializer).value());
        g = poly_mod(g, q);
        std::println("g after mod q: {}", g.Accept(serializer).value());
        g = synthetic_divide(g, f_q).second;
        std::println("g after mod f_q: {}", g.Accept(serializer).value());

        std::println("h={}, g={}, len(factors)={}", h.Accept(serializer).value(), g.Accept(serializer).value(), factors.size());

        std::vector<Polynomial> new_factors;
        for (auto& u : factors) {
            if (u.degree() > d) {
                auto gu = poly_gcd_galois(g, u, q);

                std::println("g={}, u={}, gu={}", g.Accept(serializer).value(), u.Accept(serializer).value(), gu.Accept(serializer).value());

                // if (!(gu == Real{1}.Copy()) && !gu.GetExpression()->Equals(*u.GetExpression())) {
                if (!(gu == Real{1}.Copy()) && gu.GetExpression() != u.GetExpression()) {
                    std::println("BRANCH A");
                    new_factors.emplace_back(gu.monic(q));
                    auto tmp = synthetic_divide(u, gu).first;
                    new_factors.emplace_back(tmp.monic(q));
                } else {
                    std::println("BRANCH B");
                    new_factors.emplace_back(u);
                }
            } else {
                std::println("BRANCH C");
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
