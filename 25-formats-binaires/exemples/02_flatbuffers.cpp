/* ============================================================================
   Section 25.2 : FlatBuffers : Zéro-copy serialization
   Description : Construction de messages (FlatBufferBuilder, offsets),
                 Builder pattern, lecture zero-copy, vérification de buffer
                 (Verifier), Object API (Pack/UnPack), écriture/lecture fichier
   Fichier source : 02-flatbuffers.md
   ============================================================================ */
#include "schema_generated.h"
#include <flatbuffers/flatbuffers.h>
#include <print>
#include <fstream>
#include <vector>
#include <cassert>

int main() {
    // === Construction de messages (l.260-307) ===
    std::print("=== Construction de messages ===\n");
    flatbuffers::FlatBufferBuilder builder(1024);

    // Les chaînes et vecteurs doivent être créés AVANT la table
    auto name = builder.CreateString("Alice Martin");
    auto email = builder.CreateString("alice@example.com");
    auto dept = builder.CreateString("Engineering");

    // Vecteur de chaînes
    std::vector<flatbuffers::Offset<flatbuffers::String>> tag_offsets;
    tag_offsets.push_back(builder.CreateString("senior"));
    tag_offsets.push_back(builder.CreateString("on-call"));
    auto tags = builder.CreateVector(tag_offsets);

    // Construction de la table User
    auto user = myapp::CreateUser(
        builder,
        42,                    // id
        name,                  // name (offset)
        email,                 // email (offset)
        myapp::Role_Admin,     // role
        dept,                  // department (offset)
        tags,                  // tags (offset vers vecteur)
        98.5f                  // score
    );

    // Construction de UserList
    std::vector<flatbuffers::Offset<myapp::User>> user_offsets;
    user_offsets.push_back(user);
    auto users_vec = builder.CreateVector(user_offsets);

    auto user_list = myapp::CreateUserList(builder, users_vec, 1);

    // Finaliser le buffer
    builder.Finish(user_list);

    // Accès au buffer résultant
    uint8_t* buf = builder.GetBufferPointer();
    size_t size = builder.GetSize();
    std::print("Buffer : {} octets\n", size);

    // === Builder pattern (l.323-337) ===
    std::print("\n=== Builder pattern ===\n");
    {
        flatbuffers::FlatBufferBuilder builder2(1024);
        auto n = builder2.CreateString("Bob");
        auto e = builder2.CreateString("bob@example.com");

        myapp::UserBuilder user_builder(builder2);
        user_builder.add_id(99);
        user_builder.add_name(n);
        user_builder.add_email(e);
        user_builder.add_role(myapp::Role_User);
        user_builder.add_score(75.0f);
        auto u = user_builder.Finish();

        std::vector<flatbuffers::Offset<myapp::User>> users2;
        users2.push_back(u);
        auto vec2 = builder2.CreateVector(users2);
        auto list2 = myapp::CreateUserList(builder2, vec2, 1);
        builder2.Finish(list2);
        std::print("Builder pattern OK, {} octets\n", builder2.GetSize());
    }

    // === Lecture zero-copy (l.347-374) ===
    std::print("\n=== Lecture zero-copy ===\n");
    {
        const myapp::UserList* list = myapp::GetUserList(buf);

        std::print("Total : {}\n", list->total_count());

        const auto* users = list->users();
        if (users) {
            for (size_t i = 0; i < users->size(); ++i) {
                const myapp::User* u = users->Get(i);

                std::print("ID    : {}\n", u->id());
                std::print("Name  : {}\n", u->name()->c_str());
                std::print("Email : {}\n", u->email()->c_str());
                std::print("Role  : {}\n", static_cast<int>(u->role()));
                std::print("Score : {}\n", u->score());

                if (u->department()) {
                    std::print("Dept  : {}\n", u->department()->c_str());
                }

                if (u->tags()) {
                    for (size_t j = 0; j < u->tags()->size(); ++j) {
                        std::print("Tag   : {}\n",
                            u->tags()->Get(j)->c_str());
                    }
                }
            }
        }
    }

    // === Vérification de buffer (l.400-429) ===
    std::print("\n=== Vérification buffer ===\n");
    {
        flatbuffers::Verifier verifier(buf, size);
        if (myapp::VerifyUserListBuffer(verifier)) {
            std::print("Buffer vérifié OK\n");
        } else {
            std::print(stderr, "Buffer invalide !\n");
            return 1;
        }

        // Test avec buffer corrompu
        std::vector<uint8_t> bad(10, 0xFF);
        flatbuffers::Verifier bad_verifier(bad.data(), bad.size());
        if (!myapp::VerifyUserListBuffer(bad_verifier)) {
            std::print("Buffer corrompu détecté OK\n");
        }
    }

    // === Object API (l.433-468) ===
    std::print("\n=== Object API ===\n");
    {
        // Construction avec l'Object API
        auto u = std::make_unique<myapp::UserT>();
        u->id = 42;
        u->name = "Alice Martin";
        u->email = "alice@example.com";
        u->role = myapp::Role_Admin;
        u->department = "Engineering";
        u->tags = {"senior", "on-call"};
        u->score = 98.5f;

        auto ul = std::make_unique<myapp::UserListT>();
        ul->users.push_back(std::move(u));
        ul->total_count = 1;

        // Conversion Object → Buffer
        flatbuffers::FlatBufferBuilder fb_builder;
        auto offset = myapp::UserList::Pack(fb_builder, ul.get());
        fb_builder.Finish(offset);

        // Conversion Buffer → Object
        const myapp::UserList* fb =
            myapp::GetUserList(fb_builder.GetBufferPointer());
        auto restored = fb->UnPack();

        std::print("Name : {}\n", restored->users[0]->name);
    }

    // === Écriture/lecture fichier (l.477-523) ===
    std::print("\n=== Fichier ===\n");
    {
        // Écriture
        {
            std::ofstream file("data_test.fbs.bin", std::ios::binary);
            file.write(reinterpret_cast<const char*>(buf), size);
        }

        // Lecture
        {
            std::ifstream infile("data_test.fbs.bin", std::ios::binary);
            std::vector<uint8_t> file_buf(
                std::istreambuf_iterator<char>(infile), {});

            flatbuffers::Verifier verifier(file_buf.data(), file_buf.size());
            if (myapp::VerifyUserListBuffer(verifier)) {
                const myapp::UserList* from_file =
                    myapp::GetUserList(file_buf.data());
                std::print("Depuis fichier : {}\n",
                    from_file->users()->Get(0)->name()->c_str());
            }
        }

        std::remove("data_test.fbs.bin");
    }

    std::print("\nTous les tests passent !\n");
    return 0;
}
