#include "SFML-2.5.1/include/SFML/Graphics.hpp"
#include <queue>
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <fstream>

template <typename T>
inline float dot(sf::Vector2<T> a, sf::Vector2<T> b) {
    return a.x * b.x + a.y * b.y;
}

template <typename T>
inline float normDot(sf::Vector2<T> a, sf::Vector2<T> b) {
    return (a.x * b.x + a.y * b.y) / std::sqrt(float(a.x * a.x + a.y * a.y) * (b.x * b.x + b.y * b.y));
}

template <typename T>
float diffAngle(sf::Vector2<T> a, sf::Vector2<T> b) {
    float diff = std::abs(std::atan2(a.y, a.x) - std::atan2(b.y, b.x));
    if (diff > M_PI) {
        diff = 2 * M_PI - diff;
    }
    return diff * 180 / M_PI;
}

sf::Font arialFont, consolaFont;

float myFunction(sf::Vector2f v) {
    // v *= 2.f;
    return std::max((std::sin(v.x / 10.f) + std::cos(v.y / 10.f)) / 8.f + 0.5f, std::exp((-2.f * std::pow(v.x - 75.f, 2.f) - 2.f * std::pow(v.y - 75.f, 2.f)) / 10000.f));
}

template <typename T>
inline float distanse(sf::Vector2<T> v1, sf::Vector2<T> v2) {
    return std::sqrt((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y));
}

template <typename T>
inline float length(sf::Vector2<T>& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}
template <typename T> inline float difflength(T& x, T& y)  { return std::sqrt(x * x - y * y); }
template <typename T> inline float difflength(T&& x, T& y) { return std::sqrt(x * x - y * y); }
template <typename T> inline float difflength(T& x, T&& y) { return std::sqrt(x * x - y * y); }


struct LineCoeffs {
    float a, b, c;
};

void operator*=(LineCoeffs& coeff, float x) { coeff.a *= x; coeff.b *= x; coeff.c *= x; }

inline float functionOfLine(LineCoeffs& coeff, sf::Vector2i& point) { return coeff.a * point.x + coeff.b * point.y + coeff.c; }
inline float functionOfLine(LineCoeffs& coeff, float& x, float& y) { return coeff.a * x + coeff.b * y + coeff.c; }
inline float functionOfLine(LineCoeffs& coeff, float& x, int& y)   { return coeff.a * x + coeff.b * y + coeff.c; }
inline float functionOfLine(LineCoeffs& coeff, int& x, float& y)   { return coeff.a * x + coeff.b * y + coeff.c; }
inline float functionOfLineX(LineCoeffs& coeff, float& x) { return coeff.a * x + coeff.c; }
inline float functionOfLineY(LineCoeffs& coeff, float& y) { return coeff.b * y + coeff.c; }

inline void lineByPoints(LineCoeffs& coeff, sf::Vector2i& a, sf::Vector2i& b) {
    coeff.a = b.y - a.y;
    coeff.b = a.x - b.x;
    coeff.c = - coeff.a * b.x - coeff.b * b.y; // 4
}

inline float getX(LineCoeffs& coeff, float& y)  { return (-coeff.b * y - coeff.c) / coeff.a; }
inline float getX(LineCoeffs& coeff, float&& y) { return (-coeff.b * y - coeff.c) / coeff.a; }
inline float getX(LineCoeffs& coeff, int& y)    { return (-coeff.b * y - coeff.c) / coeff.a; }
inline float getY(LineCoeffs& coeff, float& x)  { return (-coeff.a * x - coeff.c) / coeff.b; }
inline float getY(LineCoeffs& coeff, float&& x) { return (-coeff.a * x - coeff.c) / coeff.b; }
inline float getY(LineCoeffs& coeff, int& x)    { return (-coeff.a * x - coeff.c) / coeff.b; }

inline void linesByPoints(std::pair<LineCoeffs, LineCoeffs>& coeffs, sf::Vector2i& a, sf::Vector2i& b, float& s, float& c) {
    coeffs.second.c = a.x - b.x;
    coeffs.second.b = a.y - b.y;
    coeffs.first.a = coeffs.second.c * s - coeffs.second.b * c;
    coeffs.first.b = coeffs.second.c * c + coeffs.second.b * s;
    coeffs.first.c = - coeffs.first.a * b.x - coeffs.first.b * b.y;
    coeffs.second.a = coeffs.second.c * s + coeffs.second.b * c;
    coeffs.second.b = coeffs.second.b * s - coeffs.second.c * c;
    coeffs.second.c = - coeffs.second.a * b.x - coeffs.second.b * b.y;
}

inline void linesByPoints(std::pair<LineCoeffs, LineCoeffs>& coeffs, sf::Vector2i& a, sf::Vector2i& b, float& angle) {
    float c = std::cos(angle), s = std::sin(angle);
    linesByPoints(coeffs, a, b, s, c);
}

inline void subrectByLine(std::pair<sf::Vector2i, sf::Vector2i>& bounders, LineCoeffs& coeff) {
    if (coeff.a == 0) {
        if (coeff.b > 0) {
            bounders.second.y = std::min(- coeff.c / coeff.b, (float)bounders.second.y);
        } else {
            bounders.first.y = std::max(- coeff.c / coeff.b, (float)bounders.first.y);
        }
    } else if (coeff.b == 0) {
        if (coeff.a > 0) {
            bounders.second.x = std::min(- coeff.c / coeff.a, (float)bounders.second.x);
        } else {
            bounders.first.x = std::max(- coeff.c / coeff.a, (float)bounders.first.x);
        }
    } else {
        float y1 = getY(coeff, bounders.first.x), y2 = getY(coeff, bounders.second.x),
              x1 = getX(coeff, bounders.first.y), x2 = getX(coeff, bounders.second.y);
        if (coeff.b > 0) {
            bounders.second.y = std::min(std::max(y1, y2), (float)bounders.second.y);
        } else {
            bounders.first.y = std::max(std::min(y1, y2), (float)bounders.first.y);
            if (coeff.a > 0) {
                bounders.second.y = std::max(std::min(y1, y2), (float)bounders.second.y);
                bounders.second.x = std::max(std::min(x1, x2), (float)bounders.second.x);
            }
        }
        if (coeff.a > 0) {
            bounders.second.x = std::min(std::max(x1, x2), (float)bounders.second.x);
        } else {
            bounders.first.x = std::max(std::min(x1, x2), (float)bounders.first.x);
        }
    }
}

inline void subrectBy2Lines(std::pair<sf::Vector2i, sf::Vector2i>& bounders, LineCoeffs& coeff1, LineCoeffs& coeff2, sf::Vector2i& c) {
    if (coeff1.b < 0 || coeff2.b < 0) { float D = c.y;
        if (coeff2.b != 0) { float y12 = getY(coeff2, bounders.first.x);
            if (functionOfLine(coeff1, bounders.first.x, y12) < 0) D = std::min(D, y12);
        }
        if (coeff1.b != 0) { float y21 = getY(coeff1, bounders.second.x);
            if (functionOfLine(coeff2, bounders.second.x, y21) < 0) D = std::min(D, y21);
        }
        bounders.first.y = std::max(bounders.first.y, (int)std::ceil(D));
    }
    if (coeff1.b > 0 || coeff2.b > 0) { float U = c.y;
        if (coeff1.b != 0) { float y11 = getY(coeff1, bounders.first.x);
            if (functionOfLine(coeff2, bounders.first.x, y11) < 0) U = std::max(U, y11);
        }
        if (coeff2.b != 0) { float y22 = getY(coeff2, bounders.second.x);
            if (functionOfLine(coeff1, bounders.second.x, y22) < 0) U = std::max(U, y22);
        }
        bounders.second.y = std::min(bounders.second.y, (int)U);
    }
    if (coeff1.a < 0 || coeff2.a < 0) { float L = c.x;
        if (coeff1.a != 0) { float x11 = getX(coeff1, bounders.first.y);
            if (functionOfLine(coeff2, x11, bounders.first.y) < 0) L = std::min(L, x11);
        }
        if (coeff2.a != 0) { float x22 = getX(coeff2, bounders.second.y);
            if (functionOfLine(coeff1, x22, bounders.second.y) < 0) L = std::min(L, x22);
        }
        bounders.first.x = std::max(bounders.first.x, (int)std::ceil(L));
    }
    if (coeff1.a > 0 || coeff2.a > 0) { float R = c.x;
        if (coeff2.a != 0) { float x12 = getX(coeff2, bounders.first.y);
            if (functionOfLine(coeff1, x12, bounders.first.y) < 0) R = std::max(R, x12);
        }
        if (coeff1.a != 0) { float x21 = getX(coeff1, bounders.second.y);
            if (functionOfLine(coeff2, x21, bounders.second.y) < 0) R = std::max(R, x21);
        }
        bounders.second.x = std::min(bounders.second.x, (int)R);
    }
}

inline void subrectBy2Lines(int& Down, int& Up, LineCoeffs& coeff1, LineCoeffs& coeff2, sf::Vector2i& c, int& x) {
    if (coeff1.b < 0 || coeff2.b < 0) { float D = c.y;
        if (coeff2.b != 0) { float y12 = getY(coeff2, 0);
            if (functionOfLineY(coeff1, y12) < 0) D = std::min(D, y12);
        }
        if (coeff1.b != 0) { float y21 = getY(coeff1, x);
            if (functionOfLine(coeff2, x, y21) < 0) D = std::min(D, y21);
        }
        Down = std::max(Down, (int)std::ceil(D));
    }
    if (coeff1.b > 0 || coeff2.b > 0) { float U = c.y;
        if (coeff1.b != 0) { float y11 = getY(coeff1, 0);
            if (functionOfLineY(coeff2, y11) < 0) U = std::max(U, y11);
        }
        if (coeff2.b != 0) { float y22 = getY(coeff2, x);
            if (functionOfLine(coeff1, x, y22) < 0) U = std::max(U, y22);
        }
        Up = std::min(Up, (int)U);
    }
}

inline void rectBy2CircleAnd2Lines(std::pair<sf::Vector2i, sf::Vector2i>& bounders, LineCoeffs& coeff1, LineCoeffs& coeff2, sf::Vector2i& c, sf::Vector2i& b, sf::Vector2f& m1, sf::Vector2f& m2, float& r, float (*Lcomp)(float&, float&, float&), float (*Rcomp)(float&, float&, float&)) {
    float aa1 = coeff1.a * coeff1.a, ab1 = coeff1.a * coeff1.b, bc1 = coeff1.b * coeff1.c, aabb1 = aa1 + coeff1.b * coeff1.b,
          aa2 = coeff2.a * coeff2.a, ab2 = coeff2.a * coeff2.b, bc2 = coeff2.b * coeff2.c, aabb2 = aa2 + coeff2.b * coeff2.b,
          y11 = 2.f * (aa1 * m1.y - ab1 * m1.x - bc1) / aabb1 - c.y,
          y12 = 2.f * (aa2 * m1.y - ab2 * m1.x - bc2) / aabb2 - c.y,
          y21 = 2.f * (aa1 * m2.y - ab1 * m2.x - bc1) / aabb1 - c.y,
          y22 = 2.f * (aa2 * m2.y - ab2 * m2.x - bc2) / aabb2 - c.y,
          x11 = (coeff1.a == 0) ? 2.f * m1.x - c.x : getX(coeff1, y11),
          x12 = (coeff2.a == 0) ? 2.f * m1.x - c.x : getX(coeff2, y12),
          x21 = (coeff1.a == 0) ? 2.f * m2.x - c.x : getX(coeff1, y21),
          x22 = (coeff2.a == 0) ? 2.f * m2.x - c.x : getX(coeff2, y22),
          m1D = m1.y - r, m1U = m1.y + r, m1L = m1.x - r, m1R = m1.x + r,
          m2D = m2.y - r, m2U = m2.y + r, m2L = m2.x - r, m2R = m2.x + r,
          D1 = c.y, U1 = c.y, L1 = c.x, R1 = c.x,
          D2 = c.y, U2 = c.y, L2 = c.x, R2 = c.x,
          D3 = c.y, U3 = c.y, L3 = c.x, R3 = c.x;

    // первая окружность
    if (functionOfLine(coeff1, x12, y12) <= 0) {
        D1 = std::min(D1, y12); U1 = std::max(U1, y12);
        L1 = std::min(L1, x12); R1 = std::max(R1, x12);
    }
    if (functionOfLine(coeff2, x11, y11) <= 0) {
        D1 = std::min(D1, y11); U1 = std::max(U1, y11);
        L1 = std::min(L1, x11); R1 = std::max(R1, x11);
    }
    if (functionOfLine(coeff1, m1.x, m1D) <= 0 && functionOfLine(coeff2, m1.x, m1D) <= 0) D1 = m1D;
    if (functionOfLine(coeff1, m1.x, m1U) <= 0 && functionOfLine(coeff2, m1.x, m1U) <= 0) U1 = m1U;
    if (functionOfLine(coeff1, m1L, m1.y) <= 0 && functionOfLine(coeff2, m1L, m1.y) <= 0) L1 = m1L;
    if (functionOfLine(coeff1, m1R, m1.y) <= 0 && functionOfLine(coeff2, m1R, m1.y) <= 0) R1 = m1R;

    // вторая окружность
    if (functionOfLine(coeff1, x22, y22) <= 0) {
        D2 = std::min(D2, y22); U2 = std::max(U2, y22);
        L2 = std::min(L2, x22); R2 = std::max(R2, x22);
    }
    if (functionOfLine(coeff2, x21, y21) <= 0) {
        D2 = std::min(D2, y21); U2 = std::max(U2, y21);
        L2 = std::min(L2, x21); R2 = std::max(R2, x21);
    }
    if (functionOfLine(coeff1, m2.x, m2D) <= 0 && functionOfLine(coeff2, m2.x, m2D) <= 0) D2 = m2D;
    if (functionOfLine(coeff1, m2.x, m2U) <= 0 && functionOfLine(coeff2, m2.x, m2U) <= 0) U2 = m2U;
    if (functionOfLine(coeff1, m2L, m2.y) <= 0 && functionOfLine(coeff2, m2L, m2.y) <= 0) L2 = m2L;
    if (functionOfLine(coeff1, m2R, m2.y) <= 0 && functionOfLine(coeff2, m2R, m2.y) <= 0) R2 = m2R;

    D3 = std::max(m1.x, m2.x) > std::max(c.x, b.x) ? std::min(c.y, b.y) : std::max(m1.y, m2.y) - r;
    U3 = std::max(m1.x, m2.x) > std::max(c.x, b.x) ? std::max(c.y, b.y) : std::min(m1.y, m2.y) + r;
    L3 = std::max(m1.y, m2.y) > std::max(c.y, b.y) ? std::min(c.x, b.x) : std::max(m1.x, m2.x) - r;
    R3 = std::max(m1.y, m2.y) > std::max(c.y, b.y) ? std::max(c.x, b.x) : std::min(m1.x, m2.x) + r;

    bounders.first.y  = std::max((int)std::ceil(Lcomp(D1, D2, D3)), bounders.first.y);
    bounders.second.y = std::min((int)Rcomp(U1, U2, U3), bounders.second.y);
    bounders.first.x  = std::max((int)std::ceil(Lcomp(L1, L2, L3)), bounders.first.x);
    bounders.second.x = std::min((int)Rcomp(R1, R2, R3), bounders.second.x);
}
inline void yBounderByCircleAnd2Lines(int& bottom, int& top, LineCoeffs& coeff1, LineCoeffs& coeff2, sf::Vector2i& c, sf::Vector2f& m, float& r) {
    float aa1 = coeff1.a * coeff1.a, aa2 = coeff2.a * coeff2.a,
          y1 = 2.f * (aa1 * m.y - coeff1.a * coeff1.b * m.x - coeff1.b * coeff1.c) / (aa1 + coeff1.b * coeff1.b) - c.y,
          y2 = 2.f * (aa2 * m.y - coeff2.a * coeff2.b * m.x - coeff2.b * coeff2.c) / (aa2 + coeff2.b * coeff2.b) - c.y,
          x1 = (coeff1.a == 0) ? 2.f * m.x - c.x : getX(coeff1, y1),
          x2 = (coeff2.a == 0) ? 2.f * m.x - c.x : getX(coeff2, y2),
          mD = m.y - r, mU = m.y + r,
          D = c.y, U = c.y;

    if (functionOfLine(coeff1, x2, y2) <= 0) {
        D = std::min(D, y2); U = std::max(U, y2);
    }
    if (functionOfLine(coeff2, x1, y1) <= 0) {
        D = std::min(D, y1); U = std::max(U, y1);
    }
    if (functionOfLine(coeff1, m.x, mD) <= 0 && functionOfLine(coeff2, m.x, mD) <= 0) D = mD;
    if (functionOfLine(coeff1, m.x, mU) <= 0 && functionOfLine(coeff2, m.x, mU) <= 0) U = mU;

    bottom = std::max((int)std::ceil(D), bottom);
    top    = std::min((int)U, top);
}

inline int bottomBounder(float& circusY, LineCoeffs &coeff) {
    return (coeff.a == 0 && coeff.b < 0) ? std::ceil(getY(coeff, 0.f)) : circusY;
};
inline int topBounder(float& circusY, LineCoeffs &coeff) {
    return (coeff.a == 0 && coeff.b > 0) ? getY(coeff, 0.f) : circusY;
};
inline int leftBounder(int& y, LineCoeffs &coeff, sf::Vector2f& m, float& radius) {
    return std::ceil((coeff.a <= 0) ? m.x - difflength(radius, m.y - y) : std::max(m.x - difflength(radius, m.y - y), getX(coeff, y)));
};
inline int rightBounder(int& y, LineCoeffs &coeff, sf::Vector2f& m, float& radius) {
    return (coeff.a >= 0) ? m.x + difflength(radius, m.y - y) : std::min(m.x + difflength(radius, m.y - y), getX(coeff, y));
};
inline int sectorLeftBounder(int& y, LineCoeffs &coeff1, LineCoeffs &coeff2, float left) {
    if (coeff1.a < 0) {
        return std::ceil((coeff2.a < 0) ? std::max(getX(coeff1, y), getX(coeff2, y)) : getX(coeff1, y));
    } else {
        return std::ceil((coeff2.a < 0) ? getX(coeff2, y) : left);
    }
};
inline int sectorRightBounder(int& y, LineCoeffs &coeff1, LineCoeffs &coeff2, float right) {
    if (coeff1.a > 0) {
        return (coeff2.a > 0) ? std::min(getX(coeff1, y), getX(coeff2, y)) : getX(coeff1, y);
    } else {
        return (coeff2.a > 0) ? getX(coeff2, y) : right;
    }
};

std::string floatToString(float num, int m = 3) {
    int wholePart = int(num);
    float fracPart = (num - wholePart) * 10;
    std::string result = std::to_string(wholePart) + '.';
    for (int i = 0, k = 0; i < 2 && k < m; k++) {
        if ((int)fracPart % 10 != 0) {
            result += std::to_string((int)fracPart % 10);
            i++;
        } else {
            result += '0';
        }
        fracPart = (fracPart - int(fracPart)) * 10;
    }
    return result;
}

template <typename T>
std::ostream& operator<<(std::ostream& stream, sf::Vector2<T>& a) {
    return stream << a.x << ", " << a.y;
}
template <typename T>
std::istream& operator>>(std::istream& stream, sf::Vector2<T>& a) {
    return stream >> a.x >> a.y;
}

template <typename T>
std::string toString(sf::Vector2<T> v) {
    return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
}

template <typename T>
inline sf::Vector2<T> perpendicular(sf::Vector2<T> v) {
    return sf::Vector2<T>(-v.y, v.x);
}

std::vector<std::vector<float>> optimalPathLength(std::vector<std::vector<float>>& grid, sf::Vector2i start, std::vector<std::vector<bool>>& enable) {
    std::vector<std::vector<float>> result(grid.size(), std::vector<float>(grid[0].size(), std::numeric_limits<float>::infinity()));
    result[start.x][start.y] = grid[start.x][start.y];
    std::queue<sf::Vector2i> q; q.push(start);
    float temp;
    while (!q.empty()) {
        sf::Vector2i cur = q.front(); q.pop();
        if (!enable[cur.x][cur.y]) continue;
        for (int x = cur.x - 1; x <= cur.x + 1; x++) {
            if (x < 0 || x >= grid.size()) continue;
            for (int y = cur.y - 1; y <= cur.y + 1; y++) {
                if (y < 0 || y >= grid[0].size()) continue;
                temp = result[cur.x][cur.y] + grid[x][y];
                if (result[x][y] > temp) {
                    q.push(sf::Vector2i(x, y));
                    result[x][y] = temp;
                }
            }
        }
    }
    return result;
}

template <typename T>
sf::Vector2f gradient(std::vector<std::vector<float>>& grid, sf::Vector2<T> point) {
    sf::Vector2f result(0, 0);
    if (point.x > 1)                  result.x += grid[point.x - 1][point.y] - grid[point.x][point.y];
    // if (point.x < grid.size() - 2)    result.x -= grid[point.x + 1][point.y] - grid[point.x][point.y];
    if (point.y > 1)                  result.y += grid[point.x][point.y - 1] - grid[point.x][point.y];
    // if (point.y < grid[0].size() - 2) result.y -= grid[point.x][point.y + 1] - grid[point.x][point.y];
    // if (point.x > 1 && point.y > 1) {
    //     result.x += (grid[point.x - 1][point.y - 1] - grid[point.x][point.y]) / std::sqrt(2);
    //     result.y += (grid[point.x - 1][point.y - 1] - grid[point.x][point.y]) / std::sqrt(2);
    // }
    // if (point.x > 1 && point.y < grid[0].size() - 2) {
    //     result.x += (grid[point.x - 1][point.y + 1] - grid[point.x][point.y]) / std::sqrt(2);
    //     result.y -= (grid[point.x - 1][point.y + 1] - grid[point.x][point.y]) / std::sqrt(2);
    // }
    // if (point.x < grid.size() - 2 && point.y > 1) {
    //     result.x -= (grid[point.x + 1][point.y - 1] - grid[point.x][point.y]) / std::sqrt(2);
    //     result.y += (grid[point.x + 1][point.y - 1] - grid[point.x][point.y]) / std::sqrt(2);
    // }
    // if (point.x < grid.size() - 2 && point.y < grid[0].size() - 2) {
    //     result.x -= (grid[point.x + 1][point.y + 1] - grid[point.x][point.y]) / std::sqrt(2);
    //     result.y -= (grid[point.x + 1][point.y + 1] - grid[point.x][point.y]) / std::sqrt(2);
    // }
    return result;
}

std::vector<std::vector<float>> XcumsumGrid, YcumsumGrid;
float discreteIntegral(std::vector<std::vector<float>>& grid, sf::Vector2i from, sf::Vector2i to) {
    // if (from.x > to.x) return discreteIntegral(grid, to, from);
    if (to.x == from.x) {
        if (to.y == from.y) {
            return 0.f;
        }
        if (from.y < to.y) return YcumsumGrid[to.x][to.y] - YcumsumGrid[from.x][from.y] + grid[from.x][from.y] - grid[to.x][to.y];
        return YcumsumGrid[from.x][from.y] - YcumsumGrid[to.x][to.y] + grid[to.x][to.y] - grid[from.x][from.y];
    }
    if (to.y == from.y) {
        if (from.x < to.x) return XcumsumGrid[to.x][to.y] - XcumsumGrid[from.x][from.y] + grid[from.x][from.y] - grid[to.x][to.y];
        return XcumsumGrid[from.x][from.y] - XcumsumGrid[to.x][to.y] + grid[to.x][to.y] - grid[from.x][from.y];
    }
    if (from.x > to.x) std::swap(to, from); // from left to right
    sf::Vector2i d(to - from), cur(from);
    float k = float(d.y) / float(d.x);
    int dy = k > 0 ? 1 : -1, nextY = (cur.x - from.x + 1) * k + from.y, nextX;
    float result = 0.f;
    // if (k > 2.f) {
    //     result += grid[cur.x][cur.y];
    //     while (cur.y != to.y) {
    //         nextY = std::min(int((cur.x + 1 - from.x) * k + from.y), to.y);
    //         result += YcumsumGrid[cur.x][nextY] - YcumsumGrid[cur.x][cur.y] + grid[cur.x][cur.y];
    //         cur.y = nextY; cur.x++;
    //     }
    // } else if (k < -2.f) {
    //     result += grid[to.x][to.y];
    //     while (cur.y != to.y) {
    //         nextY = std::max((int)std::ceil((cur.x + 1 - from.x) * k + from.y), to.y);
    //         result += YcumsumGrid[cur.x][cur.y] - YcumsumGrid[cur.x][nextY] + grid[cur.x][nextY];
    //         cur.y = nextY; cur.x++;
    //     }
    // // } else if (k > 0 && k < 0.5f) {
    // //     while (cur.x != to.x - 1) {
    // //         nextX = std::min(int((cur.y - from.y + 1) / k + from.x), to.x) - 1;
    // //         result += grid[cur.x][cur.y] + XcumsumGrid[nextX][cur.y] - XcumsumGrid[cur.x][cur.y];
    // //         cur.x = nextX;
    // //         cur.y++;
    // //     }
    // //     result += grid[cur.x][cur.y] + XcumsumGrid[to.x - 1][cur.y] - XcumsumGrid[cur.x][cur.y];
    // } else {
        while (cur.x <= to.x && cur.y * dy <= to.y * dy) {
            result += grid[cur.x][cur.y];
            if (nextY == cur.y || cur.y == to.y) {
                cur.x++;
                nextY = (cur.x - from.x + 1) * k + from.y;
            } else {
                cur.y += dy;
            }
        }
    // }
    result -= grid[to.x][to.y];
    return result * length(d) / (d.x + std::abs(d.y));
}

bool keyPressed(sf::Event event, sf::Keyboard::Key key) {
    return event.type == sf::Event::KeyPressed && event.key.code == key;
}

class Slider : public sf::Drawable {
public:
    Slider(sf::Vector2f position, sf::Vector2f size, float value, float lo, float hi) {
        front.setPosition(position);
        front.setSize(size);

        background.setPosition(position);
        background.setSize(size);
        background.setOutlineThickness(5.f);
        background.setOutlineColor(sf::Color::White);

        lower = lo;
        upper = hi;
        setValue(value);
    }
    ~Slider() {}
    void setColors(sf::Color frontColor, sf::Color bgColor, sf::Color boundsColor) {
        front.setFillColor(frontColor);
        background.setFillColor(bgColor);
        background.setOutlineColor(boundsColor);
    }
    void setValue(float value) {
        front.setScale((value - lower) / upper, 1.f);
    }
    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const {
        target.draw(background, states);
        target.draw(front, states);
    }

    sf::RectangleShape front, background;
    float lower, upper, current;
};

class Button : public sf::Drawable {
public:
    Button(sf::Vector2f position, std::string text, sf::Vector2f size = sf::Vector2f(0, 0)) {
        this->text.setFont(arialFont);
        this->text.setCharacterSize(30);
        this->text.setString(text);

        background.setFillColor(sf::Color::White);
        background.setOutlineColor(sf::Color(100, 100, 100));
        background.setOutlineThickness(5.f);
        background.setPosition(position);
        background.setSize(size != sf::Vector2f(0, 0) ? size : this->text.getGlobalBounds().getSize() + sf::Vector2f(15, 13));

        this->text.setPosition(position + sf::Vector2f(5, -2));
        this->text.setFillColor(sf::Color::Black);
    }
    ~Button() {}
    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const {
        target.draw(background, states);
        target.draw(text, states);
    }

    bool isPressed(sf::Event& event) const {
        return event.type == sf::Event::MouseButtonPressed && background.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y);
    }

    sf::RectangleShape background;
    sf::Text text;
};

class PickButton : public sf::Drawable {
public:
    PickButton(sf::Vector2f position, std::vector<std::string> texts) {
        for (int i = 0; i < texts.size(); i++) {
            this->texts.push_back(sf::Text(texts[i], arialFont, 30));

            buttons.push_back(sf::RectangleShape(sf::Vector2f(20, 20)));
            buttons[i].setFillColor(sf::Color::White);
            buttons[i].setOutlineColor(sf::Color(100, 100, 100));
            buttons[i].setOutlineThickness(5.f);
            buttons[i].setPosition(position + sf::Vector2f(0, 40 * i));

            this->texts[i].setPosition(buttons[i].getPosition() + sf::Vector2f(30, -10));
            this->texts[i].setFillColor(sf::Color::White);
        }
        buttons[selected].setFillColor(sf::Color::Red);
    }
    ~PickButton() {}
    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const {
        for (int i = 0; i < buttons.size(); i++) {
            target.draw(buttons[i], states);
            target.draw(texts[i], states);
        }
    }

    void select(int index) {
        buttons[selected].setFillColor(sf::Color::White);
        selected = index;
        buttons[selected].setFillColor(sf::Color::Red);
    }

    std::vector<sf::RectangleShape> buttons;
    std::vector<sf::Text> texts;
    int selected = 0;
};

class SwitchButton : public sf::Drawable {
public:
    SwitchButton(sf::Vector2f position, std::string text) {
        this->text.setFont(arialFont);
        this->text.setCharacterSize(30);
        this->text.setString(text);

        button.setOutlineColor(sf::Color(100, 100, 100));
        button.setOutlineThickness(5.f);
        button.setPosition(position);
        button.setSize(sf::Vector2f(20, 20));

        this->text.setPosition(button.getPosition() + sf::Vector2f(30, -10));
        this->text.setFillColor(sf::Color::White);

        set(false);
    }
    ~SwitchButton() {}
    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const {
        target.draw(button, states);
        target.draw(text, states);
    }

    bool isPressed(sf::Event& event) const {
        return event.type == sf::Event::MouseButtonPressed && button.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y);
    }

    void Switch() {
        set(!isOn);
    }

    void set(bool value) {
        isOn = value;
        this->button.setFillColor(isOn ? sf::Color::Green : sf::Color::Red);
    }

    sf::RectangleShape button;
    sf::Text text;
    bool isOn = false;
};

class MultiSlider : public sf::Drawable {
public:
    MultiSlider(sf::Vector2f position, sf::Vector2f size, float minValue, float maxValue) {
        background.setPosition(position);
        background.setSize(size);
        sf::Image image;
        image.create(1, size.y);
        for (int i = 0; i < size.y; i++) {
            image.setPixel(0, i, sf::Color(255 * i / size.y, 255 * i / size.y, 255 * i / size.y));
        }
        sf::Texture* texture = new sf::Texture();
        texture->create(1, size.y);
        texture->update(image);
        background.setTexture(texture);
        background.setOutlineThickness(5.f);
        background.setOutlineColor(sf::Color(160, 100, 0));
        lower = minValue;
        upper = maxValue;
    }
    ~MultiSlider() {}
    void addValue(float* value, sf::Color color) {
        current.push_back(value);
        text.push_back(sf::Text("", arialFont, 30));
        text.back().setFillColor(color);
        text.back().setOutlineThickness(3.f);
        text.back().setOutlineColor(sf::Color::White);
        lines.push_back(sf::RectangleShape(sf::Vector2f(background.getSize().x, 3)));
        lines.back().setFillColor(color);
    }
    void setValue(float *value, int index) {
        current[index] = value;
    }
    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const {
        target.draw(background, states);
        for (int i = 0; i < current.size(); i++) {
            float y = background.getPosition().y + background.getSize().y * (*current[i] - lower) / (upper - lower);
            text[i].setString(floatToString(*current[i]));
            text[i].setPosition(background.getPosition().x + 30.f, y - text[i].getLocalBounds().height / 2.f - 5.f);
            lines[i].setPosition(background.getPosition().x, y);
            target.draw(lines[i], states);
            target.draw(text[i], states);
        }
    }

    sf::RectangleShape background;
    std::vector<float*> current;
    mutable std::vector<sf::Text> text;
    mutable std::vector<sf::RectangleShape> lines;
    float lower, upper;
};

class MyCircle : public sf::Drawable {
public:
    MyCircle(sf::Vector2f position, sf::Color color, std::string name) {
        shape.setRadius(radius);
        shape.setOutlineColor(color);
        shape.setOutlineThickness(5.f);
        shape.setPosition(position);
        shape.setOrigin(shape.getRadius(), shape.getRadius());

        this->name.setFont(arialFont);
        this->name.setCharacterSize(30);
        this->name.setString(name);
        this->name.setPosition(shape.getPosition() + sf::Vector2f(20.f, -20.f));
        this->name.setOutlineColor(color);
        this->name.setOutlineThickness(3.f);
    }
    ~MyCircle() {}

    sf::Vector2f getPosition() const { return shape.getPosition(); }
    void setPosition(sf::Vector2f position) {
        shape.setPosition(position);
        name.setPosition(shape.getPosition() + sf::Vector2f(20.f, -20.f));
    }
    
    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const {
        target.draw(shape, states);
        target.draw(name, states);
    }
    sf::CircleShape shape;
    sf::Text name;
    static constexpr float radius = 7.f;
};

class TextBox : public sf::Drawable {
public:
    float PosX, PosY;
    sf::Vector2f size;
    sf::Text text;
    std::string string;
    size_t cursorPos = 0;
    sf::RectangleShape rect, cursor;
    bool inputted;

    TextBox(sf::Vector2f position, sf::Vector2f size) : inputted(false) {
        PosX = position.x; PosY = position.y;
        this->size = size;

        text.setCharacterSize(27);
        text.setFont(arialFont);
        text.setFillColor(sf::Color::White);
        text.setString("");
        text.setPosition(PosX + 5, PosY - 4);

        rect.setFillColor(sf::Color(50, 50, 50, 100));
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(2);
        rect.setPosition(PosX, PosY);
        rect.setSize(size);

        cursor.setFillColor(sf::Color::White);
        cursor.setSize({3, size.y});
    }
    ~TextBox() {};
    void clear() {
        string = "";
        cursorPos = 0;
        text.setString(string);
    }
    void setString(std::string str) {
        string = str;
        text.setString(string);
        cursorPos = string.size();
    }
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const {
        target.draw(rect, states);
        target.draw(cursor, states);
        target.draw(text, states);
    }
    void InputText(sf::Event&event) {
        setlocale(LC_ALL, "rus");

        if (inputted) {
            if (event.key.control && keyPressed(event, sf::Keyboard::V)) {
                string = string.substr(0, cursorPos) + sf::Clipboard::getString() + string.substr(cursorPos, string.size() - cursorPos);
                text.setString(string);
                cursorPos += sf::Clipboard::getString().getSize();
            }

            std::string buffer;
            if (event.type == sf::Event::TextEntered && 32 <= event.text.unicode) {
                if (event.text.unicode <= 127) {
                    buffer.push_back(event.text.unicode);
                } else {
                    buffer.push_back(event.text.unicode - 1072 - 32);
                }
                string = string.substr(0, cursorPos) + buffer + string.substr(cursorPos, string.size() - cursorPos);
                text.setString(string);
                cursorPos++;
            }

            if (event.type == sf::Event::MouseButtonPressed && !rect.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y)) {
                inputted = false;
                return;
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::BackSpace && cursorPos > 0) {
                    string.erase(cursorPos - 1, 1);
                    text.setString(string);
                    cursorPos--;
                }
                if (event.key.code == sf::Keyboard::Delete && cursorPos < string.size()) {
                    string.erase(cursorPos, 1);
                    text.setString(string);
                }
                if (event.key.code == sf::Keyboard::Left && cursorPos > 0) cursorPos--;
                if (event.key.code == sf::Keyboard::Right && cursorPos < string.size()) cursorPos++;
                if (event.key.code == sf::Keyboard::Home) cursorPos = 0;
                if (event.key.code == sf::Keyboard::End) cursorPos = string.size();
            }
        } else if (event.type == sf::Event::MouseButtonPressed && rect.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y)) {
            inputted = true;
        }

        sf::Text tempText; tempText.setString(string.substr(0, cursorPos));
        tempText.setCharacterSize(27);
        tempText.setFont(arialFont);
        cursor.setPosition(PosX + 5 + tempText.getGlobalBounds().width, PosY);
    }
};