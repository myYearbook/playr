/*
 * *  C Interface: avltree.h
 * *
 * * Description:
 * *
 * *
 * * Author: Frederick F. Kautz IV <fkautz@myyearbook.com>, (C) 2008
 * *
 * * Copyright: See COPYING file that comes with this distribution
 * *
 * */


#ifndef AVLTREE_H_
#define AVLTREE_H_


typedef struct {
  void* key;
  void* value;
  void* left;
  void* right;
  void* parent;
  int size;
} AVL_NODE;

typedef struct {
  AVL_NODE* root;
  int (*cmp)(const void* key1, const void* key2);
} AVL_ROOT;

int avl_put(void* key, void* value, AVL_ROOT* tree);

void* avl_get(const void* key, AVL_ROOT* tree);

void* avl_remove();

int avl_init(AVL_ROOT* root, int (*cmp)(const void*, const void*));

void avl_destroy(AVL_ROOT* tree, int deep);

#endif // AVLTREE_H_
