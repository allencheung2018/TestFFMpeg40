/*
 * LockFreeQueue.h
 *
 *  Created on: 2013-12-4
 *      Author: sheldon
 */

#ifndef LOCKFREEQUEUE_H_
#define LOCKFREEQUEUE_H_
#include <iostream>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
using namespace std;
template <class ElementT>
class LockFreeQueue
{
    private:
        ElementT ring_array_[100];
        char flags_array_[100]; // 标记位，标记某个位置的元素是否被占用
        // flags: 0：空节点；1：已被申请，正在写入 2：已经写入，可以弹出;3,正在弹出操作;
        int size_;  // 环形数组的大小
        int element_num_; //队列中元素的个数
        int head_index_;
        int tail_index_;
    public:
        LockFreeQueue(int s = 0)
        {
        	//printf("LockFreeQueue\n\r");
            size_ = s;
            head_index_ = 0;
            tail_index_ = 0;
            element_num_ = 0;
            //printf("LockFreeQueue\n\r");
        }
        ~LockFreeQueue()
        {

        }
    public:
        // 初始化queue。分配内存，设定size
        bool Init();
        void Reset()
        {
            head_index_ = 0;
            tail_index_ = 0;
            element_num_ = 0;
            memset(flags_array_, 0, size_);
        }
        const int GetSize(void) const
        {
            return size_;
        }
        const int GetElementNum(void) const
        {
            return element_num_;
        }
        // 入队函数
        bool EnQueue(const ElementT & ele);
        // 出队函数
        bool DeQueue(ElementT * ele);
};
template <class ElementT>
bool LockFreeQueue<ElementT>::Init()
{
     //printf("flags_array_1");
    //flags_array_ = new(std::nothrow)char[size_];
    //printf("flags_array_1");
    //memset(flags_array_ ,0,size_*sizeof(char));
     //printf("flags_array_2");
    //for(int index=0;index<size_;index++)
    	//printf("flags_array_======%d",flags_array_[index]);
    if (flags_array_ == NULL)
        return false;
        // printf("flags_array_3");
    memset(flags_array_, 0, size_);
     //printf("flags_array_4");
    //ring_array_= new ElementT[size_*sizeof(ElementT)];
    if (ring_array_ == NULL)
        return false;

    return true;
}
// ThreadSafe
// 元素入队尾部
template <class ElementT>
bool LockFreeQueue<ElementT>::EnQueue(const ElementT & ele)
{
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj\n\r");
    if (!(element_num_ < size_))
        return false;
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj1\n\r");
    int cur_tail_index = tail_index_;
    char * cur_tail_flag_index = flags_array_ + cur_tail_index;
    // 忙式等待
    // while中的原子操作：如果当前tail的标记为“”已占用(1)“，则更新cur_tail_flag_index,
    // 继续循环；否则，将tail标记设为已经占用
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj2\n\r");
    while (!__sync_bool_compare_and_swap(cur_tail_flag_index, 0, 1))
    {
        cur_tail_index = tail_index_;
        cur_tail_flag_index = flags_array_ +  cur_tail_index;
        printf("   while (!__sync_bool_compare_and_swap(cur_tail_flag_index, 0, 1))\n\r");
    }
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj3\n\r");
    // 两个入队线程之间的同步
    // 取模操作可以优化
    int update_tail_index = (cur_tail_index + 1) % size_;
    // 如果已经被其他的线程更新过，则不需要更新；
    // 否则，更新为 (cur_tail_index+1) % size_;
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj5\n\r");
    __sync_bool_compare_and_swap(&tail_index_, cur_tail_index, update_tail_index);
    // 申请到可用的存储空间
    *(ring_array_ + cur_tail_index) = ele;
    // 写入完毕
////printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj5\n\r");
    __sync_fetch_and_add(cur_tail_flag_index, 1);
    // 更新size;入队线程与出队线程之间的协作
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj6\n\r");
    __sync_fetch_and_add(&element_num_, 1);
//	printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj7\n\r");
    return true;
}
// ThreadSafe
// 元素出队头部
template <class ElementT>
bool LockFreeQueue<ElementT>::DeQueue(ElementT * ele)
{
    if (!(element_num_ > 0))
        return false;
//printf("  zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz0\n\r");
    int cur_head_index = head_index_;
    char * cur_head_flag_index = flags_array_ + cur_head_index;
//printf("  zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz\n\r");
    while (!__sync_bool_compare_and_swap(cur_head_flag_index, 2, 3))
     {
        cur_head_index = head_index_;
        cur_head_flag_index = flags_array_ + cur_head_index;
	printf("  (!__sync_bool_compare_and_swap(cur_head_flag_index, 2, 3))\n\r");
        
    }
//printf(" zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz1\n\r");
    // 取模操作可以优化
    int update_head_index = (cur_head_index + 1) % size_;
    __sync_bool_compare_and_swap(&head_index_, cur_head_index, update_head_index);
    *ele = *(ring_array_ + cur_head_index);
//printf(" zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz2\n\r");
    // 弹出完毕
    __sync_fetch_and_sub(cur_head_flag_index, 3);
//printf(" zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz3\n\r");
    // 更新size
    __sync_fetch_and_sub(&element_num_, 1);
//printf(" zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz4\n\r");
    return true;
}
template <class ElementT>
class LockFreeQueueH264
{
    private:
        ElementT ring_array_[200];
        char flags_array_[200]; // 标记位，标记某个位置的元素是否被占用
        // flags: 0：空节点；1：已被申请，正在写入 2：已经写入，可以弹出;3,正在弹出操作;
        int size_;  // 环形数组的大小
        int element_num_; //队列中元素的个数
        int head_index_;
        int tail_index_;
    public:
        LockFreeQueueH264(int s = 0)
        {
        	//printf("LockFreeQueue\n\r");
            size_ = s;
            head_index_ = 0;
            tail_index_ = 0;
            element_num_ = 0;
            //printf("LockFreeQueue\n\r");
        }
        ~LockFreeQueueH264()
        {

        }
    public:
        // 初始化queue。分配内存，设定size
        bool Init();
        void Reset()
        {
            head_index_ = 0;
            tail_index_ = 0;
            element_num_ = 0;
            memset(flags_array_, 0, size_);
        }
        const int GetSize(void) const
        {
            return size_;
        }
        const int GetElementNum(void) const
        {
            return element_num_;
        }
        // 入队函数
        bool EnQueue(const ElementT & ele);
        // 出队函数
        bool DeQueue(ElementT * ele);
};
template <class ElementT>
bool LockFreeQueueH264<ElementT>::Init()
{
     //printf("flags_array_1");
    //flags_array_ = new(std::nothrow)char[size_];
    //printf("flags_array_1");
    //memset(flags_array_ ,0,size_*sizeof(char));
     //printf("flags_array_2");
    //for(int index=0;index<size_;index++)
    	//printf("flags_array_======%d",flags_array_[index]);
    if (flags_array_ == NULL)
        return false;
        // printf("flags_array_3");
    memset(flags_array_, 0, size_);
     //printf("flags_array_4");
    //ring_array_= new ElementT[size_*sizeof(ElementT)];
    if (ring_array_ == NULL)
        return false;

    return true;
}
// ThreadSafe
// 元素入队尾部
template <class ElementT>
bool LockFreeQueueH264<ElementT>::EnQueue(const ElementT & ele)
{
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj\n\r");
    if (!(element_num_ < size_))
        return false;
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj1\n\r");
    int cur_tail_index = tail_index_;
    char * cur_tail_flag_index = flags_array_ + cur_tail_index;
    // 忙式等待
    // while中的原子操作：如果当前tail的标记为“”已占用(1)“，则更新cur_tail_flag_index,
    // 继续循环；否则，将tail标记设为已经占用
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj2\n\r");
    while (!__sync_bool_compare_and_swap(cur_tail_flag_index, 0, 1))
    {
        cur_tail_index = tail_index_;
        cur_tail_flag_index = flags_array_ +  cur_tail_index;
        printf("   while (!__sync_bool_compare_and_swap(cur_tail_flag_index, 0, 1))\n\r");
    }
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj3\n\r");
    // 两个入队线程之间的同步
    // 取模操作可以优化
    int update_tail_index = (cur_tail_index + 1) % size_;
    // 如果已经被其他的线程更新过，则不需要更新；
    // 否则，更新为 (cur_tail_index+1) % size_;
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj5\n\r");
    __sync_bool_compare_and_swap(&tail_index_, cur_tail_index, update_tail_index);
    // 申请到可用的存储空间
    *(ring_array_ + cur_tail_index) = ele;
    // 写入完毕
////printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj5\n\r");
    __sync_fetch_and_add(cur_tail_flag_index, 1);
    // 更新size;入队线程与出队线程之间的协作
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj6\n\r");
    __sync_fetch_and_add(&element_num_, 1);
//	printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj7\n\r");
    return true;
}
// ThreadSafe
// 元素出队头部
template <class ElementT>
bool LockFreeQueueH264<ElementT>::DeQueue(ElementT * ele)
{
    if (!(element_num_ > 0))
        return false;
//printf("  zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz0\n\r");
    int cur_head_index = head_index_;
    char * cur_head_flag_index = flags_array_ + cur_head_index;
//printf("  zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz\n\r");
    while (!__sync_bool_compare_and_swap(cur_head_flag_index, 2, 3))
     {
        cur_head_index = head_index_;
        cur_head_flag_index = flags_array_ + cur_head_index;
	printf("  (!__sync_bool_compare_and_swap(cur_head_flag_index, 2, 3))\n\r");
        
    }
//printf(" zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz1\n\r");
    // 取模操作可以优化
    int update_head_index = (cur_head_index + 1) % size_;
    __sync_bool_compare_and_swap(&head_index_, cur_head_index, update_head_index);
    *ele = *(ring_array_ + cur_head_index);
//printf(" zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz2\n\r");
    // 弹出完毕
    __sync_fetch_and_sub(cur_head_flag_index, 3);
//printf(" zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz3\n\r");
    // 更新size
    __sync_fetch_and_sub(&element_num_, 1);
//printf(" zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz4\n\r");
    return true;
}


template <class ElementT>
class LockFreeQueueAlarmEvent
{
    private:
        ElementT ring_array_[5];
        char flags_array_[5]; // 标记位，标记某个位置的元素是否被占用
        // flags: 0：空节点；1：已被申请，正在写入 2：已经写入，可以弹出;3,正在弹出操作;
        int size_;  // 环形数组的大小
        int element_num_; //队列中元素的个数
        int head_index_;
        int tail_index_;
    public:
        LockFreeQueueAlarmEvent(int s = 0)
        {
        	//printf("LockFreeQueue\n\r");
            size_ = s;
            head_index_ = 0;
            tail_index_ = 0;
            element_num_ = 0;
            //printf("LockFreeQueue\n\r");
        }
        ~LockFreeQueueAlarmEvent()
        {

        }
    public:
        // 初始化queue。分配内存，设定size
        bool Init();
        void Reset()
        {
            head_index_ = 0;
            tail_index_ = 0;
            element_num_ = 0;
            memset(flags_array_, 0, size_);
        }
        const int GetSize(void) const
        {
            return size_;
        }
        const int GetElementNum(void) const
        {
            return element_num_;
        }
        // 入队函数
        bool EnQueue(const ElementT & ele);
        // 出队函数
        bool DeQueue(ElementT * ele);
};
template <class ElementT>
bool LockFreeQueueAlarmEvent<ElementT>::Init()
{
     //printf("flags_array_1");
    //flags_array_ = new(std::nothrow)char[size_];
    //printf("flags_array_1");
    //memset(flags_array_ ,0,size_*sizeof(char));
     //printf("flags_array_2");
    //for(int index=0;index<size_;index++)
    	//printf("flags_array_======%d",flags_array_[index]);
    if (flags_array_ == NULL)
        return false;
        // printf("flags_array_3");
    memset(flags_array_, 0, size_);
     //printf("flags_array_4");
    //ring_array_= new ElementT[size_*sizeof(ElementT)];
    if (ring_array_ == NULL)
        return false;

    return true;
}
// ThreadSafe
// 元素入队尾部
template <class ElementT>
bool LockFreeQueueAlarmEvent<ElementT>::EnQueue(const ElementT & ele)
{
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj\n\r");
    if (!(element_num_ < size_))
        return false;
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj1\n\r");
    int cur_tail_index = tail_index_;
    char * cur_tail_flag_index = flags_array_ + cur_tail_index;
    // 忙式等待
    // while中的原子操作：如果当前tail的标记为“”已占用(1)“，则更新cur_tail_flag_index,
    // 继续循环；否则，将tail标记设为已经占用
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj2\n\r");
    while (!__sync_bool_compare_and_swap(cur_tail_flag_index, 0, 1))
    {
        cur_tail_index = tail_index_;
        cur_tail_flag_index = flags_array_ +  cur_tail_index;
        printf("   while (!__sync_bool_compare_and_swap(cur_tail_flag_index, 0, 1))\n\r");
    }
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj3\n\r");
    // 两个入队线程之间的同步
    // 取模操作可以优化
    int update_tail_index = (cur_tail_index + 1) % size_;
    // 如果已经被其他的线程更新过，则不需要更新；
    // 否则，更新为 (cur_tail_index+1) % size_;
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj5\n\r");
    __sync_bool_compare_and_swap(&tail_index_, cur_tail_index, update_tail_index);
    // 申请到可用的存储空间
    *(ring_array_ + cur_tail_index) = ele;
    // 写入完毕
////printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj5\n\r");
    __sync_fetch_and_add(cur_tail_flag_index, 1);
    // 更新size;入队线程与出队线程之间的协作
//printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj6\n\r");
    __sync_fetch_and_add(&element_num_, 1);
//	printf("  jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj7\n\r");
    return true;
}
// ThreadSafe
// 元素出队头部
template <class ElementT>
bool LockFreeQueueAlarmEvent<ElementT>::DeQueue(ElementT * ele)
{
    if (!(element_num_ > 0))
        return false;
//printf("  zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz0\n\r");
    int cur_head_index = head_index_;
    char * cur_head_flag_index = flags_array_ + cur_head_index;
//printf("  zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz\n\r");
    while (!__sync_bool_compare_and_swap(cur_head_flag_index, 2, 3))
     {
        cur_head_index = head_index_;
        cur_head_flag_index = flags_array_ + cur_head_index;
	printf("  (!__sync_bool_compare_and_swap(cur_head_flag_index, 2, 3))\n\r");
        
    }
//printf(" zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz1\n\r");
    // 取模操作可以优化
    int update_head_index = (cur_head_index + 1) % size_;
    __sync_bool_compare_and_swap(&head_index_, cur_head_index, update_head_index);
    *ele = *(ring_array_ + cur_head_index);
//printf(" zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz2\n\r");
    // 弹出完毕
    __sync_fetch_and_sub(cur_head_flag_index, 3);
//printf(" zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz3\n\r");
    // 更新size
    __sync_fetch_and_sub(&element_num_, 1);
//printf(" zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz4\n\r");
    return true;
}
#endif /* LOCKFREEQUEUE_H_ */
