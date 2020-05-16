#ifndef VECTOR_H
#define VECTOR_H

#include <algorithm>
#include <cstddef>
#include <cassert>
#include <iostream>

template <typename T>
struct vector {
    typedef T* iterator;
    typedef T const* const_iterator;

    vector();                    // O(1) nothrow
    vector(vector const&);                  // O(N) strong
    vector& operator=(vector const& other); // O(N) strong

    ~vector();                              // O(N) nothrow

    T& operator[](size_t i);                // O(1) nothrow
    T const& operator[](size_t i) const;    // O(1) nothrow

    T* data();                              // O(1) nothrow
    T const* data() const;                  // O(1) nothrow
    size_t size() const;                    // O(1) nothrow

    T& front();                             // O(1) nothrow
    T const& front() const;                 // O(1) nothrow

    T& back();                              // O(1) nothrow
    T const& back() const;                  // O(1) nothrow
    void push_back(T const&);               // O(1)* strong
    void pop_back();                        // O(1) nothrow

    bool empty() const;                     // O(1) nothrow

    size_t capacity() const;                // O(1) nothrow
    void reserve(size_t);                   // O(N) strong
    void shrink_to_fit();                   // O(N) strong

    void clear();                           // O(N) nothrow

    void swap(vector&);                     // O(1) nothrow

    iterator begin();                       // O(1) nothrow
    iterator end();                         // O(1) nothrow

    const_iterator begin() const;           // O(1) nothrow
    const_iterator end() const;             // O(1) nothrow

    iterator insert(const_iterator pos, T const&); // O(N) weak

    iterator erase(const_iterator pos);     // O(N) weak

    iterator erase(const_iterator first, const_iterator last); // O(N) weak

private:
    void push_back_realloc(T const&);
    void new_buffer(size_t new_capacity);
    void copy_all(T*, T const*, size_t);
    void destruct_all(T*, size_t);

private:
    T* data_;
    size_t size_{};
    size_t capacity_{};
};

template<typename T>
vector<T>::vector(vector const& other) : vector() {
    if (other.capacity_ != 0) {
        T* new_data = static_cast<T*>(operator new(other.capacity_ * sizeof(T)));
        try {
            copy_all(new_data, other.data_, other.size_);
        } catch (...) {
            operator delete(new_data);
            throw;
        }
        data_ = new_data;
        size_ = other.size_;
        capacity_ = other.capacity_;
    } else {
        destruct_all(data_, size_);
        operator delete (data_);
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }
}

template<typename T>
vector<T>& vector<T>::operator=(vector<T> const& other) {
    vector<T> tmp = other;
    if (this != &other) {
        swap(tmp);
    }
    return *this;
}

template<typename T>
void vector<T>::swap(vector<T>& other) {
    std::swap(capacity_, other.capacity_);
    std::swap(size_, other.size_);
    std::swap(data_, other.data_);
}

template<typename T>
vector<T>::~vector() {
    if (data_) {
        destruct_all(data_, size_);
        operator delete(data_);
    }
}

template<typename T>
T& vector<T>::operator[](size_t i) {
    assert(i < size_);
    return data_[i];
}

template<typename T>
T const& vector<T>::operator[](size_t i) const {
    assert(i < size_);
    return data_[i];
}

template<typename T>
T* vector<T>::data() {
    return data_;
}

template<typename T>
T const* vector<T>::data() const {
    return data_;
}

template<typename T>
size_t vector<T>::size() const {
    return size_;
}

template<typename T>
T& vector<T>::front() {
    assert(size_ != 0);
    return *data_;
}

template<typename T>
T const &vector<T>::front() const {
    return *data_;
}

template<typename T>
T& vector<T>::back() {
    assert(size_ != 0);
    return data_[size_ - 1];
}

template<typename T>
T const& vector<T>::back() const {
    assert(size_ != 0);
    return data_[size_ - 1];
}

template<typename T>
bool vector<T>::empty() const {
    return size_ == 0;
}

template<typename T>
typename vector<T>::const_iterator vector<T>::begin() const {
    return data_;
}

template<typename T>
typename vector<T>::iterator vector<T>::begin() {
    return data_;
}

template<typename T>
size_t vector<T>::capacity() const {
    return capacity_;
}

template<typename T>
void vector<T>::reserve(size_t new_capacity) {
    if (new_capacity > capacity_) {
        vector<T> tmp(*this);
        tmp.new_buffer(new_capacity);
        copy_all(tmp.data_, data_, size_);
        assert(tmp.size() == size());
        tmp.capacity_ = new_capacity;
        swap(tmp);
    }
}

template<typename T>
void vector<T>::new_buffer(size_t new_capacity) {
    assert(new_capacity != 0);
    T* ptr = static_cast<T*>(operator new(new_capacity * sizeof(T)));
    if (data_) {
        destruct_all(data_, size_);
        operator delete(data_);
    }
    data_ = ptr;
}

template<typename T>
void vector<T>::copy_all(T* dst, T const* src, size_t size) {
    size_t i = 0;
    try {
        for (; i < size; ++i) {
            new (dst + i) T(src[i]);
        }
    } catch (...) {
        destruct_all(dst, i);
        throw;
    }
}

template<typename T>
void vector<T>::destruct_all(T* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        data[i].~T();
    }
}

template<typename T>
void vector<T>::clear() {
    destruct_all(data_, size_);
    size_ = 0;
}

template<typename T>
typename vector<T>::iterator vector<T>::end() {
    return data_ + size_;
}

template<typename T>
typename vector<T>::const_iterator vector<T>::end() const {
    return data_ + size_;
}

template<typename T>
void vector<T>::push_back(T const& el) {
    if (size_ != capacity_) {
        new (data_ + size_) T(el);
        ++size_;
    } else {
        push_back_realloc(el);
    }
}

template<typename T>
void vector<T>::push_back_realloc(T const& el) {
    if (capacity_ == 0) {
        new_buffer(1);
        new (data_) T(el);
        capacity_ = 1;
        size_ = 1;
    } else {
        vector<T> tmp(*this);
        tmp.reserve(capacity_ * 2);
        tmp.push_back(el);
        swap(tmp);
    }
}

template<typename T>
void vector<T>::pop_back() {
    assert(size_ != 0);
    data_[size_ - 1].~T();
    --size_;
}

template<typename T>
void vector<T>::shrink_to_fit() {
    if (size_ == capacity_) {
        return;
    }
    if (size_ != 0) {
        vector<T> tmp(*this);
        tmp.new_buffer(size_);
        copy_all(tmp.data_, data_, size_);
        swap(tmp);
        capacity_ = size_;
    } else {
        operator delete (data_);
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }
}

template<typename T>
typename vector<T>::iterator vector<T>::insert(vector::const_iterator pos, const T &v) {
    size_t old_size = size_;
    size_t pos_ = pos - begin();
    push_back(v);
    for (size_t i = old_size; i > pos_; --i) {
        std::swap(*(begin() + i), *(begin() + i - 1));
    }
    return begin() + pos_;
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector::const_iterator pos) {
    assert(size_ != 0);
    size_t pos_ = pos - begin();
    iterator res = begin() + pos_;
    for (size_t i = 0; i + 1 < size_ - (pos - data_); ++i) {
        std::swap(*res, *(res + 1));
    }
    pop_back();
    return res;
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector::const_iterator first, vector::const_iterator last) {
    assert(size_ != 0);
    size_t pos_first = first - begin();
    size_t pos_last = last - begin();
    size_t swaps = std::min(end() - first, end() - last);
    for (size_t i = 0; i < swaps; ++i) {
        std::swap(*(begin() + pos_first + i), *(begin() + pos_last + i));
    }
    size_-= last - first;
    shrink_to_fit();
    return begin() + pos_first;
}

template<typename T>
vector<T>::vector() {
    data_ = nullptr;
    size_ = 0;
    capacity_ = 0;
}

#endif // VECTOR_H
