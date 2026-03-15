/* ============================================================================
   Section 22.6.4 : Streaming bidirectionnel
   Description : Patterns de streaming gRPC — serveur (Subscribe), client
                 (Ingest), bidirectionnel (InteractiveQuery), métriques
   Fichier source : 06.4-streaming.md
   ============================================================================ */
#include <grpcpp/grpcpp.h>
#include "generated/streaming_demo.grpc.pb.h"

#include <print>
#include <thread>
#include <chrono>
#include <random>
#include <set>
#include <vector>

using namespace streaming::v1;

// -- Server implementation --
class MetricsServiceImpl final : public MetricsService::Service {
public:
    // Streaming serveur
    grpc::Status Subscribe(
        grpc::ServerContext* context,
        const SubscribeRequest* request,
        grpc::ServerWriter<MetricEvent>* writer) override
    {
        std::println("[Subscribe] Client connecté, intervalle={}ms",
                     request->interval_ms());

        int interval = request->interval_ms() > 0 ? request->interval_ms() : 1000;
        std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<double> cpu_dist(10.0, 95.0);
        std::uniform_real_distribution<double> mem_dist(40.0, 90.0);

        std::set<std::string> filter(
            request->metric_names().begin(),
            request->metric_names().end());
        bool send_all = filter.empty();

        while (!context->IsCancelled()) {
            auto now = std::chrono::system_clock::now();
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                now.time_since_epoch()).count();

            if (send_all || filter.contains("cpu_usage")) {
                MetricEvent event;
                event.set_name("cpu_usage");
                event.set_value(cpu_dist(rng));
                event.mutable_timestamp()->set_seconds(seconds);
                if (!writer->Write(event)) return grpc::Status::OK;
            }

            if (send_all || filter.contains("memory_usage")) {
                MetricEvent event;
                event.set_name("memory_usage");
                event.set_value(mem_dist(rng));
                event.mutable_timestamp()->set_seconds(seconds);
                if (!writer->Write(event)) return grpc::Status::OK;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }

        std::println("[Subscribe] Client annulé");
        return grpc::Status::OK;
    }

    // Streaming client
    grpc::Status Ingest(
        grpc::ServerContext* context,
        grpc::ServerReader<MetricSample>* reader,
        IngestSummary* response) override
    {
        auto start = std::chrono::steady_clock::now();

        MetricSample sample;
        int64_t received = 0;
        int64_t accepted = 0;
        int64_t rejected = 0;

        while (reader->Read(&sample)) {
            received++;
            if (sample.name().empty()) { rejected++; continue; }
            if (sample.value() < 0 || sample.value() > 100) { rejected++; continue; }
            accepted++;
        }

        auto end = std::chrono::steady_clock::now();
        double duration = std::chrono::duration<double>(end - start).count();

        response->set_samples_received(received);
        response->set_samples_accepted(accepted);
        response->set_samples_rejected(rejected);
        response->set_duration_seconds(duration);

        std::println("[Ingest] Reçu: {}, Accepté: {}, Rejeté: {}, Durée: {:.2f}s",
                     received, accepted, rejected, duration);
        return grpc::Status::OK;
    }

    // Bidirectionnel
    grpc::Status InteractiveQuery(
        grpc::ServerContext* /*context*/,
        grpc::ServerReaderWriter<QueryResult, QueryRequest>* stream) override
    {
        std::println("[InteractiveQuery] Session démarrée");

        QueryRequest request;
        while (stream->Read(&request)) {
            std::println("[Query] '{}'", request.query());

            QueryResult result;
            result.set_query(request.query());

            if (request.query().empty()) {
                result.set_error("Requête vide");
                stream->Write(result);
                continue;
            }

            int max = request.max_results() > 0 ? request.max_results() : 5;
            std::mt19937 rng{std::random_device{}()};
            std::uniform_real_distribution<double> dist(0.0, 100.0);

            for (int i = 0; i < max; ++i) {
                MetricEvent* event = result.add_results();
                event->set_name("metric_" + std::to_string(i));
                event->set_value(dist(rng));
                auto now = std::chrono::system_clock::now();
                event->mutable_timestamp()->set_seconds(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        now.time_since_epoch()).count());
            }

            if (!stream->Write(result)) break;
        }

        std::println("[InteractiveQuery] Session terminée");
        return grpc::Status::OK;
    }
};

// -- Test functions --

void test_subscribe(MetricsService::Stub* stub) {
    std::println("\n=== Test streaming serveur (Subscribe) ===");
    SubscribeRequest request;
    request.add_metric_names("cpu_usage");
    request.set_interval_ms(200);

    grpc::ClientContext context;
    auto reader = stub->Subscribe(&context, request);

    MetricEvent event;
    int count = 0;
    while (reader->Read(&event) && count < 5) {
        std::println("  {} = {:.1f}%", event.name(), event.value());
        count++;
    }
    context.TryCancel();
    reader->Finish();
    std::println("  Stream terminé après {} événements", count);
}

void test_ingest(MetricsService::Stub* stub) {
    std::println("\n=== Test streaming client (Ingest) ===");
    IngestSummary summary;
    grpc::ClientContext context;
    context.set_deadline(
        std::chrono::system_clock::now() + std::chrono::seconds(10));

    auto writer = stub->Ingest(&context, &summary);

    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<double> dist(0.0, 100.0);

    for (int i = 0; i < 100; ++i) {
        MetricSample sample;
        sample.set_name("sensor_" + std::to_string(i % 10));
        sample.set_value(dist(rng));
        writer->Write(sample);
    }

    writer->WritesDone();
    grpc::Status status = writer->Finish();

    if (status.ok()) {
        std::println("  Reçus: {}, Acceptés: {}, Rejetés: {}, Durée: {:.3f}s",
                     summary.samples_received(),
                     summary.samples_accepted(),
                     summary.samples_rejected(),
                     summary.duration_seconds());
    }
}

void test_bidi(MetricsService::Stub* stub) {
    std::println("\n=== Test streaming bidirectionnel (InteractiveQuery) ===");
    grpc::ClientContext context;
    auto stream = stub->InteractiveQuery(&context);

    // Writer thread
    std::jthread writer_thread([&stream] {
        std::vector<std::string> queries = {
            "cpu_usage > 80",
            "memory_usage > 70",
            "",  // invalid
        };

        for (const auto& q : queries) {
            QueryRequest request;
            request.set_query(q);
            request.set_max_results(2);
            stream->Write(request);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        stream->WritesDone();
    });

    // Read results
    QueryResult result;
    while (stream->Read(&result)) {
        if (!result.error().empty()) {
            std::println("  [Erreur] '{}': {}", result.query(), result.error());
        } else {
            std::println("  [Résultat] '{}' — {} résultats",
                         result.query(), result.results_size());
            for (const auto& ev : result.results()) {
                std::println("    {} = {:.1f}", ev.name(), ev.value());
            }
        }
    }

    stream->Finish();
}

int main() {
    MetricsServiceImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50052", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    auto server = builder.BuildAndStart();
    std::println("Serveur streaming gRPC sur le port 50052");

    std::jthread client_thread([&server]{
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        auto channel = grpc::CreateChannel(
            "localhost:50052", grpc::InsecureChannelCredentials());
        auto stub = MetricsService::NewStub(channel);

        test_subscribe(stub.get());
        test_ingest(stub.get());
        test_bidi(stub.get());

        std::println("\nTest streaming OK !");
        server->Shutdown();
    });

    server->Wait();
}
