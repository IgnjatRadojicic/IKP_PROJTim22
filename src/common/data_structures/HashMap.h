/**
 * @file HashMap.h
 * @brief Genericka hash mapa sa odvojenim lancima (separate chaining).
 *        Rucna implementacija bez STL-a.
 *
 * Podrzani tipovi kljuceva: int, unsigned int, uint32_t, const char*.
 * Za nove tipove - specijalizovati HashFunction<K>.
 *
 * Slozenost (amortizovano):
 *   - put/get/remove/containsKey: O(1)
 *   - resize:                     O(n)
 */
#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <cstdint>
#include <cstring>  // strcmp

// ===================================================================
// Hash funkcija - default i specijalizacije
// ===================================================================

template <typename K>
struct HashFunction {
    unsigned int operator()(const K& key) const {
        return static_cast<unsigned int>(key);
    }
};

/** Specijalizacija za const char* - djb2 algoritam. */
template <>
struct HashFunction<const char*> {
    unsigned int operator()(const char* key) const {
        unsigned int hash = 5381;
        while (*key) {
            hash = hash * 33 + static_cast<unsigned int>(*key);
            ++key;
        }
        return hash;
    }
};

// ===================================================================
// KeyEqual - poredjenje kljuceva
// ===================================================================

template <typename K>
struct KeyEqual {
    bool operator()(const K& a, const K& b) const {
        return a == b;
    }
};

template <>
struct KeyEqual<const char*> {
    bool operator()(const char* a, const char* b) const {
        return std::strcmp(a, b) == 0;
    }
};

// ===================================================================
// HashMap
// ===================================================================

template <typename K, typename V,
          typename Hash  = HashFunction<K>,
          typename Equal = KeyEqual<K>>
class HashMap {
public:
    // ---------------------------------------------------------------
    // Interni Entry cvor (za lanac u bucket-u)
    // ---------------------------------------------------------------
    struct Entry {
        K      key;
        V      value;
        Entry* next;

        Entry(const K& k, const V& v)
            : key(k), value(v), next(nullptr) {}
    };

    // ---------------------------------------------------------------
    // Iterator - prolazi kroz sve bucket-e i lance
    // ---------------------------------------------------------------
    class Iterator {
    public:
        Iterator(Entry** buckets, int capacity, int bucketIdx, Entry* entry)
            : buckets_(buckets), capacity_(capacity),
              bucketIdx_(bucketIdx), entry_(entry)
        {
            // Ako entry nije validan, nadji sledeci
            if (!entry_) {
                advance();
            }
        }

        struct KeyValuePair {
            K& key;
            V& value;
        };

        KeyValuePair operator*() {
            return { entry_->key, entry_->value };
        }

        const K& key()   const { return entry_->key; }
        V& value()             { return entry_->value; }
        const V& value() const { return entry_->value; }

        Iterator& operator++() {
            if (entry_) {
                entry_ = entry_->next;
            }
            if (!entry_) {
                ++bucketIdx_;
                advance();
            }
            return *this;
        }

        bool operator==(const Iterator& other) const {
            return entry_ == other.entry_;
        }
        bool operator!=(const Iterator& other) const {
            return entry_ != other.entry_;
        }

    private:
        void advance() {
            while (bucketIdx_ < capacity_ && !buckets_[bucketIdx_]) {
                ++bucketIdx_;
            }
            entry_ = (bucketIdx_ < capacity_) ? buckets_[bucketIdx_] : nullptr;
        }

        Entry** buckets_;
        int     capacity_;
        int     bucketIdx_;
        Entry*  entry_;
    };

    // ---------------------------------------------------------------
    // Konstruktori / destruktor
    // ---------------------------------------------------------------

    explicit HashMap(int initialCapacity = DEFAULT_CAPACITY)
        : capacity_(initialCapacity), size_(0)
    {
        buckets_ = new Entry*[capacity_];
        for (int i = 0; i < capacity_; ++i) {
            buckets_[i] = nullptr;
        }
    }

    ~HashMap() { destroyAll(); }

    /** Copy konstruktor. */
    HashMap(const HashMap& other)
        : capacity_(other.capacity_), size_(0)
    {
        buckets_ = new Entry*[capacity_];
        for (int i = 0; i < capacity_; ++i) {
            buckets_[i] = nullptr;
        }
        for (int i = 0; i < other.capacity_; ++i) {
            Entry* e = other.buckets_[i];
            while (e) {
                put(e->key, e->value);
                e = e->next;
            }
        }
    }

    /** Copy assignment. */
    HashMap& operator=(const HashMap& other) {
        if (this != &other) {
            destroyAll();
            capacity_ = other.capacity_;
            size_ = 0;
            buckets_ = new Entry*[capacity_];
            for (int i = 0; i < capacity_; ++i) {
                buckets_[i] = nullptr;
            }
            for (int i = 0; i < other.capacity_; ++i) {
                Entry* e = other.buckets_[i];
                while (e) {
                    put(e->key, e->value);
                    e = e->next;
                }
            }
        }
        return *this;
    }

    // ---------------------------------------------------------------
    // Osnovne operacije
    // ---------------------------------------------------------------

    /**
     * Ubacuje ili azurira par (key, value).
     * Ako kljuc postoji, vrednost se prepisuje.
     */
    void put(const K& key, const V& value) {
        if (shouldResize()) {
            resize(capacity_ * 2);
        }
        int idx = bucketIndex(key);
        Entry* e = buckets_[idx];
        while (e) {
            if (equal_(e->key, key)) {
                e->value = value;  // Azuriranje
                return;
            }
            e = e->next;
        }
        // Novi entry na pocetak lanca
        Entry* newEntry = new Entry(key, value);
        newEntry->next = buckets_[idx];
        buckets_[idx] = newEntry;
        ++size_;
    }

    /**
     * Dohvata vrednost za dati kljuc.
     * @return true ako kljuc postoji, false inace.
     */
    bool get(const K& key, V& outValue) const {
        int idx = bucketIndex(key);
        Entry* e = buckets_[idx];
        while (e) {
            if (equal_(e->key, key)) {
                outValue = e->value;
                return true;
            }
            e = e->next;
        }
        return false;
    }

    /**
     * Vraca referencu na vrednost za kljuc.
     * Ako kljuc ne postoji, kreira default entry.
     * Korisno za: map[key] += value;
     */
    V& operator[](const K& key) {
        if (shouldResize()) {
            resize(capacity_ * 2);
        }
        int idx = bucketIndex(key);
        Entry* e = buckets_[idx];
        while (e) {
            if (equal_(e->key, key)) {
                return e->value;
            }
            e = e->next;
        }
        // Kreiraj nov entry sa default vrednoscu
        Entry* newEntry = new Entry(key, V());
        newEntry->next = buckets_[idx];
        buckets_[idx] = newEntry;
        ++size_;
        return newEntry->value;
    }

    /** Uklanja entry po kljucu. Vraca true ako je postojao. */
    bool remove(const K& key) {
        int idx = bucketIndex(key);
        Entry* e = buckets_[idx];
        Entry* prev = nullptr;
        while (e) {
            if (equal_(e->key, key)) {
                if (prev) {
                    prev->next = e->next;
                } else {
                    buckets_[idx] = e->next;
                }
                delete e;
                --size_;
                return true;
            }
            prev = e;
            e = e->next;
        }
        return false;
    }

    /** Proverava da li kljuc postoji. */
    bool containsKey(const K& key) const {
        int idx = bucketIndex(key);
        Entry* e = buckets_[idx];
        while (e) {
            if (equal_(e->key, key)) return true;
            e = e->next;
        }
        return false;
    }

    // ---------------------------------------------------------------
    // Kapacitet
    // ---------------------------------------------------------------
    int  getSize()     const { return size_; }
    int  getCapacity() const { return capacity_; }
    bool isEmpty()     const { return size_ == 0; }

    // ---------------------------------------------------------------
    // Brisanje
    // ---------------------------------------------------------------

    /** Brise sve entry-je ali zadrzava bucket niz. */
    void clear() {
        for (int i = 0; i < capacity_; ++i) {
            Entry* e = buckets_[i];
            while (e) {
                Entry* next = e->next;
                delete e;
                e = next;
            }
            buckets_[i] = nullptr;
        }
        size_ = 0;
    }

    // ---------------------------------------------------------------
    // Iteracija
    // ---------------------------------------------------------------
    Iterator begin() { return Iterator(buckets_, capacity_, 0, nullptr); }
    Iterator end()   { return Iterator(buckets_, capacity_, capacity_, nullptr); }

private:
    static const int DEFAULT_CAPACITY = 16;
    static constexpr float MAX_LOAD_FACTOR = 0.75f;

    Entry** buckets_;
    int     capacity_;
    int     size_;
    Hash    hash_;
    Equal   equal_;

    int bucketIndex(const K& key) const {
        return hash_(key) % static_cast<unsigned int>(capacity_);
    }

    bool shouldResize() const {
        return static_cast<float>(size_ + 1) / capacity_ > MAX_LOAD_FACTOR;
    }

    void resize(int newCapacity) {
        Entry** oldBuckets = buckets_;
        int oldCapacity = capacity_;

        capacity_ = newCapacity;
        buckets_ = new Entry*[capacity_];
        for (int i = 0; i < capacity_; ++i) {
            buckets_[i] = nullptr;
        }
        size_ = 0;

        for (int i = 0; i < oldCapacity; ++i) {
            Entry* e = oldBuckets[i];
            while (e) {
                Entry* next = e->next;
                put(e->key, e->value);
                delete e;
                e = next;
            }
        }
        delete[] oldBuckets;
    }

    void destroyAll() {
        clear();
        delete[] buckets_;
        buckets_ = nullptr;
    }
};

#endif // HASH_MAP_H
