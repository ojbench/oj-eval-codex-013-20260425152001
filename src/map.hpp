/**
* implement a container like std::map
*/
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

// only for std::less<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<
   class Key,
   class T,
   class Compare = std::less <Key>
   > class map {
  public:
   /**
  * the internal type of data.
  * it should have a default constructor, a copy constructor.
  * You can use sjtu::map as value_type by typedef.
    */
   typedef pair<const Key, T> value_type;
   /**
  * see BidirectionalIterator at CppReference for help.
  *
  * if there is anything wrong throw invalid_iterator.
  *     like it = map.begin(); --it;
  *       or it = map.end(); ++end();
    */
   class const_iterator;
   struct Node {
       value_type kv;
       Node *left;
       Node *right;
       Node *parent;
       int height;
       Node(const value_type &v, Node *p = nullptr)
           : kv(v), left(nullptr), right(nullptr), parent(p), height(1) {}
   };
   class iterator {
      private:
       // data members
       Node *node;
       const map *owner;
      public:
       iterator() : node(nullptr), owner(nullptr) {}
       iterator(const iterator &other) : node(other.node), owner(other.owner) {}

       iterator operator++(int) {
           if (owner == nullptr || node == nullptr) throw invalid_iterator();
           iterator tmp = *this;
           ++(*this);
           return tmp;
       }
       iterator &operator++() {
           if (owner == nullptr || node == nullptr) throw invalid_iterator();
           Node *cur = node;
           if (cur->right) {
               cur = cur->right;
               while (cur->left) cur = cur->left;
               node = cur;
               return *this;
           }
           Node *p = cur->parent;
           while (p && p->right == cur) {
               cur = p;
               p = p->parent;
           }
           node = p; // may become nullptr
           return *this;
       }

       iterator operator--(int) {
           iterator tmp = *this;
           --(*this);
           return tmp;
       }
       iterator &operator--() {
           if (owner == nullptr) throw invalid_iterator();
           if (node == nullptr) {
               if (owner->root == nullptr) throw invalid_iterator();
               Node *cur = owner->root;
               while (cur->right) cur = cur->right;
               node = cur;
               return *this;
           }
           Node *cur = node;
           if (cur->left) {
               cur = cur->left;
               while (cur->right) cur = cur->right;
               node = cur;
               return *this;
           }
           Node *p = cur->parent;
           while (p && p->left == cur) {
               cur = p;
               p = p->parent;
           }
           if (p == nullptr) throw invalid_iterator();
           node = p;
           return *this;
       }

       value_type &operator*() const {
           if (owner == nullptr || node == nullptr) throw invalid_iterator();
           return const_cast<value_type &>(node->kv);
       }
       bool operator==(const iterator &rhs) const {
           return node == rhs.node && owner == rhs.owner;
       }
       bool operator==(const const_iterator &rhs) const;
       bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
       bool operator!=(const const_iterator &rhs) const;
       value_type *operator->() const noexcept {
           if (owner == nullptr || node == nullptr) throw invalid_iterator();
           return const_cast<value_type *>(&node->kv);
       }
       friend class const_iterator;
       friend class map;
   };
   class const_iterator {
      private:
       const Node *node;
       const map *owner;
      public:
       const_iterator() : node(nullptr), owner(nullptr) {}
       const_iterator(const const_iterator &other) : node(other.node), owner(other.owner) {}
       const_iterator(const iterator &other) : node(other.node), owner(other.owner) {}

       const_iterator operator++(int) {
           if (owner == nullptr || node == nullptr) throw invalid_iterator();
           const_iterator tmp = *this;
           ++(*this);
           return tmp;
       }
       const_iterator &operator++() {
           if (owner == nullptr || node == nullptr) throw invalid_iterator();
           const Node *cur = node;
           if (cur->right) {
               cur = cur->right;
               while (cur->left) cur = cur->left;
               node = cur;
               return *this;
           }
           const Node *p = cur->parent;
           while (p && p->right == cur) {
               cur = p;
               p = p->parent;
           }
           node = p;
           return *this;
       }
       const_iterator operator--(int) {
           const_iterator tmp = *this;
           --(*this);
           return tmp;
       }
       const_iterator &operator--() {
           if (owner == nullptr) throw invalid_iterator();
           if (node == nullptr) {
               if (owner->root == nullptr) throw invalid_iterator();
               const Node *cur = owner->root;
               while (cur->right) cur = cur->right;
               node = cur;
               return *this;
           }
           const Node *cur = node;
           if (cur->left) {
               cur = cur->left;
               while (cur->right) cur = cur->right;
               node = cur;
               return *this;
           }
           const Node *p = cur->parent;
           while (p && p->left == cur) {
               cur = p;
               p = p->parent;
           }
           if (p == nullptr) throw invalid_iterator();
           node = p;
           return *this;
       }
       const value_type &operator*() const {
           if (owner == nullptr || node == nullptr) throw invalid_iterator();
           return node->kv;
       }
       const value_type *operator->() const noexcept {
           if (owner == nullptr || node == nullptr) throw invalid_iterator();
           return &node->kv;
       }
       bool operator==(const const_iterator &rhs) const { return node == rhs.node && owner == rhs.owner; }
       bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
       bool operator==(const iterator &rhs) const { return node == rhs.node && owner == rhs.owner; }
       bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
       friend class map;
   };

   map() : root(nullptr), node_count(0), comp(Compare()) {}
   map(const map &other) : root(nullptr), node_count(0), comp(other.comp) { copy_from(other.root); }
   map &operator=(const map &other) {
       if (this == &other) return *this;
       clear(); comp = other.comp; copy_from(other.root); return *this;
   }
   ~map() { clear(); }

   T &at(const Key &key) {
       Node *n = find_node(key);
       if (!n) throw index_out_of_bound();
       return const_cast<T &>(n->kv.second);
   }
   const T &at(const Key &key) const {
       const Node *n = find_node_const(key);
       if (!n) throw index_out_of_bound();
       return n->kv.second;
   }

   T &operator[](const Key &key) {
       Node *cur = root; Node *parent = nullptr;
       while (cur) {
           if (comp(key, cur->kv.first)) { parent = cur; cur = cur->left; }
           else if (comp(cur->kv.first, key)) { parent = cur; cur = cur->right; }
           else { return const_cast<T &>(cur->kv.second); }
       }
       value_type v(key, T());
       Node *nn = new Node(v, parent);
       if (!parent) root = nn; else if (comp(key, parent->kv.first)) parent->left = nn; else parent->right = nn;
       ++node_count; rebalance_from(parent); return const_cast<T &>(nn->kv.second);
   }
   const T &operator[](const Key &key) const {
       const Node *n = find_node_const(key);
       if (!n) throw index_out_of_bound();
       return n->kv.second;
   }

   iterator begin() {
       iterator it; it.owner = this;
       Node *cur = root; if (!cur) { it.node = nullptr; return it; }
       while (cur->left) cur = cur->left; it.node = cur; return it;
   }
   const_iterator cbegin() const {
       const_iterator it; it.owner = this;
       const Node *cur = root; if (!cur) { it.node = nullptr; return it; }
       while (cur->left) cur = cur->left; it.node = cur; return it;
   }
   iterator end() { iterator it; it.owner = this; it.node = nullptr; return it; }
   const_iterator cend() const { const_iterator it; it.owner = this; it.node = nullptr; return it; }

   bool empty() const { return node_count == 0; }
   size_t size() const { return node_count; }
   void clear() { destroy(root); root = nullptr; node_count = 0; }

   pair<iterator, bool> insert(const value_type &value) {
       iterator it; it.owner = this;
       if (!root) { root = new Node(value); ++node_count; it.node = root; return pair<iterator, bool>(it, true); }
       Node *cur = root; Node *parent = nullptr;
       while (cur) {
           parent = cur;
           if (comp(value.first, cur->kv.first)) cur = cur->left;
           else if (comp(cur->kv.first, value.first)) cur = cur->right;
           else { it.node = cur; return pair<iterator, bool>(it, false); }
       }
       Node *nn = new Node(value, parent);
       if (comp(value.first, parent->kv.first)) parent->left = nn; else parent->right = nn;
       ++node_count; rebalance_from(parent); it.node = nn; return pair<iterator, bool>(it, true);
   }

   void erase(iterator pos) {
       if (pos.owner != this || pos.node == nullptr) throw invalid_iterator();
       Node *z = pos.node; Node *rebeg = nullptr;
       if (!z->left) { rebeg = z->parent; transplant(z, z->right); }
       else if (!z->right) { rebeg = z->parent; transplant(z, z->left); }
       else {
           Node *y = z->right; while (y->left) y = y->left; Node *yp = y->parent;
           if (yp != z) { rebeg = yp; transplant(y, y->right); y->right = z->right; if (y->right) y->right->parent = y; }
           else { rebeg = y; }
           transplant(z, y); y->left = z->left; if (y->left) y->left->parent = y;
       }
       delete z; --node_count; rebalance_from(rebeg);
   }

   size_t count(const Key &key) const { return find_node_const(key) ? 1 : 0; }
   iterator find(const Key &key) { iterator it; it.owner = this; it.node = find_node(key); return it.node ? it : end(); }
   const_iterator find(const Key &key) const { const_iterator it; it.owner = this; it.node = find_node_const(key); return it.node ? it : cend(); }

  private:
   Node *root; size_t node_count; Compare comp;
   int h(Node *n) const { return n ? n->height : 0; }
   void update(Node *n) { if (n) { int hl = h(n->left), hr = h(n->right); n->height = (hl > hr ? hl : hr) + 1; } }
   int bf(Node *n) const { return n ? h(n->left) - h(n->right) : 0; }
   void rotate_left(Node *x) {
       Node *y = x->right; Node *B = y->left; y->left = x; x->right = B; if (B) B->parent = x;
       Node *p = x->parent; y->parent = p; x->parent = y; if (!p) root = y; else if (p->left == x) p->left = y; else p->right = y;
       update(x); update(y);
   }
   void rotate_right(Node *x) {
       Node *y = x->left; Node *B = y->right; y->right = x; x->left = B; if (B) B->parent = x;
       Node *p = x->parent; y->parent = p; x->parent = y; if (!p) root = y; else if (p->left == x) p->left = y; else p->right = y;
       update(x); update(y);
   }
   void rebalance(Node *n) {
       update(n);
       if (bf(n) == 2) { if (bf(n->left) < 0) rotate_left(n->left); rotate_right(n); }
       else if (bf(n) == -2) { if (bf(n->right) > 0) rotate_right(n->right); rotate_left(n); }
   }
   void rebalance_from(Node *n) { while (n) { rebalance(n); n = n->parent; } }
   void transplant(Node *u, Node *v) { Node *p = u->parent; if (!p) root = v; else if (p->left == u) p->left = v; else p->right = v; if (v) v->parent = p; if (p) update(p); }
   Node *find_node(const Key &key) const {
       Node *cur = root; while (cur) { if (comp(key, cur->kv.first)) cur = cur->left; else if (comp(cur->kv.first, key)) cur = cur->right; else return cur; } return nullptr;
   }
   const Node *find_node_const(const Key &key) const {
       const Node *cur = root; while (cur) { if (comp(key, cur->kv.first)) cur = cur->left; else if (comp(cur->kv.first, key)) cur = cur->right; else return cur; } return nullptr;
   }
   void destroy(Node *n) { if (!n) return; destroy(n->left); destroy(n->right); delete n; }
   void copy_from(const Node *n) { if (!n) return; insert(n->kv); copy_from(n->left); copy_from(n->right); }
   friend class const_iterator; friend class iterator;
 };

// cross-type comparisons definitions
template<class Key, class T, class Compare>
bool map<Key,T,Compare>::iterator::operator==(const typename map<Key,T,Compare>::const_iterator &rhs) const {
    return node == rhs.node && owner == rhs.owner;
}
template<class Key, class T, class Compare>
bool map<Key,T,Compare>::iterator::operator!=(const typename map<Key,T,Compare>::const_iterator &rhs) const {
    return !(*this == rhs);
}

}

#endif
