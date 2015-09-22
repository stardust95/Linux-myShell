#include "myshell.h"

LinkList * Create_List(){
    LinkList* newList = (LinkList*)malloc(sizeof(LinkList));
    newList->head = newList->tail = NULL;
    newList->number = 0;
    return newList;
}

void List_Add(LinkList * pList, ListElem word,char *path){
    ListNode * newNode = (ListNode*)malloc(sizeof(ListNode));
    strcpy(newNode->entry,word);
    strcpy(newNode->filepath,path);
    newNode->next = NULL;
    if( pList->head==NULL ){                    // 链表为空
        pList->head = pList->tail = newNode;
        pList->number = 1;
    }else{                                      // 链表非空
        pList->tail->next = newNode;
        pList->tail = pList->tail->next;
        pList->number++;
    }
    return;
}

