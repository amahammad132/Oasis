//
// Created by Matthew McCall on 8/7/23.
//

#include "Common.hpp"

#include "catch2/catch_test_macros.hpp"

#include "Oasis/Add.hpp"
#include "Oasis/Divide.hpp"
#include "Oasis/Exponent.hpp"
#include "Oasis/Expression.hpp"
#include "Oasis/Imaginary.hpp"
#include "Oasis/Multiply.hpp"
#include "Oasis/Polynomial.hpp"
#include "Oasis/Real.hpp"
#include "Oasis/RecursiveCast.hpp"
#include "Oasis/Subtract.hpp"
#include "Oasis/Variable.hpp"
#include "catch2/internal/catch_list.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <set>
#include <tuple>
#include <vector>

namespace Oasis {

auto synthetic_divide(Polynomial& dividend, Polynomial& divisor) -> std::pair<Polynomial, Polynomial>;
auto synthetic_pseudo_divide(Polynomial& dividend, Polynomial& divisor) -> std::pair<Polynomial, Polynomial>;
auto poly_gcd(Polynomial& a, Polynomial& b) -> Polynomial;
auto poly_pow_mod(Polynomial base, int exp, Polynomial& mod_poly, int modulus) -> Polynomial;
auto distinct_degree_factor(Polynomial& f, int q) -> std::vector<std::pair<Polynomial, int>>;
auto poly_gcd_galois(Polynomial& a, Polynomial& b, const int q) -> Polynomial;
auto auto_choose_prime_for_hensel(Polynomial& f_over_Q, int start, int stop) -> unsigned int;

} // namespace Oasis

namespace {

auto RealValue(const Oasis::Expression& expression) -> long
{
    auto real = Oasis::RecursiveCast<Oasis::Real>(expression);
    if (real == nullptr) {
        throw std::runtime_error("Expected a real-valued coefficient");
    }

    return std::lround(real->GetValue());
}

auto CoefficientsAsIntegers(const std::vector<std::unique_ptr<Oasis::Expression>>& coefficients) -> std::vector<long>
{
    std::vector<long> values;
    values.reserve(coefficients.size());
    for (const auto& coefficient : coefficients) {
        values.push_back(RealValue(*coefficient));
    }
    return values;
}

auto CoefficientsAsIntegers(const Oasis::Polynomial& polynomial) -> std::vector<long>
{
    return CoefficientsAsIntegers(polynomial.get_coefficients());
}

} // namespace

// TODO: Figure out what's going out here
// TEST_CASE("7th degree polynomial with rational roots", "[factor][duplicateRoot]")
// {
//     std::vector<std::unique_ptr<Oasis::Expression>> vec;
//     long offset = -3;
//     std::vector<long> vecL = { 24750, -200'925, 573'625, -631'406, 79184, 247'799, -92631, 8820 };
//     for (size_t i = 0; i < vecL.size(); i++) {
//         Oasis::Real num = Oasis::Real(vecL[i]);
//         long exp = ((long)i) + offset;
//         if (exp < -1) {
//             vec.push_back(Oasis::Divide(num, Oasis::Exponent(Oasis::Variable("x"), Oasis::Real(-exp * 1.0))).Copy());
//         } else if (exp == -1) {
//             vec.push_back(Oasis::Divide(num, Oasis::Variable("x")).Copy());
//         } else if (exp == 0) {
//             vec.push_back(num.Copy());
//         } else if (exp == 1) {
//             vec.push_back(Oasis::Multiply(num, Oasis::Variable("x")).Copy());
//         } else {
//             vec.push_back(Oasis::Multiply(num, Oasis::Exponent(Oasis::Variable("x"), Oasis::Real(exp * 1.0))).Copy());
//         }
//     }
//     auto add = Oasis::BuildFromVector<Oasis::Add>(vec);
//     auto zeros = add->FindZeros();
//     REQUIRE(zeros.size() == 6);
//     std::set<std::tuple<long, long>> goalSet = { std::tuple(1, 3), std::tuple(6, 7), std::tuple(3, 7), std::tuple(-5, 3), std::tuple(11, 20), std::tuple(5, 1) };
//     for (auto& i : zeros) {
//         auto divideCase = Oasis::RecursiveCast<Oasis::Divide<Oasis::Real>>(*i);
//         REQUIRE(divideCase != nullptr);
//         std::tuple<long, long> asTuple = std::tuple(std::lround(divideCase->GetMostSigOp().GetValue()), std::lround(divideCase->GetLeastSigOp().GetValue()));
//         REQUIRE(goalSet.contains(asTuple));
//         goalSet.erase(asTuple);
//     }
// }

TEST_CASE("imaginary linear polynomial")
{
    Oasis::Add add {
        Oasis::Imaginary(),
        Oasis::Variable("x")
    };
    auto zeros = add.FindZeros();
    REQUIRE(zeros.size() == 1);
    if (zeros.size() == 1) {
        auto root = Oasis::RecursiveCast<Oasis::Multiply<Oasis::Real, Oasis::Imaginary>>(*zeros[0]);
        REQUIRE(root != nullptr);
        REQUIRE(root->GetMostSigOp().GetValue() == -1);
    }
}

// TODO: Figure out what's going out here
// TEST_CASE("irrational quadratic", "[quadraticFormula]")
// {
//     std::vector<std::unique_ptr<Oasis::Expression>> vec;
//     long offset = -3;
//     std::vector<long> vecL = { -1, 1, 1 };
//     for (size_t i = 0; i < vecL.size(); i++) {
//         Oasis::Real num = Oasis::Real(vecL[i]);
//         long exp = ((long)i) + offset;
//         if (exp < -1) {
//             vec.push_back(Oasis::Divide(num, Oasis::Exponent(Oasis::Variable("x"), Oasis::Real(-exp))).Copy());
//         } else if (exp == -1) {
//             vec.push_back(Oasis::Divide(num, Oasis::Variable("x")).Copy());
//         } else if (exp == 0) {
//             vec.push_back(num.Copy());
//         } else if (exp == 1) {
//             vec.push_back(Oasis::Multiply(num, Oasis::Variable("x")).Copy());
//         } else {
//             vec.push_back(Oasis::Multiply(num, Oasis::Exponent(Oasis::Variable("x"), Oasis::Real(exp))).Copy());
//         }
//     }
//     auto add = Oasis::BuildFromVector<Oasis::Add>(vec);
//     auto zeros = add->FindZeros();
//     REQUIRE(zeros.size() == 2);
//     auto negOne = Oasis::Real(-1);
//     auto two = Oasis::Real(2);
//     auto root5 = Oasis::Exponent(Oasis::Real(5), Oasis::Divide(Oasis::Real(1), two));
//     std::list<std::unique_ptr<Oasis::Expression>> goalSet = {};
//     goalSet.push_back(Oasis::Divide(Oasis::Add(negOne, root5), two).Copy());
//     goalSet.push_back(Oasis::Divide(Oasis::Subtract(negOne, root5), two).Copy());
//     for (auto& i : zeros) {
//         for (auto i2 = goalSet.begin(); i2 != goalSet.end(); i2++) {
//             if ((*i2)->Equals(*i)) {
//                 goalSet.erase(i2);
//                 break;
//             }
//         }
//     }
//     REQUIRE(goalSet.size() == 0);
// }

TEST_CASE("linear polynomial", "[factor]")
{
    Oasis::Add add {
        Oasis::Real(30),
        Oasis::Variable("x")
    };
    auto zeros = add.FindZeros();
    REQUIRE(zeros.size() == 1);
    if (zeros.size() == 1) {
        auto root = Oasis::RecursiveCast<Oasis::Divide<Oasis::Real>>(*zeros[0]);
        REQUIRE(root != nullptr);
        REQUIRE(root->GetMostSigOp().GetValue() == -30);
        REQUIRE(root->GetLeastSigOp().GetValue() == 1);
    }
}

TEST_CASE("quadratic polynomial", "[factor]")
{
    auto poly = Oasis::Polynomial {1, 9, 24, -2, -99, -135, -54};
    auto c = Oasis::Polynomial::factor_l(poly);

    // (((x+-2)*((x+1)^2))*((x+3)^3))
    Oasis::Multiply m{
        Oasis::Multiply {
            Oasis::Add {
                Oasis::Variable { "x" },
                Oasis::Real { -2 }
            },
            Oasis::Exponent {
                Oasis::Add{ Oasis::Variable { "x" }, Oasis::Real { 1 } },
                Oasis::Real { 2 }
            }
        },
        Oasis::Exponent {
            Oasis::Add{ Oasis::Variable { "x" }, Oasis::Real { 3 } },
            Oasis::Real { 3 }
        }
    };

    auto m_gen = m.Generalize();

    OASIS_CAPTURE_WITH_SERIALIZER(*c);
    OASIS_CAPTURE_WITH_SERIALIZER(*m_gen);

    REQUIRE(c->Equals(*m_gen));
}

/*
TEST_CASE("polynomial constructors preserve coefficients", "[polynomial][init]")
{
    SECTION("initializer list")
    {
        Oasis::Polynomial polynomial {3, -2, 5};
        REQUIRE(CoefficientsAsIntegers(polynomial) == std::vector<long> {3, -2, 5});
    }

    SECTION("span of real coefficients")
    {
        std::vector<Oasis::Real> coefficients {
            Oasis::Real {2},
            Oasis::Real {-3},
            Oasis::Real {1}
        };

        Oasis::Polynomial polynomial {std::span<Oasis::Real> {coefficients}};
        REQUIRE(CoefficientsAsIntegers(polynomial) == std::vector<long> {2, -3, 1});
    }

    SECTION("span of expression coefficients")
    {
        std::vector<std::unique_ptr<Oasis::Expression>> coefficients;
        coefficients.emplace_back(Oasis::Real {4}.Copy());
        coefficients.emplace_back(Oasis::Real {0}.Copy());
        coefficients.emplace_back(Oasis::Real {-7}.Copy());

        Oasis::Polynomial polynomial {std::span<std::unique_ptr<Oasis::Expression>> {coefficients}};
        REQUIRE(CoefficientsAsIntegers(polynomial) == std::vector<long> {4, 0, -7});
    }

    SECTION("expression reference")
    {
        Oasis::Add expression {
            Oasis::Multiply {Oasis::Real {2}, Oasis::Variable {"x"}},
            Oasis::Real {1}
        };

        Oasis::Polynomial polynomial {expression};
        REQUIRE(CoefficientsAsIntegers(polynomial) == std::vector<long> {2, 1});
    }

    SECTION("expression pointer")
    {
        Oasis::Add expression {
            Oasis::Exponent {Oasis::Variable {"x"}, Oasis::Real {2}},
            Oasis::Real {4}
        };

        Oasis::Polynomial polynomial {&expression};
        REQUIRE(CoefficientsAsIntegers(polynomial) == std::vector<long> {1, 0, 4});
    }

    SECTION("const unique pointer copies the expression")
    {
        auto expression = Oasis::Add {
            Oasis::Exponent {Oasis::Variable {"x"}, Oasis::Real {2}},
            Oasis::Real {-9}
        }.Generalize();

        Oasis::Polynomial polynomial {expression};
        REQUIRE(polynomial.Equals(*expression));
    }

    SECTION("moved unique pointer transfers ownership")
    {
        auto expression = Oasis::Subtract {
            Oasis::Exponent {Oasis::Variable {"x"}, Oasis::Real {2}},
            Oasis::Real {9}
        }.Generalize();
        auto expected = expression->Copy();

        Oasis::Polynomial polynomial {std::move(expression)};
        REQUIRE(expression == nullptr);
        REQUIRE(polynomial.Equals(*expected));
    }
} */

TEST_CASE("polynomial degree ignores leading zero coefficients", "[polynomial][degree]")
{
    Oasis::Polynomial polynomial {0, 0, 5, -1};
    REQUIRE(polynomial.degree() == 1);
    REQUIRE(CoefficientsAsIntegers(polynomial) == std::vector<long> {5, -1});

    Oasis::Polynomial constant {7};
    REQUIRE(constant.degree() == 0);
}

TEST_CASE("polynomial get_coefficients fills in missing terms", "[polynomial][coefficients]")
{
    Oasis::Add expression {
        Oasis::Exponent {Oasis::Variable {"x"}, Oasis::Real {3}},
        Oasis::Real {5}
    };
    Oasis::Polynomial polynomial {expression};

    REQUIRE(CoefficientsAsIntegers(polynomial.get_coefficients()) == std::vector<long> {1, 0, 0, 5});
}

TEST_CASE("polynomial monic normalizes the leading coefficient", "[polynomial][monic]")
{
    Oasis::Polynomial polynomial {2, -4, 6};
    auto monic = polynomial.monic();

    REQUIRE(CoefficientsAsIntegers(monic) == std::vector<long> {1, -2, 3});
    REQUIRE(CoefficientsAsIntegers(polynomial) == std::vector<long> {2, -4, 6});
}

TEST_CASE("synthetic divide returns quotient and remainder", "[polynomial][division]")
{
    Oasis::Polynomial dividend {1, -6, 11, -6};
    Oasis::Polynomial divisor {1, -2};

    auto [quotient, remainder] = Oasis::synthetic_divide(dividend, divisor);

    REQUIRE(CoefficientsAsIntegers(quotient) == std::vector<long> {1, -4, 3});
    REQUIRE(CoefficientsAsIntegers(remainder) == std::vector<long> {0});
}

TEST_CASE("synthetic pseudo divide preserves integer coefficients", "[polynomial][division]")
{
    Oasis::Polynomial dividend {1, 0, 1};
    Oasis::Polynomial divisor {2, 1};

    auto [quotient, remainder] = Oasis::synthetic_pseudo_divide(dividend, divisor);

    REQUIRE(CoefficientsAsIntegers(quotient) == std::vector<long> {2, -1});
    REQUIRE(CoefficientsAsIntegers(remainder) == std::vector<long> {5});
}

TEST_CASE("poly_gcd returns the monic greatest common divisor", "[polynomial][gcd]")
{
    Oasis::Polynomial lhs {1, 0, -3, 2};
    Oasis::Polynomial rhs {1, 1, -2};

    auto gcd = Oasis::poly_gcd(lhs, rhs);

    REQUIRE(CoefficientsAsIntegers(gcd) == std::vector<long> {1, 1, -2});
}

TEST_CASE("poly_pow_mod works", "[polynomial][pow_mod]")
{
    Oasis::Polynomial base {2, 3};
    Oasis::Polynomial mod_poly {1, 0, 1};
    int exp = 3;

    auto ppm = Oasis::poly_pow_mod(base, exp, mod_poly, 7);

    REQUIRE(CoefficientsAsIntegers(ppm) == std::vector<long> {-3, -2});
}

// TODO: fix this
TEST_CASE("distinct degree factorization works", "[polynomial][distinct degree factorization]")
{
    // Oasis::Polynomial p {2, 3};
    Oasis::Polynomial p {1, 8, 22, 21};

    auto ppm = Oasis::distinct_degree_factor(p, 5);



    // auto fact2 = ppm.back();

    // REQUIRE(fact1.first.degree() == 1);
    // REQUIRE(fact2.first.degree() == 2);
    REQUIRE(ppm.size() == 2);
    REQUIRE(ppm[0].second == 1);
    REQUIRE(ppm[1].second == 2);

    auto fact1 = ppm[0].first;
    auto fact1_eq = Oasis::Polynomial {1 , -2};
    auto fact2 = ppm[1].first;
    auto fact2_eq = Oasis::Polynomial {1 , 0, 2};
    OASIS_CAPTURE_WITH_SERIALIZER(fact1);
    OASIS_CAPTURE_WITH_SERIALIZER(fact2);

    REQUIRE(CoefficientsAsIntegers(fact1) == std::vector<long> {1, -2});
    REQUIRE(CoefficientsAsIntegers(fact2) == std::vector<long> {1, 0, 2});
}

TEST_CASE("polynomial gcd over finite field works", "[polynomial][gcd]")
{
    auto x = Oasis::Variable { "x" };
    Oasis::Polynomial p1 {1, 8, 22, 21};
    std::unique_ptr<Oasis::Expression> p2 = Oasis::Subtract { Oasis::Exponent{x, Oasis::Real{11}}, x}.Copy();
    Oasis::Polynomial p2_gen = Oasis::Polynomial(*p2);

    // auto gcd = Oasis::poly_gcd_galois(p2_gen, p1, 11);
    auto gcd = Oasis::poly_gcd_galois(p1, p2_gen, 11);

    REQUIRE(CoefficientsAsIntegers(gcd) == std::vector<long> {1, 3});
}

TEST_CASE("polynomial gcd over finite field works with positive root", "[polynomial][gcd]")
{
    Oasis::Polynomial p1 {-1, 4, -1};
    Oasis::Polynomial p2 {3, -2};

    auto gcd = Oasis::poly_gcd_galois(p1, p2, 11);

    REQUIRE(CoefficientsAsIntegers(gcd) == std::vector<long> {1, 3});
}

TEST_CASE("polynomial gcd over finite field works with negative root", "[polynomial][gcd]")
{
    Oasis::Polynomial p1 {1, 2, 3, 1};
    Oasis::Polynomial p2_gen {-2, -5};

    auto gcd = Oasis::poly_gcd_galois(p1, p2_gen, 11);

    REQUIRE(CoefficientsAsIntegers(gcd) == std::vector<long> {1, -3});
}

TEST_CASE("monic form of polynomial in a finite field", "[polynomial][finite field arithmetic]")
{
    Oasis::Polynomial poly {3, -2};
    Oasis::Polynomial monic = poly.monic(11);

    REQUIRE(CoefficientsAsIntegers(monic) == std::vector<long> {1, 3});
}


TEST_CASE("choosing prime value for polynomial factorization", "[polynomial][hensel prime]")
{
    Oasis::Polynomial poly {1, -4, -38, -63};

    auto val = auto_choose_prime_for_hensel(poly);

    REQUIRE(val == 5);
}

TEST_CASE("clear_denominators converts rational coefficients to integers", "[polynomial][hensel]")
{
    std::vector<std::unique_ptr<Oasis::Expression>> coefficients;
    coefficients.emplace_back(Oasis::Divide<Oasis::Real> { Oasis::Real {1}, Oasis::Real {2} }.Copy());
    coefficients.emplace_back(Oasis::Divide<Oasis::Real> { Oasis::Real {3}, Oasis::Real {2} }.Copy());
    coefficients.emplace_back(Oasis::Real {1}.Copy());

    Oasis::Polynomial polynomial {coefficients};
    const auto [denominator, cleared] = clear_denominators(polynomial);

    REQUIRE(denominator == 2);
    REQUIRE(CoefficientsAsIntegers(cleared) == std::vector<long> {1, 3, 2});
}

TEST_CASE("extended gcd over F_p returns Bezout coefficients", "[polynomial][hensel]")
{
    Oasis::Polynomial a {1, 1};
    Oasis::Polynomial b {1, 2};

    auto [s, t, g] = Oasis::extended_gcd_poly_mod_p(a, b, 5);
    auto bezout = Oasis::mod_poly_positive(((s * a).expand() + (t * b).expand()).expand(), 5);

    REQUIRE(CoefficientsAsIntegers(Oasis::mod_poly_positive(g, 5)) == std::vector<long> {1});
    REQUIRE(CoefficientsAsIntegers(bezout) == std::vector<long> {1});
}

TEST_CASE("hensel_lift_pair lifts two coprime factors", "[polynomial][hensel]")
{
    Oasis::Polynomial f {1, 8, 7};
    Oasis::Polynomial a0 {1, 1};
    Oasis::Polynomial b0 {1, 2};

    auto [a, b] = Oasis::hensel_lift_pair(f, a0, b0, 5, 2);
    auto centered_a = Oasis::center_polynomial_coefficients(a, 25);
    auto centered_b = Oasis::center_polynomial_coefficients(b, 25);
    auto product = ((centered_a * centered_b).expand());

    REQUIRE(CoefficientsAsIntegers(centered_a) == std::vector<long> {1, 1});
    REQUIRE(CoefficientsAsIntegers(centered_b) == std::vector<long> {1, 7});
    REQUIRE(CoefficientsAsIntegers(product) == std::vector<long> {1, 8, 7});
}

TEST_CASE("lift_factors_to_Q reconstructs integer factors", "[polynomial][hensel]")
{
    Oasis::Polynomial f {2, 22, 62, 42};
    std::vector<Oasis::Polynomial> modular_factors {
        Oasis::Polynomial {1, 1},
        Oasis::Polynomial {1, 2},
        Oasis::Polynomial {1, 3}
    };

    const auto lifted = Oasis::lift_factors_to_Q(f, modular_factors, 5, 2);
    const auto product = Oasis::multiply_polynomials(lifted);

    REQUIRE(lifted.size() == 3);
    REQUIRE(CoefficientsAsIntegers(product) == std::vector<long> {2, 22, 62, 42});
}

TEST_CASE("lift_factors_to_Q reconstructs a rational polynomial", "[polynomial][hensel]")
{
    std::vector<std::unique_ptr<Oasis::Expression>> coefficients;
    coefficients.emplace_back(Oasis::Divide<Oasis::Real> { Oasis::Real {1}, Oasis::Real {2} }.Copy());
    coefficients.emplace_back(Oasis::Divide<Oasis::Real> { Oasis::Real {11}, Oasis::Real {2} }.Copy());
    coefficients.emplace_back(Oasis::Divide<Oasis::Real> { Oasis::Real {31}, Oasis::Real {2} }.Copy());
    coefficients.emplace_back(Oasis::Divide<Oasis::Real> { Oasis::Real {21}, Oasis::Real {2} }.Copy());
    Oasis::Polynomial f {coefficients};

    std::vector<Oasis::Polynomial> modular_factors {
        Oasis::Polynomial {1, 1},
        Oasis::Polynomial {1, 2},
        Oasis::Polynomial {1, 3}
    };

    const auto lifted = Oasis::lift_factors_to_Q(f, modular_factors, 5, 2);
    const auto product = Oasis::multiply_polynomials(lifted);
    const auto [product_denominator, product_cleared] = Oasis::clear_denominators(product);
    const auto [original_denominator, original_cleared] = Oasis::clear_denominators(f);

    REQUIRE(lifted.size() == 3);
    REQUIRE(product_denominator == original_denominator);
    REQUIRE(CoefficientsAsIntegers(product_cleared) == CoefficientsAsIntegers(original_cleared));
}