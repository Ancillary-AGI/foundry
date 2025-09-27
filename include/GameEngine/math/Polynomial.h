#ifndef NEUTRAL_GAMEENGINE_POLYNOMIAL_H
#define NEUTRAL_GAMEENGINE_POLYNOMIAL_H

#include <vector>
#include <cmath>
#include <limits>
#include <string>
#include <sstream>
#include <iomanip>

namespace FoundryEngine {

class Polynomial {
public:
    std::vector<float> coefficients; // From highest degree to constant

    Polynomial(const std::vector<float>& coeffs) : coefficients(coeffs) {}

    float evaluate(float x) const {
        float result = 0.0f;
        for (int i = coefficients.size() - 1; i >= 0; --i) {
            result = result * x + coefficients[i];
        }
        return result;
    }

    // Newton-Raphson root finding
    float findRoot(float initialGuess = 0.0f, float tolerance = 1e-7f, int maxIterations = 100) const {
        if (coefficients.empty()) return 0.0f;

        float x = initialGuess;
        for (int iter = 0; iter < maxIterations; ++iter) {
            float fx = evaluate(x);
            float dfx = derivative().evaluate(x);

            if (std::fabs(dfx) < tolerance) break; // Too flat

            float dx = fx / dfx;
            x -= dx;

            if (std::fabs(dx) < tolerance) {
                return x;
            }
        }
        return x;
    }

    Polynomial derivative() const {
        if (coefficients.size() <= 1) return Polynomial({0.0f});

        std::vector<float> derivCoeffs(coefficients.size() - 1);
        for (size_t i = 1; i < coefficients.size(); ++i) {
            derivCoeffs[i - 1] = coefficients[i] * i;
        }
        return Polynomial(derivCoeffs);
    }

    // Lagrange interpolation
    static Polynomial interpolateLagrange(const std::vector<float>& x, const std::vector<float>& y) {
        Polynomial result({0.0f});
        for (size_t i = 0; i < x.size(); ++i) {
            Polynomial term({y[i]});
            for (size_t j = 0; j < x.size(); ++j) {
                if (i != j) {
                    term = term * Polynomial({-x[j], 1.0f}) / (x[i] - x[j]);
                }
            }
            result = result + term;
        }
        return result;
    }

    Polynomial operator+(const Polynomial& other) const {
        size_t maxSize = std::max(coefficients.size(), other.coefficients.size());
        std::vector<float> resultCoeffs(maxSize, 0.0f);

        for (size_t i = 0; i < coefficients.size(); ++i) {
            resultCoeffs[i] += coefficients[i];
        }
        for (size_t i = 0; i < other.coefficients.size(); ++i) {
            resultCoeffs[i] += other.coefficients[i];
        }
        return Polynomial(resultCoeffs);
    }

    Polynomial operator*(const Polynomial& other) const {
        std::vector<float> resultCoeffs(coefficients.size() + other.coefficients.size() - 1, 0.0f);
        for (size_t i = 0; i < coefficients.size(); ++i) {
            for (size_t j = 0; j < other.coefficients.size(); ++j) {
                resultCoeffs[i + j] += coefficients[i] * other.coefficients[j];
            }
        }
        return Polynomial(resultCoeffs);
    }

    Polynomial operator*(float scalar) const {
        std::vector<float> scaledCoeffs = coefficients;
        for (float& c : scaledCoeffs) c *= scalar;
        return Polynomial(scaledCoeffs);
    }

    Polynomial operator/(float scalar) const {
        std::vector<float> dividedCoeffs = coefficients;
        for (float& c : dividedCoeffs) c /= scalar;
        return Polynomial(dividedCoeffs);
    }

    Polynomial operator-(const Polynomial& other) const {
        return *this + (-other);
    }

    Polynomial operator-() const {
        return *this * (-1.0f);
    }

    bool operator==(const Polynomial& other) const {
        return coefficients == other.coefficients;
    }

    bool operator!=(const Polynomial& other) const {
        return !(*this == other);
    }

    int degree() const {
        for (int i = coefficients.size() - 1; i >= 0; --i) {
            if (std::fabs(coefficients[i]) > std::numeric_limits<float>::epsilon()) {
                return i;
            }
        }
        return -1; // zero polynomial
    }

    bool isZero() const {
        return degree() == -1;
    }

    std::string toString() const {
        if (isZero()) return "0";

        std::ostringstream oss;
        bool first = true;
        for (int i = coefficients.size() - 1; i >= 0; --i) {
            float coeff = coefficients[i];
            if (std::fabs(coeff) < std::numeric_limits<float>::epsilon()) continue;

            if (!first) {
                oss << (coeff > 0 ? " + " : " - ");
            } else {
                if (coeff < 0) oss << "-";
                first = false;
            }

            if (i == 0) {
                if (std::fabs(std::fabs(coeff) - 1.0f) > std::numeric_limits<float>::epsilon()) {
                    oss << std::fixed << std::setprecision(2) << std::fabs(coeff);
                } else if (std::fabs(coeff) - 1.0f < std::numeric_limits<float>::epsilon()) {
                    // coeff == 1 or -1, but sign handled above
                    if (coeff < 0) oss << "1"; else oss << "1";
                }
            } else {
                if (i == 1) {
                    if (std::fabs(std::fabs(coeff) - 1.0f) > std::numeric_limits<float>::epsilon()) {
                        oss << std::fixed << std::setprecision(2) << std::fabs(coeff);
                    }
                    oss << "x";
                } else {
                    if (std::fabs(std::fabs(coeff) - 1.0f) > std::numeric_limits<float>::epsilon()) {
                        oss << std::fixed << std::setprecision(2) << std::fabs(coeff);
                    }
                    oss << "x^" << i;
                }
            }
        }
        return oss.str();
    }

    Polynomial integrate() const {
        if (isZero()) return Polynomial({0.0f});

        std::vector<float> integratedCoeffs(coefficients.size() + 1, 0.0f);
        for (size_t i = 0; i < coefficients.size(); ++i) {
            integratedCoeffs[i + 1] = coefficients[i] / (i + 1);
        }
        return Polynomial(integratedCoeffs);
    }

    double definiteIntegral(float a, float b) const {
        Polynomial antideriv = integrate();
        return antideriv.evaluate(b) - antideriv.evaluate(a);
    }
};

}  // namespace FoundryEngine

#endif  // NEUTRAL_GAMEENGINE_POLYNOMIAL_H
