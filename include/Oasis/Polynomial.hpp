//
// Created by Amaan Mahammad on 4/18/2026.
//

#ifndef OASIS_POLYNOMIAL_HPP
#define OASIS_POLYNOMIAL_HPP
#include "Expression.hpp"
#include "Real.hpp"

namespace Oasis {

class Polynomial : public Expression {
public:
    Polynomial() = default;

    // explicit Polynomial(const Expression* ex) : expr_(ex ? ex->Copy() : nullptr);
    // explicit Polynomial(const std::unique_ptr<Expression> ex) : expr_(std::move(ex));

    Polynomial(const Polynomial& other);
    Polynomial(const Expression& expression);
    Polynomial(const Expression* expression);
    // Polynomial(std::initializer_list<Oasis::Real> coeffs);
    Polynomial(std::initializer_list<int> coeffs);
    Polynomial(std::unique_ptr<Expression> expression);

    auto operator=(const Polynomial& other) -> Polynomial&;
    auto Copy() const -> std::unique_ptr<Expression>;
    auto Equals(const Expression& other) const -> bool;
    auto Generalize() const -> std::unique_ptr<Expression>;
    auto StructurallyEquivalent(const Expression& other) const -> bool;
    auto Substitute(const Expression& var, const Expression& val) -> std::unique_ptr<Expression>;
    auto GetExpression() const -> std::unique_ptr<Expression>;
    auto AcceptInternal(Visitor& visitor) const -> any;

    auto all_coeffs1(const std::unique_ptr<Oasis::Expression>& es) -> std::vector<std::unique_ptr<Oasis::Expression>>;

private:
    std::unique_ptr<Expression> expr_;
};

} // Oasis

#endif // OASIS_POLYNOMIAL_HPP
