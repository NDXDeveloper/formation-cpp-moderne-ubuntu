/* ============================================================================
   Section 32.3 : clang-format — Formatage automatique
   Description : Code volontairement mal formaté pour démontrer clang-format.
                 Exécuter clang-format pour voir la transformation.
   Fichier source : 03-clang-format.md
   Exécution : clang-format --style=Google 03_clang_format_demo.cpp
               clang-format --style=LLVM 03_clang_format_demo.cpp
               clang-format -i --style=file 03_clang_format_demo.cpp
   ============================================================================ */

#include<vector>
#include <string>
class Config{
std::string host;int port;
public:
Config(std::string h,int p):host(h),port(p){}
std::string getHost(){return host;}
};
void traiter(std::vector<int> donnees){for(int i=0;i<donnees.size();++i){if(donnees[i]==0){int r=100/donnees[i];}}}
