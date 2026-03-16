#!/bin/bash
# ============================================================================
# Section 27.4 : Linkage statique (.a) vs dynamique (.so)
# Description : Script de build illustrant les deux modes de linkage
#               manuellement (sans CMake) comme décrit dans le .md
# Fichier source : 04-linkage-statique-dynamique.md
# ============================================================================

set -e
cd "$(dirname "$0")"
CXX=${CXX:-g++-15}

echo "=== 1. Compilation des fichiers objets ==="
$CXX -c greeter.cpp -o greeter.o
$CXX -fPIC -c greeter.cpp -o greeter_pic.o

echo ""
echo "=== 2. Bibliothèque STATIQUE (.a) ==="
ar rcs libgreeter.a greeter.o
echo "Contenu de l'archive :"
ar t libgreeter.a

echo ""
echo "=== 3. Bibliothèque DYNAMIQUE (.so) ==="
$CXX -shared -o libgreeter.so greeter_pic.o

echo ""
echo "=== 4. Linkage STATIQUE (forcer avec -Bstatic) ==="
$CXX main.cpp -L. -Wl,-Bstatic -lgreeter -Wl,-Bdynamic -o app_static
echo "Taille   : $(ls -lh app_static | awk '{print $5}')"
echo "Deps     : $(ldd app_static 2>&1 | grep -c greeter) dépendance(s) greeter"
echo "Sortie   :"
./app_static

echo ""
echo "=== 5. Linkage DYNAMIQUE ==="
$CXX main.cpp -L. -lgreeter -o app_dynamic
echo "Taille   : $(ls -lh app_dynamic | awk '{print $5}')"
echo "Deps     :"
ldd app_dynamic | grep greeter
echo "Sortie   :"
LD_LIBRARY_PATH=. ./app_dynamic

echo ""
echo "=== 6. Vérification avec file ==="
echo "Statique  : $(file app_static | grep -o 'dynamically linked\|statically linked')"
echo "Dynamique : $(file app_dynamic | grep -o 'dynamically linked\|statically linked')"

echo ""
echo "=== 7. Symboles dans libgreeter.a ==="
nm libgreeter.a | grep " T " | head -5

echo ""
echo "=== Nettoyage ==="
rm -f *.o *.a *.so app_static app_dynamic
echo "Terminé !"
