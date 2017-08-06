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
        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;
        using pointer = value_type*;
        using const_pointer = const value_type*;

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

        reference find(size_type index);
        const_reference find(size_type index) const;

        // Remove elements in [left_size, size()) and return new tree with those
        // elements.
        // Complexity: O(log size())
        SplayTree split(size_type left_size);

        // Append elements from rhs to this tree.
        // Complexity: O(log size())
        void merge(SplayTree&& rhs);

        void swap(SplayTree& rhs) noexcept;

        iterator begin();
        const_iterator begin() const;
        const_iterator cbegin() const;
        iterator end();
        const_iterator end() const;
        const_iterator cend() const;

    private:
        Node& root() const {
            return *(dummy_.son_[0]);
        }

        bool isRoot(const Node& node) const {
            return node.dad_ == &dummy_;
        }

        template <class ForwardIterator>
        std::unique_ptr<Node> buildTree(ForwardIterator, ForwardIterator);

        void rotate(Node&);

        void splay(Node&);

        Node& findNode(size_type index) const;

    private:
        Node dummy_;
};

template <class T>
class SplayTree<T>::Node {
    public:
        explicit Node(T data = T()) : data_(std::move(data)) {
            dad_ = this;
        }
        Node(const Node&) = delete;
        Node(Node&&) = default;

        // explicit Node(Node& dad) : dad_(&dad) {}

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

    private:
        using NodePtr = std::conditional_t<IsConst, const SplayTree::Node*,
                                           SplayTree::Node*>;

    public:
        Iterator() : node_(nullptr) {}
        Iterator(const Iterator<false>& rhs) {
            node_ = rhs.node_;
        }

        Iterator& operator++();
        Iterator& operator--();

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
        Iterator(NodePtr node) : node_(node) {}

    private:
        NodePtr node_;
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
            rotate(u);
        } else {
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
typename SplayTree<T>::reference SplayTree<T>::find(size_type i) {
    if (i >= size()) {
        throw std::out_of_range(std::string("Index ") + std::to_string(i) +
                                " is greater than size() which is " +
                                std::to_string(size()));
    }
    splay(findNode(i));
    return root().data_;
}

template <class T>
typename SplayTree<T>::const_reference SplayTree<T>::find(size_type i) const {
    if (i >= size()) {
        throw std::out_of_range(std::string("Index ") + std::to_string(i) +
                                " is greater than size() which is " +
                                std::to_string(size()));
    }
    return findNode(i).data_;
}

template <class T>
SplayTree<T> SplayTree<T>::split(size_type left_size) {
    if (left_size == 0) {
        SplayTree<T> result;
        result.dummy_.link(std::move(dummy_.son_[0]), 0);
        return result;
    }
    splay(findNode(left_size - 1));
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
template <bool IsConst>
SplayTree<T>::Iterator<IsConst>&
SplayTree<T>::Iterator<IsConst>::operator++() {
    if (node_->dad_ == node_) {
        throw std::runtime_error("Calling operator++ for end iterator");
    }
    if (node_->son_[1]) {
        node_ = node_->son_[1].get();
        while (node_->son_[0]) {
            node_ = node_->son_[0].get();
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
    if (node_->son_[0]) {
        node_ = node_->son_[0].get();
        while (node_->son_[1]) {
            node_ = node_->son_[1].get();
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

template class SplayTree<int>;

