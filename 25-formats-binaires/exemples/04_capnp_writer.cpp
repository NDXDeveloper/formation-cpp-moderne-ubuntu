/* ============================================================================
   Section 25.3.1 : Philosophie — le format wire EST le format memoire
   Description : Construction d'un message Cap'n Proto (zero-copy write)
   Fichier source : 03.1-philosophie-capnproto.md
   ============================================================================ */

// Compilation :
//   capnp compile -oc++ sensor.capnp
//   g++-15 -std=c++23 -O2 -o 04_capnp_writer 04_capnp_writer.cpp sensor.capnp.c++ -lcapnp -lkj

#include "sensor.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <unistd.h>

int main() {
    // MallocMessageBuilder alloue le(s) segment(s) du message.
    // TOUTES les donnees sont ecrites directement dans ces segments.
    capnp::MallocMessageBuilder message;

    // initRoot<Sensor>() retourne un BUILDER : un accesseur
    // qui ecrit directement dans le buffer du message.
    auto sensor = message.initRoot<Sensor>();

    sensor.setId(42);
    sensor.setTemperature(23.5f);
    sensor.setLocation("Paris");

    auto tags = sensor.initTags(2);
    tags.set(0, "indoor");
    tags.set(1, "lab-3");

    // writeMessageToFd() ecrit les segments directement via writev().
    // Aucune copie supplementaire, aucune transformation.
    capnp::writeMessageToFd(STDOUT_FILENO, message);

    return 0;
}
