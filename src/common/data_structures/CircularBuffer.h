/**
 * @file CircularBuffer.h
 * @brief Kruzni bafer fiksne velicine - rucna implementacija bez STL-a.
 *
 * Koristi se za baferovanje poruka, log zapisa ili merenja
 * gde je bitno ograniciti memorijski otisak.
 *
 * Slozenost:
 *   - push/pop/peek: O(1)
 *   - isFull/isEmpty: O(1)
 *
 * Ponasanje pri punom baferu: push vraca false (ne prebrisuje).
 * Za prebrisujuci rezim koristiti pushOverwrite().
 */
#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

template <typename T>
class CircularBuffer {
public:
    /**
     * @param capacity Maksimalan broj elemenata.
     *                 Mora biti > 0, inace se koristi 1.
     */
    explicit CircularBuffer(int capacity)
        : capacity_(capacity > 0 ? capacity : 1),
          head_(0), tail_(0), size_(0)
    {
        buffer_ = new T[capacity_];
    }

    ~CircularBuffer() {
        delete[] buffer_;
    }

    // Non-copyable (moguce dodati po potrebi)
    CircularBuffer(const CircularBuffer&) = delete;
    CircularBuffer& operator=(const CircularBuffer&) = delete;

    // ---------------------------------------------------------------
    // Dodavanje
    // ---------------------------------------------------------------

    /**
     * Dodaje element na kraj bafera.
     * @return true ako je uspesno, false ako je bafer pun.
     */
    bool push(const T& value) {
        if (size_ == capacity_) return false;
        buffer_[tail_] = value;
        tail_ = (tail_ + 1) % capacity_;
        ++size_;
        return true;
    }

    /**
     * Dodaje element; ako je pun, prebrisuje najstariji.
     * Uvek uspeva.
     */
    void pushOverwrite(const T& value) {
        if (size_ == capacity_) {
            // Prebrisujemo najstariji element
            buffer_[tail_] = value;
            tail_ = (tail_ + 1) % capacity_;
            head_ = (head_ + 1) % capacity_;
            // size_ ostaje isti
        } else {
            buffer_[tail_] = value;
            tail_ = (tail_ + 1) % capacity_;
            ++size_;
        }
    }

    // ---------------------------------------------------------------
    // Uklanjanje
    // ---------------------------------------------------------------

    /**
     * Uklanja i vraca najstariji element.
     * @return true ako je uspesno, false ako je prazan.
     */
    bool pop(T& out) {
        if (size_ == 0) return false;
        out = buffer_[head_];
        head_ = (head_ + 1) % capacity_;
        --size_;
        return true;
    }

    // ---------------------------------------------------------------
    // Pristup
    // ---------------------------------------------------------------

    /**
     * Cita najstariji element bez uklanjanja.
     * @return true ako postoji, false ako je prazan.
     */
    bool peek(T& out) const {
        if (size_ == 0) return false;
        out = buffer_[head_];
        return true;
    }

    /**
     * Pristup po indeksu (0 = najstariji).
     * Nema proveru granica!
     */
    T& operator[](int index) {
        return buffer_[(head_ + index) % capacity_];
    }

    const T& operator[](int index) const {
        return buffer_[(head_ + index) % capacity_];
    }

    // ---------------------------------------------------------------
    // Stanje
    // ---------------------------------------------------------------
    int  getSize()     const { return size_; }
    int  getCapacity() const { return capacity_; }
    bool isEmpty()     const { return size_ == 0; }
    bool isFull()      const { return size_ == capacity_; }

    // ---------------------------------------------------------------
    // Brisanje
    // ---------------------------------------------------------------

    /** Prazni bafer (ne oslobadja memoriju). */
    void clear() {
        head_ = 0;
        tail_ = 0;
        size_ = 0;
    }

private:
    T*  buffer_;
    int capacity_;
    int head_;     // Pozicija citanja (najstariji element)
    int tail_;     // Pozicija pisanja (sledeci slobodan slot)
    int size_;     // Trenutni broj elemenata
};

#endif // CIRCULAR_BUFFER_H
