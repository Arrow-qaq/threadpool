#include <atomic>
#include <memory>

template<typename T>
class CASQueue {
private:
    struct Node {
        std::atomic<Node*> next;
        std::shared_ptr<T> data;

        Node(): next(nullptr) {}
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;

public:
    CASQueue(): head(new Node), tail(head.load()) {}

    void enqueue(T new_value) {
        auto new_node = std::make_shared<Node>();
        new_node->data = std::make_shared<T>(new_value);

        Node* last = nullptr;
        while (true) {
            last = tail.load();
            Node* next = last->next.load(std::memory_order_acquire);

            if (last == tail.load(std::memory_order_relaxed)) {
                if (next == nullptr) {
                    if (last->next.compare_exchange_strong(next, new_node.get())) {
                        break;
                    }
                } else {
                    tail.compare_exchange_strong(last, next);
                }
            }
        }
        tail.compare_exchange_strong(last, new_node.get(), std::memory_order_release);
    }

    std::shared_ptr<T> dequeue() {
        Node* first = nullptr;
        while (true) {
            first = head.load(std::memory_order_acquire);
            Node* last = tail.load(std::memory_order_acquire);
            Node* next = first->next.load(std::memory_order_acquire);

            if (first == head.load(std::memory_order_relaxed)) {
                if (first == last) {
                    if (next == nullptr) {
                        return std::shared_ptr<T>();
                    }
                    tail.compare_exchange_strong(last, next, std::memory_order_release);
                } else {
                    std::shared_ptr<T> res = next->data;
                    if (head.compare_exchange_strong(first, next, std::memory_order_release)) {
                        return res;
                    }
                }
            }
        }
    }
};
