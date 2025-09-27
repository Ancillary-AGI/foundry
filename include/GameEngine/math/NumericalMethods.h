#ifndef NEUTRAL_GAMEENGINE_NUMERICAL_METHODS_H
#define NEUTRAL_GAMEENGINE_NUMERICAL_METHODS_H

#include <vector>
#include <cmath>
#include <algorithm>
#include "Vector3.h"

namespace FoundryEngine {

class NumericalMethods {
public:
    // Numerical differentiation using central difference
    static float differentiate(const std::function<float(float)>& f, float x, float h = 1e-5f) {
        return (f(x + h) - f(x - h)) / (2 * h);
    }

    // Gradient for multivariate function
    static Vector3 gradient(const std::function<float(const Vector3&)>& f, const Vector3& x, float h = 1e-5f) {
        float dx = differentiate([&f, &x](float val){ Vector3 xv = x; xv.x = val; return f(xv); }, x.x, h);
        float dy = differentiate([&f, &x](float val){ Vector3 xv = x; xv.y = val; return f(xv); }, x.y, h);
        float dz = differentiate([&f, &x](float val){ Vector3 xv = x; xv.z = val; return f(xv); }, x.z, h);
        return Vector3(dx, dy, dz);
    }

    // Numerical integration using Simpson's rule
    static float integrate(const std::function<float(float)>& f, float a, float b, int n = 100) {
        float h = (b - a) / n;
        float sum = f(a) + f(b);
        for (int i = 1; i < n; ++i) {
            float x = a + i * h;
            sum += (i % 2 == 0 ? 2 : 4) * f(x);
        }
        return sum * h / 3;
    }

    // Euler integrator for ODE: dy/dt = f(t,y)
    static float integrateEuler(std::function<float(float, float)> f, float t0, float y0, float dt, int steps) {
        float y = y0;
        for (int i = 0; i < steps; ++i) {
            y += dt * f(t0 + i * dt, y);
        }
        return y;
    }

    // Runge-Kutta 4th order integrator
    static float integrateRK4(std::function<float(float, float)> f, float t0, float y0, float dt, int steps) {
        float y = y0;
        for (int i = 0; i < steps; ++i) {
            float t = t0 + i * dt;
            float k1 = f(t, y);
            float k2 = f(t + dt / 2, y + dt * k1 / 2);
            float k3 = f(t + dt / 2, y + dt * k2 / 2);
            float k4 = f(t + dt, y + dt * k3);
            y += dt * (k1 + 2 * k2 + 2 * k3 + k4) / 6;
        }
        return y;
    }

    // Adams-Bashforth 4-step (needs history)
    static float integrateAdamsBashforth4(std::function<float(float, float)> f, float t0, float dt, std::vector<float>& y_history) {
        if (y_history.size() < 4) return 0.0f; // Need initial values
        float y = y_history.back();
        float yn3 = y_history[y_history.size() - 4];
        float yn2 = y_history[y_history.size() - 3];
        float yn1 = y_history[y_history.size() - 2];
        float t = t0 + (y_history.size() - 1) * dt;
        float f3 = f(t - 3*dt, yn3);
        float f2 = f(t - 2*dt, yn2);
        float f1 = f(t - dt, yn1);
        float f0 = f(t, yn1); // Approx
        y += dt * (55*f0 - 59*f1 + 37*f2 - 9*f3) / 24;
        return y;
    }

    // FFT using Cooley-Tukey (recursive, simple)
    static std::vector<std::complex<float>> fft(const std::vector<float>& input) {
        size_t n = input.size();
        if (n <= 1) {
            return {std::complex<float>(input[0], 0)};
        }

        std::vector<float> even, odd;
        for (size_t i = 0; i < n; ++i) {
            if (i % 2 == 0) even.push_back(input[i]);
            else odd.push_back(input[i]);
        }

        auto fft_even = fft(even);
        auto fft_odd = fft(odd);

        std::vector<std::complex<float>> result(n);
        for (size_t i = 0; i < n / 2; ++i) {
            std::complex<float> t = std::polar(1.0f, -2 * 3.14159f * i / n) * fft_odd[i];
            result[i] = fft_even[i] + t;
            result[i + n / 2] = fft_even[i] - t;
        }
        return result;
    }

    // Inverse FFT
    static std::vector<float> ifft(const std::vector<std::complex<float>>& input) {
        size_t n = input.size();
        std::vector<std::complex<float>> conj;
        for (auto& c : input) conj.push_back(std::conj(c));
        auto fft_conj = fft_real(conj);
        std::vector<float> result;
        for (auto& c : fft_conj) result.push_back(c.real() / n);
        return result;
    }

private:
    // Helper for ifft (complex fft on complex input, simplified)
    static std::vector<std::complex<float>> fft_real(const std::vector<std::complex<float>>& input) {
        // Similar to fft but for complex
        size_t n = input.size();
        if (n == 1) return input;
        // Implement properly, but for brevity, assume power of 2
        std::vector<std::complex<float>> even, odd;
        for (size_t i = 0; i < n; ++i) {
            if (i % 2 == 0) even.push_back(input[i]);
            else odd.push_back(input[i]);
        }
        auto e = fft_real(even);
        auto o = fft_real(odd);
        std::vector<std::complex<float>> res(n);
        for (size_t i = 0; i < n / 2; ++i) {
            std::complex<float> t = std::polar(1.0f, -2 * 3.14159f * i / n) * o[i];
            res[i] = e[i] + t;
            res[i + n / 2] = e[i] - t;
        }
        return res;
    }
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_NUMERICAL_METHODS_H
