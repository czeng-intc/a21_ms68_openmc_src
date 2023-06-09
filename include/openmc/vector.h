#ifndef OPENMC_VECTOR_H
#define OPENMC_VECTOR_H

#include <algorithm> // for copy, fill
#include <cstdlib> // for malloc
#include <iterator> // for reverse_iterator
#include <utility> // for swap

#include <omp.h>

namespace openmc {

template<typename T>
class vector {
public:
  // Types, aliases
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using iterator = T*;
  using const_iterator = const T*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // Constructors, destructors
  vector() : data_(nullptr), size_(0), capacity_(0) { }
  vector(size_type n) : vector() { this->resize(n); }
  vector(const_iterator begin, const_iterator end);

  // Copy/move constructors/assignments
  vector(const vector& other);
  vector(vector&& other) noexcept;
  vector& operator=(const vector& other);
  vector& operator=(vector&& other) noexcept;

  // TODO: If you have a global variable that is of type vector<T>, its
  // destructor gets called which then requires ~T() to be available on device.
  // Figure out a way around this
  ~vector() {
    if (data_) {
      if (omp_is_initial_device()) {
        this->clear();
        std::free(data_);
      } else {
#pragma omp target exit data map(delete: data_)
      }
    }
  }

  // Element access
  reference operator[](size_type pos) { return data_[pos]; }
  const_reference operator[](size_type pos) const { return data_[pos]; }
  reference at(size_type pos) { return data_[pos]; }
  const_reference at(size_type pos) const { return data_[pos]; }
  reference front() { return data_[0]; }
  const_reference front() const { return data_[0]; }
  reference back() { return data_[size_ - 1]; }
  const_reference back() const { return data_[size_ - 1]; }
  pointer data() { return data_; }
  const_pointer data() const { return data_; }

  // Iterators
  iterator begin() { return data_; }
  const_iterator begin() const { return data_; }
  iterator end() { return data_ + size_; }
  const_iterator end() const { return data_ + size_; }
  const_iterator cbegin() const { return data_; }
  const_iterator cend() const { return data_ + size_; }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
  const_reverse_iterator crbegin() const { return rbegin(); }
  const_reverse_iterator crend() const { return rend(); }

  // Capacity
  bool empty() const { return size_ == 0; }
  size_type size() const { return size_; }
  size_type capacity() const { return capacity_; }

  // Methods
  void clear() {
    for (size_type i = 0; i < size_; ++i) {
      data_[i].~T();
    }
    size_ = 0;
  }
  void shrink_to_fit() { }
  void pop_back() { this->resize(size_ - 1); }

  void push_back(const T& value);
  void push_back(T&& value);
  template<class... Args>
  void emplace_back(Args&&... args);

  iterator erase(const_iterator pos);
  iterator insert(const_iterator pos, const T& value);

  template<class InputIt>
  iterator insert(const_iterator pos, InputIt first, InputIt last);

  template<class InputIt>
  void assign(InputIt first, InputIt last);

  void resize(size_type count) {
    this->reserve(count);
    if (size_ < count) {
      // Default insert new elements
      for (size_type i = size_; i < count; ++i) {
        new(data_ + i) T();
      }
    } else if (count < size_) {
      // If new size is smaller, call destructor on erased elements
      for (size_type i = count; i < size_; ++i) {
        data_[i].~T();
      }
    }
    size_ = count;
  }

  void resize(size_type count, const value_type& value) {
    this->reserve(count);
    if (size_ < count) {
      std::fill(end(), begin() + count, value);
    } else if (count < size_) {
      // If new size is smaller, call destructor on erased elements
      for (size_type i = count; i < size_; ++i) {
        data_[i].~T();
      }
    }
    size_ = count;
  }

  void reserve(size_type n) {
    if (n <= capacity_) return;

    // Make new allocation
    T* data_new = static_cast<T*>(std::malloc(n * sizeof(T)));

    // Copy existing elements
    for (size_type i = 0; i < size_; ++i) {
      ::new(data_new + i) T(std::move(data_[i]));
    }

    // Remove older allocation
    if (data_) {
      for (size_type i = 0; i < size_; ++i) {
        data_[i].~T();
      }
      std::free(data_);
    }

    // Set data members
    data_ = data_new;
    capacity_ = n;
  }

  void swap(vector& other) noexcept {
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
    std::swap(data_, other.data_);
  }

  void copy_to_device() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wopenmp-mapping"
#pragma omp target enter data map(to: data_[:size_])
#pragma GCC diagnostic pop
  }

  void allocate_on_device() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wopenmp-mapping"
#pragma omp target enter data map(alloc: data_[:size_])
#pragma GCC diagnostic pop
  }

  void update_to_device() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wopenmp-mapping"
#pragma omp target update to(data_[:size_])
#pragma GCC diagnostic pop
  }

  void update_from_device() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wopenmp-mapping"
#pragma omp target update from(data_[:size_])
#pragma GCC diagnostic pop
  }

  void copy_from_device() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wopenmp-mapping"
#pragma omp target exit data map(from: data_[:size_])
#pragma GCC diagnostic pop
  }

  void release_device() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wopenmp-mapping"
#pragma omp target exit data map(release: data_[:size_])
#pragma GCC diagnostic pop
  }

private:
  T* data_;
  size_type size_;
  size_type capacity_;
};

template<typename T>
vector<T>::vector(vector<T>::const_iterator first, vector<T>::const_iterator last)
  : vector(last - first)
{
  std::copy(first, last, this->begin());
}

template<typename T>
vector<T>::vector(const vector<T>& other)
  : vector()
{
  this->resize(other.size());
  std::copy(other.cbegin(), other.cend(), this->begin());
}

template<typename T>
vector<T>::vector(vector&& other) noexcept
  : vector()
{
  other.swap(*this);
}

template<typename T>
vector<T>& vector<T>::operator=(const vector<T>& other)
{
  // Copy-and-swap idiom
  vector<T> copy(other);
  copy.swap(*this);
  return *this;
}

template<typename T>
vector<T>& vector<T>::operator=(vector<T>&& other) noexcept
{
  other.swap(*this);
  return *this;
}

template<typename T>
void vector<T>::push_back(const T& value)
{
  if (capacity_ == 0) {
    this->reserve(8);
  } else if (size_ == capacity_ ) {
    this->reserve(2*capacity_);
  }

  data_[size_] = value;
  ++size_;
}

template<typename T>
void vector<T>::push_back(T&& value)
{
  if (capacity_ == 0) {
    this->reserve(8);
  } else if (size_ == capacity_ ) {
    this->reserve(2*capacity_);
  }

  new(data_ + size_) T(std::move(value));
  ++size_;
}


template<typename T>
template<class... Args>
void vector<T>::emplace_back(Args&&... args)
{
  if (capacity_ == 0) {
    this->reserve(8);
  } else if (size_ == capacity_ ) {
    this->reserve(2*capacity_);
  }

  new(data_ + size_++) T(std::forward<Args>(args)...);
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector<T>::const_iterator pos)
{
  // Get index of position
  size_type idx = std::distance(this->cbegin(), pos);

  // Move elements from [pos + 1, end) one back
  for (size_type i = idx; i < size_ - 1; ++i) {
    data_[i] = data_[i + 1];
  }

  // Add space for one more element
  this->resize(size_ - 1);

  // Return new iterator
  return data_ + idx;
}


template<typename T>
typename vector<T>::iterator vector<T>::insert(vector<T>::const_iterator pos, const T& value)
{
  // Get index of position
  size_type idx = std::distance(this->cbegin(), pos);

  // Add space for one more element
  this->resize(size_ + 1);

  // Move elements from [pos, end) one forward
  if (size_ - 1 > 0) {
    for (size_type i = size_ - 2; i >= idx; --i) {
      data_[i + 1] = data_[i];
    }
  }

  // Insert given element
  data_[idx] = value;

  // Return new iterator
  return data_ + idx;
}

template<typename T>
template<class InputIt>
typename vector<T>::iterator vector<T>::insert(
  vector<T>::const_iterator pos,
  InputIt first,
  InputIt last)
{
  // Get index of position
  size_type start = std::distance(this->cbegin(), pos);
  size_type n = std::distance(first, last);

  // Add space for one more element
  this->resize(size_ + n);

  // Move elements from [pos, end) n forward
  if (size_ - n > 0) {
    for (size_type i = size_ - n - 1; i >= start; --i) {
      data_[i + n] = data_[i];
    }
  }

  // Insert elements
  for (size_type i = 0; i < n; ++i) {
    data_[start + i] = *first;
    ++first;
  }

  // Return new iterator
  return data_ + start;
}

template<typename T>
template<class InputIt>
void vector<T>::assign(InputIt first, InputIt last)
{
  // Get index of position
  size_type n = std::distance(first, last);

  // Replace original contents with the n values between first and last
  this->resize(n);
  for (size_type i = 0; i < n; ++i) {
    data_[i] = *first;
    ++first;
  }
}


} // namespace openmc

#endif
