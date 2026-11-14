/**
 * @file LinkedList.h
 * @brief Genericka dvostruko povezana lista - rucna implementacija bez STL-a.
 *
 * Podrska za: pushFront, pushBack, popFront, popBack, remove, find,
 * iteracija (begin/end), copy/move semantika.
 *
 * Slozenost:
 *   - push/pop (front/back): O(1)
 *   - remove/find:           O(n)
 *   - pristup po indeksu:    O(n)
 */
#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <cstddef>  // size_t, nullptr

template <typename T>
class LinkedList {
public:
    // ---------------------------------------------------------------
    // Interni cvor
    // ---------------------------------------------------------------
    struct Node {
        T      data;
        Node*  prev;
        Node*  next;

        explicit Node(const T& value)
            : data(value), prev(nullptr), next(nullptr) {}
    };

    // ---------------------------------------------------------------
    // Iterator (forward, mutable)
    // ---------------------------------------------------------------
    class Iterator {
    public:
        explicit Iterator(Node* node) : current_(node) {}

        T& operator*()  { return current_->data; }
        T* operator->() { return &current_->data; }

        Iterator& operator++()    { current_ = current_->next; return *this; }
        Iterator  operator++(int) { Iterator tmp(*this); ++(*this); return tmp; }

        Iterator& operator--()    { current_ = current_->prev; return *this; }
        Iterator  operator--(int) { Iterator tmp(*this); --(*this); return tmp; }

        bool operator==(const Iterator& other) const { return current_ == other.current_; }
        bool operator!=(const Iterator& other) const { return current_ != other.current_; }

        Node* getNode() const { return current_; }

    private:
        Node* current_;
    };

    // ---------------------------------------------------------------
    // Konstruktori / destruktor
    // ---------------------------------------------------------------
    LinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}

    ~LinkedList() { clear(); }

    /** Copy konstruktor - duboka kopija. */
    LinkedList(const LinkedList& other)
        : head_(nullptr), tail_(nullptr), size_(0)
    {
        for (Node* n = other.head_; n != nullptr; n = n->next) {
            pushBack(n->data);
        }
    }

    /** Copy assignment operator - duboka kopija. */
    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            clear();
            for (Node* n = other.head_; n != nullptr; n = n->next) {
                pushBack(n->data);
            }
        }
        return *this;
    }

    // ---------------------------------------------------------------
    // Dodavanje elemenata
    // ---------------------------------------------------------------

    /** Dodaje element na pocetak liste. O(1). */
    void pushFront(const T& value) {
        Node* node = new Node(value);
        node->next = head_;
        if (head_) {
            head_->prev = node;
        } else {
            tail_ = node;
        }
        head_ = node;
        ++size_;
    }

    /** Dodaje element na kraj liste. O(1). */
    void pushBack(const T& value) {
        Node* node = new Node(value);
        node->prev = tail_;
        if (tail_) {
            tail_->next = node;
        } else {
            head_ = node;
        }
        tail_ = node;
        ++size_;
    }

    // ---------------------------------------------------------------
    // Uklanjanje elemenata
    // ---------------------------------------------------------------

    /** Uklanja i vraca element sa pocetka. Vraca false ako je prazna. */
    bool popFront(T& out) {
        if (!head_) return false;
        Node* old = head_;
        out = old->data;
        head_ = head_->next;
        if (head_) {
            head_->prev = nullptr;
        } else {
            tail_ = nullptr;
        }
        delete old;
        --size_;
        return true;
    }

    /** Uklanja i vraca element sa kraja. Vraca false ako je prazna. */
    bool popBack(T& out) {
        if (!tail_) return false;
        Node* old = tail_;
        out = old->data;
        tail_ = tail_->prev;
        if (tail_) {
            tail_->next = nullptr;
        } else {
            head_ = nullptr;
        }
        delete old;
        --size_;
        return true;
    }

    /** Uklanja konkretan cvor po pokazivacu. O(1). */
    void removeNode(Node* node) {
        if (!node) return;
        if (node->prev) {
            node->prev->next = node->next;
        } else {
            head_ = node->next;
        }
        if (node->next) {
            node->next->prev = node->prev;
        } else {
            tail_ = node->prev;
        }
        delete node;
        --size_;
    }

    /** Uklanja prvo pojavljivanje vrednosti. Vraca true ako je nasao. */
    bool remove(const T& value) {
        Node* cur = head_;
        while (cur) {
            if (cur->data == value) {
                removeNode(cur);
                return true;
            }
            cur = cur->next;
        }
        return false;
    }

    // ---------------------------------------------------------------
    // Pretraga i pristup
    // ---------------------------------------------------------------

    /** Trazi prvo pojavljivanje. Vraca end() ako ne nadje. */
    Iterator find(const T& value) const {
        Node* cur = head_;
        while (cur) {
            if (cur->data == value) return Iterator(cur);
            cur = cur->next;
        }
        return Iterator(nullptr);
    }

    /** Pristup po indeksu. Nema proveru granica - pazljivo koristiti! */
    T& operator[](int index) {
        Node* cur = head_;
        for (int i = 0; i < index; ++i) {
            cur = cur->next;
        }
        return cur->data;
    }

    const T& operator[](int index) const {
        Node* cur = head_;
        for (int i = 0; i < index; ++i) {
            cur = cur->next;
        }
        return cur->data;
    }

    T& front()             { return head_->data; }
    const T& front() const { return head_->data; }
    T& back()              { return tail_->data; }
    const T& back()  const { return tail_->data; }

    // ---------------------------------------------------------------
    // Kapacitet
    // ---------------------------------------------------------------
    int  getSize() const { return size_; }
    bool isEmpty() const { return size_ == 0; }

    // ---------------------------------------------------------------
    // Brisanje
    // ---------------------------------------------------------------

    /** Brise sve elemente i oslobadja memoriju. */
    void clear() {
        Node* cur = head_;
        while (cur) {
            Node* next = cur->next;
            delete cur;
            cur = next;
        }
        head_ = nullptr;
        tail_ = nullptr;
        size_ = 0;
    }

    // ---------------------------------------------------------------
    // Iteracija
    // ---------------------------------------------------------------
    Iterator begin() const { return Iterator(head_); }
    Iterator end()   const { return Iterator(nullptr); }

private:
    Node* head_;
    Node* tail_;
    int   size_;
};

#endif // LINKED_LIST_H
