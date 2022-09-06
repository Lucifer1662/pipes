#include <functional>
#include <iostream>
#include <optional>
#include <queue>
#include <memory>

struct AddMessage {
    AddMessage(int x, int y) : x(x), y(y) {}

    int x;
    int y;
};

struct NumMessage {
    NumMessage(int x) : x(x) {}
    int x;
};

std::ostream& operator<<(std::ostream& os, const NumMessage& m) {
    os << m.x;
    return os;
}

struct Adder {
    NumMessage add(const AddMessage& m) { return NumMessage(m.x + m.y); }
};

template <typename Message>
struct IMessageQueueOut {
    virtual void push(Message& message) = 0;
};

template <typename Message>
struct IMessageQueueIn {
    virtual std::function<void()> registerPop(
        std::function<void(Message&)>&& callback) = 0;
};

template <typename Message>
struct IMessageQueue : public IMessageQueueIn<Message>,
                       public IMessageQueueOut<Message> {};

template <typename Message>
struct DirectQueue : public IMessageQueue<Message> {
    std::function<void(Message&)> callback = [](auto m) {};
    virtual void push(Message& message) override { callback(message); }
    virtual std::function<void()> registerPop(
        std::function<void(Message&)>&& callback) override {
        this->callback = std::move(callback);
        return [this]() { this->callback = [](auto m) {}; };
    }
};

template <typename MessageIn, typename MessageOut>
class Pipe {
    IMessageQueueIn<MessageIn>* in = nullptr;
    std::function<void()> unregisterIn;
    IMessageQueueOut<MessageOut>* out = nullptr;

   public:
    void SetIn(IMessageQueueIn<MessageIn>* in) {
        if (this->in != nullptr) {
            unregisterIn();
        }

        this->in = in;
        unregisterIn =
            in->registerPop([this](auto& m) { this->HandleMessage(m); });
    }

    void SetOut(IMessageQueueOut<MessageOut>* out) { this->out = out; }

    virtual void HandleMessage(MessageIn& messageIn) = 0;

   protected:
    void send(MessageOut& messageOut) {
        if (out != nullptr) {
            out->push(messageOut);
        }
    }

    ~Pipe() {
        if (this->in != nullptr) {
            unregisterIn();
        }
    }
};

class AdderPipe : public Pipe<AddMessage, NumMessage> {
    Adder adder;

   public:
    virtual void HandleMessage(AddMessage& m) {
        auto m2 = adder.add(m);
        send(m2);
    }
};

template <typename Message>
struct StreamOut : public IMessageQueueOut<Message> {
    std::ostream& os;
    StreamOut<Message>(std::ostream& os) : os(os) {}

    virtual void push(Message& message) override { os << message; }
};

template <typename T>
struct Monoid {
    T t;
    Monoid(T&& t) : t(t) {}
    Monoid(const T& t) : t(t) {}

    template <typename M>
    auto bind(std::function<M(T&)> func) {
        if constexpr (std::is_void<T>().value) {
            func(t);
        } else {
            return Monoid<M>(func(t));
        }
    }
};

template <typename T, typename QueueIn>
struct QueueMonoid {
    std::unique_ptr<QueueIn> queue;
    QueueMonoid(QueueIn* queue) : queue(queue) {}

    void foo() {}

    template <typename M, typename QueueOut>
    QueueMonoid<M, QueueOut> bind(QueueOut* queueOut,
                                  std::function<M(T&)> func) {
        queue->registerPop([queueOut, func](auto& m) {
            auto m1 = func(m);
            queueOut->push(m1);
        });

        return QueueMonoid<M, QueueOut>(queueOut);
    }
};

int main(int, char**) {
    Monoid<AddMessage> m0(AddMessage(1, 2));

    auto m1 =
        m0.bind<NumMessage>([](auto& t) { return NumMessage(t.x + t.y); });

    m1.bind<int>([](auto& t) {
        std::cout << t << std::endl;
        return 0;
    });



    auto qM = QueueMonoid<AddMessage, DirectQueue<AddMessage>>(
        new DirectQueue<AddMessage>());

    auto gM2 = qM.bind<NumMessage, StreamOut<NumMessage>>(
        new StreamOut<NumMessage>(std::cout),
        [](auto& t) { return NumMessage(t.x + t.y); });

    qM.queue->push(AddMessage(1,2));
    


    

    DirectQueue<AddMessage> q1;
    StreamOut<NumMessage> out(std::cout);

    AdderPipe pipe;
    pipe.SetIn(&q1);
    pipe.SetOut(&out);

    auto m = AddMessage(1, 2);
    q1.push(m);
}