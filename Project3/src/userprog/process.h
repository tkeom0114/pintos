#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
//struct mmap_file// table of file mappings에 사용됨
//{
	//struct hash_elem mmap_elem;//hash element로 사용
	//struct list page_list;//해당 파일이 저장된 page들의 list
	//int mapid;//mmap을 통해 할당받은 mapid
	//struct file *file;//mapping한 file을 저장
//};
struct mmap_file {
    struct page *page;
    int mapid;
    struct list_elem elem;
};

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
bool install_page (void *upage, void *kpage, bool writable);
struct file* process_get_file(int fd);
int process_add_file(struct file *f);
bool process_add_mmap(struct page *page);
void process_remove_mmap(int mapid);

#endif /* userprog/process.h */
