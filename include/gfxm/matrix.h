#pragma once

#include <algorithm>
#include <math.h>
#include <assert.h>
#include <cstring>
#include <array>
#include <string>

// TODO use std valarray?

namespace gfxm {

template <unsigned char ROWS, unsigned char COLS>
class Matrix {
    std::array<float, ROWS * COLS> mat;

public:
    constexpr Matrix() noexcept { mat.fill(0.0f); }

    // Construct from column-major order
    constexpr Matrix(const std::array<float, ROWS * COLS>& list) noexcept {
        std::copy(list.begin(), list.end(), mat.data());
    }

    // Construct from another matrix
    // If the other matrix is larger its elements outside the new matrix size will be discarded
    // If the other matrix is smaller elements in the new matrix outside the old matrix size will be 0.
    template <unsigned char OTHER_ROWS, unsigned char OTHER_COLS>
    explicit constexpr Matrix(const Matrix<OTHER_ROWS, OTHER_COLS>& from) {
        mat.fill(0.0f);

        for (int c = 0; c < OTHER_COLS && c < COLS; c++) {
            for (int r = 0; r < OTHER_ROWS && r < ROWS; r++) {
                (*this)[r, c] = from[r, c];
            }
        }
    }

    // Construct from row-major order
    static constexpr Matrix<ROWS, COLS> from_rowmajor(const std::array<float, ROWS * COLS>& list) noexcept {
        Matrix<ROWS, COLS> out;

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                out[r, c] = list[r * COLS + c];
            }
        }

        return out;
    }

    static constexpr Matrix<ROWS, COLS> from_columns(const std::array<Matrix<ROWS, 1>, COLS>& cols) noexcept {
        Matrix<ROWS, COLS> out;
        std::copy((float*)cols.begin(), (float*)cols.end(), out.array().data());
        return out;
    }

    Matrix<ROWS, COLS> copy() const { return *this; }

    float operator[](unsigned char row, unsigned char col) const {
        assert(row < ROWS && col < COLS);
        return mat[col * ROWS + row];
    }

    float& operator[](unsigned char row, unsigned char col) {
        assert(row < ROWS && col < COLS);
        return mat[col * ROWS + row];
    }

    float operator[](unsigned char row) const {
        static_assert(COLS == 1);
        assert(row < ROWS);
        return mat[row];
    }

    float& operator[](unsigned char row) {
        static_assert(COLS == 1);
        assert(row < ROWS);
        return mat[row];
    }

    static constexpr Matrix<ROWS, COLS> identity() {
        static_assert(ROWS == COLS, "identity only defined for square matrices");

        Matrix<ROWS, COLS> out;

        for (int i = 0; i < ROWS; i++) {
            out[i, i] = 1.0f;
        }

        return out;
    }

    Matrix<3, 1> cross(const Matrix<3, 1>& rhs) const {
        static_assert(ROWS == 3 && COLS == 1, "cross product only defined for 3D column vectors");

        return Matrix<3, 1>({(*this)[1, 0] * rhs[2, 0] - (*this)[2, 0] * rhs[1, 0],
                             (*this)[2, 0] * rhs[0, 0] - (*this)[0, 0] * rhs[2, 0],
                             (*this)[0, 0] * rhs[1, 0] - (*this)[1, 0] * rhs[0, 0]});
    }

    float dot(const Matrix<ROWS, 1>& rhs) const {
        static_assert(COLS == 1, "dot product only defined for column vectors");

        float ans = 0.0f;
        for (int i = 0; i < ROWS; i++) {
            ans += (*this)[i, 0] * rhs[i, 0];
        }

        return ans;
    }

    float magnitude() const {
        float sum = 0.0f;

        for (int i = 0; i < ROWS; i++) {
            sum += (*this)[i, 0] * (*this)[i, 0];
        }

        return sqrtf(sum);
    }

    // Normalise the vector and return a reference to it to allow chaining.
    Matrix<ROWS, 1>& normalise() {
        float magnitude = this->magnitude();

        for (int i = 0; i < ROWS; i++) {
            (*this)[i, 0] /= magnitude;
        }

        return *this;
    }

    // Adds values in the array to each element in the final column other than the last, from top to bottom.
    // The transformation is applied to the instance, and a reference to the instance is returned to allow chaining.
    Matrix<ROWS, COLS>& translate(const Matrix<ROWS - 1, 1>& translation) {
        for (int i = 0; i < ROWS - 1; i++) {
            (*this)[i, COLS - 1] += translation[i, 0];
        }

        return *this;
    }

    // Transpose the matrix
    // The transformation is applied to the instance, and a reference to the instance is returned to allow chaining.
    Matrix<ROWS, COLS>& transpose() {
        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < c; r++) {
                std::swap((*this)[r, c], (*this)[c, r]);
            }
        }

        return *this;
    }

    std::array<float, ROWS * COLS>& array() { return mat; }

    Matrix<ROWS + 1, 1> appending_element(float k) const {
        static_assert(COLS == 1, "appending_element is only defined for column vectors");

        Matrix<ROWS + 1, 1> out;
        std::copy(mat.begin(), mat.end(), out.array().data());
        out[ROWS, 0] = k;

        return out;
    }

    template <unsigned char OTHER_COLS>
    Matrix<ROWS, OTHER_COLS> operator*(const Matrix<COLS, OTHER_COLS>& rhs) const {
        Matrix<ROWS, OTHER_COLS> out;

        for (int rcl = 0; rcl < OTHER_COLS; rcl++) {
            for (int lcl = 0; lcl < COLS; lcl++) {
                for (int lrw = 0; lrw < ROWS; lrw++) {
                    out[lrw, rcl] += (*this)[lrw, lcl] * rhs[lcl, rcl];
                }
            }
        }

        return out;
    }

    Matrix<ROWS, COLS> operator*(float scalar) const {
        Matrix<ROWS, COLS> out;

        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < ROWS; r++) {
                out[r, c] = (*this)[r, c] * scalar;
            }
        }

        return out;
    }

    Matrix<ROWS, COLS> operator+(const Matrix<ROWS, COLS>& rhs) const {
        Matrix<ROWS, COLS> out;

        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < ROWS; r++) {
                out[r, c] = (*this)[r, c] + rhs[r, c];
            }
        }

        return out;
    }

    Matrix<ROWS, COLS> operator-() const {
        Matrix<ROWS, COLS> out;

        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < ROWS; r++) {
                out[r, c] = -(*this)[r, c];
            }
        }

        return out;
    }

    Matrix<ROWS, COLS> operator-(const Matrix<ROWS, COLS> rhs) const {
        Matrix<ROWS, COLS> out;

        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < ROWS; r++) {
                out[r, c] = (*this)[r, c] - rhs[r, c];
            }
        }

        return out;
    }

    std::string to_string() const {
        std::string out;

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                out += std::to_string((*this)[r, c]) + " ";
            }

            out += "\n";
        }

        return out;
    }
};

template <unsigned char ROWS>
using Vec = Matrix<ROWS, 1>;

}  // namespace gfxm