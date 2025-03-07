/*
   Copyright [2017-2019] [IBM Corporation]
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/



/*
  Authors:

  Copyright (C) 2016, Daniel G. Waddington <daniel.waddington@ibm.com>
*/

#ifndef __CORE_AVL_TREE_H__
#define __CORE_AVL_TREE_H__

#include <common/assert.h>
#include <common/exceptions.h>
#include <stdio.h>
#include <thread>
#include <vector>

namespace Core
{
/** Comparison types. */
enum cmp_t {
  MIN_CMP = -1,  //>! less than
  EQ_CMP = 0,    //>! equal to
  MAX_CMP = 1    //>! greater than
};

enum balance_t { LEFT_HEAVY = -1, BALANCED = 0, RIGHT_HEAVY = 1 };

enum height_effect_t { HEIGHT_NOCHANGE = 0, HEIGHT_CHANGE = 1 };

enum dir_t { LEFT = 0, RIGHT = 1 };

/** Type of search. */
enum search_t {
  REG_OVERLAP,
  REG_CONTAINMENT,
  REG_EMPTY,
  REG_MATCH,
};

enum traversal_order_t { LTREE, KEY, RTREE };

/**
 * A node in an AVL tree.
 */
template <class T>
class AVL_node {
  /* utility functions */
  inline static int node_min(int a, int b) { return (a < b) ? a : b; }

  inline static int node_max(int a, int b) { return (a > b) ? a : b; }

  /** Returns true if the tree is too heavy on the left side. */
  inline static int LEFT_IMBALANCE(short bal) { return (bal < LEFT_HEAVY); }

  /** Returns true if the tree is too heavy on the right side. */
  inline static int RIGHT_IMBALANCE(short bal) { return (bal > RIGHT_HEAVY); }

 public:
  AVL_node<T> *subtree[2];

  /**
   * The balance factor.
   * - if -1, left subtree is taller than right subtree;
   * - if  0, the height of two subtrees are equal;
   * - if  1, right subtree is taller than left subtree.
   */
  short bf;

 private:
  /**
   * Comparison operator which must be provisioned by the
   * superclass for the node
   *
   * @param n
   *
   * @return
   */
  virtual bool higher(AVL_node<T> *n) = 0;

  /**
   * The actual compare function
   *
   * If n.key < this.key return node_min_CMP
   * else if n.key > this.key return node_max_CMP
   * else return EQ_CMP;
   */
  cmp_t compare(AVL_node<T> *n) {
    assert(n);
    return (higher(n) ? MIN_CMP : n->higher(this) ? MAX_CMP : EQ_CMP);
  }

  /**
   *  The compare function.
   *  It merges the data comparision and successor search.
   *  I really don't like it. :(
   */
  cmp_t compare(AVL_node *n, cmp_t cmp) {
    switch (cmp) {
      case EQ_CMP:
        return (static_cast<T *>(this)->compare(static_cast<T *>(n)));
      case MIN_CMP:  // Find the minimal element in this tree
        return (subtree[LEFT] == nullptr) ? EQ_CMP : MIN_CMP;
      case MAX_CMP:  // Find the maximal element in this tree
        return (subtree[RIGHT] == nullptr) ? EQ_CMP : MAX_CMP;
    }
    throw Logic_exception("unexpected condition");
    return EQ_CMP;
  }

  /** Gets opposite direction.  */
  static dir_t opposite(dir_t dir) { return dir_t(1 - int(dir)); }

  /** Performs a single rotation (LL case or RR case). */
  static int rotate_once(AVL_node *&root, dir_t dir) {
    dir_t otherDir = opposite(dir);
    AVL_node *oldRoot = root;

    int heightChange =
        (root->subtree[otherDir]->bf == 0) ? HEIGHT_NOCHANGE : HEIGHT_CHANGE;

    root = oldRoot->subtree[otherDir];

    oldRoot->subtree[otherDir] = root->subtree[dir];
    root->subtree[dir] = oldRoot;

    oldRoot->bf = -((dir == LEFT) ? --(root->bf) : ++(root->bf));

    return heightChange;
  }

  /** Performs double rotation (RL case or LR case). */
  static int rotate_twice(AVL_node *&root, dir_t dir) {
    dir_t otherDir = opposite(dir);
    AVL_node *oldRoot = root;
    AVL_node *oldOtherDirSubtree = root->subtree[otherDir];

    // assign new root
    root = oldRoot->subtree[otherDir]->subtree[dir];

    oldRoot->subtree[otherDir] = root->subtree[dir];
    root->subtree[dir] = oldRoot;

    oldOtherDirSubtree->subtree[dir] = root->subtree[otherDir];
    root->subtree[otherDir] = oldOtherDirSubtree;

    root->subtree[LEFT]->bf = -node_max(root->bf, 0);
    root->subtree[RIGHT]->bf = -node_min(root->bf, 0);
    root->bf = 0;

    return HEIGHT_CHANGE;
  }

  /**
   * Rebalances a tree.
   * @param root the root of the tree.
   */
  static int rebalance(AVL_node *&root) {
    int heightChange = HEIGHT_NOCHANGE;

    if (root == nullptr) throw API_exception("root is nullptr");

    if (LEFT_IMBALANCE(root->bf)) {
      if (root->subtree[LEFT] == nullptr) throw Logic_exception("avl tree");

      if (root->subtree[LEFT]->bf == RIGHT_HEAVY) {
        heightChange = rotate_twice(root, RIGHT);
      }
      else {
        heightChange = rotate_once(root, RIGHT);
      }
    }
    else if (RIGHT_IMBALANCE(root->bf)) {
      if (root->subtree[RIGHT] == nullptr) throw Logic_exception("avl tree");

      if (root->subtree[RIGHT]->bf == LEFT_HEAVY) {
        heightChange = rotate_twice(root, LEFT);
      }
      else {
        heightChange = rotate_once(root, LEFT);
      }
    }
    return heightChange;
  }

  /**
   * Inserts a node into a tree.
   * @param n the node.
   * @param root the root of the tree.
   * @param change
   * @returns
   */
  static AVL_node *insert(AVL_node *n, AVL_node *&root, int &change) {
    if (root == nullptr) {
      root = n;
      change = HEIGHT_CHANGE;
      return nullptr;
    }

    int increase = 0;
    AVL_node *ret;

    cmp_t result = root->compare(n, EQ_CMP);
    dir_t dir = (result == MIN_CMP) ? LEFT : RIGHT;

    if (result != EQ_CMP) {
      ret = insert(n, root->subtree[dir], change);
      if (ret) {
        throw API_exception("equivalent node already existing in tree");
      }
      increase = result * change;
    }
    else {
      increase = HEIGHT_NOCHANGE; /* a node than compares equal is already in
                                     the tree */
      throw API_exception("inserting a conflicting region!");
      return root;
    }

    root->bf += increase;
    change = (increase && root->bf) ? (1 - rebalance(root)) : HEIGHT_NOCHANGE;
    return nullptr;
  }

  /**
   * Removes a node from a tree.
   * @param n the node.
   * @param root the root of the tree.
   * @param change
   * @param cmp
   * @returns
   */
  static AVL_node *remove(AVL_node *n, AVL_node *&root, int &change,
                          cmp_t cmp) {
    if (root == nullptr) {
      change = HEIGHT_NOCHANGE;
      return nullptr;
    }

    int decrease = 0;
    AVL_node *ret;

    cmp_t result = root->compare(n, cmp);
    dir_t dir = (result == MIN_CMP) ? LEFT : RIGHT;

    if (result != EQ_CMP) {
      ret = remove(n, root->subtree[dir], change, cmp);
      if (!ret) {
        throw API_exception("node cannot be found for removal!");
      }
      decrease = result * change;
    }
    else {
      ret = root;
      if ((root->subtree[LEFT] == nullptr) &&
          (root->subtree[RIGHT] == nullptr)) {
        root = nullptr;
        change = HEIGHT_CHANGE;
        return ret;
      }
      else if ((root->subtree[LEFT] == nullptr) ||
               (root->subtree[RIGHT] == nullptr)) {
        root = root->subtree[(root->subtree[RIGHT]) ? RIGHT : LEFT];
        change = HEIGHT_CHANGE;
        return ret;
      }
      else {
        root = remove(n, root->subtree[RIGHT], decrease, MIN_CMP);
        root->subtree[LEFT] = ret->subtree[LEFT];
        root->subtree[RIGHT] = ret->subtree[RIGHT];
        root->bf = ret->bf;
      }
    }
    root->bf -= decrease;
    if (decrease) {
      if (root->bf) {
        change = rebalance(root);
      }
      else {
        change = HEIGHT_CHANGE;
      }
    }
    else {
      change = HEIGHT_NOCHANGE;
    }
    return ret;
  }

  /** Returns the height of this node in its tree. */
  int height() const {
    int l = (subtree[LEFT]) ? subtree[LEFT]->height() : 0;
    int r = (subtree[RIGHT]) ? subtree[RIGHT]->height() : 0;
    return (1 + node_max(l, r));
  }

  /** Validates the position of this node. */
  bool validate() {
    bool valid = true;

    if (subtree[LEFT]) valid = valid && subtree[LEFT]->validate();
    if (subtree[RIGHT]) valid = valid && subtree[RIGHT]->validate();

    int l = (subtree[LEFT]) ? subtree[LEFT]->height() : 0;
    int r = (subtree[RIGHT]) ? subtree[RIGHT]->height() : 0;

    int diff = r - l;

    if (LEFT_IMBALANCE(diff) || RIGHT_IMBALANCE(diff)) valid = false;

    if (diff != bf) valid = false;

    if (subtree[LEFT] && subtree[LEFT]->compare(this) == MIN_CMP) valid = false;

    if (subtree[RIGHT] && subtree[RIGHT]->compare(this) == MAX_CMP)
      valid = false;

    return valid;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Public interface starts here!
  ///////////////////////////////////////////////////////////////////////////

 public:
  /** Constructor. */
  AVL_node() : bf(0) { subtree[LEFT] = subtree[RIGHT] = nullptr; }

  /**
   * Inserts a node into a tree.
   * @param n the node.
   * @param root the root of the tree.
   */
  static void insert(AVL_node *n, AVL_node *&root) {
    int change;
    insert(n, root, change);
  }

  /**
   * Removes a node from a tree.
   * @param n the node.
   * @param root the root of the tree.
   */
  static void remove(AVL_node *n, AVL_node *&root) {
    int change;
    remove(n, root, change, EQ_CMP);
  }

  /**
   * Validates a tree.
   * @param root the root of a tree.
   */
  static bool validate(AVL_node *const root) {
    if (!root)
      return true;
    else
      return root->validate();
  }

} __attribute__((packed));

/**
 * An AVL tree. Node allocation is outside of this class.  The memory location
 * for the root pointer is passed into the constructor
 */
template <class T>
class AVL_tree {
 public:
  /**
   * Constructor
   *
   * @param root Pointer to space for the root pointer (this may be persistent)
   */
  AVL_tree(AVL_node<T> **root) {
    if (root == nullptr)
      throw Constructor_exception("AVL_tree: requires root pointer space");

    _root = root;
  }

  virtual ~AVL_tree() {}

  /**
   * Apply function on nodes in a bottom-up traversal
   *
   * @param func Function to apply; param is pointer to value
   */
  void apply_topdown(std::function<void(void *, size_t)> func) {
#ifdef USE_RECURSION  // recursion breaks the stack for large trees
    __apply_topdown(*_root, 0, func);
#else
    std::vector<AVL_node<T> *> stack;
    stack.push_back(*_root);

    while (!stack.empty()) {
      AVL_node<T> *node = stack.back();
      stack.pop_back();

      AVL_node<T> *l = node->subtree[LEFT];
      if (l) stack.push_back(l);

      AVL_node<T> *r = node->subtree[RIGHT];
      if (r) stack.push_back(r);

      func(node, sizeof(AVL_node<T>));
    }

#endif
  }

  /** Returns true if the tree is empty; false otherwise. */
  bool is_empty() { return *_root == nullptr; }

  /**
   * Set the root node of the tree.  Useful for deserialization.
   *
   * @param r Pointer to root node.
   */
  void set_root(AVL_node<T> **r) { _root = r; }

  /**
   * Inserts a node into the tree.
   *
   * @param n the node.
   */
  void insert_node(AVL_node<T> *n) { AVL_node<T>::insert(n, *_root); }

  /**
   * Removes a node from the tree.
   *
   * @param n the node.
   */
  void remove_node(AVL_node<T> *n) { AVL_node<T>::remove(n, *_root); }

  /**
   * Checks whether or not the tree is valid.
   *
   * @return True if valid
   */
  bool validate() { return AVL_node<T>::validate(*_root); }

  /**
   * Get pointer to the root of the tree (debugging purposes)
   *
   * @return Pointer to the root of the tree (note this can change) through
   * rebalancing.
   */
  AVL_node<T> **root() const { return _root; }

  /**
   * Dump debugging information
   *
   * @param node
   * @param level
   */
  static void dump(AVL_node<T> *node, int level = 0) {
    if (node == nullptr) {
      printf("***EMPTY TREE***\n");
    }
    else {
      dump(RTREE, node, level);
      if (node->subtree[RIGHT] != nullptr) {
        dump(node->subtree[RIGHT], level + 1);
      }
      dump(KEY, node, level);
      if (node->subtree[LEFT] != nullptr) {
        dump(node->subtree[LEFT], level + 1);
      }
      dump(LTREE, node, level);
    }
  }

 protected:
  AVL_node<T> **_root; /**< Root of the tree.*/

 private:
  static void indent(int len) {
    for (int i = 0; i < len; i++) {
      printf("  ");
    }
  }

  static void dump(traversal_order_t order, AVL_node<T> *node, int level = 0) {
    int verbose = 1;
    unsigned len = (level * 5) + 1;

    if ((order == LTREE) && (node->subtree[LEFT] == nullptr)) {
      indent(len);
      printf("         **NULL**\n");
    }
    if (order == KEY) {
      indent(len);
      if (verbose) {
        static_cast<T *>(node)->dump();
        printf("@[%p] bf=%x left=%p right=%p level=%d\n", node,
               (unsigned) node->bf, (void *) node->subtree[LEFT],
               (void *) node->subtree[RIGHT], level);
      }
      else {
        static_cast<T *>(node)->dump();
        printf("\n");
      }
    }
    if ((order == RTREE) && (node->subtree[RIGHT] == nullptr)) {
      indent(len);
      printf("         **NULL**\n");
    }
  }

  static void __apply_topdown(AVL_node<T> *node, int level,
                              std::function<void(void *, size_t)> func) {
    if (node == nullptr) {
      return;
    }
    else {
      func(node, sizeof(AVL_node<T>));
      if (node->subtree[RIGHT] != nullptr) {
        __apply_topdown(node->subtree[RIGHT], level + 1, func);
      }
      if (node->subtree[LEFT] != nullptr) {
        __apply_topdown(node->subtree[LEFT], level + 1, func);
      }
    }
  }
};
}  // namespace Core

#endif  // __CORE_AVL_TREE_H__
