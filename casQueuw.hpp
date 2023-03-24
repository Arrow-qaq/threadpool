#include <atomic>
#include <cstddef>

template<typename T>
class CASQueue {
private:
    struct Node {
        T data;
        std::atomic<Node*> next;

        Node(const T& data) : data(data), next(nullptr) {}
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;

public:
    CASQueue() : head(new Node(T())), tail(head.load()) {}
    ~CASQueue() {
        while (head) {
            Node* tmp = head;
            head = tmp->next;
            delete tmp;
        }
    }

    bool enqueue(const T& data) {
        Node* newNode = new Node(data);
        while (true) {
            Node* curTail = tail.load();
            Node* curNext = curTail->next.load();
            if (curTail != tail.load()) {
                continue;
            }
            if (curNext == nullptr) {
                if (std::atomic_compare_exchange_strong(&curTail->next, &curNext, newNode)) {
                    break;
                }
            } else {
                std::atomic_compare_exchange_strong(&tail, &curTail, curNext);
            }
        }
        std::atomic_compare_exchange_strong(&tail, &curTail, newNode);
        return true;
    }

    bool dequeue(T& data) {
        while (true) {
            Node* curHead = head.load();
            Node* curTail = tail.load();
            Node* curNext = curHead->next.load();
            if (curHead != head.load()) {
                continue;
            }
            if (curHead == curTail) {
                if (curNext == nullptr) {
                    return false;
                }
                std::atomic_compare_exchange_strong(&tail, &curTail, curNext);
            } else {
                data = curNext->data;
                if (std::atomic_compare_exchange_strong(&head, &curHead, curNext)) {
                    break;
                }
            }
        }
        delete curHead;
        return true;
    }
};
