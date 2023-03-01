#include <iterator>
#include <vector>

template <class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {

  using Item = std::pair<const KeyType, ValueType>;

  struct Bucket {
    Item item;
    size_t distance = 0;
    bool empty = true;

    Bucket() {}

    Bucket(const Item &item, size_t distance = 0, bool empty = true)
        : item(item), distance(distance), empty(empty) {}
  };

  template <bool IsConst> struct hm_iter {

    using iterv_type =
        typename std::conditional<IsConst,
                                  typename std::vector<Bucket>::const_iterator,
                                  typename std::vector<Bucket>::iterator>::type;
    using item_ref_type =
        typename std::conditional<IsConst, const Item &, Item &>::type;
    using item_ptr_type =
        typename std::conditional<IsConst, const Item *, Item *>::type;

    iterv_type iterv, endv;

    hm_iter(iterv_type iterv, iterv_type endv) : iterv(iterv), endv(endv) {}

    hm_iter() {}

    hm_iter &operator++() {
      while (this->iterv != this->endv) {
        ++this->iterv;

        if (this->iterv == this->endv || !this->iterv->empty)
          break;
      }

      return *this;
    }

    hm_iter operator++(int) {
      hm_iter cpy = *this;
      ++(*this);
      return cpy;
    }

    item_ref_type operator*() { return this->iterv->item; }

    item_ptr_type operator->() { return &this->iterv->item; }

    bool operator==(const hm_iter &other) const {
      return this->iterv == other.iterv;
    }

    bool operator!=(const hm_iter &other) const {
      return this->iterv != other.iterv;
    }
  };

  /*  private variables  */

  Hash hasher_;
  std::vector<Bucket> table_;

  size_t size_;
  size_t capacity_;

  const std::vector<size_t> PRIME_CAPS_ = {
      67,    137,   277,   557,    1117,   2237,   4481,    8963,
      17929, 35863, 71741, 143483, 286973, 573953, 1147921, 2295859};
  size_t cap_index_ = 0;

  /*  private methods  */

  void initialize_() {
    this->size_ = 0;
    this->capacity_ = this->PRIME_CAPS_[this->cap_index_];

    this->table_.clear();
    this->table_.resize(this->capacity_);
  }

  size_t mod_hash_(const KeyType &key) const {
    return this->hasher_(key) % this->capacity_;
  }

  template <class InputIt> void create_(InputIt first, InputIt last) {
    this->initialize_();

    for (auto it = first; it != last; ++it) {
      this->insert(*it);
    }
  }

  bool overload_() const { return 10 * this->size_ >= 9 * this->capacity_; }

  bool underload_() const {
    return (this->cap_index_ > 0) && (100 * this->size_ < this->capacity_);
  }

  void rehash_() {
    std::vector<Item> items;
    items.reserve(this->size_);

    auto first = this->begin();
    auto last = this->end();

    for (auto it = first; it != last; ++it)
      items.emplace_back(*it);

    this->create_(items.begin(), items.end());
  }

  size_t first_used_() const {
    auto firstv = this->table_.begin();
    auto iterv = firstv;
    auto lastv = this->table_.end();

    while (iterv != lastv && iterv->empty) {
      ++iterv;
    }

    return (iterv - firstv);
  }

  size_t find_index(const KeyType &key) const {
    size_t index = mod_hash_(key);
    size_t distance = 0;

    auto firstv = this->table_.begin();
    auto lastv = this->table_.end();
    auto iterv = firstv + index;

    while (!iterv->empty && iterv->distance >= distance) {

      if (iterv->item.first == key)
        return (iterv - firstv);

      ++iterv;
      ++distance;

      if (iterv == lastv)
        iterv = firstv;
    }

    return this->capacity_;
  }

public:
  using iterator = hm_iter<false>;
  using const_iterator = hm_iter<true>;

  HashMap(const Hash &hasher = Hash()) : hasher_(hasher) {
    this->initialize_();
  }

  template <class InputIt>
  HashMap(InputIt first, InputIt last, const Hash &hasher = Hash())
      : hasher_(hasher) {
    this->create_(first, last);
  }

  HashMap(std::initializer_list<Item> init, const Hash &hasher = Hash())
      : hasher_(hasher) {
    this->create_(init.begin(), init.end());
  }

  HashMap &operator=(const HashMap &other) {
    if (&other != this) {
      this->clear();
      this->create_(other.begin(), other.end());
    }

    return *this;
  }

  size_t size() const { return this->size_; }

  bool empty() const { return this->size_ == 0; }

  Hash hash_function() const { return this->hasher_; }

  void insert(const Item &item, size_t distance = 0, ssize_t after = -1) {
    auto last = this->end();
    bool first_insert = (after == -1);

    if (first_insert && this->find(item.first) != last)
      return;

    size_t index = after;

    if (after == -1)
      index = this->mod_hash_(item.first);

    auto firstv = this->table_.begin();
    auto lastv = this->table_.end();
    auto iterv = firstv + index;

    if (iterv == lastv)
      iterv = firstv;

    while (!iterv->empty && iterv->distance >= distance) {
      ++iterv;
      ++distance;

      if (iterv == lastv)
        iterv = firstv;
    }

    auto prev_item = iterv->item;
    size_t prev_dist = iterv->distance;
    bool prev_empty = iterv->empty;

    set(iterv, item, distance);

    if (!prev_empty) {
      index = iterv - firstv;
      insert(prev_item, prev_dist + 1, index + 1);
    }

    if (first_insert) {
      ++this->size_;

      if (this->overload_()) {
        ++this->cap_index_;
        this->rehash_();
      }
    }
  }

  void set(typename std::vector<Bucket>::iterator iterv, const Item &item,
           size_t distance) {
    const_cast<KeyType &>(iterv->item.first) = item.first;
    iterv->item.second = item.second;
    iterv->distance = distance;
    iterv->empty = false;
  }

  void erase(const KeyType &key) {
    auto it = this->find(key);

    if (it == this->end())
      return;

    it.iterv->empty = true;

    auto firstv = this->table_.begin();

    auto prev_iterv = it.iterv;
    auto iterv = prev_iterv + 1;
    auto lastv = this->table_.end();

    if (iterv == lastv)
      iterv = firstv;

    while (!iterv->empty && iterv->distance > 0) {
      set(prev_iterv, iterv->item, iterv->distance - 1);

      iterv->empty = true;

      ++iterv;
      ++prev_iterv;

      if (iterv == lastv)
        iterv = firstv;

      if (prev_iterv == lastv)
        prev_iterv = firstv;
    }

    --this->size_;

    if (this->underload_()) {
      --this->cap_index_;
      this->rehash_();
    }
  }

  iterator find(const KeyType &key) {
    size_t index = find_index(key);

    auto iterv = this->table_.begin() + index;
    auto lastv = this->table_.end();

    return iterator(iterv, lastv);
  }

  const_iterator find(const KeyType &key) const {
    size_t index = find_index(key);

    auto iterv = this->table_.begin() + index;
    auto lastv = this->table_.end();

    return const_iterator(iterv, lastv);
  }

  iterator begin() {
    auto iterv = this->table_.begin() + first_used_();
    auto lastv = this->table_.end();

    return iterator(iterv, lastv);
  }

  iterator end() {
    auto lastv = this->table_.end();

    return iterator(lastv, lastv);
  }

  const_iterator begin() const {
    auto iterv = this->table_.begin() + first_used_();
    auto lastv = this->table_.end();

    return const_iterator(iterv, lastv);
  }

  const_iterator end() const {
    auto lastv = this->table_.end();

    return const_iterator(lastv, lastv);
  }

  ValueType &operator[](const KeyType &key) {
    iterator it = this->find(key);

    if (it == this->end()) {
      Item item(key, ValueType());
      insert(item);
      it = this->find(key);
    }

    return it->second;
  }

  const ValueType &at(const KeyType &key) const {
    const_iterator it = find(key);

    if (it == end())
      throw std::out_of_range("HashMap doesn't contain this key");

    return it->second;
  }

  void clear() {
    this->size_ = 0;
    this->cap_index_ = 0;
    this->capacity_ = this->PRIME_CAPS_[0];

    this->table_.clear();
    this->table_.resize(this->capacity_);
  }
};
