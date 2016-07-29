#include <iostream>

#include "linear_ptr/linear_ptr.h"

using namespace hf;

int main() {
    linear_ptr<int> x(new int(1));

    constexpr int N=5;

    std::cout << "x: ";
    if (x) std::cout << *x; else std::cout << "nup";
    std::cout << "\n";

    linear_ptr<int> y[N];
    for (auto& yi: y) yi=x;

    int round=1;
    while (!x) {
        std::cout << "round " << round++ << "\n";

        #pragma omp parallel for 
        for (int i=0; i<N; ++i) {
            auto& yi=y[i];
            if (!yi) continue;
            ++*yi;
            yi.reset();
        }

        for (int i=0; i<N; ++i) {
            std::cout << "y[" << i << "]: ";
            if (y[i]) std::cout << *(y[i]); else std::cout << "nup";
            std::cout << "\n";
        }
    }

    std::cout << "x: ";
    if (x) std::cout << *x; else std::cout << "nup";
    std::cout << "\n";
}
