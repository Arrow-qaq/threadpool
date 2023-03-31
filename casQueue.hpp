#include <atomic>
#include <memory>

template<typename T>
class CASQueue {
private:
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next;

        Node(T const& data_): data(std::make_shared<T>(data_)) {}
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;

public:
    CASQueue() {
        Node* node = new Node(T{});
        head.store(node);
        tail.store(node);
    }

    CASQueue(const CASQueue& other) = delete;
    CASQueue& operator=(const CASQueue& other) = delete;

    ~CASQueue() {
        while (head != nullptr) {
            Node* tmp = head;
            head = head->next;
            delete tmp;
        }
    }

    void enqueue(T const& value) {
        Node* new_node = new Node(value);
        Node* last = tail.load();
        Node* next = nullptr;

        while (true) {
            next = last->next.load();
            if (next == nullptr) {
                if (last->next.compare_exchange_weak(next, new_node)) {
                    break;
                }
            } else {
                tail.compare_exchange_weak(last, next);
            }
            last = tail.load();
        }
        tail.compare_exchange_weak(last, new_node);
    }

    std::shared_ptr<T> dequeue() {
        Node* first = nullptr;
        Node* last = nullptr;
        Node* next = nullptr;

        while (true) {
            first = head.load();
            last = tail.load();
            next = first->next.load();

            if (first == head.load()) {
                if (first == last) {
                    if (next == nullptr) {
                        return nullptr;
                    }
                    tail.compare_exchange_weak(last, next);
                } else {
                    if (head.compare_exchange_weak(first, next)) {
                        std::shared_ptr<T> res = next->data;
                        delete first;
                        return res;
                    }
                }
            }
        }
    }
};
