#pragma once
#include <stdint.h>
#include <string>
#include <array>
#include <memory>
#include <iterator>
#include <type_traits>

template <class T>
class SplayTree {
    public:
        template <bool IsConst>
        class Iterator;

        using value_type = T;
        using size_type = std::size_t;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    private:
        class Node;

    public:
        SplayTree() {}
        SplayTree(const SplayTree&) = delete;
        SplayTree(SplayTree&&);

        template <class ForwardIterator>
        SplayTree(ForwardIterator begin, ForwardIterator end);

        SplayTree(std::initializer_list<T> ilist) : SplayTree(ilist.begin(),
                                                              ilist.end()) {}

        SplayTree& operator=(const SplayTree&) = delete;
        SplayTree& operator=(SplayTree&&);

        bool empty() const noexcept {
            return !dummy_.son_[0];
        }

        size_type size() const noexcept {
            return dummy_.leftSubtreeSize();
        }

        // Return reference to element at given position.
        // Average complexity is O(log size()).
        // Recently accessed elements are accessed faster.
        reference at(size_type index);
        const_reference at(size_type index) const;

        // Remove elements in [left_size, size()) and return new tree with those
        // elements.
        // Complexity: O(log size())
        SplayTree split(size_type left_size);

        // Split before given iterator.
        SplayTree split(const_iterator);

        // Append elements from rhs to this tree.
        // Complexity: O(log size())
        void merge(SplayTree&& rhs);

        // Reverse elements in range [first, last)
        // Complexity: O(log size())
        void reverse(size_type first, size_type last);

        iterator insert(const_iterator, T);

        iterator erase(const_iterator pos);
        iterator erase(const_iterator first, const_iterator last);

        void swap(SplayTree& rhs) noexcept;

        // Iterating doesn't change tree structure.
        iterator begin();
        const_iterator begin() const;
        const_iterator cbegin() const;
        iterator end();
        const_iterator end() const;
        const_iterator cend() const;

        reverse_iterator rbegin();
        const_reverse_iterator rbegin() const;
        const_reverse_iterator crbegin() const;
        reverse_iterator rend();
        const_reverse_iterator rend() const;
        const_reverse_iterator crend() const;

    private:
        Node& root() const {
            return *(dummy_.son_[0]);
        }

        bool isRoot(const Node& node) const {
            return node.dad_ == node.dad_->dad_;
        }

        template <class ForwardIterator>
        std::unique_ptr<Node> buildTree(ForwardIterator, ForwardIterator);

        void rotate(Node&);

        // Move node up making it root while rebalancing nodes on its path.
        void splay(Node&);

        Node& findNode(size_type index) const;

        void reverseTree() noexcept;

    private:
        mutable Node dummy_;
};

template <class T>
class SplayTree<T>::Node {
    public:
        explicit Node(T data = T()) : data_(std::move(data)) {
            dad_ = this;
        }
        Node(const Node&) = delete;
        Node(Node&&) = default;

        bool whichSon() const noexcept {
            return dad_->son_[1].get() == this;
        }

        SplayTree<T>::size_type leftSubtreeSize() const noexcept {
            return son_[0] ? son_[0]->subtree_size_ : 0;
        }

        void updateSubtreeSize() noexcept {
            subtree_size_ = 1;
            for (bool dir : {0, 1}) {
                if (son_[dir]) {
                    subtree_size_ += son_[dir]->subtree_size_;
                }
            }
        }

        void push() {
            using std::swap;
            if (reverse_) {
                swap(son_[0], son_[1]);
                for (bool dir : {0, 1}) {
                    if (son_[dir]) {
                        son_[dir]->reverse_ ^= 1;
                    }
                }
                reverse_ = false;
            }
        }

        void link(std::unique_ptr<Node>&& son, bool dir) {
            son_[dir] = std::move(son);
            if (son_[dir]) {
                son_[dir]->dad_ = this;
            }
            updateSubtreeSize();
        }

    public:
        T data_;
        std::array<std::unique_ptr<Node>, 2> son_;
        Node* dad_;
        typename SplayTree<T>::size_type subtree_size_ = 1;
        bool reverse_ = false;
};

template <class T>
template <bool IsConst>
class SplayTree<T>::Iterator :
        public std::iterator<std::forward_iterator_tag, T> {
    friend class SplayTree<T>;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = SplayTree::value_type;
        // using difference_type = std::allocator<T>::difference_type;
        using reference = std::conditional_t<IsConst,
                                             SplayTree::const_reference,
                                             SplayTree::reference>;
        using pointer = std::conditional_t<IsConst,
                                           SplayTree::const_pointer,
                                           SplayTree::pointer>;

    public:
        Iterator() : node_(nullptr) {}
        Iterator(const Iterator<false>& rhs) {
            node_ = rhs.node_;
        }

        // Average complexity for increment / decrement operators is O(1)
        // Worst case complexity is O(height)
        Iterator& operator++();

        Iterator operator++(int) {
            auto result = *this;
            operator++();
            return result;
        }

        Iterator& operator--();

        Iterator operator--(int) {
            auto result = *this;
            operator--();
            return result;
        }

        friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
            return lhs.node_ == rhs.node_;
        }

        friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
            return !operator==(lhs, rhs);
        }

        reference operator*() const {
            return node_->data_;
        }

        pointer operator->() const {
            return &node_->data_;
        }

    private:
        Iterator(SplayTree::Node* node) : node_(node) {}

    private:
        SplayTree::Node* node_;
};



template <class T>
SplayTree<T>::SplayTree(SplayTree<T>&& rhs) {
    swap(rhs);
}

template <class T>
SplayTree<T>& SplayTree<T>::operator=(SplayTree&& rhs) {
    swap(rhs);
    return *this;
}

template <class T>
template <class ForwardIterator>
SplayTree<T>::SplayTree(ForwardIterator begin, ForwardIterator end) {
    dummy_.link(buildTree(begin, end), 0);
}


template <class T>
template <class ForwardIterator>
std::unique_ptr<typename SplayTree<T>::Node> SplayTree<T>::buildTree(
        ForwardIterator begin, ForwardIterator end) {
    const auto dist = std::distance(begin, end);
    if (dist == 0) {
        return nullptr;
    }
    if (dist == 1) {
        return std::make_unique<Node>(*begin);
    }
    auto mid = begin;
    std::advance(mid, dist / 2);
    auto result = std::make_unique<Node>(*mid);
    result->link(buildTree(begin, mid), 0);
    result->link(buildTree(++mid, end), 1);
    return result;
}

template <class T>
void SplayTree<T>::swap(SplayTree<T>& rhs) noexcept {
    using std::swap;
    swap(rhs.dummy_.son_[0], dummy_.son_[0]);
    if (dummy_.son_[0]) {
        dummy_.son_[0]->dad_ = &dummy_;
    }
    if (rhs.dummy_.son_[0]) {
        rhs.dummy_.son_[0]->dad_ = &rhs.dummy_;
    }
    dummy_.updateSubtreeSize();
    rhs.dummy_.updateSubtreeSize();
}

template <class T>
void SplayTree<T>::rotate(Node& u) {
    if (&u == &dummy_ || isRoot(u)) {
        throw std::runtime_error("Attempt to rotate root");
    }
    Node& v = *u.dad_;
    Node& w = *v.dad_;
    const bool dir = u.whichSon();
    const bool dad_dir = v.whichSon();
    u.son_[!dir] = std::exchange(w.son_[dad_dir], std::exchange(v.son_[dir],
            std::exchange(u.son_[!dir], nullptr)));
    if (v.son_[dir]) {
        v.son_[dir]->dad_ = &v;
    }
    v.dad_ = &u;
    u.dad_ = &w;
    v.updateSubtreeSize();
    u.updateSubtreeSize();
    w.updateSubtreeSize();
}

template <class T>
void SplayTree<T>::splay(Node& u) {
    if (&u == &dummy_) {
        throw std::runtime_error("called splay(dummy_)"); //TODO
    }
    while (!isRoot(u)) {
        Node& v = *u.dad_;
        if (isRoot(v)) {
            v.push();
            u.push();
            rotate(u);
        } else {
            v.dad_->push();
            v.push();
            u.push();
            const bool u_dir = u.whichSon();
            const bool v_dir = v.whichSon();
            if (u_dir == v_dir) {
                rotate(v);
                rotate(u);
            } else {
                rotate(u);
                rotate(u);
            }
        }
    }
}

template <class T>
typename SplayTree<T>::Node& SplayTree<T>::findNode(size_type i) const {
    size_type cumulated = 0;
    Node* p = &root();
    while (true) {
        p->push();
        const size_type index = cumulated + p->leftSubtreeSize();
        if (index == i) {
            return *p;
        }
        if (i < index) {
            p = p->son_[0].get();
        } else {
            p = p->son_[1].get();
            cumulated = index + 1;
        }
    }
}

template <class T>
typename SplayTree<T>::reference SplayTree<T>::at(size_type i) {
    if (i >= size()) {
        throw std::out_of_range(std::string("Index ") + std::to_string(i) +
                                " is greater than size() which is " +
                                std::to_string(size()));
    }
    splay(findNode(i));
    return root().data_;
}

template <class T>
typename SplayTree<T>::const_reference SplayTree<T>::at(size_type i) const {
    if (i >= size()) {
        throw std::out_of_range(std::string("Index ") + std::to_string(i) +
                                " is greater than size() which is " +
                                std::to_string(size()));
    }
    return findNode(i).data_;
}

template <class T>
SplayTree<T> SplayTree<T>::split(size_type left_size) {
    return split(const_iterator(&findNode(left_size)));
}

template <class T>
SplayTree<T> SplayTree<T>::split(const_iterator it) {
    auto prev = it;
    try {
        --prev;
    } catch (std::runtime_error) {
        return std::move(*this);
    }
    splay(*prev.node_);
    SplayTree<T> result;
    result.dummy_.link(std::move(root().son_[1]), 0);
    root().updateSubtreeSize();
    return result;
}

template <class T>
void SplayTree<T>::merge(SplayTree<T>&& rhs) {
    if (empty()) {
        swap(rhs);
        return;
    }
    if (rhs.empty()) {
        return;
    }
    splay(findNode(size() - 1));
    root().link(std::move(rhs.dummy_.son_[0]), 1);
}

template <class T>
void SplayTree<T>::reverseTree() noexcept {
    if (!empty()) {
        root().reverse_ ^= true;
    }
}

template <class T>
void SplayTree<T>::reverse(size_type first, size_type last) {
    if (first == 0 && last == size()) {
        reverseTree();
        return;
    }
    if (first > last) {
        throw std::runtime_error(std::string("Invalid range: [") +
                                 std::to_string(first) +
                                 ", " + std::to_string(last) + ")");
    }
    if (last > size()) {
        throw std::out_of_range(std::string("last index is ") +
                                std::to_string(last) +
                                " which is greater than size() which is " +
                                std::to_string(size()));
    }
    auto right = split(last);
    auto center = split(first);
    center.reverseTree();
    merge(std::move(center));
    merge(std::move(right));
}

template <class T>
typename SplayTree<T>::iterator SplayTree<T>::insert(const_iterator it, T e) {
    Node* new_node = nullptr;
    if (!it.node_->son_[0]) {
        it.node_->link(std::make_unique<Node>(std::move(e)), 0);
        new_node = it.node_->son_[0].get();
    } else {
        Node* p = it.node_->son_[0].get();
        while (p->son_[1]) {
            p = p->son_[1].get();
        }
        p->link(std::make_unique<Node>(std::move(e)), 1);
        new_node = p->son_[1].get();
    }
    splay(*new_node);
    return {new_node};
}

template <class T>
typename SplayTree<T>::iterator SplayTree<T>::erase(const_iterator pos) {
    const auto copy = pos;
    return erase(copy, ++pos);
}

template <class T>
typename SplayTree<T>::iterator SplayTree<T>::erase(const_iterator first,
                                                    const_iterator last) {
    auto right = split(last);
    split(first);
    merge(std::move(right));
    if (empty() || !root().son_[1]) {
        return end();
    } else {
        return iterator(root().son_[1].get());
    }
}

template <class T>
typename SplayTree<T>::iterator SplayTree<T>::begin() {
    if (empty()) {
        return end();
    }
    return iterator(&findNode(0));
}

template <class T>
typename SplayTree<T>::const_iterator SplayTree<T>::begin() const {
    if (empty()) {
        return end();
    }
    return const_iterator(&findNode(0));
}

template <class T>
typename SplayTree<T>::const_iterator SplayTree<T>::cbegin() const {
    if (empty()) {
        return cend();
    }
    return begin();
}

template <class T>
typename SplayTree<T>::iterator SplayTree<T>::end() {
    return iterator(&dummy_);
}

template <class T>
typename SplayTree<T>::const_iterator SplayTree<T>::end() const {
    return const_iterator(&dummy_);
}

template <class T>
typename SplayTree<T>::const_iterator SplayTree<T>::cend() const {
    return end();
}

template <class T>
typename SplayTree<T>::reverse_iterator SplayTree<T>::rbegin() {
    return reverse_iterator(end());
}

template <class T>
typename SplayTree<T>::const_reverse_iterator SplayTree<T>::rbegin() const {
    return const_reverse_iterator(end());
}

template <class T>
typename SplayTree<T>::const_reverse_iterator SplayTree<T>::crbegin() const {
    return const_reverse_iterator(end());
}

template <class T>
typename SplayTree<T>::reverse_iterator SplayTree<T>::rend() {
    return reverse_iterator(begin());
}

template <class T>
typename SplayTree<T>::const_reverse_iterator SplayTree<T>::rend() const {
    return const_reverse_iterator(begin());
}

template <class T>
typename SplayTree<T>::const_reverse_iterator SplayTree<T>::crend() const {
    return const_reverse_iterator(begin());
}


template <class T>
template <bool IsConst>
SplayTree<T>::Iterator<IsConst>&
SplayTree<T>::Iterator<IsConst>::operator++() {
    if (node_->dad_ == node_) {
        throw std::runtime_error("Calling operator++ for end iterator");
    }
    node_->push();
    if (node_->son_[1]) {
        node_ = node_->son_[1].get();
        node_->push();
        while (node_->son_[0]) {
            node_ = node_->son_[0].get();
            node_->push();
        }
    } else {
        while (node_->whichSon()) {
            node_ = node_->dad_;
        }
        node_ = node_->dad_;
    }
    return *this;
}

template <class T>
template <bool IsConst>
SplayTree<T>::Iterator<IsConst>&
SplayTree<T>::Iterator<IsConst>::operator--() {
    node_->push();
    if (node_->son_[0]) {
        node_ = node_->son_[0].get();
        node_->push();
        while (node_->son_[1]) {
            node_ = node_->son_[1].get();
            node_->push();
        }
    } else {
        while (!node_->whichSon()) {
            node_ = node_->dad_;
            if (node_->dad_ == node_) {
                throw std::runtime_error(
                        "Calling operator-- for begin iterator");
            }
        }
        node_ = node_->dad_;
    }
    return *this;
}

