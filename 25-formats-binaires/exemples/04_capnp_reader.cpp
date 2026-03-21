/* ============================================================================
   Section 25.3.1 : Philosophie — le format wire EST le format memoire
   Description : Lecture d'un message Cap'n Proto (zero-copy read)
   Fichier source : 03.1-philosophie-capnproto.md
   ============================================================================ */

// Compilation :
//   capnp compile -oc++ sensor.capnp
//   g++-15 -std=c++23 -O2 -o 04_capnp_reader 04_capnp_reader.cpp sensor.capnp.c++ -lcapnp -lkj
// Execution :
//   ./04_capnp_writer | ./04_capnp_reader

#include "sensor.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <iostream>
#include <unistd.h>

int main() {
    // StreamFdMessageReader lit les donnees depuis un fd
    // dans un buffer interne. Pas de parsing — acces direct.
    capnp::StreamFdMessageReader message(STDIN_FILENO);

    // getRoot<Sensor>() retourne un READER : un accesseur
    // qui lit directement depuis le buffer recu.
    auto sensor = message.getRoot<Sensor>();

    std::cout << "ID:          " << sensor.getId() << "\n";
    std::cout << "Temperature: " << sensor.getTemperature() << " C\n";
    std::cout << "Location:    " << sensor.getLocation().cStr() << "\n";

    std::cout << "Tags:\n";
    for (auto tag : sensor.getTags()) {
        std::cout << "  - " << tag.cStr() << "\n";
    }

    return 0;
}
