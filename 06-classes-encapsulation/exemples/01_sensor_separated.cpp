/* ============================================================================
   Section 6.1 : Définition de classes — Membres et méthodes
   Description : Séparation déclaration (.h) / définition (.cpp) — implémentation
   Fichier source : 01-definition-classes.md
   Compilation : g++ -std=c++17 -Wall -Wextra 01_sensor_separated.cpp
                     01_sensor_separated_main.cpp -o 01_sensor_separated
   ============================================================================ */
#include "01_sensor_separated.h"

#include <sstream>

Sensor::Sensor(int id, const std::string& name)
    : id_(id), name_(name) {}

void Sensor::read(double new_value) {
    value_ = new_value;
}

double Sensor::value() const {
    return value_;
}

std::string Sensor::to_string() const {
    std::ostringstream oss;
    oss << name_ << " (#" << id_ << "): " << value_;
    return oss.str();
}
