/* ============================================================================
   Section 23.4 : Wrapper RAII — MessageQueue
   Description : Classe RAII pour message queues + producteur/consommateur via fork
   Fichier source : 04-message-queues.md
   ============================================================================ */
#include <mqueue.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <system_error>
#include <chrono>
#include <print>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>

struct MQConfig {
    long max_messages = 10;
    long max_message_size = 4096;
};

class MessageQueue {
public:
    struct ReceivedMessage {
        std::string data;
        unsigned int priority;
    };

    static MessageQueue create(const std::string& name, MQConfig config = {}) {
        mq_attr attr{};
        attr.mq_maxmsg = config.max_messages;
        attr.mq_msgsize = config.max_message_size;

        mqd_t mq = mq_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666, &attr);
        if (mq == static_cast<mqd_t>(-1)) {
            throw std::system_error(errno, std::system_category(),
                                    "mq_open(create: " + name + ")");
        }

        return MessageQueue(name, mq, config.max_message_size, true);
    }

    static MessageQueue open(const std::string& name, int flags = O_RDWR) {
        mqd_t mq = mq_open(name.c_str(), flags);
        if (mq == static_cast<mqd_t>(-1)) {
            throw std::system_error(errno, std::system_category(),
                                    "mq_open(open: " + name + ")");
        }

        mq_attr attr;
        mq_getattr(mq, &attr);

        return MessageQueue(name, mq, attr.mq_msgsize, false);
    }

    ~MessageQueue() {
        if (mq_ != static_cast<mqd_t>(-1)) {
            mq_close(mq_);
        }
        if (owner_) {
            mq_unlink(name_.c_str());
        }
    }

    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;

    MessageQueue(MessageQueue&& other) noexcept
        : name_{std::move(other.name_)}, mq_{other.mq_},
          msg_size_{other.msg_size_}, owner_{other.owner_} {
        other.mq_ = static_cast<mqd_t>(-1);
        other.owner_ = false;
    }

    MessageQueue& operator=(MessageQueue&& other) noexcept {
        if (this != &other) {
            if (mq_ != static_cast<mqd_t>(-1)) mq_close(mq_);
            if (owner_) mq_unlink(name_.c_str());

            name_ = std::move(other.name_);
            mq_ = other.mq_;
            msg_size_ = other.msg_size_;
            owner_ = other.owner_;
            other.mq_ = static_cast<mqd_t>(-1);
            other.owner_ = false;
        }
        return *this;
    }

    void send(std::string_view message, unsigned int priority = 0) {
        if (mq_send(mq_, message.data(), message.size(), priority) == -1) {
            throw std::system_error(errno, std::system_category(), "mq_send()");
        }
    }

    bool try_send(std::string_view message, unsigned int priority,
                  std::chrono::seconds timeout) {
        timespec deadline = make_deadline(timeout);
        int ret = mq_timedsend(mq_, message.data(), message.size(),
                               priority, &deadline);
        if (ret == -1) {
            if (errno == ETIMEDOUT) return false;
            throw std::system_error(errno, std::system_category(), "mq_timedsend()");
        }
        return true;
    }

    ReceivedMessage receive() {
        std::vector<char> buffer(static_cast<size_t>(msg_size_));
        unsigned int priority;

        ssize_t n = mq_receive(mq_, buffer.data(), buffer.size(), &priority);
        if (n == -1) {
            throw std::system_error(errno, std::system_category(), "mq_receive()");
        }

        return {std::string(buffer.data(), static_cast<size_t>(n)), priority};
    }

    std::optional<ReceivedMessage> try_receive(std::chrono::seconds timeout) {
        std::vector<char> buffer(static_cast<size_t>(msg_size_));
        unsigned int priority;
        timespec deadline = make_deadline(timeout);

        ssize_t n = mq_timedreceive(mq_, buffer.data(), buffer.size(),
                                     &priority, &deadline);
        if (n == -1) {
            if (errno == ETIMEDOUT) return std::nullopt;
            throw std::system_error(errno, std::system_category(), "mq_timedreceive()");
        }

        return ReceivedMessage{
            std::string(buffer.data(), static_cast<size_t>(n)),
            priority
        };
    }

    long pending() const {
        mq_attr attr;
        mq_getattr(mq_, &attr);
        return attr.mq_curmsgs;
    }

    mqd_t native_handle() const noexcept { return mq_; }

private:
    MessageQueue(std::string name, mqd_t mq, long msg_size, bool owner)
        : name_{std::move(name)}, mq_{mq}, msg_size_{msg_size}, owner_{owner} {}

    static timespec make_deadline(std::chrono::seconds timeout) {
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout.count();
        return ts;
    }

    std::string name_;
    mqd_t mq_ = static_cast<mqd_t>(-1);
    long msg_size_ = 0;
    bool owner_ = false;
};

int main() {
    // Cleanup
    mq_unlink("/task_queue");

    std::println("=== Test MessageQueue RAII ===");
    std::fflush(stdout);

    pid_t pid = fork();
    if (pid == 0) {
        // Consumer (child)
        sleep(1);  // Wait for queue creation
        auto mq = MessageQueue::open("/task_queue", O_RDONLY);

        while (true) {
            auto msg = mq.try_receive(std::chrono::seconds(5));
            if (!msg) {
                std::println("[Consumer] Timeout");
                std::fflush(stdout);
                break;
            }
            if (msg->data == "__STOP__") {
                std::println("[Consumer] Signal d'arrêt reçu — fin");
                std::fflush(stdout);
                break;
            }
            std::println("[Consumer] Traité [prio={}]: {}", msg->priority, msg->data);
            std::fflush(stdout);
        }
        _exit(0);
    }

    // Producer (parent)
    auto mq = MessageQueue::create("/task_queue", {
        .max_messages = 10,
        .max_message_size = 1024
    });

    for (int i = 0; i < 5; ++i) {
        unsigned int prio = (i % 3 == 0) ? 5 : 1;
        std::string msg = "Tâche #" + std::to_string(i);
        mq.send(msg, prio);
        std::println("[Producer] Envoyé [prio={}]: {}", prio, msg);
    }

    mq.send("__STOP__", 0);
    std::println("[Producer] Signal d'arrêt envoyé");

    waitpid(pid, nullptr, 0);
    std::println("=== Test terminé ===");
}
