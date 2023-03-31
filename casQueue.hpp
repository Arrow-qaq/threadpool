#include <atomic>
#include <memory>

template<typename T>
class LockFreeQueue {
private:
    struct Node {
        std::shared_ptr<T> data;
        Node* next;

        Node() : next(nullptr) {}
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;

public:
    LockFreeQueue() : head(new Node), tail(head.load()) {}

    LockFreeQueue(const LockFreeQueue& other) = delete;
    LockFreeQueue& operator=(const LockFreeQueue& other) = delete;

    ~LockFreeQueue() {
        while (Node* const old_head = head.load()) {
            head.store(old_head->next);
            delete old_head;
        }
    }

    void push(T value) {
        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(value)));
        Node* new_node = new Node;
        Node* tail_ptr = tail.load();
        tail_ptr->data = new_data;
        tail_ptr->next = new_node;
        tail.store(new_node);
    }

    std::shared_ptr<T> pop() {
        Node* old_head = head.load();
        while (old_head != tail.load()) {
            if (head.compare_exchange_strong(old_head, old_head->next)) {
                std::shared_ptr<T> const res(old_head->data);
                delete old_head;
                return res;
            }
            old_head = head.load();
        }
        return std::shared_ptr<T>();
    }
};
