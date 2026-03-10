#include <cmath>
#include <print>
#include <map>

#include <Oasis/Add.hpp>
#include <Oasis/Derivative.hpp>
#include <Oasis/DifferentiateVisitor.hpp>
#include <Oasis/Divide.hpp>
#include <Oasis/Exponent.hpp>
#include <Oasis/Integral.hpp>
#include <Oasis/Multiply.hpp>
#include <Oasis/RecursiveCast.hpp>
#include <Oasis/Subtract.hpp>
#include <Oasis/Variable.hpp>
#include "../io/include/Oasis/InFixSerializer.hpp"

std::vector<long long> getAllFactors(long long n)
{
    std::vector<long long> answer;
    for (long long i = 1; i * i <= n; i++) {
        if (n % i == 0) {
            answer.push_back(i);
        }
    }
    if (std::abs(n) != 1) {
        answer.push_back(std::abs(n));
    }
    return answer;
}

long long gcf(long long a, long long b)
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
}

auto expand_polynomial(const std::unique_ptr<Oasis::Expression>& polynom)
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


            // std::println("lhs # of items added: {}, rhs # of items added: {}", lhs.size(), rhs.size());
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

auto pfd_static(std::unique_ptr<Oasis::Expression> numerator, std::unique_ptr<Oasis::Expression> denominator) -> std::unique_ptr<Oasis::Expression>
{
    using namespace Oasis;

    Oasis::Variable x {"x"};

    Oasis::InFixSerializer serializer;

    auto opts = SimplifyOpts { .simplifyFractions = true };
    auto opts_eval = SimplifyOpts { .simplifyFractions = false };
    Oasis::SimplifyVisitor simplify_visitor { opts };
    Oasis::SimplifyVisitor eval_visitor { opts_eval };



    auto denominator_simplified = std::move(denominator->Accept(simplify_visitor)).value();
    auto denominator_zeroes = denominator_simplified->FindZeros();


    auto vis = denominator->Accept(simplify_visitor);
    auto vis2 = std::move(vis).value().get();
    auto vis3 = vis2->Accept(serializer);
    if (vis3.has_value())
        std::println("Expression has {} zeros: {}", denominator_zeroes.size(), vis3.value());

    auto coefficient = highest_order_coefficient(expand_polynomial(denominator_simplified->Copy()));
    auto coefficient_real = (coefficient->Is<Real>())? dynamic_cast<Oasis::Real&>(*coefficient) : Real { 1 };
    std::println("highest order coefficient: {}", coefficient->Accept(serializer).value());


    std::vector<std::unique_ptr<Oasis::Expression>> expressions;
    std::vector<std::unique_ptr<Divide<Real>>> all_zeros;


    auto actual_zeroes = std::vector<Oasis::Divide<Real>>();
    bool flag = false;
    for (auto& expr : denominator_zeroes) {
        auto root = Oasis::RecursiveCast<Oasis::Divide<Oasis::Expression>>(*expr);
        auto simplified_s = std::move(expr->Accept(simplify_visitor)).value();
        Subtract sum1exp {x, *simplified_s};
        auto sum1 = sum1exp.Copy();

        if (root != nullptr) {
            auto num_simplified = root->GetMostSigOp().Simplify();
            auto den_simplified = root->GetLeastSigOp().Simplify();

            if (num_simplified->Is<Oasis::Real>() && den_simplified->Is<Oasis::Real>()) {
                auto denominatorReal = dynamic_cast<Oasis::Real&>(*den_simplified).GetValue();
                auto d = std::llround(denominatorReal);

                if (!flag) {
                    auto si = coefficient_real.GetValue();
                    auto casted = std::llround(si);

                    if (std::abs(si - casted) < std::numeric_limits<float>::epsilon() && d % casted == 0) {
                        sum1 = Subtract(Multiply { coefficient_real, x }, Multiply { coefficient_real, *simplified_s }).Copy();
                        flag = true;
                    }
                }
            }
        }

        // if a function ax^2+bx+c has the zeros x = s and x = t, then the factored expression is a*(x-s)*(x-t)
        if (!flag) {
            sum1 = Subtract(Multiply { coefficient_real, x }, Multiply { coefficient_real, *simplified_s }).Copy();
        }


        expressions.emplace_back(sum1->Simplify());
        auto res1 = simplified_s->Accept(serializer);

        auto res2 = expr->Accept(serializer);
        if (res1.has_value() && res2.has_value()) {
            std::println("denominator zero: {} == {}", res2.value(), res1.value());
        }

        if (simplified_s->Is<Oasis::Real>()) {
            auto expr_real_cast = dynamic_cast<Oasis::Real&>(*simplified_s);
            all_zeros.emplace_back(std::make_unique<Divide<Real>>(expr_real_cast, Real{1}));
        } else {
            all_zeros.emplace_back(RecursiveCast<Divide<Real>>(*simplified_s));
        }
    }

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




    std::println("\n\n\n");


    char var = 'A';
    std::vector<std::unique_ptr<Expression>> final_pfd;
    int i = 0;
    for (auto& zero : all_zeros) {
        // print_expr(zero);
        auto& v = expressions[i];
        // print_expr(v);

        auto sub_lhs = numerator->Substitute(x, *zero);
        auto sub_rhs = expression_pfd->Substitute(x, *zero);

        // std::println("lhs ==> {} = {} <== rhs", sub_lhs->Accept(serializer).value(), sub_rhs->Accept(serializer).value());
        auto divd1a = Divide { *sub_rhs, Variable{std::string{ var }} };
        auto divd1a_s = divd1a.Accept(simplify_visitor).value();
        auto divda = Divide { *sub_lhs, *divd1a_s };
        auto divda_s = divda.Accept(simplify_visitor).value();
        auto divda_s2 = divda_s->Accept(simplify_visitor).value();

        /*
         * TODO: instead of evaluating the expression at this point (which may introduce error),
         * use simplify visitor (after improving fraction simplification)
         */
        auto divda_s2_eval = divda_s2->Accept(eval_visitor).value();

        // std::println("x = {} => {} = {}", zero->Accept(serializer).value(), var, divda_s2->Accept(serializer).value());
        final_pfd.emplace_back(std::make_unique<Divide<Expression>>(*divda_s2_eval, *v));

        var++;
        i++;
    }

    auto final_pfd_result = Oasis::BuildFromVector<Add>(final_pfd);
    // std::println("FINAL PFD RESULT: {}", final_pfd_result->Accept(serializer).value());
    return final_pfd_result;
}

namespace Oasis {
// currently only supports polynomials of one variable.
/**
 * The FindZeros function finds all rational zeros of a polynomial. Currently assumes an expression of the form a+bx+cx^2+dx^3+... where a, b, c, d are a integers.
 *
 * @tparam origonalExpresion The expression for which all the factors will be found.
 */
auto Expression::FindZeros() const -> std::vector<std::unique_ptr<Expression>>
{
    SimplifyVisitor simplifyVisitor {};

    std::vector<std::unique_ptr<Expression>> results;
    std::vector<std::unique_ptr<Expression>> termsE;
    if (auto addCase = RecursiveCast<Add<Expression>>(*this); addCase != nullptr) {
        addCase->Flatten(termsE);
    } else {
        termsE.push_back(Copy());
    }
    std::string varName = "";
    std::vector<std::unique_ptr<Expression>> posCoefficents;
    std::vector<std::unique_ptr<Expression>> negCoefficents;
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
        } else if (auto prodExpCase = RecursiveCast<Multiply<Expression, Exponent<Variable, Real>>>(*i); prodExpCase
            != nullptr) {
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
        long flooredExponent = std::lround(exponent);
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

    std::unique_ptr<Expression> s = this->Copy();
    auto var = Variable{varName};
    auto ex1192 = s->Substitute(var, Real{0});

    if (ex1192->Is<Oasis::Real>()) {
        auto simplifiedReal = dynamic_cast<Oasis::Real&>(*ex1192);
        if (simplifiedReal.Equals(Oasis::Real{0})) {
            Oasis::InFixSerializer serializer;

            auto simplifiedExpr = this->Copy() / std::make_unique<Variable>(var);
            auto res31 = simplifiedExpr->Accept(serializer);
            // if (res31.has_value())
            //     std::println("The expression evaluated with x=0 is 0, so using simplified expr: {}", simplifiedReal.GetValue(), res31.value());

            std::vector<std::unique_ptr<Expression>> rets = simplifiedExpr->FindZeros();
            rets.emplace_back(Oasis::Real{0}.Copy());
            return std::move(rets);
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
        coefficents.push_back(negCoefficents[i - 1]->Accept(simplifyVisitor).value());
    }
    for (const std::unique_ptr<Expression>& i : posCoefficents) {
        coefficents.push_back(i->Accept(simplifyVisitor).value());
    }
    if (coefficents.size() <= 1) {
        return {};
    }
    std::vector<long long> termsC;
    for (auto& i : coefficents) {
        auto realCase = RecursiveCast<Real>(*i);
        if (realCase == nullptr) {
            break;
        }
        double value = realCase->GetValue();
        if (value != std::round(value)) {
            break;
        } else {
            termsC.push_back(lround(value));
        }
    }
    if (termsC.size() == coefficents.size()) {
        std::reverse(termsC.begin(), termsC.end());
        for (auto pv : getAllFactors(termsC.back())) {
            for (auto qv : getAllFactors(termsC.front())) {
                if (gcf(pv, qv) == 1) {
                    for (long long sign : { -1, 1 }) {
                        bool doAdd = true;
                        while (true) {
                            long long mpv = pv * sign;
                            std::vector<long long> newTermsC;
                            long long h = 0;
                            for (long long i : termsC) {
                                h *= mpv;
                                if (h % qv != 0) {
                                    break;
                                }
                                h /= qv;
                                h += i;
                                newTermsC.push_back(h);
                            }
                            if (newTermsC.size() == termsC.size() && newTermsC.back() == 0) {
                                termsC = newTermsC;
                                if (doAdd) {
                                    results.push_back(
                                        std::make_unique<Divide<Real>>(Real(1.0 * mpv), Real(1.0 * qv)));
                                    doAdd = false;
                                }
                                do {
                                    termsC.pop_back();
                                } while (termsC.back() == 0);
                                if (termsC.size() <= 1) {
                                    break;
                                }
                            } else {
                                break;
                            }
                        }
                    }
                }
                if (termsC.size() <= 1) {
                    break;
                }
            }
            if (termsC.size() <= 1) {
                break;
            }
        }
        coefficents.clear();
        std::reverse(termsC.begin(), termsC.end());
        for (auto i : termsC) {
            coefficents.push_back(Real(i * 1.0).Copy());
        }
    }
    if (coefficents.size() == 2) {
        results.push_back(
            Divide(Multiply(Real(-1), *coefficents[0]), *coefficents[1]).Accept(simplifyVisitor).value());
    } else if (coefficents.size() == 3) {
        auto& a = coefficents[2];
        auto& b = coefficents[1];
        auto& c = coefficents[0];
        auto negB = Multiply(Real(-1.0), *b).Accept(simplifyVisitor).value();
        auto sqrt = Exponent(
            *Add(Multiply(*b, *b), Multiply(Real(-4), Multiply(*a, *c))).Accept(simplifyVisitor).value(),
            Divide(Real(1), Real(2)))
                        .Copy();
        auto twoA = Multiply(Real(2), *a).Accept(simplifyVisitor).value();
        results.push_back(Divide(Add(*negB, *sqrt), *twoA).Copy());
        results.push_back(Divide(Subtract(*negB, *sqrt), *twoA).Copy());
    }
    return results;
}


auto Expression::PartialFractionDecomp() const -> std::unique_ptr<Oasis::Expression> {
    if (this->Is<Divide<Expression>>()) {
        if (auto divideCase = RecursiveCast<Divide<Expression>>(*this); divideCase != nullptr) {
            return pfd_static(divideCase->GetMostSigOp().Generalize(), divideCase->GetLeastSigOp().Generalize());
        }
    }

    return this->Copy();
}

auto Expression::GetCategory() const -> uint32_t
{
    return 0;
}

auto Expression::Differentiate(const Expression& differentiationVariable) const -> std::unique_ptr<Expression>
{
    DifferentiateVisitor dv { differentiationVariable.Copy() };
    auto diffed = Accept(dv);
    if (!diffed) {
        return Derivative<Expression, Expression> { *(this->Copy()), differentiationVariable }.Generalize();
    }
    return std::move(diffed).value();
}

auto Expression::GetType() const -> ExpressionType
{
    return ExpressionType::None;
}

auto Expression::Generalize() const -> std::unique_ptr<Expression>
{
    return Copy();
}

auto Expression::Integrate(const Expression& variable) const -> std::unique_ptr<Expression>
{
    Integral<Expression, Expression> integral { *(this->Copy()), *(variable.Copy()) };

    return integral.Copy();
}

auto Expression::IntegrateWithBounds(const Expression& variable, const Expression&,
    const Expression&) -> std::unique_ptr<Expression>
{
    Integral<Expression, Expression> integral { *(this->Copy()), *(variable.Copy()) };

    return integral.Copy();
}

auto Expression::Simplify() const -> std::unique_ptr<Expression>
{
    SimplifyVisitor sV {};
    return std::move(Accept(sV)).value();
}
} // namespace Oasis
std::unique_ptr<Oasis::Expression> operator+(const std::unique_ptr<Oasis::Expression>& lhs,
    const std::unique_ptr<Oasis::Expression>& rhs)
{
    Oasis::SimplifyVisitor sV {};
    auto e = Oasis::Add { *lhs, *rhs };
    auto s = e.Accept(sV);
    if (!s) {
        return e.Generalize();
    }
    return std::move(s).value();
}

std::unique_ptr<Oasis::Expression> operator-(const std::unique_ptr<Oasis::Expression>& lhs,
    const std::unique_ptr<Oasis::Expression>& rhs)
{
    Oasis::SimplifyVisitor sV {};
    auto e = Oasis::Subtract { *lhs, *rhs };
    auto s = e.Accept(sV);
    if (!s) {
        return e.Generalize();
    }
    return std::move(s).value();
}

std::unique_ptr<Oasis::Expression> operator*(const std::unique_ptr<Oasis::Expression>& lhs,
    const std::unique_ptr<Oasis::Expression>& rhs)
{
    Oasis::SimplifyVisitor sV {};
    auto e = Oasis::Multiply { *lhs, *rhs };
    auto s = e.Accept(sV);
    if (!s) {
        return e.Generalize();
    }
    return std::move(s).value();
}

std::unique_ptr<Oasis::Expression> operator/(const std::unique_ptr<Oasis::Expression>& lhs,
    const std::unique_ptr<Oasis::Expression>& rhs)
{
    Oasis::SimplifyVisitor sV {};
    auto e = Oasis::Divide { *lhs, *rhs };
    auto s = e.Accept(sV);
    if (!s) {
        return e.Generalize();
    }
    return std::move(s).value();
}
