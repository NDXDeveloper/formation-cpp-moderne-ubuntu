/* ============================================================================
   Section 25.1.2 : Génération de code C++
   Description : Construction et sérialisation, DebugString, désérialisation
                 et lecture, copie/move/comparaison, MergeFrom, conversion
                 JSON bidirectionnelle, Arenas
   Fichier source : 01.2-generation-code.md
   ============================================================================ */
#include "user.pb.h"
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>
#include <google/protobuf/arena.h>
#include <print>
#include <cassert>

myapp::User create_user() {
    myapp::User user;
    user.set_id(42);
    user.set_name("Alice Martin");
    user.set_email("alice@example.com");
    user.set_role(myapp::ROLE_ADMIN);
    user.set_department("Engineering");

    user.add_tags("senior");
    user.add_tags("on-call");

    return user;
}

int main() {
    // === Construction et sérialisation (l.366-397) ===
    std::print("=== Construction et sérialisation ===\n");
    std::string binary;
    {
        auto user = create_user();

        bool ok = user.SerializeToString(&binary);
        assert(ok);
        std::print("Taille binaire : {} octets\n", binary.size());

        std::print("Debug :\n{}\n", user.DebugString());
    }

    // === Désérialisation et lecture (l.413-430) ===
    std::print("=== Désérialisation ===\n");
    {
        myapp::User restored;
        if (!restored.ParseFromString(binary)) {
            std::print(stderr, "Désérialisation échouée\n");
            return 1;
        }

        std::print("Nom  : {}\n", restored.name());
        std::print("Role : {}\n", myapp::Role_Name(restored.role()));

        if (restored.has_department()) {
            std::print("Dept : {}\n", restored.department());
        }

        for (const auto& tag : restored.tags()) {
            std::print("Tag  : {}\n", tag);
        }
    }

    // === Copie, move, comparaison (l.436-453) ===
    std::print("\n=== Copie, move, comparaison ===\n");
    {
        myapp::User user1 = create_user();

        // Copie
        myapp::User user2 = user1;
        assert(google::protobuf::util::MessageDifferencer::Equals(user1, user2));

        // Modification
        user2.set_name("Bob");
        assert(!google::protobuf::util::MessageDifferencer::Equals(user1, user2));

        // Move
        myapp::User user3 = std::move(user1);

        // Swap
        user2.Swap(&user3);
        std::print("Après swap : user2={}, user3={}\n",
            user2.name(), user3.name());
    }

    // === MergeFrom (l.461-475) ===
    std::print("\n=== MergeFrom ===\n");
    {
        myapp::User base;
        base.set_name("Alice");
        base.set_role(myapp::ROLE_USER);
        base.add_tags("team-a");

        myapp::User override_msg;
        override_msg.set_role(myapp::ROLE_ADMIN);
        override_msg.add_tags("on-call");

        base.MergeFrom(override_msg);
        std::print("name={}, role={}\n",
            base.name(), myapp::Role_Name(base.role()));
        std::print("tags:");
        for (const auto& tag : base.tags()) {
            std::print(" {}", tag);
        }
        std::print("\n");
    }

    // === Conversion JSON (l.485-527) ===
    std::print("\n=== Conversion JSON ===\n");
    {
        auto user = create_user();

        // Protobuf → JSON
        std::string json_output;
        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = true;
        options.always_print_primitive_fields = true;
        options.preserve_proto_field_names = true;

        auto status = google::protobuf::util::MessageToJsonString(
            user, &json_output, options);

        if (status.ok()) {
            std::print("JSON :\n{}\n", json_output);
        }

        // JSON → Protobuf
        std::string json_input = R"({
            "id": 99,
            "name": "Bob",
            "email": "bob@example.com",
            "role": "ROLE_USER",
            "tags": ["new-hire"]
        })";

        myapp::User from_json;
        google::protobuf::util::JsonParseOptions parse_options;
        parse_options.ignore_unknown_fields = true;

        status = google::protobuf::util::JsonStringToMessage(
            json_input, &from_json, parse_options);

        if (status.ok()) {
            std::print("Depuis JSON : {} ({})\n",
                from_json.name(), myapp::Role_Name(from_json.role()));
        }
    }

    // === Arenas (l.543-559) ===
    std::print("\n=== Arenas ===\n");
    {
        google::protobuf::Arena arena;

        auto* request =
            google::protobuf::Arena::CreateMessage<myapp::User>(&arena);
        request->set_name("Alice");
        request->set_id(42);

        std::print("Arena user: id={}, name={}\n",
            request->id(), request->name());
    }

    std::print("\nTous les tests passent !\n");
    return 0;
}
