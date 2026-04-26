//
// Created by Amaan Mahammad on 4/18/2026.
//

#ifndef OASIS_POLYNOMIAL_HPP
#define OASIS_POLYNOMIAL_HPP
#include "Derivative.hpp"
#include "DifferentiateVisitor.hpp"
#include "Expression.hpp"
#include "Real.hpp"

#include <optional>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

namespace Oasis {

class Polynomial : public Expression {
public:
    Polynomial() = default;

    explicit Polynomial(std::unique_ptr<Expression>&& other);
    Polynomial(const Polynomial& other);
    Polynomial(const Expression& expression);
    Polynomial(const Expression* expression);
    explicit Polynomial(std::span<Real>&& coeffs);
    Polynomial(std::span<std::unique_ptr<Expression>>&& coeffs);
    Polynomial(std::initializer_list<int> coeffs);
    Polynomial(std::unique_ptr<Expression> expression);
    explicit Polynomial(const std::unique_ptr<Expression>& expression) : expr_(expression->Copy())
    {
    }

    // explicit Polynomial(const std::unique_ptr<Oasis::Expression>& unique);
    [[nodiscard]] auto Differentiate(const Expression& differentiationVariable) const -> Polynomial
    {
        DifferentiateVisitor dv { differentiationVariable.Copy() };
        auto diffed = Accept(dv);
        if (!diffed) {
            auto s = Derivative<Expression, Expression> { *(this->Copy()), differentiationVariable }.Generalize();
            return Polynomial { *s };
        }
        return std::move(diffed).value().get();
    }

    static auto factor(Polynomial& f) -> std::list<Polynomial>;
    static auto factor_l(Polynomial& f) -> std::unique_ptr<Expression>;
    auto operator=(const Polynomial& other) -> Polynomial&;
    [[nodiscard]] auto Copy() const -> std::unique_ptr<Expression> override;
    [[nodiscard]] auto Copy_P() const -> Polynomial;
    [[nodiscard]] auto Equals(const Expression& other) const -> bool override;
    [[nodiscard]] auto Generalize() const -> std::unique_ptr<Expression> override;
    [[nodiscard]] auto StructurallyEquivalent(const Expression& other) const -> bool override;
    auto Substitute(const Expression& var, const Expression& val) -> std::unique_ptr<Expression> override;
    [[nodiscard]] auto GetExpression() const -> std::unique_ptr<Expression>;
    [[nodiscard]] auto degree() const -> int;
    [[nodiscard]] auto monic() const -> Polynomial;
    [[nodiscard]] auto monic(int q) const -> Polynomial;


    bool operator==(const std::unique_ptr<Expression>& copy) const
    {
        return expr_->Equals(*copy);
    }

    // sketchy
    Polynomial operator+(const Polynomial& rhs) const
    {
        auto v = expr_ + rhs.expr_;
        return Polynomial {*v};
    }
    Polynomial operator-(const Polynomial& rhs) const
    {
        auto v = expr_ - rhs.expr_;
        return Polynomial {*v};
    }
    Polynomial operator*(const Polynomial& rhs) const
    {
        auto v = expr_ * rhs.expr_;
        return Polynomial {*v};
    }
    Polynomial operator/(const Polynomial& rhs) const
    {
        auto v = expr_ / rhs.expr_;
        return Polynomial {*v};
    }

    static auto cantor_zassenhaus_equal_degree(Polynomial& f, int d, int q) -> std::vector<Polynomial>;
    auto AcceptInternal(Visitor& visitor) const -> any override;

    template <IExpression T>
    [[nodiscard]] bool Is() const
    {
        return expr_->GetType() == T::GetStaticType();
    }

    template <template <typename> typename T>
        requires(DerivedFromUnaryExpression<T<Expression>> && !DerivedFromBinaryExpression<T<Expression>>)
    [[nodiscard]] bool Is() const
    {
        return expr_->GetType() == T<Expression>::GetStaticType();
    }

    template <template <typename, typename> typename T>
        requires DerivedFromBinaryExpression<T<Expression, Expression>>
    [[nodiscard]] bool Is() const
    {
        return expr_->GetType() == T<Expression, Expression>::GetStaticType();
    }

    template <IVisitor T>
    requires ExpectedWithString<typename T::RetT>
    auto Accept(T& visitor) const -> typename T::RetT
    {
        // return Polynomial(expr_->Accept(visitor));
        try {
            return boost::any_cast<typename T::RetT>(this->AcceptInternal(static_cast<Visitor&>(visitor)));
        } catch (boost::bad_any_cast& e) {
            return std::unexpected { e.what() };
        }
    }

    template <IVisitor T>
    auto Accept(T& visitor) const -> std::expected<typename T::RetT, std::string_view>
    {
        // return Polynomial(expr_->Accept(visitor));
        try {
            return boost::any_cast<typename T::RetT>(this->AcceptInternal(visitor));
        } catch (boost::bad_any_cast& e) {
            return std::unexpected { e.what() };
        }
    }

    auto get_coefficients() const -> std::vector<std::unique_ptr<Expression>>;
    auto LC() const -> std::unique_ptr<Expression>;
    auto expand() const -> Polynomial;
    Polynomial primitive_part() const;

private:
    std::unique_ptr<Expression> expr_;
};

std::pair<Polynomial, Polynomial> synthetic_divide(Polynomial& dividend, Polynomial& divisor);
auto poly_mod(const Polynomial& p, int modulus) -> Polynomial;
auto poly_mod(Polynomial& p, int modulus) -> Polynomial;
auto polynomial_content(const Polynomial& polynomial) -> long long;
auto primitive_part_with_content(const Polynomial& polynomial) -> std::pair<long long, Polynomial>;
auto clear_denominators(const Polynomial& polynomial) -> std::pair<long long, Polynomial>;
auto lift_factor_from_fp_to_zz(const Polynomial& g_fp, int p) -> Polynomial;
auto mod_poly_positive(const Polynomial& g, int modulus) -> Polynomial;
auto extended_gcd_poly_mod_p(const Polynomial& a, const Polynomial& b, int p) -> std::tuple<Polynomial, Polynomial, Polynomial>;
auto multiply_polynomials(const std::vector<Polynomial>& polys, std::optional<int> modulus = std::nullopt) -> Polynomial;
auto hensel_lift_pair(const Polynomial& f_int, const Polynomial& a0, const Polynomial& b0, int p, int k_target)
    -> std::pair<Polynomial, Polynomial>;
auto hensel_lift_list(const Polynomial& f_int, const std::vector<Polynomial>& fs_mod_p, int p, int k_target) -> std::vector<Polynomial>;
auto center_polynomial_coefficients(const Polynomial& g, int modulus) -> Polynomial;
auto lift_factors_to_Q(const Polynomial& f_over_Q, const std::vector<Polynomial>& factors_over_fp, int p,
                       std::optional<int> k = std::nullopt) -> std::vector<Polynomial>;
auto factor_mod_p(const Polynomial& f_over_Q, int p) -> std::vector<Polynomial>;
auto factor_over_fp_and_lift_to_Q(const Polynomial& f_over_Q, int start = 3, int stop = 200,
                                  std::optional<int> k = std::nullopt)
    -> std::tuple<int, std::vector<Polynomial>, std::vector<Polynomial>>;
auto factor_squarefree_over_Q(const Polynomial& f_over_Q, int start = 3, int stop = 200,
                              std::optional<int> k = std::nullopt) -> std::vector<Polynomial>;
auto auto_choose_prime_for_hensel(Polynomial& f, int start = 3, int stop = 200) -> unsigned int;
auto yuns(Polynomial& f) -> std::list<Polynomial>;

} // Oasis

#endif // OASIS_POLYNOMIAL_HPP
