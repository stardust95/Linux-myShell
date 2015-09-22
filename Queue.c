#include<stdio.h>
#include<stdlib.h>
#include "myshell.h"

Queue * CreateQueue(){
	Queue * PtrQ = (Queue*)malloc(sizeof(Queue));
    memset(PtrQ->Data,0,sizeof(QueueElem)*PROCESS_MAX);
    PtrQ->front = PtrQ->rear = -1;
    return PtrQ;
}

int IsEmptyQ(Queue * PtrQ){
	int flag = 0;						//默认非空
	if( PtrQ->front==PtrQ->rear ){
		flag = 1;
	}
	return flag;
}

void AddQ(Queue * PtrQ,QueueElem item){				//这里的S也是空的头结点
	PtrQ->Data[++(PtrQ->rear)] = item;
	PtrQ->size++;
	return ;
}

QueueElem DeleteQ(Queue * PtrQ){
    PtrQ->size--;
	return PtrQ->Data[++(PtrQ->front)];
}

