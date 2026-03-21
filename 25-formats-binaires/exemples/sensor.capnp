# ============================================================================
# Section 25.3.1 : Philosophie — le format wire EST le format memoire
# Description : Schema Cap'n Proto pour un capteur (Sensor)
# Fichier source : 03.1-philosophie-capnproto.md
# ============================================================================

@0xdbb9ad1f14bf0b36;

struct Sensor {
  id          @0 : UInt32;
  temperature @1 : Float32;
  location    @2 : Text;
  tags        @3 : List(Text);
}
