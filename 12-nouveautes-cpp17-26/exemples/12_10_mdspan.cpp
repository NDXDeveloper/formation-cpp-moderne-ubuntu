/* ============================================================================
   Section 12.10 : std::mdspan (C++23)
   Description : Vues multidimensionnelles - mdspan basique, extents
                 (dynamic, static, mixed, CTAD), traitement d'image,
                 multiplication matricielle, inspection
   NOTE : Ne compile pas avec GCC 15 (libstdc++ ne fournit pas <mdspan>)
          Necessite un compilateur avec support complet de mdspan.
   Fichier source : 10-mdspan.md
   ============================================================================ */
#include <mdspan>
#include <vector>
#include <print>
#include <cassert>
#include <cstdint>

void test_basic() {
    std::print("=== mdspan basic ===\n");
    // Lignes 58-73
    std::vector<double> data(3 * 4, 0.0);
    std::mdspan matrix(data.data(), 3, 4);
    matrix[1, 2] = 3.14;
    std::print("matrix[1,2] = {}\n", matrix[1, 2]);
}

void test_extents() {
    std::print("=== extents ===\n");
    // Lignes 123-137
    double buf[12] = {};

    // Dynamic
    std::mdspan<double, std::dextents<size_t, 2>> m1(buf, 3, 4);
    std::print("dynamic: {}x{}\n", m1.extent(0), m1.extent(1));

    // Static
    std::mdspan<double, std::extents<size_t, 3, 4>> m2(buf);
    std::print("static: {}x{}\n", m2.extent(0), m2.extent(1));

    // Mixed
    std::mdspan<double, std::extents<size_t, std::dynamic_extent, 4>> m3(buf, 3);
    std::print("mixed: {}x{}\n", m3.extent(0), m3.extent(1));

    // CTAD
    std::vector<double> data2(12);
    std::mdspan matrix(data2.data(), 3, 4);
    std::print("CTAD: {}x{}\n", matrix.extent(0), matrix.extent(1));
}

void test_image() {
    std::print("=== image processing ===\n");
    // Lignes 231-259 (simplified)
    constexpr size_t W = 4, H = 3;
    std::vector<uint8_t> pixels(H * W * 3, 0);
    std::mdspan image(pixels.data(), H, W, 3);

    image[1, 2, 0] = 255;   // Rouge
    image[1, 2, 1] = 128;   // Vert
    image[1, 2, 2] = 0;     // Bleu

    std::print("pixel[1,2] RGB = ({}, {}, {})\n",
        image[1, 2, 0], image[1, 2, 1], image[1, 2, 2]);

    // Grayscale conversion
    for (size_t y = 0; y < image.extent(0); ++y) {
        for (size_t x = 0; x < image.extent(1); ++x) {
            auto r = image[y, x, 0];
            auto g = image[y, x, 1];
            auto b = image[y, x, 2];
            uint8_t gray = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
            image[y, x, 0] = gray;
            image[y, x, 1] = gray;
            image[y, x, 2] = gray;
        }
    }
    std::print("after grayscale: pixel[1,2] = ({}, {}, {})\n",
        image[1, 2, 0], image[1, 2, 1], image[1, 2, 2]);
}

// === Matrix multiplication (lignes 265-303) ===
using matrix_t = std::mdspan<double, std::dextents<size_t, 2>>;

void mat_mul(matrix_t A, matrix_t B, matrix_t C) {
    assert(A.extent(1) == B.extent(0));
    assert(C.extent(0) == A.extent(0));
    assert(C.extent(1) == B.extent(1));

    for (size_t i = 0; i < A.extent(0); ++i) {
        for (size_t j = 0; j < B.extent(1); ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < A.extent(1); ++k) {
                sum += A[i, k] * B[k, j];
            }
            C[i, j] = sum;
        }
    }
}

void test_matmul() {
    std::print("=== matrix multiplication ===\n");
    std::vector<double> a_data = {1, 2, 3, 4, 5, 6};     // 2x3
    std::vector<double> b_data = {7, 8, 9, 10, 11, 12};   // 3x2
    std::vector<double> c_data(4, 0.0);                    // 2x2

    matrix_t A(a_data.data(), 2, 3);
    matrix_t B(b_data.data(), 3, 2);
    matrix_t C(c_data.data(), 2, 2);

    mat_mul(A, B, C);
    std::print("C[0,0]={}, C[0,1]={}, C[1,0]={}, C[1,1]={}\n",
               C[0, 0], C[0, 1], C[1, 0], C[1, 1]);
    // C[0,0]=58, C[0,1]=64, C[1,0]=139, C[1,1]=154
}

void test_inspection() {
    std::print("=== inspection ===\n");
    // Lignes 390-399
    std::vector<double> data(12);
    std::mdspan matrix(data.data(), 3, 4);

    std::print("rank={}, extent(0)={}, extent(1)={}, size={}, empty={}\n",
        matrix.rank(), matrix.extent(0), matrix.extent(1),
        matrix.size(), matrix.empty());
}

int main() {
    test_basic();
    test_extents();
    test_image();
    test_matmul();
    test_inspection();
}
