/* ============================================================================
   Section 22.6.3 : Génération de code et implémentation
   Description : Serveur/client gRPC TaskManager — CreateTask, GetTask,
                 ListTasks avec pagination, gestion NOT_FOUND, deadline
   Fichier source : 06.3-generation-code.md
   ============================================================================ */
#include <grpcpp/grpcpp.h>
#include "generated/taskmanager.grpc.pb.h"

#include <print>
#include <map>
#include <mutex>
#include <string>
#include <chrono>
#include <atomic>
#include <thread>
#include <optional>
#include <vector>

using namespace taskmanager::v1;

// -- Server implementation (from .md) --
class TaskManagerImpl final : public TaskManager::Service {
public:
    grpc::Status CreateTask(
        grpc::ServerContext* /*context*/,
        const CreateTaskRequest* request,
        CreateTaskResponse* response) override
    {
        std::string id = "task-" + std::to_string(next_id_++);

        Task task;
        task.set_id(id);
        task.set_title(request->title());
        task.set_description(request->description());
        task.set_completed(false);

        auto now = std::chrono::system_clock::now();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
        task.mutable_created_at()->set_seconds(seconds);

        {
            std::lock_guard lock{mutex_};
            tasks_[id] = task;
        }

        *response->mutable_task() = task;

        std::println("[CreateTask] Créé: {} - '{}'", id, task.title());
        return grpc::Status::OK;
    }

    grpc::Status GetTask(
        grpc::ServerContext* /*context*/,
        const GetTaskRequest* request,
        GetTaskResponse* response) override
    {
        std::lock_guard lock{mutex_};
        auto it = tasks_.find(request->task_id());

        if (it == tasks_.end()) {
            return grpc::Status(
                grpc::StatusCode::NOT_FOUND,
                "Tâche non trouvée: " + request->task_id()
            );
        }

        *response->mutable_task() = it->second;
        return grpc::Status::OK;
    }

    grpc::Status ListTasks(
        grpc::ServerContext* /*context*/,
        const ListTasksRequest* request,
        ListTasksResponse* response) override
    {
        std::lock_guard lock{mutex_};

        int page_size = request->page_size() > 0 ? request->page_size() : 10;
        bool past_token = request->page_token().empty();
        int count = 0;

        for (const auto& [id, task] : tasks_) {
            if (!past_token) {
                if (id == request->page_token()) {
                    past_token = true;
                }
                continue;
            }

            if (count >= page_size) {
                response->set_next_page_token(id);
                break;
            }

            *response->add_tasks() = task;
            count++;
        }

        return grpc::Status::OK;
    }

private:
    std::map<std::string, Task> tasks_;
    std::mutex mutex_;
    std::atomic<int> next_id_{1};
};

// -- Client (from .md) --
class TaskManagerClient {
public:
    explicit TaskManagerClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(TaskManager::NewStub(channel)) {}

    std::optional<Task> CreateTask(const std::string& title,
                                    const std::string& description) {
        CreateTaskRequest request;
        request.set_title(title);
        request.set_description(description);

        CreateTaskResponse response;
        grpc::ClientContext context;
        context.set_deadline(
            std::chrono::system_clock::now() + std::chrono::seconds(5));

        grpc::Status status = stub_->CreateTask(&context, request, &response);

        if (!status.ok()) {
            std::println(stderr, "CreateTask failed: {} ({})",
                         status.error_message(),
                         static_cast<int>(status.error_code()));
            return std::nullopt;
        }

        return response.task();
    }

    std::optional<Task> GetTask(const std::string& task_id) {
        GetTaskRequest request;
        request.set_task_id(task_id);

        GetTaskResponse response;
        grpc::ClientContext context;
        context.set_deadline(
            std::chrono::system_clock::now() + std::chrono::seconds(5));

        grpc::Status status = stub_->GetTask(&context, request, &response);

        if (!status.ok()) {
            if (status.error_code() == grpc::StatusCode::NOT_FOUND) {
                std::println("Tâche non trouvée: {}", task_id);
            } else {
                std::println(stderr, "GetTask failed: {}", status.error_message());
            }
            return std::nullopt;
        }

        return response.task();
    }

    std::vector<Task> ListTasks(int page_size = 10) {
        ListTasksRequest request;
        request.set_page_size(page_size);

        ListTasksResponse response;
        grpc::ClientContext context;
        context.set_deadline(
            std::chrono::system_clock::now() + std::chrono::seconds(10));

        grpc::Status status = stub_->ListTasks(&context, request, &response);

        if (!status.ok()) {
            std::println(stderr, "ListTasks failed: {}", status.error_message());
            return {};
        }

        return {response.tasks().begin(), response.tasks().end()};
    }

private:
    std::unique_ptr<TaskManager::Stub> stub_;
};

int main() {
    // Start server in a thread
    TaskManagerImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    auto server = builder.BuildAndStart();
    std::println("Serveur gRPC en écoute sur 0.0.0.0:50051");

    // Run client test
    std::jthread client_thread([&server]{
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        auto channel = grpc::CreateChannel(
            "localhost:50051",
            grpc::InsecureChannelCredentials());

        TaskManagerClient client(channel);

        // Créer des tâches
        auto task1 = client.CreateTask("Installer gRPC", "Configurer la toolchain");
        auto task2 = client.CreateTask("Écrire le serveur", "Implémenter TaskManager");
        auto task3 = client.CreateTask("Écrire le client", "Tester les appels RPC");

        if (task1) {
            std::println("Créé: {} - '{}'", task1->id(), task1->title());
        }

        // Récupérer une tâche
        if (task1) {
            auto retrieved = client.GetTask(task1->id());
            if (retrieved) {
                std::println("Récupéré: '{}' (completed={})",
                             retrieved->title(), retrieved->completed());
            }
        }

        // Tâche inexistante
        client.GetTask("task-999");

        // Lister
        auto tasks = client.ListTasks();
        std::println("\n{} tâches:", tasks.size());
        for (const auto& t : tasks) {
            std::println("  [{}] {} - '{}'",
                         t.completed() ? "x" : " ", t.id(), t.title());
        }

        server->Shutdown();
    });

    server->Wait();
    std::println("Test gRPC server/client OK !");
}
