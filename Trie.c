#include "myshell.h"

// 字典树数据结构， 当输入的命令不存在时， 用于搜索并列出相似命令



TrieNode * Create_Node(){
    TrieNode * NewNode = (TrieNode*)malloc(sizeof(TrieNode));
    NewNode->Letter = 0;
    NewNode->isWord = 0;
    NewNode->number = 0;
    int i;
    for(i=0; i<SIZE; i++){
        NewNode->Next[i] = NULL;
    }
    return NewNode;
}

int isWord(char * word){
    // 检查word是否为字典树的一个单词， 是则返回1， 否则返回2
    LinkList * pList = (LinkList*)Create_List();
    Trie_Search(Tree,pList,word);
    if( pList->number==0 ){                 // 搜索完链表还是空的则不存在
        return 0;
    }else{
        return 1;
    }
}

void Trie_Insert(WORD word, char * path){
    int i,j,len;
    char nextletter;

    TrieNode *CurNode = Tree;
/*---
    字典树字节点下标j的分布：0～25字母，26～35数字，36'-',37'.',38'_'(下划线)
*/
    len = strlen(word);
    for( i=0; i<len; i++){
        if( isalpha(word[i]) ){
            nextletter = word[i] | 0x20;            //大写转小写
            j = nextletter - 'a';
        }else if( isdigit(word[i]) ){
            j = word[i] - '0' + 26;
        }else if( word[i]=='-' ){
            j = 36;
        }else if( word[i]=='.' ){
            j = 37;
        }else if( word[i]=='_' ){
            j = 38;
        }else{
            // 名称不合法, 不存入字典树d
            return ;
        }
        if( CurNode->Next[j]==NULL ){
            CurNode->Next[j] = Create_Node();
            CurNode->Next[j]->Letter = nextletter;
        }
        if( i==len-1 && CurNode->Next[j]->number+1 < CONFLICT ){
            CurNode->Next[j]->isWord = 1;
            strcpy(CurNode->Next[j]->Word[CurNode->Next[j]->number],word);      //字符串赋值不能用=
            strcpy(CurNode->Next[j]->filepath,path);
            CurNode->Next[j]->number++;
        }
        CurNode = CurNode->Next[j];
    }
    return ;
}

void Trie_Search(TrieNode * Tree, LinkList *pList, char *Word){
    // 利用递归在生成的树中搜索
    TrieNode * Cur = Tree;
    int pWord = 0;
    int i,j,index;
    if( Tree==NULL ) return ;

    if( Word[pWord]=='\0' ){                        // 搜索到头了
        if( Tree->isWord ){
            for( i=0; i<Tree->number; i++){
                List_Add(pList,Tree->Word[i],Tree->filepath);
            }
        }
        return ;
    }else if( Word[pWord]=='*' ){                   // 可以是任意长度的字符串
        for( i=0; i<SIZE; i++){
            Trie_Search(Tree->Next[i], pList, Word);
            Trie_Search(Tree->Next[i], pList, (Word+1));
        }
    }else if( Word[pWord]=='?' ){                   // 不确定, 可以是任何字母
        for( i=0; i<SIZE; i++){
            Trie_Search(Tree->Next[i], pList, (Word+1));
        }
    }else if( Word[pWord]>='a' && Word[pWord] <='z' ){                  // 是字母,继续往下读
        index = Word[pWord] - 'a';
        Trie_Search(Tree->Next[index], pList, (Word+1));
    }
    return ;
}
