#include <atomic>
#include <memory>

template <typename T>
class CASQueue {
public:
    struct Node {
        T data;
        std::shared_ptr<Node> next;

        Node(const T& data_) : data(data_) {}
    };

    CASQueue() {
        std::shared_ptr<Node> dummy_node(new Node(T()));
        tail = head = dummy_node;
    }

    void push(const T& value) {
        std::shared_ptr<Node> new_node(new Node(value));
        std::shared_ptr<Node> old_tail = tail.load();
        while (true) {
            std::shared_ptr<Node> old_next = old_tail->next;
            if (old_tail == tail.load()) {
                if (old_next == nullptr) {
                    if (std::atomic_compare_exchange_strong(
                            &(old_tail->next), nullptr, new_node)) {
                        break;
                    }
                } else {
                    tail.compare_exchange_strong(old_tail, old_next);
                }
            }
            old_tail = tail.load();
        }
        tail.compare_exchange_strong(old_tail, new_node);
    }

    std::shared_ptr<T> pop() {
        std::shared_ptr<Node> old_head = head.load();
        while (true) {
            std::shared_ptr<Node> old_tail = tail.load();
            std::shared_ptr<Node> old_next = old_head->next;
            if (old_head == head.load()) {
                if (old_head == old_tail) {
                    if (old_next == nullptr) {
                        return nullptr;
                    }
                    tail.compare_exchange_strong(old_tail, old_next);
                } else {
                    std::shared_ptr<T> result(
                        std::make_shared<T>(old_next->data));
                    if (head.compare_exchange_strong(old_head, old_next)) {
                        return result;
                    }
                }
            }
        }
    }

private:
    std::atomic<std::shared_ptr<Node>> head;
    std::atomic<std::shared_ptr<Node>> tail;
};
