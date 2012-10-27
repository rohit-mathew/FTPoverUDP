#include "unprtt.h"
#include<stdio.h>
#include<stdlib.h>

struct node{
	struct msghdr *data;
	struct node *next;
	struct node *prev;
};
/* Create a double linked list of window size*/ 
struct node *makelist(int winsize){
	struct node *start = (struct node *)malloc(sizeof(struct node));
	struct node *samp1 = (struct node *)malloc(sizeof(struct node));
	start->prev = NULL;
	start->next = samp1;
	samp1->prev = start;
	while(winsize>0){
		struct node *samp2 = (struct node *)malloc(sizeof(struct node));
		samp1->next = samp2;
		samp2->prev = samp1;
		samp1 = samp2;
		winsize--;
	}
	samp1->next = NULL;
	return start;
}
/* Add a node to the end of the list*/
struct node *addlist(struct node *firstnode, int counter){
	struct node *samp2 = (struct node *)malloc(sizeof(struct node *));
	//struct node *samp3 = (struct node *)malloc(sizeof(struct node *));
	samp2 = firstnode->next; /* Need to modify. BUG */
	while(samp2->next != NULL){
		samp2 = samp2->next;
	}
	while(counter>0){
		struct node *samp1 = (struct node *)malloc(sizeof(struct node *));
		samp2->next = samp1;
		samp1->prev = samp2;
		samp2 = samp1;
		counter--;
	}	
	samp2->next = NULL;
	return samp2;	
}
/* Remove nodes from the begining of the list*/
struct node *removenode(struct node *firstnode, int counter){
	struct node *samp1 = (struct node *)malloc(sizeof(struct node *));
	struct node *samp2 = (struct node *)malloc(sizeof(struct node *));
	samp1 = firstnode->next;
	while(counter-1>0){
		samp1 = samp1->next;
		counter--;
	}
	samp2 = samp1->next;
	samp1->next = samp2->next;
	samp2 = samp2->next;
	samp2->prev = samp1;
	return samp2;
}
	
	
	

	
