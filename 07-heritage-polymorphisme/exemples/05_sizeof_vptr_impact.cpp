/* ============================================================================
   Section 7.5 : Cout du polymorphisme — Surcout memoire du vptr
   Description : Comparaison sizeof entre une classe sans virtual et une avec.
                 Le vptr double la taille d'un petit objet (Point3D).
   Fichier source : 05-cout-polymorphisme.md
   ============================================================================ */
#include <print>

class Point3D {
    float x_, y_, z_;   // 12 octets
};

class Point3DVirtual {
    float x_, y_, z_;   // 12 octets
public:
    virtual ~Point3DVirtual() = default;
};

static_assert(sizeof(Point3D) == 12);
static_assert(sizeof(Point3DVirtual) == 24);   // 8 (vptr) + 12 + 4 (padding)

int main() {
    std::println("sizeof(Point3D)        = {} octets", sizeof(Point3D));
    std::println("sizeof(Point3DVirtual) = {} octets", sizeof(Point3DVirtual));
    std::println("Ratio = {:.1f}x", static_cast<double>(sizeof(Point3DVirtual)) / sizeof(Point3D));
}
