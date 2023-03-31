#include <atomic>
#include <memory>

template <typename T>
class CASQueue {
private:
    struct Node {
        std::unique_ptr<T> data;
        Node* next;
        Node(T* data): data(data), next(nullptr) {}
    };
    std::atomic<Node*> head;
    std::atomic<Node*> tail;
public:
    CASQueue(): head(new Node(nullptr)), tail(head.load()) {}
    ~CASQueue() {
        while (Node* const old_head = head.load()) {
            head.store(old_head->next);
            delete old_head;
        }
    }
    CASQueue(const CASQueue&) = delete;
    CASQueue& operator=(const CASQueue&) = delete;

    void enqueue(T* data) {
        std::unique_ptr<Node> new_node(new Node(data));
        Node* old_tail = tail.load();
        while (true) {
            Node* const next = old_tail->next;
            if (!next) {
                if (tail.compare_exchange_strong(old_tail, new_node.get())) {
                    old_tail->next = new_node.release();
                    return;
                }
            } else {
                tail.compare_exchange_strong(old_tail, next);
            }
            old_tail = tail.load();
        }
    }

    std::unique_ptr<T> dequeue() {
        Node* old_head = head.load();
        while (true) {
            Node* const head_next = old_head->next;
            if (!head_next) {
                return nullptr;
            }
            if (head.compare_exchange_strong(old_head, head_next)) {
                std::unique_ptr<Node> old_head_ptr(old_head);
                T* const res = old_head_ptr->data.release();
                return std::unique_ptr<T>(res);
            }
        }
    }
};
