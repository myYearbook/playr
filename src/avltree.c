/*
 * *  C Implementation: avltree
 * *
 * * Description:
 * *
 * *
 * * Author: Frederick F. Kautz IV <fkautz@myyearbook.com>, (C) 2008
 * *
 * * Copyright: See COPYING file that comes with this distribution
 * *
 * */



#include "avltree.h"
#include "stdlib.h"
#include "stdio.h"
#define DEBUG

void avl_rr(AVL_NODE* node, AVL_ROOT* tree);
void avl_rl(AVL_NODE* node, AVL_ROOT* tree);
void avl_ll(AVL_NODE* node, AVL_ROOT* tree);
void avl_lr(AVL_NODE* node, AVL_ROOT* tree);
int avl_getheight(AVL_NODE* node);
int avl_getbalance(AVL_NODE* node);
void avl_setheight(AVL_NODE* node);
int avl_rotate(AVL_NODE* node, AVL_ROOT* tree);

int avl_put(void* key, void* value, AVL_ROOT* tree)
{
	AVL_NODE* newnode;
	AVL_NODE* curnode;
	int cmpresult;
	if(key == 0 || value == 0 || tree == 0)
		return -1;

	newnode = (AVL_NODE*)malloc(sizeof(AVL_NODE));
	if(newnode == 0)
		return -1;
	newnode->key = (void*)key;
	newnode->value = (void*)value;
	newnode->left = 0;
	newnode->right = 0;
	newnode->parent = 0;
	newnode->size = 1;

#ifdef DEBUG
	printf("Adding: %s\n", (char*)key);
#endif				// DEBUG

	if(tree->root == 0) {
		tree->root = newnode;
		return 0;
	}

	curnode = tree->root;

	while(1) { // breaks out when current insertion point is found
		newnode->parent = curnode;
		cmpresult = 0;
		cmpresult = tree->cmp(curnode->key, newnode->key);
#ifdef DEBUG
		printf("traversing: %p -> %s\n", (char*)curnode, 
(char*)curnode->value);
#endif				// DEBUG
		if(cmpresult == 0) {
			/* TODO fix memory leak:
			Implicitly only storage curnode->value (type 
void *) not
			released before assignment: curnode->value = 
(void *)value
			A memory leak has been detected. Only-qualified 
storage is not released
			before the last reference to it is lost. (Use 
-mustfreeonly to inhibit
			warning) -- splint output
			*/
			if(curnode->key == curnode->value)
				curnode->key = newnode->key;
			free(curnode->value);
			curnode->value = (void*)value;
			free(newnode);
			break;
		} else if(cmpresult>0) { // go to the left
			if(curnode->left == 0) {
				curnode->left = newnode;
				break;
			}
			curnode = curnode->left;
			continue;
		} else { //cmpresult < 0, go to the right
			if(curnode->right == 0) {
				curnode->right = newnode;
				break;
			}
			curnode = curnode->right;
			continue;
		}
	}


	while(curnode != 0) {
#ifdef DEBUG
		printf("checking %s\n", curnode->key);
#endif				// DEBUG
		avl_setheight(curnode);
		avl_rotate(curnode, tree);
		avl_setheight(curnode);
		curnode = curnode->parent;
	}

	return 0;
}


int avl_init(AVL_ROOT* tree, int (*cmp)(const void*, const void*)) {
	if(tree == 0 || cmp == 0) {
		return -1;
	}
	tree->root = 0;
	tree->cmp = cmp;
	return 0;
}

int avl_rotate(AVL_NODE* node, AVL_ROOT* tree) {
	int balance = 0;

	if(node == 0) {
#ifdef DEBUG
		printf("%s:%s:null pointer passed to avl_rotate\n",__FILE__,__LINE__);
#endif
		return -1;
	}



	balance = avl_getbalance(node);
#ifdef DEBUG
	printf("balance: %d\n", balance);
#endif				// DEBUG

	if(balance == -2) {
		if(avl_getbalance(node->right)==-1) {
#ifdef DEBUG
			printf("Calling avl_rr on %s\n", 
(char*)node->key);
#endif				// DEBUG
			avl_rr(node, tree);
		} else {
#ifdef DEBUG
			printf("Calling avl_rl on %s\n", 
(char*)node->key);
#endif				// DEBUG
			avl_rl(node, tree);
		}
	} else if(balance == 2) {
		if(avl_getbalance(node->left)==1) {
#ifdef DEBUG
			printf("Calling avl_ll on %s\n", 
(char*)node->key);
#endif				// DEBUG
			avl_ll(node, tree);
		} else {
#ifdef DEBUG
			printf("Calling avl_lr on %s\n", 
(char*)node->key);
#endif				// DEBUG
			avl_lr(node, tree);
		}
	}
	return 0;
}

void avl_setheight(AVL_NODE* node) {
	int left;
	int right;
	left = avl_getheight(node->left);
	right = avl_getheight(node->right);
	if(left>right) {
		node->size = left+1;
	} else {
		node->size = right+1;
	}
}

int avl_getbalance(AVL_NODE* node) {
	if(node == 0)
		return 0;
	return avl_getheight((AVL_NODE*)(node->left)) - 
avl_getheight((AVL_NODE*)(node->right));
}

int avl_getheight(AVL_NODE* node) {
	if(node == 0) {
		return 0;
	}
	return node->size;
}

void avl_rr(AVL_NODE* node, AVL_ROOT* tree)
{
	AVL_NODE* parent = 0;
	AVL_NODE* tmp = 0;
	if(node == 0 ||  node->right == 0 || 
((AVL_NODE*)node->right)->right == 0) {
#ifdef DEBUG
		printf("%s:%s: Tree is in a bad state\n", 
__FILE__,__LINE__);
#endif
		return;
	}
	parent = node->parent;
	tmp = node->right;
#ifdef DEBUG
	printf("parent: %p tmp: %p\n", parent, tmp);
#endif				// DEBUG


	node->right = tmp->left;
	if(((AVL_NODE*)node->right) != 0)
		((AVL_NODE*)node->right)->parent = node;
	tmp->parent = node->parent;

	// we now have two separate trees properly made
	// attach node to tmp->left
	tmp->left = node;
	node->parent = tmp;
	tmp->parent = parent;

	if(tmp->parent == 0)
		tree->root = tmp;
	else {
		if(parent->left == node)
			parent->left = tmp;
		else
			parent->right = tmp;
	}

	avl_setheight(node);
	avl_setheight(tmp);
#ifdef DEBUG
	printf("final(%s): %p %p\n", tmp->key, tmp->left, tmp->right);
#endif				// DEBUG
}
void avl_rl(AVL_NODE* node, AVL_ROOT* tree)
{
	if(node == 0 ||  node->right == 0 || 
((AVL_NODE*)node->right)->left == 0) {
		return;
	}
	avl_ll(node->right, tree);
	avl_rr(node, tree);
}
void avl_ll(AVL_NODE* node, AVL_ROOT* tree)
{
	AVL_NODE* parent = 0;
	AVL_NODE* tmp = 0;
	if(node == 0 ||  node->left == 0) {
#ifdef DEBUG
		printf("%s:%s: Tree is in a bad state\n", 
__FILE__,__LINE__);
#endif
		return;
	}
	parent = node->parent;
	tmp = node->left;
#ifdef DEBUG
	printf("parent: %p tmp: %p\n", parent, tmp);
#endif				// DEBUG


	node->left = tmp->right;
	if(((AVL_NODE*)node->left) != 0)
		((AVL_NODE*)node->left)->parent = node;
	tmp->parent = node->parent;

	// we now have two separate trees properly made
	// attach node to tmp->right
	tmp->right = node;
	node->parent = tmp;
	tmp->parent = parent;

	if(tmp->parent == 0)
		tree->root = tmp;
	else {
		if(parent->right == node)
			parent->right = tmp;
		else
			parent->left = tmp;
	}

	avl_setheight(node);
	avl_setheight(tmp);
#ifdef DEBUG
	printf("final(%s): %p %p\n", tmp->key, tmp->left, tmp->right);
#endif				// DEBUG
}

void avl_lr(AVL_NODE* node, AVL_ROOT* tree)
{
	if(node == 0 ||  node->left == 0 || 
((AVL_NODE*)node->left)->left == 0) {
		return;
	}
	avl_rr(node->left, tree);
	avl_ll(node, tree);
}

void* avl_get(const void* key, AVL_ROOT* tree) {
	AVL_NODE* node;
	node = tree->root;
	int cmpresult;
	
	while(node != 0) {
		cmpresult = tree->cmp(key, node->key);
		if(cmpresult == 0)
			return node->value;
		else if(cmpresult > 0)
			node = node->right;
		else
			node = node->left;
	}
	return 0;
}

