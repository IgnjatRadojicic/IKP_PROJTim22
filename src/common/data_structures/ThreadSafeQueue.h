/**
 * @file ThreadSafeQueue.h
 * @brief Thread-safe red (queue) sa Windows CRITICAL_SECTION i CONDITION_VARIABLE.
 *        Rucna implementacija bez STL-a.
 *
 * Podrska za:
 *   - push:     dodaje element i budi jednog cekajuceg (O(1))
 *   - pop:      blokira dok ne stigne element ili shutdown (O(1))
 *   - tryPop:   ne blokira, vraca false ako je prazno (O(1))
 *   - shutdown: budi sve blokirane niti da se zavrse
 */
#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue()
        : head_(nullptr), tail_(nullptr), size_(0), stopped_(false)
    {
        InitializeCriticalSection(&cs_);
        InitializeConditionVariable(&cvNotEmpty_);
    }

    ~ThreadSafeQueue() {
        clear();
        DeleteCriticalSection(&cs_);
        // CONDITION_VARIABLE ne treba eksplicitno brisati na Windows-u
    }

    // Non-copyable
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // ---------------------------------------------------------------
    // Dodavanje
    // ---------------------------------------------------------------

    /** Dodaje element na kraj reda. Thread-safe. */
    void push(const T& value) {
        EnterCriticalSection(&cs_);
        Node* node = new Node(value);
        if (!tail_) {
            head_ = tail_ = node;
        } else {
            tail_->next = node;
            tail_ = node;
        }
        ++size_;
        WakeConditionVariable(&cvNotEmpty_);
        LeaveCriticalSection(&cs_);
    }

    // ---------------------------------------------------------------
    // Uklanjanje
    // ---------------------------------------------------------------

    /**
     * Blokira nit dok ne bude dostupan element ili dok se ne pozove shutdown().
     * @return true ako je element uspesno preuzet, false pri shutdown-u.
     */
    bool pop(T& out) {
        EnterCriticalSection(&cs_);
        while (size_ == 0 && !stopped_) {
            SleepConditionVariableCS(&cvNotEmpty_, &cs_, INFINITE);
        }
        if (size_ == 0 && stopped_) {
            LeaveCriticalSection(&cs_);
            return false;
        }
        out = extractFront();
        LeaveCriticalSection(&cs_);
        return true;
    }

    /**
     * Pokusava da preuzme element bez cekanja.
     * @return true ako je element preuzet, false ako je red prazan.
     */
    bool tryPop(T& out) {
        EnterCriticalSection(&cs_);
        if (size_ == 0) {
            LeaveCriticalSection(&cs_);
            return false;
        }
        out = extractFront();
        LeaveCriticalSection(&cs_);
        return true;
    }

    /**
     * Preuzima sve elemente odjednom u niz.
     * Korisniku odgovornost da delete[] rezultat.
     * @param outCount broj preuzetih elemenata
     * @return niz elemenata ili nullptr ako prazno
     */
    T* popAll(int& outCount) {
        EnterCriticalSection(&cs_);
        if (size_ == 0) {
            outCount = 0;
            LeaveCriticalSection(&cs_);
            return nullptr;
        }
        outCount = size_;
        T* result = new T[size_];
        for (int i = 0; i < outCount; ++i) {
            result[i] = extractFront();
        }
        LeaveCriticalSection(&cs_);
        return result;
    }

    // ---------------------------------------------------------------
    // Kontrola
    // ---------------------------------------------------------------

    /**
     * Signalizira svim blokiranim nitima da se zavrse.
     * Posle poziva, pop() vraca false za sve niti koje cekaju.
     */
    void shutdown() {
        EnterCriticalSection(&cs_);
        stopped_ = true;
        WakeAllConditionVariable(&cvNotEmpty_);
        LeaveCriticalSection(&cs_);
    }

    /** Resetuje shutdown flag - dozvoljava ponovno koriscenje. */
    void restart() {
        EnterCriticalSection(&cs_);
        stopped_ = false;
        LeaveCriticalSection(&cs_);
    }

    // ---------------------------------------------------------------
    // Stanje
    // ---------------------------------------------------------------

    int getSize() {
        EnterCriticalSection(&cs_);
        int s = size_;
        LeaveCriticalSection(&cs_);
        return s;
    }

    bool isEmpty() {
        return getSize() == 0;
    }

    bool isStopped() {
        EnterCriticalSection(&cs_);
        bool s = stopped_;
        LeaveCriticalSection(&cs_);
        return s;
    }

    // ---------------------------------------------------------------
    // Brisanje
    // ---------------------------------------------------------------

    /** Brise sve elemente iz reda. Thread-safe. */
    void clear() {
        EnterCriticalSection(&cs_);
        while (head_) {
            Node* next = head_->next;
            delete head_;
            head_ = next;
        }
        tail_ = nullptr;
        size_ = 0;
        LeaveCriticalSection(&cs_);
    }

private:
    struct Node {
        T     data;
        Node* next;
        explicit Node(const T& value) : data(value), next(nullptr) {}
    };

    Node* head_;
    Node* tail_;
    int   size_;
    bool  stopped_;

    CRITICAL_SECTION   cs_;
    CONDITION_VARIABLE cvNotEmpty_;

    /** Interni: vadi prednji element. MORA se pozivati unutar CS. */
    T extractFront() {
        Node* old = head_;
        T data = old->data;
        head_ = head_->next;
        if (!head_) {
            tail_ = nullptr;
        }
        delete old;
        --size_;
        return data;
    }
};

#endif // THREAD_SAFE_QUEUE_H
