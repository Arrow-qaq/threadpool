#include <atomic>
#include <memory>

template<typename T>
class CASQueue {
private:
    struct Node {
        std::shared_ptr<T> data;
        Node* next;
        Node() : next(nullptr) {}
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;

public:
    CASQueue() : head(new Node), tail(head.load()) {}

    CASQueue(const CASQueue& other) = delete;
    CASQueue& operator=(const CASQueue& other) = delete;

    ~CASQueue() {
        while (Node* old_head = head.load()) {
            head.store(old_head->next);
            delete old_head;
        }
    }

    std::shared_ptr<T> try_pop() {
        Node* old_head = head.load();
        while (old_head != tail.load()) {
            if (head.compare_exchange_strong(old_head, old_head->next)) {
                std::shared_ptr<T> res(old_head->data);
                delete old_head;
                return res;
            }
            old_head = head.load();
        }
        return std::shared_ptr<T>();
    }

    void push(T new_value) {
        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
        Node* p = new Node;
        Node* old_tail = tail.load();
        old_tail->data.swap(new_data);
        old_tail->next = p;
        tail.compare_exchange_strong(old_tail, p);
    }
};
