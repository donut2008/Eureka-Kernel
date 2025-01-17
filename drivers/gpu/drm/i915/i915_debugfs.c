#ifndef BTRFS_EXPORT_H
#define BTRFS_EXPORT_H

#include <linux/exportfs.h>

extern const struct export_operations btrfs_export_ops;

struct btrfs_fid {
	u64 objectid;
	u64 root_objectid;
	u32 gen;

	u64 parent_objectid;
	u32 parent_gen;

	u64 parent_root_objectid;
} __attribute__ ((packed));

struct dentry *btrfs_get_dentry(struct super_block *sb, u64 objectid,
				u64 root_objectid, u64 generation,
				int check_generation);
struct dentry *btrfs_get_parent(struct dentry *child);

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   #ifndef __EXTENTIO__
#define __EXTENTIO__

#include <linux/rbtree.h>
#include "ulist.h"

/* bits for the extent state */
#define EXTENT_DIRTY		(1U << 0)
#define EXTENT_WRITEBACK	(1U << 1)
#define EXTENT_UPTODATE		(1U << 2)
#define EXTENT_LOCKED		(1U << 3)
#define EXTENT_NEW		(1U << 4)
#define EXTENT_DELALLOC		(1U << 5)
#define EXTENT_DEFRAG		(1U << 6)
#define EXTENT_BOUNDARY		(1U << 9)
#define EXTENT_NODATASUM	(1U << 10)
#define EXTENT_DO_ACCOUNTING	(1U << 11)
#define EXTENT_FIRST_DELALLOC	(1U << 12)
#define EXTENT_NEED_WAIT	(1U << 13)
#define EXTENT_DAMAGED		(1U << 14)
#define EXTENT_NORESERVE	(1U << 15)
#define EXTENT_QGROUP_RESERVED	(1U << 16)
#define EXTENT_IOBITS		(EXTENT_LOCKED | EXTENT_WRITEBACK)
#define EXTENT_CTLBITS		(EXTENT_DO_ACCOUNTING | EXTENT_FIRST_DELALLOC)

/*
 * flags for bio submission. The high bits indicate the compression
 * type for this bio
 */
#define EXTENT_BIO_COMPRESSED 1
#define EXTENT_BIO_TREE_LOG 2
#define EXTENT_BIO_PARENT_LOCKED 4
#define EXTENT_BIO_FLAG_SHIFT 16

/* these are bit numbers for test/set bit */
#define EXTENT_BUFFER_UPTODATE 0
#define EXTENT_BUFFER_DIRTY 2
#define EXTENT_BUFFER_CORRUPT 3
#define EXTENT_BUFFER_READAHEAD 4	/* this got triggered by readahead */
#define EXTENT_BUFFER_TREE_REF 5
#define EXTENT_BUFFER_STALE 6
#define EXTENT_BUFFER_WRITEBACK 7
#define EXTENT_BUFFER_READ_ERR 8        /* read IO error */
#define EXTENT_BUFFER_DUMMY 9
#define EXTENT_BUFFER_IN_TREE 10
#define EXTENT_BUFFER_WRITE_ERR 11    /* write IO error */

/* these are flags for extent_clear_unlock_delalloc */
#define PAGE_UNLOCK		(1 << 0)
#define PAGE_CLEAR_DIRTY	(1 << 1)
#define PAGE_SET_WRITEBACK	(1 << 2)
#define PAGE_END_WRITEBACK	(1 << 3)
#define PAGE_SET_PRIVATE2	(1 << 4)
#define PAGE_SET_ERROR		(1 << 5)

/*
 * page->private values.  Every page that is controlled by the extent
 * map has page->private set to one.
 */
#define EXTENT_PAGE_PRIVATE 1

struct extent_state;
struct btrfs_root;
struct btrfs_io_bio;

typedef	int (extent_submit_bio_hook_t)(struct inode *inode, int rw,
				       struct bio *bio, int mirror_num,
				       unsigned long bio_flags, u64 bio_offset);
struct extent_io_ops {
	int (*fill_delalloc)(struct inode *inode, struct page *locked_page,
			     u64 start, u64 end, int *page_started,
			     unsigned long *nr_written);
	int (*writepage_start_hook)(struct page *page, u64 start, u64 end);
	int (*writepage_io_hook)(struct page *page, u64 start, u64 end);
	extent_submit_bio_hook_t *submit_bio_hook;
	int (*merge_bio_hook)(int rw, struct page *page, unsigned long offset,
			      size_t size, struct bio *bio,
			      unsigned long bio_flags);
	int (*readpage_io_failed_hook)(struct page *page, int failed_mirror);
	int (*readpage_end_io_hook)(struct btrfs_io_bio *io_bio, u64 phy_offset,
				    struct page *page, u64 start, u64 end,
				    int mirror);
	int (*writepage_end_io_hook)(struct page *page, u64 start, u64 end,
				      struct extent_state *state, int uptodate);
	void (*set_bit_hook)(struct inode *inode, struct extent_state *state,
			     unsigned *bits);
	void (*clear_bit_hook)(struct inode *inode, struct extent_state *state,
			       unsigned *bits);
	void (*merge_extent_hook)(struct inode *inode,
				  struct extent_state *new,
				  struct extent_state *other);
	void (*split_extent_hook)(struct inode *inode,
				  struct extent_state *orig, u64 split);
};

struct extent_io_tree {
	struct rb_root state;
	struct address_space *mapping;
	u64 dirty_bytes;
	int track_uptodate;
	spinlock_t lock;
	const struct extent_io_ops *ops;
};

struct extent_state {
	u64 start;
	u64 end; /* inclusive */
	struct rb_node rb_node;

	/* ADD NEW ELEMENTS AFTER THIS */
	wait_queue_head_t wq;
	atomic_t refs;
	unsigned state;

	/* for use by the FS */
	u64 private;

#ifdef CONFIG_BTRFS_DEBUG
	struct list_head leak_list;
#endif
};

#define INLINE_EXTENT_BUFFER_PAGES 16
#define MAX_INLINE_EXTENT_BUFFER_SIZE (INLINE_EXTENT_BUFFER_PAGES * PAGE_CACHE_SIZE)
struct extent_buffer {
	u64 start;
	unsigned long len;
	unsigned long bflags;
	struct btrfs_fs_info *fs_info;
	spinlock_t refs_lock;
	atomic_t refs;
	atomic_t io_pages;
	int read_mirror;
	struct rcu_head rcu_head;
	pid_t lock_owner;

	/* count of read lock holders on the extent buffer */
	atomic_t write_locks;
	atomic_t read_locks;
	atomic_t blocking_writers;
	atomic_t blocking_readers;
	atomic_t spinning_readers;
	atomic_t spinning_writers;
	short lock_nested;
	/* >= 0 if eb belongs to a log tree, -1 otherwise */
	short log_index;

	/* protects write locks */
	rwlock_t lock;

	/* readers use lock_wq while they wait for the write
	 * lock holders to unlock
	 */
	wait_queue_head_t write_lock_wq;

	/* writers use read_lock_wq while they wait for readers
	 * to unlock
	 */
	wait_queue_head_t read_lock_wq;
	struct page *pages[INLINE_EXTENT_BUFFER_PAGES];
#ifdef CONFIG_BTRFS_DEBUG
	struct list_head leak_list;
#endif
};

/*
 * Structure to record how many bytes and which ranges are set/cleared
 */
struct extent_changeset {
	/* How many bytes are set/cleared in this operation */
	u64 bytes_changed;

	/* Changed ranges */
	struct ulist *range_changed;
};

static inline void extent_set_compress_type(unsigned long *bio_flags,
					    int compress_type)
{
	*bio_flags |= compress_type << EXTENT_BIO_FLAG_SHIFT;
}

static inline int extent_compress_type(unsigned long bio_flags)
{
	return bio_flags >> EXTENT_BIO_FLAG_SHIFT;
}

struct extent_map_tree;

typedef struct extent_map *(get_extent_t)(struct inode *inode,
					  struct page *page,
					  size_t pg_offset,
					  u64 start, u64 len,
					  int create);

void extent_io_tree_init(struct extent_io_tree *tree,
			 struct address_space *mapping);
int try_release_extent_mapping(struct extent_map_tree *map,
			       struct extent_io_tree *tree, struct page *page,
			       gfp_t mask);
int try_release_extent_buffer(struct page *page);
int lock_extent(struct extent_io_tree *tree, u64 start, u64 end);
int lock_extent_bits(struct extent_io_tree *tree, u64 start, u64 end,
		     unsigned bits, struct extent_state **cached);
int unlock_extent(struct extent_io_tree *tree, u64 start, u64 end);
int unlock_extent_cached(struct extent_io_tree *tree, u64 start, u64 end,
			 struct extent_state **cached, gfp_t mask);
int try_lock_extent(struct extent_io_tree *tree, u64 start, u64 end);
int extent_read_full_page(struct extent_io_tree *tree, struct page *page,
			  get_extent_t *get_extent, int mirror_num);
int extent_read_full_page_nolock(struct extent_io_tree *tree, struct page *page,
				 get_extent_t *get_extent, int mirror_num);
int __init extent_io_init(void);
void extent_io_exit(void);

u64 count_range_bits(struct extent_io_tree *tree,
		     u64 *start, u64 search_end,
		     u64 max_bytes, unsigned bits, int contig);

void free_extent_state(struct extent_state *state);
int test_range_bit(struct extent_io_tree *tree, u64 start, u64 end,
		   unsigned bits, int filled,
		   struct extent_state *cached_state);
int clear_extent_bits(struct extent_io_tree *tree, u64 start, u64 end,
		      unsigned bits, gfp_t mask);
int clear_record_extent_bits(struct extent_io_tree *tree, u64 start, u64 end,
			     unsigned bits, gfp_t mask,
			     struct extent_changeset *changeset);
int clear_extent_bit(struct extent_io_tree *tree, u64 start, u64 end,
		     unsigned bits, int wake, int delete,
		     struct extent_state **cached, gfp_t mask);
int set_extent_bits(struct extent_io_tree *tree, u64 start, u64 end,
		    unsigned bits, gfp_t mask);
int set_record_extent_bits(struct extent_io_tree *tree, u64 start, u64 end,
			   unsigned bits, gfp_t mask,
			   struct extent_changeset *changeset);
int set_extent_bit(struct extent_io_tree *tree, u64 start, u64 end,
		   unsigned bits, u64 *failed_start,
		   struct extent_state **cached_state, gfp_t mask);
int set_extent_uptodate(struct extent_io_tree *tree, u64 start, u64 end,
			struct extent_state **cached_state, gfp_t mask);
int clear_extent_uptodate(struct extent_io_tree *tree, u64 start, u64 end,
			  struct extent_state **cached_state, gfp_t mask);
int set_extent_new(struct extent_io_tree *tree, u64 start, u64 end,
		   gfp_t mask);
int set_extent_dirty(struct extent_io_tree *tree, u64 start, u64 end,
		     gfp_t mask);
int clear_extent_dirty(struct extent_io_tree *tree, u64 start, u64 end,
		       gfp_t mask);
int convert_extent_bit(struct extent_io_tree *tree, u64 start, u64 end,
		       unsigned bits, unsigned clear_bits,
		       struct extent_state **cached_state, gfp_t mask);
int set_extent_delalloc(struct extent_io_tree *tree, u64 start, u64 end,
			struct extent_state **cached_state, gfp_t mask);
int set_extent_defrag(struct extent_io_tree *tree, u64 start, u64 end,
		      struct extent_state **cached_state, gfp_t mask);
int find_first_extent_bit(struct extent_io_tree *tree, u64 start,
			  u64 *start_ret, u64 *end_ret, unsigned bits,
			  struct extent_state **cached_state);
int extent_invalidatepage(struct extent_io_tree *tree,
			  struct page *page, unsigned long offset);
int extent_write_full_page(struct extent_io_tree *tree, struct page *page,
			  get_extent_t *get_extent,
			  struct writeback_control *wbc);
int extent_write_locked_range(struct extent_io_tree *tree, struct inode *inode,
			      u64 start, u64 end, get_extent_t *get_extent,
			      int mode);
int extent_writepages(struct extent_io_tree *tree,
		      struct address_space *mapping,
		      get_extent_t *get_extent,
		      struct writeback_control *wbc);
int btree_write_cache_pages(struct address_space *mapping,
			    struct writeback_control *wbc);
int extent_readpages(struct extent_io_tree *tree,
		     struct address_space *mapping,
		     struct list_head *pages, unsigned nr_pages,
		     get_extent_t get_extent);
int extent_fiemap(struct inode *inode, struct fiemap_extent_info *fieinfo,
		__u64 start, __u64 len, get_extent_t *get_extent);
int get_state_private(struct extent_io_tree *tree, u64 start, u64 *private);
void set_page_extent_mapped(struct page *page);

struct extent_buffer *alloc_extent_buffer(struct btrfs_fs_info *fs_info,
					  u64 start);
struct extent_buffer *alloc_dummy_extent_buffer(struct btrfs_fs_info *fs_info,
		u64 start);
struct extent_buffer *btrfs_clone_extent_buffer(struct extent_buffer *src);
struct extent_buffer *find_extent_buffer(struct btrfs_fs_info *fs_info,
					 u64 start);
void free_extent_buffer(struct extent_buffer *eb);
void free_extent_buffer_stale(struct extent_buffer *eb);
#define WAIT_NONE	0
#define WAIT_COMPLETE	1
#define WAIT_PAGE_LOCK	2
int read_extent_buffer_pages(struct extent_io_tree *tree,
			     struct extent_buffer *eb, u64 start, int wait,
			     get_extent_t *get_extent, int mirror_num);
void wait_on_extent_buffer_writeback(struct extent_buffer *eb);

static inline unsigned long num_extent_pages(u64 start, u64 len)
{
	return ((start + len + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT) -
		(start >> PAGE_CACHE_SHIFT);
}

static inline void extent_buffer_get(struct extent_buffer *eb)
{
	atomic_inc(&eb->refs);
}

int memcmp_extent_buffer(const struct extent_buffer *eb, const void *ptrv,
			 unsigned long start, unsigned long len);
void read_extent_buffer(const struct extent_buffer *eb, void *dst,
			unsigned long start,
			unsigned long len);
int read_extent_buffer_to_user_nofault(const struct extent_buffer *eb,
				       void __user *dst, unsigned long start,
				       unsigned long len);
void write_extent_buffer(struct extent_buffer *eb, const void *src,
			 unsigned long start, unsigned long len);
void copy_extent_buffer(struct extent_buffer *dst, struct extent_buffer *src,
			unsigned long dst_offset, unsigned long src_offset,
			unsigned long len);
void memcpy_extent_buffer(struct extent_buffer *dst, unsigned long dst_offset,
			   unsigned long src_offset, unsigned long len);
void memmove_extent_buffer(struct extent_buffer *dst, unsigned long dst_offset,
			   unsigned long src_offset, unsigned long len);
void memset_extent_buffer(struct extent_buffer *eb, char c,
			  unsigned long start, unsigned long len);
void clear_extent_buffer_dirty(struct extent_buffer *eb);
int set_extent_buffer_dirty(struct extent_buffer *eb);
int set_extent_buffer_uptodate(struct extent_buffer *eb);
int clear_extent_buffer_uptodate(struct extent_buffer *eb);
int extent_buffer_uptodate(struct extent_buffer *eb);
int extent_buffer_under_io(struct extent_buffer *eb);
int map_private_extent_buffer(const struct extent_buffer *eb,
			      unsigned long offset, unsigned long min_len,
			      char **map, unsigned long *map_start,
			      unsigned long *map_len);
int extent_range_clear_dirty_for_io(struct inode *inode, u64 start, u64 end);
int extent_range_redirty_for_io(struct inode *inode, u64 start, u64 end);
int extent_clear_unlock_delalloc(struct inode *inode, u64 start, u64 end,
				 struct page *locked_page,
				 unsigned bits_to_clear,
				 unsigned long page_ops);
struct bio *
btrfs_bio_alloc(struct block_device *bdev, u64 first_sector, int nr_vecs,
		gfp_t gfp_flags);
struct bio *btrfs_io_bio_alloc(gfp_t gfp_mask, unsigned int nr_iovecs);
struct bio *btrfs_bio_clone(struct bio *bio, gfp_t gfp_mask);

struct btrfs_fs_info;

int repair_io_failure(struct inode *inode, u64 start, u64 length, u64 logical,
		      struct page *page, unsigned int pg_offset,
		      int mirror_num);
int clean_io_failure(struct inode *inode, u64 start, struct page *page,
		     unsigned int pg_offset);
int end_extent_writepage(struct page *page, int err, u64 start, u64 end);
int repair_eb_io_failure(struct btrfs_root *root, struct extent_buffer *eb,
			 int mirror_num);

/*
 * When IO fails, either with EIO or csum verification fails, we
 * try other mirrors that might have a good copy of the data.  This
 * io_failure_record is used to record state as we go through all the
 * mirrors.  If another mirror has good data, the page is set up to date
 * and things continue.  If a good mirror can't be found, the original
 * bio end_io callback is called to indicate things have failed.
 */
struct io_failure_record {
	struct page *page;
	u64 start;
	u64 len;
	u64 logical;
	unsigned long bio_flags;
	int this_mirror;
	int failed_mirror;
	int in_validation;
};

void btrfs_free_io_failure_record(struct inode *inode, u64 start, u64 end);
int btrfs_get_io_failure_record(struct inode *inode, u64 start, u64 end,
				struct io_failure_record **failrec_ret);
int btrfs_check_repairable(struct inode *inode, struct bio *failed_bio,
			   struct io_failure_record *failrec, int fail_mirror);
struct bio *btrfs_create_repair_bio(struct inode *inode, struct bio *failed_bio,
				    struct io_failure_record *failrec,
				    struct page *page, int pg_offset, int icsum,
				    bio_end_io_t *endio_func, void *data);
int free_io_failure(struct inode *inode, struct io_failure_record *rec);
#ifdef CONFIG_BTRFS_FS_RUN_SANITY_TESTS
noinline u64 find_lock_delalloc_range(struct inode *inode,
				      struct extent_io_tree *tree,
				      struct page *locked_page, u64 *start,
				      u64 *end, u64 max_bytes);
#endif
struct extent_buffer *alloc_test_extent_buffer(struct btrfs_fs_info *fs_info,
					       u64 start);
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               #include <linux/err.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/hardirq.h>
#include "ctree.h"
#include "extent_map.h"


static struct kmem_cache *extent_map_cache;

int __init extent_map_init(void)
{
	extent_map_cache = kmem_cache_create("btrfs_extent_map",
			sizeof(struct extent_map), 0,
			SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD, NULL);
	if (!extent_map_cache)
		return -ENOMEM;
	return 0;
}

void extent_map_exit(void)
{
	if (extent_map_cache)
		kmem_cache_destroy(extent_map_cache);
}

/**
 * extent_map_tree_init - initialize extent map tree
 * @tree:		tree to initialize
 *
 * Initialize the extent tree @tree.  Should be called for each new inode
 * or other user of the extent_map interface.
 */
void extent_map_tree_init(struct extent_map_tree *tree)
{
	tree->map = RB_ROOT;
	INIT_LIST_HEAD(&tree->modified_extents);
	rwlock_init(&tree->lock);
}

/**
 * alloc_extent_map - allocate new extent map structure
 *
 * Allocate a new extent_map structure.  The new structure is
 * returned with a reference count of one and needs to be
 * freed using free_extent_map()
 */
struct extent_map *alloc_extent_map(void)
{
	struct extent_map *em;
	em = kmem_cache_zalloc(extent_map_cache, GFP_NOFS);
	if (!em)
		return NULL;
	RB_CLEAR_NODE(&em->rb_node);
	em->flags = 0;
	em->compress_type = BTRFS_COMPRESS_NONE;
	em->generation = 0;
	atomic_set(&em->refs, 1);
	INIT_LIST_HEAD(&em->list);
	return em;
}

/**
 * free_extent_map - drop reference count of an extent_map
 * @em:		extent map beeing releasead
 *
 * Drops the reference out on @em by one and free the structure
 * if the reference count hits zero.
 */
void free_extent_map(struct extent_map *em)
{
	if (!em)
		return;
	WARN_ON(atomic_read(&em->refs) == 0);
	if (atomic_dec_and_test(&em->refs)) {
		WARN_ON(extent_map_in_tree(em));
		WARN_ON(!list_empty(&em->list));
		if (test_bit(EXTENT_FLAG_FS_MAPPING, &em->flags))
			kfree(em->map_lookup);
		kmem_cache_free(extent_map_cache, em);
	}
}

/* simple helper to do math around the end of an extent, handling wrap */
static u64 range_end(u64 start, u64 len)
{
	if (start + len < start)
		return (u64)-1;
	return start + len;
}

static int tree_insert(struct rb_root *root, struct extent_map *em)
{
	struct rb_node **p = &root->rb_node;
	struct rb_node *parent = NULL;
	struct extent_map *entry = NULL;
	struct rb_node *orig_parent = NULL;
	u64 end = range_end(em->start, em->len);

	while (*p) {
		parent = *p;
		entry = rb_entry(parent, struct extent_map, rb_node);

		if (em->start < entry->start)
			p = &(*p)->rb_left;
		else if (em->start >= extent_map_end(entry))
			p = &(*p)->rb_right;
		else
			return -EEXIST;
	}

	orig_parent = parent;
	while (parent && em->start >= extent_map_end(entry)) {
		parent = rb_next(parent);
		entry = rb_entry(parent, struct extent_map, rb_node);
	}
	if (parent)
		if (end > entry->start && em->start < extent_map_end(entry))
			return -EEXIST;

	parent = orig_parent;
	entry = rb_entry(parent, struct extent_map, rb_node);
	while (parent && em->start < entry->start) {
		parent = rb_prev(parent);
		entry = rb_entry(parent, struct extent_map, rb_node);
	}
	if (parent)
		if (end > entry->start && em->start < extent_map_end(entry))
			return -EEXIST;

	rb_link_node(&em->rb_node, orig_parent, p);
	rb_insert_color(&em->rb_node, root);
	return 0;
}

/*
 * search through the tree for an extent_map with a given offset.  If
 * it can't be found, try to find some neighboring extents
 */
static struct rb_node *__tree_search(struct rb_root *root, u64 offset,
				     struct rb_node **prev_ret,
				     struct rb_node **next_ret)
{
	struct rb_node *n = root->rb_node;
	struct rb_node *prev = NULL;
	struct rb_node *orig_prev = NULL;
	struct extent_map *entry;
	struct extent_map *prev_entry = NULL;

	while (n) {
		entry = rb_entry(n, struct extent_map, rb_node);
		prev = n;
		prev_entry = entry;

		if (offset < entry->start)
			n = n->rb_left;
		else if (offset >= extent_map_end(entry))
			n = n->rb_right;
		else
			return n;
	}

	if (prev_ret) {
		orig_prev = prev;
		while (prev && offset >= extent_map_end(prev_entry)) {
			prev = rb_next(prev);
			prev_entry = rb_entry(prev, struct extent_map, rb_node);
		}
		*prev_ret = prev;
		prev = orig_prev;
	}

	if (next_ret) {
		prev_entry = rb_entry(prev, struct extent_map, rb_node);
		while (prev && offset < prev_entry->start) {
			prev = rb_prev(prev);
			prev_entry = rb_entry(prev, struct extent_map, rb_node);
		}
		*next_ret = prev;
	}
	return NULL;
}

/* check to see if two extent_map structs are adjacent and safe to merge */
static int mergable_maps(struct extent_map *prev, struct extent_map *next)
{
	if (test_bit(EXTENT_FLAG_PINNED, &prev->flags))
		return 0;

	/*
	 * don't merge compressed extents, we need to know their
	 * actual size
	 */
	if (test_bit(EXTENT_FLAG_COMPRESSED, &prev->flags))
		return 0;

	if (test_bit(EXTENT_FLAG_LOGGING, &prev->flags) ||
	    test_bit(EXTENT_FLAG_LOGGING, &next->flags))
		return 0;

	/*
	 * We don't want to merge stuff that hasn't been written to the log yet
	 * since it may not reflect exactly what is on disk, and that would be
	 * bad.
	 */
	if (!list_empty(&prev->list) || !list_empty(&next->list))
		return 0;

	if (extent_map_end(prev) == next->start &&
	    prev->flags == next->flags &&
	    prev->bdev == next->bdev &&
	    ((next->block_start == EXTENT_MAP_HOLE &&
	      prev->block_start == EXTENT_MAP_HOLE) ||
	     (next->block_start == EXTENT_MAP_INLINE &&
	      prev->block_start == EXTENT_MAP_INLINE) ||
	     (next->block_start == EXTENT_MAP_DELALLOC &&
	      prev->block_start == EXTENT_MAP_DELALLOC) ||
	     (next->block_start < EXTENT_MAP_LAST_BYTE - 1 &&
	      next->block_start == extent_map_block_end(prev)))) {
		return 1;
	}
	return 0;
}

static void try_merge_map(struct extent_map_tree *tree, struct extent_map *em)
{
	struct extent_map *merge = NULL;
	struct rb_node *rb;

	/*
	 * We can't modify an extent map that is in the tree and that is being
	 * used by another task, as it can cause that other task to see it in
	 * inconsistent state during the merging. We always have 1 reference for
	 * the tree and 1 for this task (which is unpinning the extent map or
	 * clearing the logging flag), so anything > 2 means it's being used by
	 * other tasks too.
	 */
	if (atomic_read(&em->refs) > 2)
		return;

	if (em->start != 0) {
		rb = rb_prev(&em->rb_node);
		if (rb)
			merge = rb_entry(rb, struct extent_map, rb_node);
		if (rb && mergable_maps(merge, em)) {
			em->start = merge->start;
			em->orig_start = merge->orig_start;
			em->len += merge->len;
			em->block_len += merge->block_len;
			em->block_start = merge->block_start;
			em->mod_len = (em->mod_len + em->mod_start) - merge->mod_start;
			em->mod_start = merge->mod_start;
			em->generation = max(em->generation, merge->generation);

			rb_erase(&merge->rb_node, &tree->map);
			RB_CLEAR_NODE(&merge->rb_node);
			free_extent_map(merge);
		}
	}

	rb = rb_next(&em->rb_node);
	if (rb)
		merge = rb_entry(rb, struct extent_map, rb_node);
	if (rb && mergable_maps(em, merge)) {
		em->len += merge->len;
		em->block_len += merge->block_len;
		rb_erase(&merge->rb_node, &tree->map);
		RB_CLEAR_NODE(&merge->rb_node);
		em->mod_len = (merge->mod_start + merge->mod_len) - em->mod_start;
		em->generation = max(em->generation, merge->generation);
		free_extent_map(merge);
	}
}

/**
 * unpin_extent_cache - unpin an extent from the cache
 * @tree:	tree to unpin the extent in
 * @start:	logical offset in the file
 * @len:	length of the extent
 * @gen:	generation that this extent has been modified in
 *
 * Called after an extent has been written to disk properly.  Set the generation
 * to the generation that actually added the file item to the inode so we know
 * we need to sync this extent when we call fsync().
 */
int unpin_extent_cache(struct extent_map_tree *tree, u64 start, u64 len,
		       u64 gen)
{
	int ret = 0;
	struct extent_map *em;
	bool prealloc = false;

	write_lock(&tree->lock);
	em = lookup_extent_mapping(tree, start, len);

	WARN_ON(!em || em->start != start);

	if (!em)
		goto out;

	em->generation = gen;
	clear_bit(EXTENT_FLAG_PINNED, &em->flags);
	em->mod_start = em->start;
	em->mod_len = em->len;

	if (test_bit(EXTENT_FLAG_FILLING, &em->flags)) {
		prealloc = true;
		clear_bit(EXTENT_FLAG_FILLING, &em->flags);
	}

	try_merge_map(tree, em);

	if (prealloc) {
		em->mod_start = em->start;
		em->mod_len = em->len;
	}

	free_extent_map(em);
out:
	write_unlock(&tree->lock);
	return ret;

}

void clear_em_logging(struct extent_map_tree *tree, struct extent_map *em)
{
	clear_bit(EXTENT_FLAG_LOGGING, &em->flags);
	if (extent_map_in_tree(em))
		try_merge_map(tree, em);
}

static inline void setup_extent_mapping(struct extent_map_tree *tree,
					struct extent_map *em,
					int modified)
{
	atomic_inc(&em->refs);
	em->mod_start = em->start;
	em->mod_len = em->len;

	if (modified)
		list_move(&em->list, &tree->modified_extents);
	else
		try_merge_map(tree, em);
}

/**
 * add_extent_mapping - add new extent map to the extent tree
 * @tree:	tree to insert new map in
 * @em:		map to insert
 *
 * Insert @em into @tree or perform a simple forward/backward merge with
 * existing mappings.  The extent_map struct passed in will be inserted
 * into the tree directly, with an additional reference taken, or a
 * reference dropped if the merge attempt was successful.
 */
int add_extent_mapping(struct extent_map_tree *tree,
		       struct extent_map *em, int modified)
{
	int ret = 0;

	ret = tree_insert(&tree->map, em);
	if (ret)
		goto out;

	setup_extent_mapping(tree, em, modified);
out:
	return ret;
}

static struct extent_map *
__lookup_extent_mapping(struct extent_map_tree *tree,
			u64 start, u64 len, int strict)
{
	struct extent_map *em;
	struct rb_node *rb_node;
	struct rb_node *prev = NULL;
	struct rb_node *next = NULL;
	u64 end = range_end(start, len);

	rb_node = __tree_search(&tree->map, start, &prev, &next);
	if (!rb_node) {
		if (prev)
			rb_node = prev;
		else if (next)
			rb_node = next;
		else
			return NULL;
	}

	em = rb_entry(rb_node, struct extent_map, rb_node);

	if (strict && !(end > em->start && start < extent_map_end(em)))
		return NULL;

	atomic_inc(&em->refs);
	return em;
}

/**
 * lookup_extent_mapping - lookup extent_map
 * @tree:	tree to lookup in
 * @start:	byte offset to start the search
 * @len:	length of the lookup range
 *
 * Find and return the first extent_map struct in @tree that intersects the
 * [start, len] range.  There may be additional objects in the tree that
 * intersect, so check the object returned carefully to make sure that no
 * additional lookups are needed.
 */
struct extent_map *lookup_extent_mapping(struct extent_map_tree *tree,
					 u64 start, u64 len)
{
	return __lookup_extent_mapping(tree, start, len, 1);
}

/**
 * search_extent_mapping - find a nearby extent map
 * @tree:	tree to lookup in
 * @start:	byte offset to start the search
 * @len:	length of the lookup range
 *
 * Find and return the first extent_map struct in @tree that intersects the
 * [start, len] range.
 *
 * If one can't be found, any nearby extent may be returned
 */
struct extent_map *search_extent_mapping(struct extent_map_tree *tree,
					 u64 start, u64 len)
{
	return __lookup_extent_mapping(tree, start, len, 0);
}

/**
 * remove_extent_mapping - removes an extent_map from the extent tree
 * @tree:	extent tree to remove from
 * @em:		extent map beeing removed
 *
 * Removes @em from @tree.  No reference counts are dropped, and no checks
 * are done to see if the range is in use
 */
int remove_extent_mapping(struct extent_map_tree *tree, struct extent_map *em)
{
	int ret = 0;

	WARN_ON(test_bit(EXTENT_FLAG_PINNED, &em->flags));
	rb_erase(&em->rb_node, &tree->map);
	if (!test_bit(EXTENT_FLAG_LOGGING, &em->flags))
		list_del_init(&em->list);
	RB_CLEAR_NODE(&em->rb_node);
	return ret;
}

void replace_extent_mapping(struct extent_map_tree *tree,
			    struct extent_map *cur,
			    struct extent_map *new,
			    int modified)
{
	WARN_ON(test_bit(EXTENT_FLAG_PINNED, &cur->flags));
	ASSERT(extent_map_in_tree(cur));
	if (!test_bit(EXTENT_FLAG_LOGGING, &cur->flags))
		list_del_init(&cur->list);
	rb_replace_node(&cur->rb_node, &new->rb_node, &tree->map);
	RB_CLEAR_NODE(&cur->rb_node);

	setup_extent_mapping(tree, new, modified);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                #ifndef __EXTENTMAP__
#define __EXTENTMAP__

#include <linux/rbtree.h>

#define EXTENT_MAP_LAST_BYTE ((u64)-4)
#define EXTENT_MAP_HOLE ((u64)-3)
#define EXTENT_MAP_INLINE ((u64)-2)
#define EXTENT_MAP_DELALLOC ((u64)-1)

/* bits for the flags field */
#define EXTENT_FLAG_PINNED 0 /* this entry not yet on disk, don't free it */
#define EXTENT_FLAG_COMPRESSED 1
#define EXTENT_FLAG_VACANCY 2 /* no file extent item found */
#define EXTENT_FLAG_PREALLOC 3 /* pre-allocated extent */
#define EXTENT_FLAG_LOGGING 4 /* Logging this extent */
#define EXTENT_FLAG_FILLING 5 /* Filling in a preallocated extent */
#define EXTENT_FLAG_FS_MAPPING 6 /* filesystem extent mapping type */

struct extent_map {
	struct rb_node rb_node;

	/* all of these are in bytes */
	u64 start;
	u64 len;
	u64 mod_start;
	u64 mod_len;
	u64 orig_start;
	u64 orig_block_len;
	u64 ram_bytes;
	u64 block_start;
	u64 block_len;
	u64 generation;
	unsigned long flags;
	union {
		struct block_device *bdev;

		/*
		 * used for chunk mappings
		 * flags & EXTENT_FLAG_FS_MAPPING must be set
		 */
		struct map_lookup *map_lookup;
	};
	atomic_t refs;
	unsigned int compress_type;
	struct list_head list;
};

struct extent_map_tree {
	struct rb_root map;
	struct list_head modified_extents;
	rwlock_t lock;
};

static inline int extent_map_in_tree(const struct extent_map *em)
{
	return !RB_EMPTY_NODE(&em->rb_node);
}

static inline u64 extent_map_end(struct extent_map *em)
{
	if (em->start + em->len < em->start)
		return (u64)-1;
	return em->start + em->len;
}

static inline u64 extent_map_block_end(struct extent_map *em)
{
	if (em->block_start + em->block_len < em->block_start)
		return (u64)-1;
	return em->block_start + em->block_len;
}

void extent_map_tree_init(struct extent_map_tree *tree);
struct extent_map *lookup_extent_mapping(struct extent_map_tree *tree,
					 u64 start, u64 len);
int add_extent_mapping(struct extent_map_tree *tree,
		       struct extent_map *em, int modified);
int remove_extent_mapping(struct extent_map_tree *tree, struct extent_map *em);
void replace_extent_mapping(struct extent_map_tree *tree,
			    struct extent_map *cur,
			    struct extent_map *new,
			    int modified);

struct extent_map *alloc_extent_map(void);
void free_extent_map(struct extent_map *em);
int __init extent_map_init(void);
void extent_map_exit(void);
int unpin_extent_cache(struct extent_map_tree *tree, u64 start, u64 len, u64 gen);
void clear_em_logging(struct extent_map_tree *tree, struct extent_map *em);
struct extent_map *search_extent_mapping(struct extent_map_tree *tree,
					 u64 start, u64 len);
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <linux/bio.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include "ctree.h"
#include "disk-io.h"
#include "transaction.h"
#include "volumes.h"
#include "print-tree.h"

#define __MAX_CSUM_ITEMS(r, size) ((unsigned long)(((BTRFS_LEAF_DATA_SIZE(r) - \
				   sizeof(struct btrfs_item) * 2) / \
				  size) - 1))

#define MAX_CSUM_ITEMS(r, size) (min_t(u32, __MAX_CSUM_ITEMS(r, size), \
				       PAGE_CACHE_SIZE))

#define MAX_ORDERED_SUM_BYTES(r) ((PAGE_SIZE - \
				   sizeof(struct btrfs_ordered_sum)) / \
				   sizeof(u32) * (r)->sectorsize)

int btrfs_insert_file_extent(struct btrfs_trans_handle *trans,
			     struct btrfs_root *root,
			     u64 objectid, u64 pos,
			     u64 disk_offset, u64 disk_num_bytes,
			     u64 num_bytes, u64 offset, u64 ram_bytes,
			     u8 compression, u8 encryption, u16 other_encoding)
{
	int ret = 0;
	struct btrfs_file_extent_item *item;
	struct btrfs_key file_key;
	struct btrfs_path *path;
	struct extent_buffer *leaf;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;
	file_key.objectid = objectid;
	file_key.offset = pos;
	file_key.type = BTRFS_EXTENT_DATA_KEY;

	path->leave_spinning = 1;
	ret = btrfs_insert_empty_item(trans, root, path, &file_key,
				      sizeof(*item));
	if (ret < 0)
		goto out;
	BUG_ON(ret); /* Can't happen */
	leaf = path->nodes[0];
	item = btrfs_item_ptr(leaf, path->slots[0],
			      struct btrfs_file_extent_item);
	btrfs_set_file_extent_disk_bytenr(leaf, item, disk_offset);
	btrfs_set_file_extent_disk_num_bytes(leaf, item, disk_num_bytes);
	btrfs_set_file_extent_offset(leaf, item, offset);
	btrfs_set_file_extent_num_bytes(leaf, item, num_bytes);
	btrfs_set_file_extent_ram_bytes(leaf, item, ram_bytes);
	btrfs_set_file_extent_generation(leaf, item, trans->transid);
	btrfs_set_file_extent_type(leaf, item, BTRFS_FILE_EXTENT_REG);
	btrfs_set_file_extent_compression(leaf, item, compression);
	btrfs_set_file_extent_encryption(leaf, item, encryption);
	btrfs_set_file_extent_other_encoding(leaf, item, other_encoding);

	btrfs_mark_buffer_dirty(leaf);
out:
	btrfs_free_path(path);
	return ret;
}

static struct btrfs_csum_item *
btrfs_lookup_csum(struct btrfs_trans_handle *trans,
		  struct btrfs_root *root,
		  struct btrfs_path *path,
		  u64 bytenr, int cow)
{
	int ret;
	struct btrfs_key file_key;
	struct btrfs_key found_key;
	struct btrfs_csum_item *item;
	struct extent_buffer *leaf;
	u64 csum_offset = 0;
	u16 csum_size = btrfs_super_csum_size(root->fs_info->super_copy);
	int csums_in_item;

	file_key.objectid = BTRFS_EXTENT_CSUM_OBJECTID;
	file_key.offset = bytenr;
	file_key.type = BTRFS_EXTENT_CSUM_KEY;
	ret = btrfs_search_slot(trans, root, &file_key, path, 0, cow);
	if (ret < 0)
		goto fail;
	leaf = path->nodes[0];
	if (ret > 0) {
		ret = 1;
		if (path->slots[0] == 0)
			goto fail;
		path->slots[0]--;
		btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
		if (found_key.type != BTRFS_EXTENT_CSUM_KEY)
			goto fail;

		csum_offset = (bytenr - found_key.offset) >>
				root->fs_info->sb->s_blocksize_bits;
		csums_in_item = btrfs_item_size_nr(leaf, path->slots[0]);
		csums_in_item /= csum_size;

		if (csum_offset == csums_in_item) {
			ret = -EFBIG;
			goto fail;
		} else if (csum_offset > csums_in_item) {
			goto fail;
		}
	}
	item = btrfs_item_ptr(leaf, path->slots[0], struct btrfs_csum_item);
	item = (struct btrfs_csum_item *)((unsigned char *)item +
					  csum_offset * csum_size);
	return item;
fail:
	if (ret > 0)
		ret = -ENOENT;
	return ERR_PTR(ret);
}

int btrfs_lookup_file_extent(struct btrfs_trans_handle *trans,
			     struct btrfs_root *root,
			     struct btrfs_path *path, u64 objectid,
			     u64 offset, int mod)
{
	int ret;
	struct btrfs_key file_key;
	int ins_len = mod < 0 ? -1 : 0;
	int cow = mod != 0;

	file_key.objectid = objectid;
	file_key.offset = offset;
	file_key.type = BTRFS_EXTENT_DATA_KEY;
	ret = btrfs_search_slot(trans, root, &file_key, path, ins_len, cow);
	return ret;
}

static void btrfs_io_bio_endio_readpage(struct btrfs_io_bio *bio, int err)
{
	kfree(bio->csum_allocated);
}

static int __btrfs_lookup_bio_sums(struct btrfs_root *root,
				   struct inode *inode, struct bio *bio,
				   u64 logical_offset, u32 *dst, int dio)
{
	struct bio_vec *bvec = bio->bi_io_vec;
	struct btrfs_io_bio *btrfs_bio = btrfs_io_bio(bio);
	struct btrfs_csum_item *item = NULL;
	struct extent_io_tree *io_tree = &BTRFS_I(inode)->io_tree;
	struct btrfs_path *path;
	u8 *csum;
	u64 offset = 0;
	u64 item_start_offset = 0;
	u64 item_last_offset = 0;
	u64 disk_bytenr;
	u32 diff;
	int nblocks;
	int bio_index = 0;
	int count;
	u16 csum_size = btrfs_super_csum_size(root->fs_info->super_copy);

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	nblocks = bio->bi_iter.bi_size >> inode->i_sb->s_blocksize_bits;
	if (!dst) {
		if (nblocks * csum_size > BTRFS_BIO_INLINE_CSUM_SIZE) {
			btrfs_bio->csum_allocated = kmalloc_array(nblocks,
					csum_size, GFP_NOFS);
			if (!btrfs_bio->csum_allocated) {
				btrfs_free_path(path);
				return -ENOMEM;
			}
			btrfs_bio->csum = btrfs_bio->csum_allocated;
			btrfs_bio->end_io = btrfs_io_bio_endio_readpage;
		} else {
			btrfs_bio->csum = btrfs_bio->csum_inline;
		}
		csum = btrfs_bio->csum;
	} else {
		csum = (u8 *)dst;
	}

	if (bio->bi_iter.bi_size > PAGE_CACHE_SIZE * 8)
		path->reada = 2;

	WARN_ON(bio->bi_vcnt <= 0);

	/*
	 * the free space stuff is only read when it hasn't been
	 * updated in the current transaction.  So, we can safely
	 * read from the commit root and sidestep a nasty deadlock
	 * between reading the free space cache and updating the csum tree.
	 */
	if (btrfs_is_free_space_inode(inode)) {
		path->search_commit_root = 1;
		path->skip_locking = 1;
	}

	disk_bytenr = (u64)bio->bi_iter.bi_sector << 9;
	if (dio)
		offset = logical_offset;
	while (bio_index < bio->bi_vcnt) {
		if (!dio)
			offset = page_offset(bvec->bv_page) + bvec->bv_offset;
		count = btrfs_find_ordered_sum(inode, offset, disk_bytenr,
					       (u32 *)csum, nblocks);
		if (count)
			goto found;

		if (!item || disk_bytenr < item_start_offset ||
		    disk_bytenr >= item_last_offset) {
			struct btrfs_key found_key;
			u32 item_size;

			if (item)
				btrfs_release_path(path);
			item = btrfs_lookup_csum(NULL, root->fs_info->csum_root,
						 path, disk_bytenr, 0);
			if (IS_ERR(item)) {
				count = 1;
				memset(csum, 0, csum_size);
				if (BTRFS_I(inode)->root->root_key.objectid ==
				    BTRFS_DATA_RELOC_TREE_OBJECTID) {
					set_extent_bits(io_tree, offset,
						offset + bvec->bv_len - 1,
						EXTENT_NODATASUM, GFP_NOFS);
				} else {
					btrfs_info(BTRFS_I(inode)->root->fs_info,
						   "no csum found for inode %llu start %llu",
					       btrfs_ino(inode), offset);
				}
				item = NULL;
				btrfs_release_path(path);
				goto found;
			}
			btrfs_item_key_to_cpu(path->nodes[0], &found_key,
					      path->slots[0]);

			item_start_offset = found_key.offset;
			item_size = btrfs_item_size_nr(path->nodes[0],
						       path->slots[0]);
			item_last_offset = item_start_offset +
				(item_size / csum_size) *
				root->sectorsize;
			item = btrfs_item_ptr(path->nodes[0], path->slots[0],
					      struct btrfs_csum_item);
		}
		/*
		 * this byte range must be able to fit inside
		 * a single leaf so it will also fit inside a u32
		 */
		diff = disk_bytenr - item_start_offset;
		diff = diff / root->sectorsize;
		diff = diff * csum_size;
		count = min_t(int, nblocks, (item_last_offset - disk_bytenr) >>
					    inode->i_sb->s_blocksize_bits);
		read_extent_buffer(path->nodes[0], csum,
				   ((unsigned long)item) + diff,
				   csum_size * count);
found:
		csum += count * csum_size;
		nblocks -= count;
		bio_index += count;
		while (count--) {
			disk_bytenr += bvec->bv_len;
			offset += bvec->bv_len;
			bvec++;
		}
	}
	btrfs_free_path(path);
	return 0;
}

int btrfs_lookup_bio_sums(struct btrfs_root *root, struct inode *inode,
			  struct bio *bio, u32 *dst)
{
	return __btrfs_lookup_bio_sums(root, inode, bio, 0, dst, 0);
}

int btrfs_lookup_bio_sums_dio(struct btrfs_root *root, struct inode *inode,
			      struct bio *bio, u64 offset)
{
	return __btrfs_lookup_bio_sums(root, inode, bio, offset, NULL, 1);
}

int btrfs_lookup_csums_range(struct btrfs_root *root, u64 start, u64 end,
			     struct list_head *list, int search_commit)
{
	struct btrfs_key key;
	struct btrfs_path *path;
	struct extent_buffer *leaf;
	struct btrfs_ordered_sum *sums;
	struct btrfs_csum_item *item;
	LIST_HEAD(tmplist);
	unsigned long offset;
	int ret;
	size_t size;
	u64 csum_end;
	u16 csum_size = btrfs_super_csum_size(root->fs_info->super_copy);

	ASSERT(IS_ALIGNED(start, root->sectorsize) &&
	       IS_ALIGNED(end + 1, root->sectorsize));

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	if (search_commit) {
		path->skip_locking = 1;
		path->reada = 2;
		path->search_commit_root = 1;
	}

	key.objectid = BTRFS_EXTENT_CSUM_OBJECTID;
	key.offset = start;
	key.type = BTRFS_EXTENT_CSUM_KEY;

	ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
	if (ret < 0)
		goto fail;
	if (ret > 0 && path->slots[0] > 0) {
		leaf = path->nodes[0];
		btrfs_item_key_to_cpu(leaf, &key, path->slots[0] - 1);
		if (key.objectid == BTRFS_EXTENT_CSUM_OBJECTID &&
		    key.type == BTRFS_EXTENT_CSUM_KEY) {
			offset = (start - key.offset) >>
				 root->fs_info->sb->s_blocksize_bits;
			if (offset * csum_size <
			    btrfs_item_size_nr(leaf, path->slots[0] - 1))
				path->slots[0]--;
		}
	}

	while (start <= end) {
		leaf = path->nodes[0];
		if (path->slots[0] >= btrfs_header_nritems(leaf)) {
			ret = btrfs_next_leaf(root, path);
			if (ret < 0)
				goto fail;
			if (ret > 0)
				break;
			leaf = path->nodes[0];
		}

		btrfs_item_key_to_cpu(leaf, &key, path->slots[0]);
		if (key.objectid != BTRFS_EXTENT_CSUM_OBJECTID ||
		    key.type != BTRFS_EXTENT_CSUM_KEY ||
		    key.offset > end)
			break;

		if (key.offset > start)
			start = key.offset;

		size = btrfs_item_size_nr(leaf, path->slots[0]);
		csum_end = key.offset + (size / csum_size) * root->sectorsize;
		if (csum_end <= start) {
			path->slots[0]++;
			continue;
		}

		csum_end = min(csum_end, end + 1);
		item = btrfs_item_ptr(path->nodes[0], path->slots[0],
				      struct btrfs_csum_item);
		while (start < csum_end) {
			size = min_t(size_t, csum_end - start,
				     MAX_ORDERED_SUM_BYTES(root));
			sums = kzalloc(btrfs_ordered_sum_size(root, size),
				       GFP_NOFS);
			if (!sums) {
				ret = -ENOMEM;
				goto fail;
			}

			sums->bytenr = start;
			sums->len = (int)size;

			offset = (start - key.offset) >>
				root->fs_info->sb->s_blocksize_bits;
			offset *= csum_size;
			size >>= root->fs_info->sb->s_blocksize_bits;

			read_extent_buffer(path->nodes[0],
					   sums->sums,
					   ((unsigned long)item) + offset,
					   csum_size * size);

			start += root->sectorsize * size;
			list_add_tail(&sums->list, &tmplist);
		}
		path->slots[0]++;
	}
	ret = 0;
fail:
	while (ret < 0 && !list_empty(&tmplist)) {
		sums = list_entry(tmplist.next, struct btrfs_ordered_sum, list);
		list_del(&sums->list);
		kfree(sums);
	}
	list_splice_tail(&tmplist, list);

	btrfs_free_path(path);
	return ret;
}

int btrfs_csum_one_bio(struct btrfs_root *root, struct inode *inode,
		       struct bio *bio, u64 file_start, int contig)
{
	struct btrfs_ordered_sum *sums;
	struct btrfs_ordered_extent *ordered;
	char *data;
	struct bio_vec *bvec = bio->bi_io_vec;
	int bio_index = 0;
	int index;
	unsigned long total_bytes = 0;
	unsigned long this_sum_bytes = 0;
	u64 offset;

	WARN_ON(bio->bi_vcnt <= 0);
	sums = kzalloc(btrfs_ordered_sum_size(root, bio->bi_iter.bi_size),
		       GFP_NOFS);
	if (!sums)
		return -ENOMEM;

	sums->len = bio->bi_iter.bi_size;
	INIT_LIST_HEAD(&sums->list);

	if (contig)
		offset = file_start;
	else
		offset = page_offset(bvec->bv_page) + bvec->bv_offset;

	ordered = btrfs_lookup_ordered_extent(inode, offset);
	BUG_ON(!ordered); /* Logic error */
	sums->bytenr = (u64)bio->bi_iter.bi_sector << 9;
	index = 0;

	while (bio_index < bio->bi_vcnt) {
		if (!contig)
			offset = page_offset(bvec->bv_page) + bvec->bv_offset;

		if (offset >= ordered->file_offset + ordered->len ||
		    offset < ordered->file_offset) {
			unsigned long bytes_left;
			sums->len = this_sum_bytes;
			this_sum_bytes = 0;
			btrfs_add_ordered_sum(inode, ordered, sums);
			btrfs_put_ordered_extent(ordered);

			bytes_left = bio->bi_iter.bi_size - total_bytes;

			sums = kzalloc(btrfs_ordered_sum_size(root, bytes_left),
				       GFP_NOFS);
			BUG_ON(!sums); /* -ENOMEM */
			sums->len = bytes_left;
			ordered = btrfs_lookup_ordered_extent(inode, offset);
			BUG_ON(!ordered); /* Logic error */
			sums->bytenr = ((u64)bio->bi_iter.bi_sector << 9) +
				       total_bytes;
			index = 0;
		}

		data = kmap_atomic(bvec->bv_page);
		sums->sums[index] = ~(u32)0;
		sums->sums[index] = btrfs_csum_data(data + bvec->bv_offset,
						    sums->sums[index],
						    bvec->bv_len);
		kunmap_atomic(data);
		btrfs_csum_final(sums->sums[index],
				 (char *)(sums->sums + index));

		bio_index++;
		index++;
		total_bytes += bvec->bv_len;
		this_sum_bytes += bvec->bv_len;
		offset += bvec->bv_len;
		bvec++;
	}
	this_sum_bytes = 0;
	btrfs_add_ordered_sum(inode, ordered, sums);
	btrfs_put_ordered_extent(ordered);
	return 0;
}

/*
 * helper function for csum removal, this expects the
 * key to describe the csum pointed to by the path, and it expects
 * the csum to overlap the range [bytenr, len]
 *
 * The csum should not be entirely contained in the range and the
 * range should not be entirely contained in the csum.
 *
 * This calls btrfs_truncate_item with the correct args based on the
 * overlap, and fixes up the key as required.
 */
static noinline void truncate_one_csum(struct btrfs_root *root,
				       struct btrfs_path *path,
				       struct btrfs_key *key,
				       u64 bytenr, u64 len)
{
	struct extent_buffer *leaf;
	u16 csum_size = btrfs_super_csum_size(root->fs_info->super_copy);
	u64 csum_end;
	u64 end_byte = bytenr + len;
	u32 blocksize_bits = root->fs_info->sb->s_blocksize_bits;

	leaf = path->nodes[0];
	csum_end = btrfs_item_size_nr(leaf, path->slots[0]) / csum_size;
	csum_end <<= root->fs_info->sb->s_blocksize_bits;
	csum_end += key->offset;

	if (key->offset < bytenr && csum_end <= end_byte) {
		/*
		 *         [ bytenr - len ]
		 *         [   ]
		 *   [csum     ]
		 *   A simple truncate off the end of the item
		 */
		u32 new_size = (bytenr - key->offset) >> blocksize_bits;
		new_size *= csum_size;
		btrfs_truncate_item(root, path, new_size, 1);
	} else if (key->offset >= bytenr && csum_end > end_byte &&
		   end_byte > key->offset) {
		/*
		 *         [ bytenr - len ]
		 *                 [ ]
		 *                 [csum     ]
		 * we need to truncate from the beginning of the csum
		 */
		u32 new_size = (csum_end - end_byte) >> blocksize_bits;
		new_size *= csum_size;

		btrfs_truncate_item(root, path, new_size, 0);

		key->offset = end_byte;
		btrfs_set_item_key_safe(root->fs_info, path, key);
	} else {
		BUG();
	}
}

/*
 * deletes the csum items from the csum tree for a given
 * range of bytes.
 */
int btrfs_del_csums(struct btrfs_trans_handle *trans,
		    struct btrfs_root *root, u64 bytenr, u64 len)
{
	struct btrfs_path *path;
	struct btrfs_key key;
	u64 end_byte = bytenr + len;
	u64 csum_end;
	struct extent_buffer *leaf;
	int ret;
	u16 csum_size = btrfs_super_csum_size(root->fs_info->super_copy);
	int blocksize_bits = root->fs_info->sb->s_blocksize_bits;

	root = root->fs_info->csum_root;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	while (1) {
		key.objectid = BTRFS_EXTENT_CSUM_OBJECTID;
		key.offset = end_byte - 1;
		key.type = BTRFS_EXTENT_CSUM_KEY;

		path->leave_spinning = 1;
		ret = btrfs_search_slot(trans, root, &key, path, -1, 1);
		if (ret > 0) {
			if (path->slots[0] == 0)
				break;
			path->slots[0]--;
		} else if (ret < 0) {
			break;
		}

		leaf = path->nodes[0];
		btrfs_item_key_to_cpu(leaf, &key, path->slots[0]);

		if (key.objectid != BTRFS_EXTENT_CSUM_OBJECTID ||
		    key.type != BTRFS_EXTENT_CSUM_KEY) {
			break;
		}

		if (key.offset >= end_byte)
			break;

		csum_end = btrfs_item_size_nr(leaf, path->slots[0]) / csum_size;
		csum_end <<= blocksize_bits;
		csum_end += key.offset;

		/* this csum ends before we start, we're done */
		if (csum_end <= bytenr)
			break;

		/* delete the entire item, it is inside our range */
		if (key.offset >= bytenr && csum_end <= end_byte) {
			ret = btrfs_del_item(trans, root, path);
			if (ret)
				goto out;
			if (key.offset == bytenr)
				break;
		} else if (key.offset < bytenr && csum_end > end_byte) {
			unsigned long offset;
			unsigned long shift_len;
			unsigned long item_offset;
			/*
			 *        [ bytenr - len ]
			 *     [csum                ]
			 *
			 * Our bytes are in the middle of the csum,
			 * we need to split this item and insert a new one.
			 *
			 * But we can't drop the path because the
			 * csum could change, get removed, extended etc.
			 *
			 * The trick here is the max size of a csum item leaves
			 * enough room in the tree block for a single
			 * item header.  So, we split the item in place,
			 * adding a new header pointing to the existing
			 * bytes.  Then we loop around again and we have
			 * a nicely formed csum item that we can neatly
			 * truncate.
			 */
			offset = (bytenr - key.offset) >> blocksize_bits;
			offset *= csum_size;

			shift_len = (len >> blocksize_bits) * csum_size;

			item_offset = btrfs_item_ptr_offset(leaf,
							    path->slots[0]);

			memset_extent_buffer(leaf, 0, item_offset + offset,
					     shift_len);
			key.offset = bytenr;

			/*
			 * btrfs_split_item returns -EAGAIN when the
			 * item changed size or key
			 */
			ret = btrfs_split_item(trans, root, path, &key, offset);
			if (ret && ret != -EAGAIN) {
				btrfs_abort_transaction(trans, root, ret);
				goto out;
			}

			key.offset = end_byte - 1;
		} else {
			truncate_one_csum(root, path, &key, bytenr, len);
			if (key.offset < bytenr)
				break;
		}
		btrfs_release_path(path);
	}
	ret = 0;
out:
	btrfs_free_path(path);
	return ret;
}

int btrfs_csum_file_blocks(struct btrfs_trans_handle *trans,
			   struct btrfs_root *root,
			   struct btrfs_ordered_sum *sums)
{
	struct btrfs_key file_key;
	struct btrfs_key found_key;
	struct btrfs_path *path;
	struct btrfs_csum_item *item;
	struct btrfs_csum_item *item_end;
	struct extent_buffer *leaf = NULL;
	u64 next_offset;
	u64 total_bytes = 0;
	u64 csum_offset;
	u64 bytenr;
	u32 nritems;
	u32 ins_size;
	int index = 0;
	int found_next;
	int ret;
	u16 csum_size = btrfs_super_csum_size(root->fs_info->super_copy);

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;
again:
	next_offset = (u64)-1;
	found_next = 0;
	bytenr = sums->bytenr + total_bytes;
	file_key.objectid = BTRFS_EXTENT_CSUM_OBJECTID;
	file_key.offset = bytenr;
	file_key.type = BTRFS_EXTENT_CSUM_KEY;

	item = btrfs_lookup_csum(trans, root, path, bytenr, 1);
	if (!IS_ERR(item)) {
		ret = 0;
		leaf = path->nodes[0];
		item_end = btrfs_item_ptr(leaf, path->slots[0],
					  struct btrfs_csum_item);
		item_end = (struct btrfs_csum_item *)((char *)item_end +
			   btrfs_item_size_nr(leaf, path->slots[0]));
		goto found;
	}
	ret = PTR_ERR(item);
	if (ret != -EFBIG && ret != -ENOENT)
		goto fail_unlock;

	if (ret == -EFBIG) {
		u32 item_size;
		/* we found one, but it isn't big enough yet */
		leaf = path->nodes[0];
		item_size = btrfs_item_size_nr(leaf, path->slots[0]);
		if ((item_size / csum_size) >=
		    MAX_CSUM_ITEMS(root, csum_size)) {
			/* already at max size, make a new one */
			goto insert;
		}
	} else {
		int slot = path->slots[0] + 1;
		/* we didn't find a csum item, insert one */
		nritems = btrfs_header_nritems(path->nodes[0]);
		if (!nritems || (path->slots[0] >= nritems - 1)) {
			ret = btrfs_next_leaf(root, path);
			if (ret < 0) {
				goto out;
			} else if (ret > 0) {
				found_next = 1;
				goto insert;
			}
			slot = path->slots[0];
		}
		btrfs_item_key_to_cpu(path->nodes[0], &found_key, slot);
		if (found_key.objectid != BTRFS_EXTENT_CSUM_OBJECTID ||
		    found_key.type != BTRFS_EXTENT_CSUM_KEY) {
			found_next = 1;
			goto insert;
		}
		next_offset = found_key.offset;
		found_next = 1;
		goto insert;
	}

	/*
	 * at this point, we know the tree has an item, but it isn't big
	 * enough yet to put our csum in.  Grow it
	 */
	btrfs_release_path(path);
	ret = btrfs_search_slot(trans, root, &file_key, path,
				csum_size, 1);
	if (ret < 0)
		goto fail_unlock;

	if (ret > 0) {
		if (path->slots[0] == 0)
			goto insert;
		path->slots[0]--;
	}

	leaf = path->nodes[0];
	btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
	csum_offset = (bytenr - found_key.offset) >>
			root->fs_info->sb->s_blocksize_bits;

	if (found_key.type != BTRFS_EXTENT_CSUM_KEY ||
	    found_key.objectid != BTRFS_EXTENT_CSUM_OBJECTID ||
	    csum_offset >= MAX_CSUM_ITEMS(root, csum_size)) {
		goto insert;
	}

	if (csum_offset == btrfs_item_size_nr(leaf, path->slots[0]) /
	    csum_size) {
		int extend_nr;
		u64 tmp;
		u32 diff;
		u32 free_space;

		if (btrfs_leaf_free_space(root, leaf) <
				 sizeof(struct btrfs_item) + csum_size * 2)
			goto insert;

		free_space = btrfs_leaf_free_space(root, leaf) -
					 sizeof(struct btrfs_item) - csum_size;
		tmp = sums->len - total_bytes;
		tmp >>= root->fs_info->sb->s_blocksize_bits;
		WARN_ON(tmp < 1);

		extend_nr = max_t(int, 1, (int)tmp);
		diff = (csum_offset + extend_nr) * csum_size;
		diff = min(diff, MAX_CSUM_ITEMS(root, csum_size) * csum_size);

		diff = diff - btrfs_item_size_nr(leaf, path->slots[0]);
		diff = min(free_space, diff);
		diff /= csum_size;
		diff *= csum_size;

		btrfs_extend_item(root, path, diff);
		ret = 0;
		goto csum;
	}

insert:
	btrfs_release_path(path);
	csum_offset = 0;
	if (found_next) {
		u64 tmp;

		tmp = sums->len - total_bytes;
		tmp >>= root->fs_info->sb->s_blocksize_bits;
		tmp = min(tmp, (next_offset - file_key.offset) >>
					 root->fs_info->sb->s_blocksize_bits);

		tmp = max((u64)1, tmp);
		tmp = min(tmp, (u64)MAX_CSUM_ITEMS(root, csum_size));
		ins_size = csum_size * tmp;
	} else {
		ins_size = csum_size;
	}
	path->leave_spinning = 1;
	ret = btrfs_insert_empty_item(trans, root, path, &file_key,
				      ins_size);
	path->leave_spinning = 0;
	if (ret < 0)
		goto fail_unlock;
	if (WARN_ON(ret != 0))
		goto fail_unlock;
	leaf = path->nodes[0];
csum:
	item = btrfs_item_ptr(leaf, path->slots[0], struct btrfs_csum_item);
	item_end = (struct btrfs_csum_item *)((unsigned char *)item +
				      btrfs_item_size_nr(leaf, path->slots[0]));
	item = (struct btrfs_csum_item *)((unsigned char *)item +
					  csum_offset * csum_size);
found:
	ins_size = (u32)(sums->len - total_bytes) >>
		   root->fs_info->sb->s_blocksize_bits;
	ins_size *= csum_size;
	ins_size = min_t(u32, (unsigned long)item_end - (unsigned long)item,
			      ins_size);
	write_extent_buffer(leaf, sums->sums + index, (unsigned long)item,
			    ins_size);

	ins_size /= csum_size;
	total_bytes += ins_size * root->sectorsize;
	index += ins_size;

	btrfs_mark_buffer_dirty(path->nodes[0]);
	if (total_bytes < sums->len) {
		btrfs_release_path(path);
		cond_resched();
		goto again;
	}
out:
	btrfs_free_path(path);
	return ret;

fail_unlock:
	goto out;
}

void btrfs_extent_item_to_extent_map(struct inode *inode,
				     const struct btrfs_path *path,
				     struct btrfs_file_extent_item *fi,
				     const bool new_inline,
				     struct extent_map *em)
{
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct extent_buffer *leaf = path->nodes[0];
	const int slot = path->slots[0];
	struct btrfs_key key;
	u64 extent_start, extent_end;
	u64 bytenr;
	u8 type = btrfs_file_extent_type(leaf, fi);
	int compress_type = btrfs_file_extent_compression(leaf, fi);

	em->bdev = root->fs_info->fs_devices->latest_bdev;
	btrfs_item_key_to_cpu(leaf, &key, slot);
	extent_start = key.offset;

	if (type == BTRFS_FILE_EXTENT_REG ||
	    type == BTRFS_FILE_EXTENT_PREALLOC) {
		extent_end = extent_start +
			btrfs_file_extent_num_bytes(leaf, fi);
	} else if (type == BTRFS_FILE_EXTENT_INLINE) {
		size_t size;
		size = btrfs_file_extent_inline_len(leaf, slot, fi);
		extent_end = ALIGN(extent_start + size, root->sectorsize);
	}

	em->ram_bytes = btrfs_file_extent_ram_bytes(leaf, fi);
	if (type == BTRFS_FILE_EXTENT_REG ||
	    type == BTRFS_FILE_EXTENT_PREALLOC) {
		em->start = extent_start;
		em->len = extent_end - extent_start;
		em->orig_start = extent_start -
			btrfs_file_extent_offset(leaf, fi);
		em->orig_block_len = btrfs_file_extent_disk_num_bytes(leaf, fi);
		bytenr = btrfs_file_extent_disk_bytenr(leaf, fi);
		if (bytenr == 0) {
			em->block_start = EXTENT_MAP_HOLE;
			return;
		}
		if (compress_type != BTRFS_COMPRESS_NONE) {
			set_bit(EXTENT_FLAG_COMPRESSED, &em->flags);
			em->compress_type = compress_type;
			em->block_start = bytenr;
			em->block_len = em->orig_block_len;
		} else {
			bytenr += btrfs_file_extent_offset(leaf, fi);
			em->block_start = bytenr;
			em->block_len = em->len;
			if (type == BTRFS_FILE_EXTENT_PREALLOC)
				set_bit(EXTENT_FLAG_PREALLOC, &em->flags);
		}
	} else if (type == BTRFS_FILE_EXTENT_INLINE) {
		em->block_start = EXTENT_MAP_INLINE;
		em->start = extent_start;
		em->len = extent_end - extent_start;
		/*
		 * Initialize orig_start and block_len with the same values
		 * as in inode.c:btrfs_get_extent().
		 */
		em->orig_start = EXTENT_MAP_HOLE;
		em->block_len = (u64)-1;
		if (!new_inline && compress_type != BTRFS_COMPRESS_NONE) {
			set_bit(EXTENT_FLAG_COMPRESSED, &em->flags);
			em->compress_type = compress_type;
		}
	} else {
		btrfs_err(root->fs_info,
			  "unknown file extent item type %d, inode %llu, offset %llu, root %llu",
			  type, btrfs_ino(inode), extent_start,
			  root->root_key.objectid);
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 * Copyright (C) 2009 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#ifndef __BTRFS_FREE_SPACE_CACHE
#define __BTRFS_FREE_SPACE_CACHE

struct btrfs_free_space {
	struct rb_node offset_index;
	u64 offset;
	u64 bytes;
	u64 max_extent_size;
	unsigned long *bitmap;
	struct list_head list;
};

struct btrfs_free_space_ctl {
	spinlock_t tree_lock;
	struct rb_root free_space_offset;
	u64 free_space;
	int extents_thresh;
	int free_extents;
	int total_bitmaps;
	int unit;
	u64 start;
	struct btrfs_free_space_op *op;
	void *private;
	struct mutex cache_writeout_mutex;
	struct list_head trimming_ranges;
};

struct btrfs_free_space_op {
	void (*recalc_thresholds)(struct btrfs_free_space_ctl *ctl);
	bool (*use_bitmap)(struct btrfs_free_space_ctl *ctl,
			   struct btrfs_free_space *info);
};

struct btrfs_io_ctl;

struct inode *lookup_free_space_inode(struct btrfs_root *root,
				      struct btrfs_block_group_cache
				      *block_group, struct btrfs_path *path);
int create_free_space_inode(struct btrfs_root *root,
			    struct btrfs_trans_handle *trans,
			    struct btrfs_block_group_cache *block_group,
			    struct btrfs_path *path);

int btrfs_check_trunc_cache_free_space(struct btrfs_root *root,
				       struct btrfs_block_rsv *rsv);
int btrfs_truncate_free_space_cache(struct btrfs_root *root,
				    struct btrfs_trans_handle *trans,
				    struct btrfs_block_group_cache *block_group,
				    struct inode *inode);
int load_free_space_cache(struct btrfs_fs_info *fs_info,
			  struct btrfs_block_group_cache *block_group);
int btrfs_wait_cache_io(struct btrfs_root *root,
			struct btrfs_trans_handle *trans,
			struct btrfs_block_group_cache *block_group,
			struct btrfs_io_ctl *io_ctl,
			struct btrfs_path *path, u64 offset);
int btrfs_write_out_cache(struct btrfs_root *root,
			  struct btrfs_trans_handle *trans,
			  struct btrfs_block_group_cache *block_group,
			  struct btrfs_path *path);
struct inode *lookup_free_ino_inode(struct btrfs_root *root,
				    struct btrfs_path *path);
int create_free_ino_inode(struct btrfs_root *root,
			  struct btrfs_trans_handle *trans,
			  struct btrfs_path *path);
int load_free_ino_cache(struct btrfs_fs_info *fs_info,
			struct btrfs_root *root);
int btrfs_write_out_ino_cache(struct btrfs_root *root,
			      struct btrfs_trans_handle *trans,
			      struct btrfs_path *path,
			      struct inode *inode);

void btrfs_init_free_space_ctl(struct btrfs_block_group_cache *block_group);
int __btrfs_add_free_space(struct btrfs_free_space_ctl *ctl,
			   u64 bytenr, u64 size);
static inline int
btrfs_add_free_space(struct btrfs_block_group_cache *block_group,
		     u64 bytenr, u64 size)
{
	return __btrfs_add_free_space(block_group->free_space_ctl,
				      bytenr, size);
}
int btrfs_remove_free_space(struct btrfs_block_group_cache *block_group,
			    u64 bytenr, u64 size);
void __btrfs_remove_free_space_cache(struct btrfs_free_space_ctl *ctl);
void btrfs_remove_free_space_cache(struct btrfs_block_group_cache
				     *block_group);
u64 btrfs_find_space_for_alloc(struct btrfs_block_group_cache *block_group,
			       u64 offset, u64 bytes, u64 empty_size,
			       u64 *max_extent_size);
u64 btrfs_find_ino_for_alloc(struct btrfs_root *fs_root);
void btrfs_dump_free_space(struct btrfs_block_group_cache *block_group,
			   u64 bytes);
int btrfs_find_space_cluster(struct btrfs_root *root,
			     struct btrfs_block_group_cache *block_group,
			     struct btrfs_free_cluster *cluster,
			     u64 offset, u64 bytes, u64 empty_size);
void btrfs_init_free_cluster(struct btrfs_free_cluster *cluster);
u64 btrfs_alloc_from_cluster(struct btrfs_block_group_cache *block_group,
			     struct btrfs_free_cluster *cluster, u64 bytes,
			     u64 min_start, u64 *max_extent_size);
int btrfs_return_cluster_to_free_space(
			       struct btrfs_block_group_cache *block_group,
			       struct btrfs_free_cluster *cluster);
int btrfs_trim_block_group(struct btrfs_block_group_cache *block_group,
			   u64 *trimmed, u64 start, u64 end, u64 minlen);

/* Support functions for runnint our sanity tests */
#ifdef CONFIG_BTRFS_FS_RUN_SANITY_TESTS
int test_add_free_space_entry(struct btrfs_block_group_cache *cache,
			      u64 offset, u64 bytes, bool bitmap);
int test_check_exists(struct btrfs_block_group_cache *cache,
		      u64 offset, u64 bytes);
#endif

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * Copyright (C) 2014 Filipe David Borba Manana <fdmanana@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <crypto/hash.h>
#include <linux/err.h>
#include "hash.h"

static struct crypto_shash *tfm;

int __init btrfs_hash_init(void)
{
	tfm = crypto_alloc_shash("crc32c", 0, 0);

	return PTR_ERR_OR_ZERO(tfm);
}

void btrfs_hash_exit(void)
{
	crypto_free_shash(tfm);
}

u32 btrfs_crc32c(u32 crc, const void *address, unsigned int length)
{
	SHASH_DESC_ON_STACK(shash, tfm);
	u32 *ctx = (u32 *)shash_desc_ctx(shash);
	u32 retval;
	int err;

	shash->tfm = tfm;
	shash->flags = 0;
	*ctx = crc;

	err = crypto_shash_update(shash, address, length);
	BUG_ON(err);

	retval = *ctx;
	barrier_data(ctx);
	return retval;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#ifndef __HASH__
#define __HASH__

int __init btrfs_hash_init(void);

void btrfs_hash_exit(void);

u32 btrfs_crc32c(u32 crc, const void *address, unsigned int length);

static inline u64 btrfs_name_hash(const char *name, int len)
{
	return btrfs_crc32c((u32)~1, name, len);
}

/*
 * Figure the key offset of an extended inode ref
 */
static inline u64 btrfs_extref_hash(u64 parent_objectid, const char *name,
				    int len)
{
	return (u64) btrfs_crc32c(parent_objectid, name, len);
}

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include "ctree.h"
#include "disk-io.h"
#include "hash.h"
#include "transaction.h"
#include "print-tree.h"

static int find_name_in_backref(struct btrfs_path *path, const char *name,
			 int name_len, struct btrfs_inode_ref **ref_ret)
{
	struct extent_buffer *leaf;
	struct btrfs_inode_ref *ref;
	unsigned long ptr;
	unsigned long name_ptr;
	u32 item_size;
	u32 cur_offset = 0;
	int len;

	leaf = path->nodes[0];
	item_size = btrfs_item_size_nr(leaf, path->slots[0]);
	ptr = btrfs_item_ptr_offset(leaf, path->slots[0]);
	while (cur_offset < item_size) {
		ref = (struct btrfs_inode_ref *)(ptr + cur_offset);
		len = btrfs_inode_ref_name_len(leaf, ref);
		name_ptr = (unsigned long)(ref + 1);
		cur_offset += len + sizeof(*ref);
		if (len != name_len)
			continue;
		if (memcmp_extent_buffer(leaf, name, name_ptr, name_len) == 0) {
			*ref_ret = ref;
			return 1;
		}
	}
	return 0;
}

int btrfs_find_name_in_ext_backref(struct btrfs_path *path, u64 ref_objectid,
				   const char *name, int name_len,
				   struct btrfs_inode_extref **extref_ret)
{
	struct extent_buffer *leaf;
	struct btrfs_inode_extref *extref;
	unsigned long ptr;
	unsigned long name_ptr;
	u32 item_size;
	u32 cur_offset = 0;
	int ref_name_len;

	leaf = path->nodes[0];
	item_size = btrfs_item_size_nr(leaf, path->slots[0]);
	ptr = btrfs_item_ptr_offset(leaf, path->slots[0]);

	/*
	 * Search all extended backrefs in this item. We're only
	 * looking through any collisions so most of the time this is
	 * just going to compare against one buffer. If all is well,
	 * we'll return success and the inode ref object.
	 */
	while (cur_offset < item_size) {
		extref = (struct btrfs_inode_extref *) (ptr + cur_offset);
		name_ptr = (unsigned long)(&extref->name);
		ref_name_len = btrfs_inode_extref_name_len(leaf, extref);

		if (ref_name_len == name_len &&
		    btrfs_inode_extref_parent(leaf, extref) == ref_objectid &&
		    (memcmp_extent_buffer(leaf, name, name_ptr, name_len) == 0)) {
			if (extref_ret)
				*extref_ret = extref;
			return 1;
		}

		cur_offset += ref_name_len + sizeof(*extref);
	}
	return 0;
}

/* Returns NULL if no extref found */
struct btrfs_inode_extref *
btrfs_lookup_inode_extref(struct btrfs_trans_handle *trans,
			  struct btrfs_root *root,
			  struct btrfs_path *path,
			  const char *name, int name_len,
			  u64 inode_objectid, u64 ref_objectid, int ins_len,
			  int cow)
{
	int ret;
	struct btrfs_key key;
	struct btrfs_inode_extref *extref;

	key.objectid = inode_objectid;
	key.type = BTRFS_INODE_EXTREF_KEY;
	key.offset = btrfs_extref_hash(ref_objectid, name, name_len);

	ret = btrfs_search_slot(trans, root, &key, path, ins_len, cow);
	if (ret < 0)
		return ERR_PTR(ret);
	if (ret > 0)
		return NULL;
	if (!btrfs_find_name_in_ext_backref(path, ref_objectid, name, name_len, &extref))
		return NULL;
	return extref;
}

static int btrfs_del_inode_extref(struct btrfs_trans_handle *trans,
				  struct btrfs_root *root,
				  const char *name, int name_len,
				  u64 inode_objectid, u64 ref_objectid,
				  u64 *index)
{
	struct btrfs_path *path;
	struct btrfs_key key;
	struct btrfs_inode_extref *extref;
	struct extent_buffer *leaf;
	int ret;
	int del_len = name_len + sizeof(*extref);
	unsigned long ptr;
	unsigned long item_start;
	u32 item_size;

	key.objectid = inode_objectid;
	key.type = BTRFS_INODE_EXTREF_KEY;
	key.offset = btrfs_extref_hash(ref_objectid, name, name_len);

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	path->leave_spinning = 1;

	ret = btrfs_search_slot(trans, root, &key, path, -1, 1);
	if (ret > 0)
		ret = -ENOENT;
	if (ret < 0)
		goto out;

	/*
	 * Sanity check - did we find the right item for this name?
	 * This should always succeed so error here will make the FS
	 * readonly.
	 */
	if (!btrfs_find_name_in_ext_backref(path, ref_objectid,
					    name, name_len, &extref)) {
		btrfs_std_error(root->fs_info, -ENOENT, NULL);
		ret = -EROFS;
		goto out;
	}

	leaf = path->nodes[0];
	item_size = btrfs_item_size_nr(leaf, path->slots[0]);
	if (index)
		*index = btrfs_inode_extref_index(leaf, extref);

	if (del_len == item_size) {
		/*
		 * Common case only one ref in the item, remove the
		 * whole item.
		 */
		ret = btrfs_del_item(trans, root, path);
		goto out;
	}

	ptr = (unsigned long)extref;
	item_start = btrfs_item_ptr_offset(leaf, path->slots[0]);

	memmove_extent_buffer(leaf, ptr, ptr + del_len,
			      item_size - (ptr + del_len - item_start));

	btrfs_truncate_item(root, path, item_size - del_len, 1);

out:
	btrfs_free_path(path);

	return ret;
}

int btrfs_del_inode_ref(struct btrfs_trans_handle *trans,
			struct btrfs_root *root,
			const char *name, int name_len,
			u64 inode_objectid, u64 ref_objectid, u64 *index)
{
	struct btrfs_path *path;
	struct btrfs_key key;
	struct btrfs_inode_ref *ref;
	struct extent_buffer *leaf;
	unsigned long ptr;
	unsigned long item_start;
	u32 item_size;
	u32 sub_item_len;
	int ret;
	int search_ext_refs = 0;
	int del_len = name_len + sizeof(*ref);

	key.objectid = inode_objectid;
	key.offset = ref_objectid;
	key.type = BTRFS_INODE_REF_KEY;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	path->leave_spinning = 1;

	ret = btrfs_search_slot(trans, root, &key, path, -1, 1);
	if (ret > 0) {
		ret = -ENOENT;
		search_ext_refs = 1;
		goto out;
	} else if (ret < 0) {
		goto out;
	}
	if (!find_name_in_backref(path, name, name_len, &ref)) {
		ret = -ENOENT;
		search_ext_refs = 1;
		goto out;
	}
	leaf = path->nodes[0];
	item_size = btrfs_item_size_nr(leaf, path->slots[0]);

	if (index)
		*index = btrfs_inode_ref_index(leaf, ref);

	if (del_len == item_size) {
		ret = btrfs_del_item(trans, root, path);
		goto out;
	}
	ptr = (unsigned long)ref;
	sub_item_len = name_len + sizeof(*ref);
	item_start = btrfs_item_ptr_offset(leaf, path->slots[0]);
	memmove_extent_buffer(leaf, ptr, ptr + sub_item_len,
			      item_size - (ptr + sub_item_len - item_start));
	btrfs_truncate_item(root, path, item_size - sub_item_len, 1);
out:
	btrfs_free_path(path);

	if (search_ext_refs) {
		/*
		 * No refs were found, or we could not find the
		 * name in our ref array. Find and remove the extended
		 * inode ref then.
		 */
		return btrfs_del_inode_extref(trans, root, name, name_len,
					      inode_objectid, ref_objectid, index);
	}

	return ret;
}

/*
 * btrfs_insert_inode_extref() - Inserts an extended inode ref into a tree.
 *
 * The caller must have checked against BTRFS_LINK_MAX already.
 */
static int btrfs_insert_inode_extref(struct btrfs_trans_handle *trans,
				     struct btrfs_root *root,
				     const char *name, int name_len,
				     u64 inode_objectid, u64 ref_objectid, u64 index)
{
	struct btrfs_inode_extref *extref;
	int ret;
	int ins_len = name_len + sizeof(*extref);
	unsigned long ptr;
	struct btrfs_path *path;
	struct btrfs_key key;
	struct extent_buffer *leaf;
	struct btrfs_item *item;

	key.objectid = inode_objectid;
	key.type = BTRFS_INODE_EXTREF_KEY;
	key.offset = btrfs_extref_hash(ref_objectid, name, name_len);

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	path->leave_spinning = 1;
	ret = btrfs_insert_empty_item(trans, root, path, &key,
				      ins_len);
	if (ret == -EEXIST) {
		if (btrfs_find_name_in_ext_backref(path, ref_objectid,
						   name, name_len, NULL))
			goto out;

		btrfs_extend_item(root, path, ins_len);
		ret = 0;
	}
	if (ret < 0)
		goto out;

	leaf = path->nodes[0];
	item = btrfs_item_nr(path->slots[0]);
	ptr = (unsigned long)btrfs_item_ptr(leaf, path->slots[0], char);
	ptr += btrfs_item_size(leaf, item) - ins_len;
	extref = (struct btrfs_inode_extref *)ptr;

	btrfs_set_inode_extref_name_len(path->nodes[0], extref, name_len);
	btrfs_set_inode_extref_index(path->nodes[0], extref, index);
	btrfs_set_inode_extref_parent(path->nodes[0], extref, ref_objectid);

	ptr = (unsigned long)&extref->name;
	write_extent_buffer(path->nodes[0], name, ptr, name_len);
	btrfs_mark_buffer_dirty(path->nodes[0]);

out:
	btrfs_free_path(path);
	return ret;
}

/* Will return 0, -ENOMEM, -EMLINK, or -EEXIST or anything from the CoW path */
int btrfs_insert_inode_ref(struct btrfs_trans_handle *trans,
			   struct btrfs_root *root,
			   const char *name, int name_len,
			   u64 inode_objectid, u64 ref_objectid, u64 index)
{
	struct btrfs_path *path;
	struct btrfs_key key;
	struct btrfs_inode_ref *ref;
	unsigned long ptr;
	int ret;
	int ins_len = name_len + sizeof(*ref);

	key.objectid = inode_objectid;
	key.offset = ref_objectid;
	key.type = BTRFS_INODE_REF_KEY;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	path->leave_spinning = 1;
	path->skip_release_on_error = 1;
	ret = btrfs_insert_empty_item(trans, root, path, &key,
				      ins_len);
	if (ret == -EEXIST) {
		u32 old_size;

		if (find_name_in_backref(path, name, name_len, &ref))
			goto out;

		old_size = btrfs_item_size_nr(path->nodes[0], path->slots[0]);
		btrfs_extend_item(root, path, ins_len);
		ref = btrfs_item_ptr(path->nodes[0], path->slots[0],
				     struct btrfs_inode_ref);
		ref = (struct btrfs_inode_ref *)((unsigned long)ref + old_size);
		btrfs_set_inode_ref_name_len(path->nodes[0], ref, name_len);
		btrfs_set_inode_ref_index(path->nodes[0], ref, index);
		ptr = (unsigned long)(ref + 1);
		ret = 0;
	} else if (ret < 0) {
		if (ret == -EOVERFLOW) {
			if (find_name_in_backref(path, name, name_len, &ref))
				ret = -EEXIST;
			else
				ret = -EMLINK;
		}
		goto out;
	} else {
		ref = btrfs_item_ptr(path->nodes[0], path->slots[0],
				     struct btrfs_inode_ref);
		btrfs_set_inode_ref_name_len(path->nodes[0], ref, name_len);
		btrfs_set_inode_ref_index(path->nodes[0], ref, index);
		ptr = (unsigned long)(ref + 1);
	}
	write_extent_buffer(path->nodes[0], name, ptr, name_len);
	btrfs_mark_buffer_dirty(path->nodes[0]);

out:
	btrfs_free_path(path);

	if (ret == -EMLINK) {
		struct btrfs_super_block *disk_super = root->fs_info->super_copy;
		/* We ran out of space in the ref array. Need to
		 * add an extended ref. */
		if (btrfs_super_incompat_flags(disk_super)
		    & BTRFS_FEATURE_INCOMPAT_EXTENDED_IREF)
			ret = btrfs_insert_inode_extref(trans, root, name,
							name_len,
							inode_objectid,
							ref_objectid, index);
	}

	return ret;
}

int btrfs_insert_empty_inode(struct btrfs_trans_handle *trans,
			     struct btrfs_root *root,
			     struct btrfs_path *path, u64 objectid)
{
	struct btrfs_key key;
	int ret;
	key.objectid = objectid;
	key.type = BTRFS_INODE_ITEM_KEY;
	key.offset = 0;

	ret = btrfs_insert_empty_item(trans, root, path, &key,
				      sizeof(struct btrfs_inode_item));
	return ret;
}

int btrfs_lookup_inode(struct btrfs_trans_handle *trans, struct btrfs_root
		       *root, struct btrfs_path *path,
		       struct btrfs_key *location, int mod)
{
	int ins_len = mod < 0 ? -1 : 0;
	int cow = mod != 0;
	int ret;
	int slot;
	struct extent_buffer *leaf;
	struct btrfs_key found_key;

	ret = btrfs_search_slot(trans, root, location, path, ins_len, cow);
	if (ret > 0 && location->type == BTRFS_ROOT_ITEM_KEY &&
	    location->offset == (u64)-1 && path->slots[0] != 0) {
		slot = path->slots[0] - 1;
		leaf = path->nodes[0];
		btrfs_item_key_to_cpu(leaf, &found_key, slot);
		if (found_key.objectid == location->objectid &&
		    found_key.type == location->type) {
			path->slots[0]--;
			return 0;
		}
	}
	return ret;
}
                                                                                                                                                                                                                                                                                                                 /*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/pagemap.h>

#include "ctree.h"
#include "disk-io.h"
#include "free-space-cache.h"
#include "inode-map.h"
#include "transaction.h"

static int caching_kthread(void *data)
{
	struct btrfs_root *root = data;
	struct btrfs_fs_info *fs_info = root->fs_info;
	struct btrfs_free_space_ctl *ctl = root->free_ino_ctl;
	struct btrfs_key key;
	struct btrfs_path *path;
	struct extent_buffer *leaf;
	u64 last = (u64)-1;
	int slot;
	int ret;

	if (!btrfs_test_opt(root, INODE_MAP_CACHE))
		return 0;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	/* Since the commit root is read-only, we can safely skip locking. */
	path->skip_locking = 1;
	path->search_commit_root = 1;
	path->reada = 2;

	key.objectid = BTRFS_FIRST_FREE_OBJECTID;
	key.offset = 0;
	key.type = BTRFS_INODE_ITEM_KEY;
again:
	/* need to make sure the commit_root doesn't disappear */
	down_read(&fs_info->commit_root_sem);

	ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
	if (ret < 0)
		goto out;

	while (1) {
		if (btrfs_fs_closing(fs_info))
			goto out;

		leaf = path->nodes[0];
		slot = path->slots[0];
		if (slot >= btrfs_header_nritems(leaf)) {
			ret = btrfs_next_leaf(root, path);
			if (ret < 0)
				goto out;
			else if (ret > 0)
				break;

			if (need_resched() ||
			    btrfs_transaction_in_commit(fs_info)) {
				leaf = path->nodes[0];

				if (WARN_ON(btrfs_header_nritems(leaf) == 0))
					break;

				/*
				 * Save the key so we can advances forward
				 * in the next search.
				 */
				btrfs_item_key_to_cpu(leaf, &key, 0);
				btrfs_release_path(path);
				root->ino_cache_progress = last;
				up_read(&fs_info->commit_root_sem);
				schedule_timeout(1);
				goto again;
			} else
				continue;
		}

		btrfs_item_key_to_cpu(leaf, &key, slot);

		if (key.type != BTRFS_INODE_ITEM_KEY)
			goto next;

		if (key.objectid >= root->highest_objectid)
			break;

		if (last != (u64)-1 && last + 1 != key.objectid) {
			__btrfs_add_free_space(ctl, last + 1,
					       key.objectid - last - 1);
			wake_up(&root->ino_cache_wait);
		}

		last = key.objectid;
next:
		path->slots[0]++;
	}

	if (last < root->highest_objectid - 1) {
		__btrfs_add_free_space(ctl, last + 1,
				       root->highest_objectid - last - 1);
	}

	spin_lock(&root->ino_cache_lock);
	root->ino_cache_state = BTRFS_CACHE_FINISHED;
	spin_unlock(&root->ino_cache_lock);

	root->ino_cache_progress = (u64)-1;
	btrfs_unpin_free_ino(root);
out:
	wake_up(&root->ino_cache_wait);
	up_read(&fs_info->commit_root_sem);

	btrfs_free_path(path);

	return ret;
}

static void start_caching(struct btrfs_root *root)
{
	struct btrfs_free_space_ctl *ctl = root->free_ino_ctl;
	struct task_struct *tsk;
	int ret;
	u64 objectid;

	if (!btrfs_test_opt(root, INODE_MAP_CACHE))
		return;

	spin_lock(&root->ino_cache_lock);
	if (root->ino_cache_state != BTRFS_CACHE_NO) {
		spin_unlock(&root->ino_cache_lock);
		return;
	}

	root->ino_cache_state = BTRFS_CACHE_STARTED;
	spin_unlock(&root->ino_cache_lock);

	ret = load_free_ino_cache(root->fs_info, root);
	if (ret == 1) {
		spin_lock(&root->ino_cache_lock);
		root->ino_cache_state = BTRFS_CACHE_FINISHED;
		spin_unlock(&root->ino_cache_lock);
		wake_up(&root->ino_cache_wait);
		return;
	}

	/*
	 * It can be quite time-consuming to fill the cache by searching
	 * through the extent tree, and this can keep ino allocation path
	 * waiting. Therefore at start we quickly find out the highest
	 * inode number and we know we can use inode numbers which fall in
	 * [highest_ino + 1, BTRFS_LAST_FREE_OBJECTID].
	 */
	ret = btrfs_find_free_objectid(root, &objectid);
	if (!ret && objectid <= BTRFS_LAST_FREE_OBJECTID) {
		__btrfs_add_free_space(ctl, objectid,
				       BTRFS_LAST_FREE_OBJECTID - objectid + 1);
	}

	tsk = kthread_run(caching_kthread, root, "btrfs-ino-cache-%llu",
			  root->root_key.objectid);
	if (IS_ERR(tsk)) {
		btrfs_warn(root->fs_info, "failed to start inode caching task");
		btrfs_clear_pending_and_info(root->fs_info, INODE_MAP_CACHE,
				"disabling inode map caching");
	}
}

int btrfs_find_free_ino(struct btrfs_root *root, u64 *objectid)
{
	if (!btrfs_test_opt(root, INODE_MAP_CACHE))
		return btrfs_find_free_objectid(root, objectid);

again:
	*objectid = btrfs_find_ino_for_alloc(root);

	if (*objectid != 0)
		return 0;

	start_caching(root);

	wait_event(root->ino_cache_wait,
		   root->ino_cache_state == BTRFS_CACHE_FINISHED ||
		   root->free_ino_ctl->free_space > 0);

	if (root->ino_cache_state == BTRFS_CACHE_FINISHED &&
	    root->free_ino_ctl->free_space == 0)
		return -ENOSPC;
	else
		goto again;
}

void btrfs_return_ino(struct btrfs_root *root, u64 objectid)
{
	struct btrfs_free_space_ctl *pinned = root->free_ino_pinned;

	if (!btrfs_test_opt(root, INODE_MAP_CACHE))
		return;
again:
	if (root->ino_cache_state == BTRFS_CACHE_FINISHED) {
		__btrfs_add_free_space(pinned, objectid, 1);
	} else {
		down_write(&root->fs_info->commit_root_sem);
		spin_lock(&root->ino_cache_lock);
		if (root->ino_cache_state == BTRFS_CACHE_FINISHED) {
			spin_unlock(&root->ino_cache_lock);
			up_write(&root->fs_info->commit_root_sem);
			goto again;
		}
		spin_unlock(&root->ino_cache_lock);

		start_caching(root);

		__btrfs_add_free_space(pinned, objectid, 1);

		up_write(&root->fs_info->commit_root_sem);
	}
}

/*
 * When a transaction is committed, we'll move those inode numbers which are
 * smaller than root->ino_cache_progress from pinned tree to free_ino tree, and
 * others will just be dropped, because the commit root we were searching has
 * changed.
 *
 * Must be called with root->fs_info->commit_root_sem held
 */
void btrfs_unpin_free_ino(struct btrfs_root *root)
{
	struct btrfs_free_space_ctl *ctl = root->free_ino_ctl;
	struct rb_root *rbroot = &root->free_ino_pinned->free_space_offset;
	spinlock_t *rbroot_lock = &root->free_ino_pinned->tree_lock;
	struct btrfs_free_space *info;
	struct rb_node *n;
	u64 count;

	if (!btrfs_test_opt(root, INODE_MAP_CACHE))
		return;

	while (1) {
		bool add_to_ctl = true;

		spin_lock(rbroot_lock);
		n = rb_first(rbroot);
		if (!n) {
			spin_unlock(rbroot_lock);
			break;
		}

		info = rb_entry(n, struct btrfs_free_space, offset_index);
		BUG_ON(info->bitmap); /* Logic error */

		if (info->offset > root->ino_cache_progress)
			add_to_ctl = false;
		else if (info->offset + info->bytes > root->ino_cache_progress)
			count = root->ino_cache_progress - info->offset + 1;
		else
			count = info->bytes;

		rb_erase(&info->offset_index, rbroot);
		spin_unlock(rbroot_lock);
		if (add_to_ctl)
			__btrfs_add_free_space(ctl, info->offset, count);
		kmem_cache_free(btrfs_free_space_cachep, info);
	}
}

#define INIT_THRESHOLD	(((1024 * 32) / 2) / sizeof(struct btrfs_free_space))
#define INODES_PER_BITMAP (PAGE_CACHE_SIZE * 8)

/*
 * The goal is to keep the memory used by the free_ino tree won't
 * exceed the memory if we use bitmaps only.
 */
static void recalculate_thresholds(struct btrfs_free_space_ctl *ctl)
{
	struct btrfs_free_space *info;
	struct rb_node *n;
	int max_ino;
	int max_bitmaps;

	n = rb_last(&ctl->free_space_offset);
	if (!n) {
		ctl->extents_thresh = INIT_THRESHOLD;
		return;
	}
	info = rb_entry(n, struct btrfs_free_space, offset_index);

	/*
	 * Find the maximum inode number in the filesystem. Note we
	 * ignore the fact that this can be a bitmap, because we are
	 * not doing precise calculation.
	 */
	max_ino = info->bytes - 1;

	max_bitmaps = ALIGN(max_ino, INODES_PER_BITMAP) / INODES_PER_BITMAP;
	if (max_bitmaps <= ctl->total_bitmaps) {
		ctl->extents_thresh = 0;
		return;
	}

	ctl->extents_thresh = (max_bitmaps - ctl->total_bitmaps) *
				PAGE_CACHE_SIZE / sizeof(*info);
}

/*
 * We don't fall back to bitmap, if we are below the extents threshold
 * or this chunk of inode numbers is a big one.
 */
static bool use_bitmap(struct btrfs_free_space_ctl *ctl,
		       struct btrfs_free_space *info)
{
	if (ctl->free_extents < ctl->extents_thresh ||
	    info->bytes > INODES_PER_BITMAP / 10)
		return false;

	return true;
}

static struct btrfs_free_space_op free_ino_op = {
	.recalc_thresholds	= recalculate_thresholds,
	.use_bitmap		= use_bitmap,
};

static void pinned_recalc_thresholds(struct btrfs_free_space_ctl *ctl)
{
}

static bool pinned_use_bitmap(struct btrfs_free_space_ctl *ctl,
			      struct btrfs_free_space *info)
{
	/*
	 * We always use extents for two reasons:
	 *
	 * - The pinned tree is only used during the process of caching
	 *   work.
	 * - Make code simpler. See btrfs_unpin_free_ino().
	 */
	return false;
}

static struct btrfs_free_space_op pinned_free_ino_op = {
	.recalc_thresholds	= pinned_recalc_thresholds,
	.use_bitmap		= pinned_use_bitmap,
};

void btrfs_init_free_ino_ctl(struct btrfs_root *root)
{
	struct btrfs_free_space_ctl *ctl = root->free_ino_ctl;
	struct btrfs_free_space_ctl *pinned = root->free_ino_pinned;

	spin_lock_init(&ctl->tree_lock);
	ctl->unit = 1;
	ctl->start = 0;
	ctl->private = NULL;
	ctl->op = &free_ino_op;
	INIT_LIST_HEAD(&ctl->trimming_ranges);
	mutex_init(&ctl->cache_writeout_mutex);

	/*
	 * Initially we allow to use 16K of ram to cache chunks of
	 * inode numbers before we resort to bitmaps. This is somewhat
	 * arbitrary, but it will be adjusted in runtime.
	 */
	ctl->extents_thresh = INIT_THRESHOLD;

	spin_lock_init(&pinned->tree_lock);
	pinned->unit = 1;
	pinned->start = 0;
	pinned->private = NULL;
	pinned->extents_thresh = 0;
	pinned->op = &pinned_free_ino_op;
}

int btrfs_save_ino_cache(struct btrfs_root *root,
			 struct btrfs_trans_handle *trans)
{
	struct btrfs_free_space_ctl *ctl = root->free_ino_ctl;
	struct btrfs_path *path;
	struct inode *inode;
	struct btrfs_block_rsv *rsv;
	u64 num_bytes;
	u64 alloc_hint = 0;
	int ret;
	int prealloc;
	bool retry = false;

	/* only fs tree and subvol/snap needs ino cache */
	if (root->root_key.objectid != BTRFS_FS_TREE_OBJECTID &&
	    (root->root_key.objectid < BTRFS_FIRST_FREE_OBJECTID ||
	     root->root_key.objectid > BTRFS_LAST_FREE_OBJECTID))
		return 0;

	/* Don't save inode cache if we are deleting this root */
	if (btrfs_root_refs(&root->root_item) == 0)
		return 0;

	if (!btrfs_test_opt(root, INODE_MAP_CACHE))
		return 0;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	rsv = trans->block_rsv;
	trans->block_rsv = &root->fs_info->trans_block_rsv;

	num_bytes = trans->bytes_reserved;
	/*
	 * 1 item for inode item insertion if need
	 * 4 items for inode item update (in the worst case)
	 * 1 items for slack space if we need do truncation
	 * 1 item for free space object
	 * 3 items for pre-allocation
	 */
	trans->bytes_reserved = btrfs_calc_trans_metadata_size(root, 10);
	ret = btrfs_block_rsv_add(root, trans->block_rsv,
				  trans->bytes_reserved,
				  BTRFS_RESERVE_NO_FLUSH);
	if (ret)
		goto out;
	trace_btrfs_space_reservation(root->fs_info, "ino_cache",
				      trans->transid, trans->bytes_reserved, 1);
again:
	inode = lookup_free_ino_inode(root, path);
	if (IS_ERR(inode) && (PTR_ERR(inode) != -ENOENT || retry)) {
		ret = PTR_ERR(inode);
		goto out_release;
	}

	if (IS_ERR(inode)) {
		BUG_ON(retry); /* Logic error */
		retry = true;

		ret = create_free_ino_inode(root, trans, path);
		if (ret)
			goto out_release;
		goto again;
	}

	BTRFS_I(inode)->generation = 0;
	ret = btrfs_update_inode(trans, root, inode);
	if (ret) {
		btrfs_abort_transaction(trans, root, ret);
		goto out_put;
	}

	if (i_size_read(inode) > 0) {
		ret = btrfs_truncate_free_space_cache(root, trans, NULL, inode);
		if (ret) {
			if (ret != -ENOSPC)
				btrfs_abort_transaction(trans, root, ret);
			goto out_put;
		}
	}

	spin_lock(&root->ino_cache_lock);
	if (root->ino_cache_state != BTRFS_CACHE_FINISHED) {
		ret = -1;
		spin_unlock(&root->ino_cache_lock);
		goto out_put;
	}
	spin_unlock(&root->ino_cache_lock);

	spin_lock(&ctl->tree_lock);
	prealloc = sizeof(struct btrfs_free_space) * ctl->free_extents;
	prealloc = ALIGN(prealloc, PAGE_CACHE_SIZE);
	prealloc += ctl->total_bitmaps * PAGE_CACHE_SIZE;
	spin_unlock(&ctl->tree_lock);

	/* Just to make sure we have enough space */
	prealloc += 8 * PAGE_CACHE_SIZE;

	ret = btrfs_delalloc_reserve_space(inode, 0, prealloc);
	if (ret)
		goto out_put;

	ret = btrfs_prealloc_file_range_trans(inode, trans, 0, 0, prealloc,
					      prealloc, prealloc, &alloc_hint);
	if (ret) {
		btrfs_delalloc_release_space(inode, 0, prealloc);
		goto out_put;
	}
	btrfs_free_reserved_data_space(inode, 0, prealloc);

	ret = btrfs_write_out_ino_cache(root, trans, path, inode);
out_put:
	iput(inode);
out_release:
	trace_btrfs_space_reservation(root->fs_info, "ino_cache",
				      trans->transid, trans->bytes_reserved, 0);
	btrfs_block_rsv_release(root, trans->block_rsv, trans->bytes_reserved);
out:
	trans->block_rsv = rsv;
	trans->bytes_reserved = num_bytes;

	btrfs_free_path(path);
	return ret;
}

int btrfs_find_highest_objectid(struct btrfs_root *root, u64 *objectid)
{
	struct btrfs_path *path;
	int ret;
	struct extent_buffer *l;
	struct btrfs_key search_key;
	struct btrfs_key found_key;
	int slot;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	search_key.objectid = BTRFS_LAST_FREE_OBJECTID;
	search_key.type = -1;
	search_key.offset = (u64)-1;
	ret = btrfs_search_slot(NULL, root, &search_key, path, 0, 0);
	if (ret < 0)
		goto error;
	BUG_ON(ret == 0); /* Corruption */
	if (path->slots[0] > 0) {
		slot = path->slots[0] - 1;
		l = path->nodes[0];
		btrfs_item_key_to_cpu(l, &found_key, slot);
		*objectid = max_t(u64, found_key.objectid,
				  BTRFS_FIRST_FREE_OBJECTID - 1);
	} else {
		*objectid = BTRFS_FIRST_FREE_OBJECTID - 1;
	}
	ret = 0;
error:
	btrfs_free_path(path);
	return ret;
}

int btrfs_find_free_objectid(struct btrfs_root *root, u64 *objectid)
{
	int ret;
	mutex_lock(&root->objectid_mutex);

	if (unlikely(root->highest_objectid >= BTRFS_LAST_FREE_OBJECTID)) {
		ret = -ENOSPC;
		goto out;
	}

	*objectid = ++root->highest_objectid;
	ret = 0;
out:
	mutex_unlock(&root->objectid_mutex);
	return ret;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                #ifndef __BTRFS_INODE_MAP
#define __BTRFS_INODE_MAP

void btrfs_init_free_ino_ctl(struct btrfs_root *root);
void btrfs_unpin_free_ino(struct btrfs_root *root);
void btrfs_return_ino(struct btrfs_root *root, u64 objectid);
int btrfs_find_free_ino(struct btrfs_root *root, u64 *objectid);
int btrfs_save_ino_cache(struct btrfs_root *root,
			 struct btrfs_trans_handle *trans);

int btrfs_find_free_objectid(struct btrfs_root *root, u64 *objectid);
int btrfs_find_highest_objectid(struct btrfs_root *root, u64 *objectid);

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /*
 * Copyright (C) 2008 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */
#include <linux/sched.h>
#include <linux/pagemap.h>
#include <linux/spinlock.h>
#include <linux/page-flags.h>
#include <asm/bug.h>
#include "ctree.h"
#include "extent_io.h"
#include "locking.h"

static void btrfs_assert_tree_read_locked(struct extent_buffer *eb);

/*
 * if we currently have a spinning reader or writer lock
 * (indicated by the rw flag) this will bump the count
 * of blocking holders and drop the spinlock.
 */
void btrfs_set_lock_blocking_rw(struct extent_buffer *eb, int rw)
{
	/*
	 * no lock is required.  The lock owner may change if
	 * we have a read lock, but it won't change to or away
	 * from us.  If we have the write lock, we are the owner
	 * and it'll never change.
	 */
	if (eb->lock_nested && current->pid == eb->lock_owner)
		return;
	if (rw == BTRFS_WRITE_LOCK) {
		if (atomic_read(&eb->blocking_writers) == 0) {
			WARN_ON(atomic_read(&eb->spinning_writers) != 1);
			atomic_dec(&eb->spinning_writers);
			btrfs_assert_tree_locked(eb);
			atomic_inc(&eb->blocking_writers);
			write_unlock(&eb->lock);
		}
	} else if (rw == BTRFS_READ_LOCK) {
		btrfs_assert_tree_read_locked(eb);
		atomic_inc(&eb->blocking_readers);
		WARN_ON(atomic_read(&eb->spinning_readers) == 0);
		atomic_dec(&eb->spinning_readers);
		read_unlock(&eb->lock);
	}
	return;
}

/*
 * if we currently have a blocking lock, take the spinlock
 * and drop our blocking count
 */
void btrfs_clear_lock_blocking_rw(struct extent_buffer *eb, int rw)
{
	/*
	 * no lock is required.  The lock owner may change if
	 * we have a read lock, but it won't change to or away
	 * from us.  If we have the write lock, we are the owner
	 * and it'll never change.
	 */
	if (eb->lock_nested && current->pid == eb->lock_owner)
		return;

	if (rw == BTRFS_WRITE_LOCK_BLOCKING) {
		BUG_ON(atomic_read(&eb->blocking_writers) != 1);
		write_lock(&eb->lock);
		WARN_ON(atomic_read(&eb->spinning_writers));
		atomic_inc(&eb->spinning_writers);
		/*
		 * atomic_dec_and_test implies a barrier for waitqueue_active
		 */
		if (atomic_dec_and_test(&eb->blocking_writers) &&
		    waitqueue_active(&eb->write_lock_wq))
			wake_up(&eb->write_lock_wq);
	} else if (rw == BTRFS_READ_LOCK_BLOCKING) {
		BUG_ON(atomic_read(&eb->blocking_readers) == 0);
		read_lock(&eb->lock);
		atomic_inc(&eb->spinning_readers);
		/*
		 * atomic_dec_and_test implies a barrier for waitqueue_active
		 */
		if (atomic_dec_and_test(&eb->blocking_readers) &&
		    waitqueue_active(&eb->read_lock_wq))
			wake_up(&eb->read_lock_wq);
	}
	return;
}

/*
 * take a spinning read lock.  This will wait for any blocking
 * writers
 */
void btrfs_tree_read_lock(struct extent_buffer *eb)
{
again:
	BUG_ON(!atomic_read(&eb->blocking_writers) &&
	       current->pid == eb->lock_owner);

	read_lock(&eb->lock);
	if (atomic_read(&eb->blocking_writers) &&
	    current->pid == eb->lock_owner) {
		/*
		 * This extent is already write-locked by our thread. We allow
		 * an additional read lock to be added because it's for the same
		 * thread. btrfs_find_all_roots() depends on this as it may be
		 * called on a partly (write-)locked tree.
		 */
		BUG_ON(eb->lock_nested);
		eb->lock_nested = 1;
		read_unlock(&eb->lock);
		return;
	}
	if (atomic_read(&eb->blocking_writers)) {
		read_unlock(&eb->lock);
		wait_event(eb->write_lock_wq,
			   atomic_read(&eb->blocking_writers) == 0);
		goto again;
	}
	atomic_inc(&eb->read_locks);
	atomic_inc(&eb->spinning_readers);
}

/*
 * take a spinning read lock.
 * returns 1 if we get the read lock and 0 if we don't
 * this won't wait for blocking writers
 */
int btrfs_tree_read_lock_atomic(struct extent_buffer *eb)
{
	if (atomic_read(&eb->blocking_writers))
		return 0;

	read_lock(&eb->lock);
	if (atomic_read(&eb->blocking_writers)) {
		read_unlock(&eb->lock);
		return 0;
	}
	atomic_inc(&eb->read_locks);
	atomic_inc(&eb->spinning_readers);
	return 1;
}

/*
 * returns 1 if we get the read lock and 0 if we don't
 * this won't wait for blocking writers
 */
int btrfs_try_tree_read_lock(struct extent_buffer *eb)
{
	if (atomic_read(&eb->blocking_writers))
		return 0;

	if (!read_trylock(&eb->lock))
		return 0;

	if (atomic_read(&eb->blocking_writers)) {
		read_unlock(&eb->lock);
		return 0;
	}
	atomic_inc(&eb->read_locks);
	atomic_inc(&eb->spinning_readers);
	return 1;
}

/*
 * returns 1 if we get the read lock and 0 if we don't
 * this won't wait for blocking writers or readers
 */
int btrfs_try_tree_write_lock(struct extent_buffer *eb)
{
	if (atomic_read(&eb->blocking_writers) ||
	    atomic_read(&eb->blocking_readers))
		return 0;

	write_lock(&eb->lock);
	if (atomic_read(&eb->blocking_writers) ||
	    atomic_read(&eb->blocking_readers)) {
		write_unlock(&eb->lock);
		return 0;
	}
	atomic_inc(&eb->write_locks);
	atomic_inc(&eb->spinning_writers);
	eb->lock_owner = current->pid;
	return 1;
}

/*
 * drop a spinning read lock
 */
void btrfs_tree_read_unlock(struct extent_buffer *eb)
{
	/*
	 * if we're nested, we have the write lock.  No new locking
	 * is needed as long as we are the lock owner.
	 * The write unlock will do a barrier for us, and the lock_nested
	 * field only matters to the lock owner.
	 */
	if (eb->lock_nested && current->pid == eb->lock_owner) {
		eb->lock_nested = 0;
		return;
	}
	btrfs_assert_tree_read_locked(eb);
	WARN_ON(atomic_read(&eb->spinning_readers) == 0);
	atomic_dec(&eb->spinning_readers);
	atomic_dec(&eb->read_locks);
	read_unlock(&eb->lock);
}

/*
 * drop a blocking read lock
 */
void btrfs_tree_read_unlock_blocking(struct extent_buffer *eb)
{
	/*
	 * if we're nested, we have the write lock.  No new locking
	 * is needed as long as we are the lock owner.
	 * The write unlock will do a barrier for us, and the lock_nested
	 * field only matters to the lock owner.
	 */
	if (eb->lock_nested && current->pid == eb->lock_owner) {
		eb->lock_nested = 0;
		return;
	}
	btrfs_assert_tree_read_locked(eb);
	WARN_ON(atomic_read(&eb->blocking_readers) == 0);
	/*
	 * atomic_dec_and_test implies a barrier for waitqueue_active
	 */
	if (atomic_dec_and_test(&eb->blocking_readers) &&
	    waitqueue_active(&eb->read_lock_wq))
		wake_up(&eb->read_lock_wq);
	atomic_dec(&eb->read_locks);
}

/*
 * take a spinning write lock.  This will wait for both
 * blocking readers or writers
 */
void btrfs_tree_lock(struct extent_buffer *eb)
{
	WARN_ON(eb->lock_owner == current->pid);
again:
	wait_event(eb->read_lock_wq, atomic_read(&eb->blocking_readers) == 0);
	wait_event(eb->write_lock_wq, atomic_read(&eb->blocking_writers) == 0);
	write_lock(&eb->lock);
	if (atomic_read(&eb->blocking_readers)) {
		write_unlock(&eb->lock);
		wait_event(eb->read_lock_wq,
			   atomic_read(&eb->blocking_readers) == 0);
		goto again;
	}
	if (atomic_read(&eb->blocking_writers)) {
		write_unlock(&eb->lock);
		wait_event(eb->write_lock_wq,
			   atomic_read(&eb->blocking_writers) == 0);
		goto again;
	}
	WARN_ON(atomic_read(&eb->spinning_writers));
	atomic_inc(&eb->spinning_writers);
	atomic_inc(&eb->write_locks);
	eb->lock_owner = current->pid;
}

/*
 * drop a spinning or a blocking write lock.
 */
void btrfs_tree_unlock(struct extent_buffer *eb)
{
	int blockers = atomic_read(&eb->blocking_writers);

	BUG_ON(blockers > 1);

	btrfs_assert_tree_locked(eb);
	eb->lock_owner = 0;
	atomic_dec(&eb->write_locks);

	if (blockers) {
		WARN_ON(atomic_read(&eb->spinning_writers));
		atomic_dec(&eb->blocking_writers);
		/*
		 * Make sure counter is updated before we wake up waiters.
		 */
		smp_mb();
		if (waitqueue_active(&eb->write_lock_wq))
			wake_up(&eb->write_lock_wq);
	} else {
		WARN_ON(atomic_read(&eb->spinning_writers) != 1);
		atomic_dec(&eb->spinning_writers);
		write_unlock(&eb->lock);
	}
}

void btrfs_assert_tree_locked(struct extent_buffer *eb)
{
	BUG_ON(!atomic_read(&eb->write_locks));
}

static void btrfs_assert_tree_read_locked(struct extent_buffer *eb)
{
	BUG_ON(!atomic_read(&eb->read_locks));
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * Copyright (C) 2008 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#ifndef __BTRFS_LOCKING_
#define __BTRFS_LOCKING_

#define BTRFS_WRITE_LOCK 1
#define BTRFS_READ_LOCK 2
#define BTRFS_WRITE_LOCK_BLOCKING 3
#define BTRFS_READ_LOCK_BLOCKING 4

void btrfs_tree_lock(struct extent_buffer *eb);
void btrfs_tree_unlock(struct extent_buffer *eb);

void btrfs_tree_read_lock(struct extent_buffer *eb);
void btrfs_tree_read_unlock(struct extent_buffer *eb);
void btrfs_tree_read_unlock_blocking(struct extent_buffer *eb);
void btrfs_set_lock_blocking_rw(struct extent_buffer *eb, int rw);
void btrfs_clear_lock_blocking_rw(struct extent_buffer *eb, int rw);
void btrfs_assert_tree_locked(struct extent_buffer *eb);
int btrfs_try_tree_read_lock(struct extent_buffer *eb);
int btrfs_try_tree_write_lock(struct extent_buffer *eb);
int btrfs_tree_read_lock_atomic(struct extent_buffer *eb);


static inline void btrfs_tree_unlock_rw(struct extent_buffer *eb, int rw)
{
	if (rw == BTRFS_WRITE_LOCK || rw == BTRFS_WRITE_LOCK_BLOCKING)
		btrfs_tree_unlock(eb);
	else if (rw == BTRFS_READ_LOCK_BLOCKING)
		btrfs_tree_read_unlock_blocking(eb);
	else if (rw == BTRFS_READ_LOCK)
		btrfs_tree_read_unlock(eb);
	else
		BUG();
}

static inline void btrfs_set_lock_blocking(struct extent_buffer *eb)
{
	btrfs_set_lock_blocking_rw(eb, BTRFS_WRITE_LOCK);
}

static inline void btrfs_clear_lock_blocking(struct extent_buffer *eb)
{
	btrfs_clear_lock_blocking_rw(eb, BTRFS_WRITE_LOCK_BLOCKING);
}
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * Copyright (C) 2008 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/pagemap.h>
#include <linux/bio.h>
#include <linux/lzo.h>
#include "compression.h"

#define LZO_LEN	4

struct workspace {
	void *mem;
	void *buf;	/* where decompressed data goes */
	void *cbuf;	/* where compressed data goes */
	struct list_head list;
};

static void lzo_free_workspace(struct list_head *ws)
{
	struct workspace *workspace = list_entry(ws, struct workspace, list);

	vfree(workspace->buf);
	vfree(workspace->cbuf);
	vfree(workspace->mem);
	kfree(workspace);
}

static struct list_head *lzo_alloc_workspace(void)
{
	struct workspace *workspace;

	workspace = kzalloc(sizeof(*workspace), GFP_NOFS);
	if (!workspace)
		return ERR_PTR(-ENOMEM);

	workspace->mem = vmalloc(LZO1X_MEM_COMPRESS);
	workspace->buf = vmalloc(lzo1x_worst_compress(PAGE_CACHE_SIZE));
	workspace->cbuf = vmalloc(lzo1x_worst_compress(PAGE_CACHE_SIZE));
	if (!workspace->mem || !workspace->buf || !workspace->cbuf)
		goto fail;

	INIT_LIST_HEAD(&workspace->list);

	return &workspace->list;
fail:
	lzo_free_workspace(&workspace->list);
	return ERR_PTR(-ENOMEM);
}

static inline void write_compress_length(char *buf, size_t len)
{
	__le32 dlen;

	dlen = cpu_to_le32(len);
	memcpy(buf, &dlen, LZO_LEN);
}

static inline size_t read_compress_length(char *buf)
{
	__le32 dlen;

	memcpy(&dlen, buf, LZO_LEN);
	return le32_to_cpu(dlen);
}

static int lzo_compress_pages(struct list_head *ws,
			      struct address_space *mapping,
			      u64 start, unsigned long len,
			      struct page **pages,
			      unsigned long nr_dest_pages,
			      unsigned long *out_pages,
			      unsigned long *total_in,
			      unsigned long *total_out,
			      unsigned long max_out)
{
	struct workspace *workspace = list_entry(ws, struct workspace, list);
	int ret = 0;
	char *data_in;
	char *cpage_out;
	int nr_pages = 0;
	struct page *in_page = NULL;
	struct page *out_page = NULL;
	unsigned long bytes_left;

	size_t in_len;
	size_t out_len;
	char *buf;
	unsigned long tot_in = 0;
	unsigned long tot_out = 0;
	unsigned long pg_bytes_left;
	unsigned long out_offset;
	unsigned long bytes;

	*out_pages = 0;
	*total_out = 0;
	*total_in = 0;

	in_page = find_get_page(mapping, start >> PAGE_CACHE_SHIFT);
	data_in = kmap(in_page);

	/*
	 * store the size of all chunks of compressed data in
	 * the first 4 bytes
	 */
	out_page = alloc_page(GFP_NOFS | __GFP_HIGHMEM);
	if (out_page == NULL) {
		ret = -ENOMEM;
		goto out;
	}
	cpage_out = kmap(out_page);
	out_offset = LZO_LEN;
	tot_out = LZO_LEN;
	pages[0] = out_page;
	nr_pages = 1;
	pg_bytes_left = PAGE_CACHE_SIZE - LZO_LEN;

	/* compress at most one page of data each time */
	in_len = min(len, PAGE_CACHE_SIZE);
	while (tot_in < len) {
		ret = lzo1x_1_compress(data_in, in_len, workspace->cbuf,
				       &out_len, workspace->mem);
		if (ret != LZO_E_OK) {
			printk(KERN_DEBUG "BTRFS: deflate in loop returned %d\n",
			       ret);
			ret = -EIO;
			goto out;
		}

		/* store the size of this chunk of compressed data */
		write_compress_length(cpage_out + out_offset, out_len);
		tot_out += LZO_LEN;
		out_offset += LZO_LEN;
		pg_bytes_left -= LZO_LEN;

		tot_in += in_len;
		tot_out += out_len;

		/* copy bytes from the working buffer into the pages */
		buf = workspace->cbuf;
		while (out_len) {
			bytes = min_t(unsigned long, pg_bytes_left, out_len);

			memcpy(cpage_out + out_offset, buf, bytes);

			out_len -= bytes;
			pg_bytes_left -= bytes;
			buf += bytes;
			out_offset += bytes;

			/*
			 * we need another page for writing out.
			 *
			 * Note if there's less than 4 bytes left, we just
			 * skip to a new page.
			 */
			if ((out_len == 0 && pg_bytes_left < LZO_LEN) ||
			    pg_bytes_left == 0) {
				if (pg_bytes_left) {
					memset(cpage_out + out_offset, 0,
					       pg_bytes_left);
					tot_out += pg_bytes_left;
				}

				/* we're done, don't allocate new page */
				if (out_len == 0 && tot_in >= len)
					break;

				kunmap(out_page);
				if (nr_pages == nr_dest_pages) {
					out_page = NULL;
					ret = -E2BIG;
					goto out;
				}

				out_page = alloc_page(GFP_NOFS | __GFP_HIGHMEM);
				if (out_page == NULL) {
					ret = -ENOMEM;
					goto out;
				}
				cpage_out = kmap(out_page);
				pages[nr_pages++] = out_page;

				pg_bytes_left = PAGE_CACHE_SIZE;
				out_offset = 0;
			}
		}

		/* we're making it bigger, give up */
		if (tot_in > 8192 && tot_in < tot_out) {
			ret = -E2BIG;
			goto out;
		}

		/* we're all done */
		if (tot_in >= len)
			break;

		if (tot_out > max_out)
			break;

		bytes_left = len - tot_in;
		kunmap(in_page);
		page_cache_release(in_page);

		start += PAGE_CACHE_SIZE;
		in_page = find_get_page(mapping, start >> PAGE_CACHE_SHIFT);
		data_in = kmap(in_page);
		in_len = min(bytes_left, PAGE_CACHE_SIZE);
	}

	if (tot_out > tot_in)
		goto out;

	/* store the size of all chunks of compressed data */
	cpage_out = kmap(pages[0]);
	write_compress_length(cpage_out, tot_out);

	kunmap(pages[0]);

	ret = 0;
	*total_out = tot_out;
	*total_in = tot_in;
out:
	*out_pages = nr_pages;
	if (out_page)
		kunmap(out_page);

	if (in_page) {
		kunmap(in_page);
		page_cache_release(in_page);
	}

	return ret;
}

static int lzo_decompress_biovec(struct list_head *ws,
				 struct page **pages_in,
				 u64 disk_start,
				 struct bio_vec *bvec,
				 int vcnt,
				 size_t srclen)
{
	struct workspace *workspace = list_entry(ws, struct workspace, list);
	int ret = 0, ret2;
	char *data_in;
	unsigned long page_in_index = 0;
	unsigned long page_out_index = 0;
	unsigned long total_pages_in = DIV_ROUND_UP(srclen, PAGE_CACHE_SIZE);
	unsigned long buf_start;
	unsigned long buf_offset = 0;
	unsigned long bytes;
	unsigned long working_bytes;
	unsigned long pg_offset;

	size_t in_len;
	size_t out_len;
	unsigned long in_offset;
	unsigned long in_page_bytes_left;
	unsigned long tot_in;
	unsigned long tot_out;
	unsigned long tot_len;
	char *buf;
	bool may_late_unmap, need_unmap;

	data_in = kmap(pages_in[0]);
	tot_len = read_compress_length(data_in);

	tot_in = LZO_LEN;
	in_offset = LZO_LEN;
	tot_len = min_t(size_t, srclen, tot_len);
	in_page_bytes_left = PAGE_CACHE_SIZE - LZO_LEN;

	tot_out = 0;
	pg_offset = 0;

	while (tot_in < tot_len) {
		in_len = read_compress_length(data_in + in_offset);
		in_page_bytes_left -= LZO_LEN;
		in_offset += LZO_LEN;
		tot_in += LZO_LEN;

		tot_in += in_len;
		working_bytes = in_len;
		may_late_unmap = need_unmap = false;

		/* fast path: avoid using the working buffer */
		if (in_page_bytes_left >= in_len) {
			buf = data_in + in_offset;
			bytes = in_len;
			may_late_unmap = true;
			goto cont;
		}

		/* copy bytes from the pages into the working buffer */
		buf = workspace->cbuf;
		buf_offset = 0;
		while (working_bytes) {
			bytes = min(working_bytes, in_page_bytes_left);

			memcpy(buf + buf_offset, data_in + in_offset, bytes);
			buf_offset += bytes;
cont:
			working_bytes -= bytes;
			in_page_bytes_left -= bytes;
			in_offset += bytes;

			/* check if we need to pick another page */
			if ((working_bytes == 0 && in_page_bytes_left < LZO_LEN)
			    || in_page_bytes_left == 0) {
				tot_in += in_page_bytes_left;

				if (working_bytes == 0 && tot_in >= tot_len)
					break;

				if (page_in_index + 1 >= total_pages_in) {
					ret = -EIO;
					goto done;
				}

				if (may_late_unmap)
					need_unmap = true;
				else
					kunmap(pages_in[page_in_index]);

				data_in = kmap(pages_in[++page_in_index]);

				in_page_bytes_left = PAGE_CACHE_SIZE;
				in_offset = 0;
			}
		}

		out_len = lzo1x_worst_compress(PAGE_CACHE_SIZE);
		ret = lzo1x_decompress_safe(buf, in_len, workspace->buf,
					    &out_len);
		if (need_unmap)
			kunmap(pages_in[page_in_index - 1]);
		if (ret != LZO_E_OK) {
			printk(KERN_WARNING "BTRFS: decompress failed\n");
			ret = -EIO;
			break;
		}

		buf_start = tot_out;
		tot_out += out_len;

		ret2 = btrfs_decompress_buf2page(workspace->buf, buf_start,
						 tot_out, disk_start,
						 bvec, vcnt,
						 &page_out_index, &pg_offset);
		if (ret2 == 0)
			break;
	}
done:
	kunmap(pages_in[page_in_index]);
	if (!ret)
		btrfs_clear_biovec_end(bvec, vcnt, page_out_index, pg_offset);
	return ret;
}

static int lzo_decompress(struct list_head *ws, unsigned char *data_in,
			  struct page *dest_page,
			  unsigned long start_byte,
			  size_t srclen, size_t destlen)
{
	struct workspace *workspace = list_entry(ws, struct workspace, list);
	size_t in_len;
	size_t out_len;
	size_t tot_len;
	int ret = 0;
	char *kaddr;
	unsigned long bytes;

	BUG_ON(srclen < LZO_LEN);

	tot_len = read_compress_length(data_in);
	data_in += LZO_LEN;

	in_len = read_compress_length(data_in);
	data_in += LZO_LEN;

	out_len = PAGE_CACHE_SIZE;
	ret = lzo1x_decompress_safe(data_in, in_len, workspace->buf, &out_len);
	if (ret != LZO_E_OK) {
		printk(KERN_WARNING "BTRFS: decompress failed!\n");
		ret = -EIO;
		goto out;
	}

	if (out_len < start_byte) {
		ret = -EIO;
		goto out;
	}

	/*
	 * the caller is already checking against PAGE_SIZE, but lets
	 * move this check closer to the memcpy/memset
	 */
	destlen = min_t(unsigned long, destlen, PAGE_SIZE);
	bytes = min_t(unsigned long, destlen, out_len - start_byte);

	kaddr = kmap_atomic(dest_page);
	memcpy(kaddr, workspace->buf + start_byte, bytes);

	/*
	 * btrfs_getblock is doing a zero on the tail of the page too,
	 * but this will cover anything missing from the decompressed
	 * data.
	 */
	if (bytes < destlen)
		memset(kaddr+bytes, 0, destlen-bytes);
	kunmap_atomic(kaddr);
out:
	return ret;
}

const struct btrfs_compress_op btrfs_lzo_compress = {
	.alloc_workspace	= lzo_alloc_workspace,
	.free_workspace		= lzo_free_workspace,
	.compress_pages		= lzo_compress_pages,
	.decompress_biovec	= lzo_decompress_biovec,
	.decompress		= lzo_decompress,
};
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               