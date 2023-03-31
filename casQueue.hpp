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

        while (true) {
            Node* last = tail.load();
            Node* next = last->next.load();

            if (last == tail) {
                if (next == nullptr) {
                    if (last->next.compare_exchange_strong(next, new_node.get())) {
                        tail.compare_exchange_strong(last, new_node.get());
                        return;
                    }
                } else {
                    tail.compare_exchange_strong(last, next);
                }
            }
        }
    }

    std::shared_ptr<T> dequeue() {
        while (true) {
            Node* first = head.load();
            Node* last = tail.load();
            Node* next = first->next.load();

            if (first == head) {
                if (first == last) {
                    if (next == nullptr) {
                        return std::shared_ptr<T>();
                    }
                    tail.compare_exchange_strong(last, next);
                } else {
                    std::shared_ptr<T> res = next->data;
                    if (head.compare_exchange_strong(first, next)) {
                        return res;
                    }
                }
            }
        }
    }
};
