/* ============================================================================
   Section 6.1 : Définition de classes — Membres et méthodes
   Description : Séparation déclaration (.h) / définition (.cpp) — header
   Fichier source : 01-definition-classes.md
   ============================================================================ */
#pragma once

#include <string>

class Sensor {
public:
    Sensor(int id, const std::string& name);

    void read(double new_value);
    double value() const;
    std::string to_string() const;

private:
    int id_;
    std::string name_;
    double value_ = 0.0;
};
