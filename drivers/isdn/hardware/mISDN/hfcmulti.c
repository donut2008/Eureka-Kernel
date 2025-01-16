 * Since we're under a transaction reserve_metadata_bytes could
		 * try to commit the transaction which will make it return
		 * EAGAIN to make us stop the transaction we have, so return
		 * ENOSPC instead so that btrfs_dirty_inode knows what to do.
		 */
		if (ret == -EAGAIN)
			ret = -ENOSPC;
		if (!ret) {
			node->bytes_reserved = num_bytes;
			trace_btrfs_space_reservation(root->fs_info,
						      "delayed_inode",
						      btrfs_ino(inode),
						      num_bytes, 1);
		}
		return ret;
	} else if (src_rsv->type == BTRFS_BLOCK_RSV_DELALLOC) {
		spin_lock(&BTRFS_I(inode)->lock);
		if (test_and_clear_bit(BTRFS_INODE_DELALLOC_META_RESERVED,
				       &BTRFS_I(inode)->runtime_flags)) {
			spin_unlock(&BTRFS_I(inode)->lock);
			release = true;
			goto migrate;
		}
		spin_unlock(&BTRFS_I(inode)->lock);

		/* Ok we didn't have space pre-reserved.  This shouldn't happen
		 * too often but it can happen if we do delalloc to an existing
		 * inode which gets dirtied because of the time update, and then
		 * isn't touched again until after the transaction commits and
		 * then we try to write out the data.  First try to be nice and
		 * reserve something strictly for us.  If not be a pain and try
		 * to steal from the delalloc block rsv.
		 */
		ret = btrfs_block_rsv_add(root, dst_rsv, num_bytes,
					  BTRFS_RESERVE_NO_FLUSH);
		if (!ret)
			goto out;

		ret = btrfs_block_rsv_migrate(src_rsv, dst_rsv, num_bytes);
		if (!WARN_ON(ret))
			goto out;

		/*
		 * Ok this is a problem, let's just steal from the global rsv
		 * since this really shouldn't happen that often.
		 */
		ret = btrfs_block_rsv_migrate(&root->fs_info->global_block_rsv,
					      dst_rsv, num_bytes);
		goto out;
	}

migrate:
	ret = btrfs_block_rsv_migrate(src_rsv, dst_rsv, num_bytes);

out:
	/*
	 * Migrate only takes a reservation, it doesn't touch the size of the
	 * block_rsv.  This is to simplify people who don't normally have things
	 * migrated from their block rsv.  If they go to release their
	 * reservation, that will decrease the size as well, so if migrate
	 * reduced size we'd end up with a negative size.  But for the
	 * delalloc_meta_reserved stuff we will only know to drop 1 reservation,
	 * but we could in fact do this reserve/migrate dance several times
	 * between the time we did the original reservation and we'd clean it
	 * up.  So to take care of this, release the space for the meta
	 * reservation here.  I think it may be time for a documentation page on
	 * how block rsvs. work.
	 */
	if (!ret) {
		trace_btrfs_space_reservation(root->fs_info, "delayed_inode",
					      btrfs_ino(inode), num_bytes, 1);
		node->bytes_reserved = num_bytes;
	}

	if (release) {
		trace_btrfs_space_reservation(root->fs_info, "delalloc",
					      btrfs_ino(inode), num_bytes, 0);
		btrfs_block_rsv_release(root, src_rsv, num_bytes);
	}

	return ret;
}

static void btrfs_delayed_inode_release_metadata(struct btrfs_root *root,
						struct btrfs_delayed_node *node)
{
	struct btrfs_block_rsv *rsv;

	if (!node->bytes_reserved)
		return;

	rsv = &root->fs_info->delayed_block_rsv;
	trace_btrfs_space_reservation(root->fs_info, "delayed_inode",
				      node->inode_id, node->bytes_reserved, 0);
	btrfs_block_rsv_release(root, rsv,
				node->bytes_reserved);
	node->bytes_reserved = 0;
}

/*
 * This helper will insert some continuous items into the same leaf according
 * to the free space of the leaf.
 */
static int btrfs_batch_insert_items(struct btrfs_root *root,
				    struct btrfs_path *path,
				    struct btrfs_delayed_item *item)
{
	struct btrfs_delayed_item *curr, *next;
	int free_space;
	int total_data_size = 0, total_size = 0;
	struct extent_buffer *leaf;
	char *data_ptr;
	struct btrfs_key *keys;
	u32 *data_size;
	struct list_head head;
	int slot;
	int nitems;
	int i;
	int ret = 0;

	BUG_ON(!path->nodes[0]);

	leaf = path->nodes[0];
	free_space = btrfs_leaf_free_space(root, leaf);
	INIT_LIST_HEAD(&head);

	next = item;
	nitems = 0;

	/*
	 * count the number of the continuous items that we can insert in batch
	 */
	while (total_size + next->data_len + sizeof(struct btrfs_item) <=
	       free_space) {
		total_data_size += next->data_len;
		total_size += next->data_len + sizeof(struct btrfs_item);
		list_add_tail(&next->tree_list, &head);
		nitems++;

		curr = next;
		next = __btrfs_next_delayed_item(curr);
		if (!next)
			break;

		if (!btrfs_is_continuous_delayed_item(curr, next))
			break;
	}

	if (!nitems) {
		ret = 0;
		goto out;
	}

	/*
	 * we need allocate some memory space, but it might cause the task
	 * to sleep, so we set all locked nodes in the path to blocking locks
	 * first.
	 */
	btrfs_set_path_blocking(path);

	keys = kmalloc_array(nitems, sizeof(struct btrfs_key), GFP_NOFS);
	if (!keys) {
		ret = -ENOMEM;
		goto out;
	}

	data_size = kmalloc_array(nitems, sizeof(u32), GFP_NOFS);
	if (!data_size) {
		ret = -ENOMEM;
		goto error;
	}

	/* get keys of all the delayed items */
	i = 0;
	list_for_each_entry(next, &head, tree_list) {
		keys[i] = next->key;
		data_size[i] = next->data_len;
		i++;
	}

	/* reset all the locked nodes in the patch to spinning locks. */
	btrfs_clear_path_blocking(path, NULL, 0);

	/* insert the keys of the items */
	setup_items_for_insert(root, path, keys, data_size,
			       total_data_size, total_size, nitems);

	/* insert the dir index items */
	slot = path->slots[0];
	list_for_each_entry_safe(curr, next, &head, tree_list) {
		data_ptr = btrfs_item_ptr(leaf, slot, char);
		write_extent_buffer(leaf, &curr->data,
				    (unsigned long)data_ptr,
				    curr->data_len);
		slot++;

		btrfs_delayed_item_release_metadata(root, curr);

		list_del(&curr->tree_list);
		btrfs_release_delayed_item(curr);
	}

error:
	kfree(data_size);
	kfree(keys);
out:
	return ret;
}

/*
 * This helper can just do simple insertion that needn't extend item for new
 * data, such as directory name index insertion, inode insertion.
 */
static int btrfs_insert_delayed_item(struct btrfs_trans_handle *trans,
				     struct btrfs_root *root,
				     struct btrfs_path *path,
				     struct btrfs_delayed_item *delayed_item)
{
	struct extent_buffer *leaf;
	char *ptr;
	int ret;

	ret = btrfs_insert_empty_item(trans, root, path, &delayed_item->key,
				      delayed_item->data_len);
	if (ret < 0 && ret != -EEXIST)
		return ret;

	leaf = path->nodes[0];

	ptr = btrfs_item_ptr(leaf, path->slots[0], char);

	write_extent_buffer(leaf, delayed_item->data, (unsigned long)ptr,
			    delayed_item->data_len);
	btrfs_mark_buffer_dirty(leaf);

	btrfs_delayed_item_release_metadata(root, delayed_item);
	return 0;
}

/*
 * we insert an item first, then if there are some continuous items, we try
 * to insert those items into the same leaf.
 */
static int btrfs_insert_delayed_items(struct btrfs_trans_handle *trans,
				      struct btrfs_path *path,
				      struct btrfs_root *root,
				      struct btrfs_delayed_node *node)
{
	struct btrfs_delayed_item *curr, *prev;
	int ret = 0;

do_again:
	mutex_lock(&node->mutex);
	curr = __btrfs_first_delayed_insertion_item(node);
	if (!curr)
		goto insert_end;

	ret = btrfs_insert_delayed_item(trans, root, path, curr);
	if (ret < 0) {
		btrfs_release_path(path);
		goto insert_end;
	}

	prev = curr;
	curr = __btrfs_next_delayed_item(prev);
	if (curr && btrfs_is_continuous_delayed_item(prev, curr)) {
		/* insert the continuous items into the same leaf */
		path->slots[0]++;
		btrfs_batch_insert_items(root, path, curr);
	}
	btrfs_release_delayed_item(prev);
	btrfs_mark_buffer_dirty(path->nodes[0]);

	btrfs_release_path(path);
	mutex_unlock(&node->mutex);
	goto do_again;

insert_end:
	mutex_unlock(&node->mutex);
	return ret;
}

static int btrfs_batch_delete_items(struct btrfs_trans_handle *trans,
				    struct btrfs_root *root,
				    struct btrfs_path *path,
				    struct btrfs_delayed_item *item)
{
	struct btrfs_delayed_item *curr, *next;
	struct extent_buffer *leaf;
	struct btrfs_key key;
	struct list_head head;
	int nitems, i, last_item;
	int ret = 0;

	BUG_ON(!path->nodes[0]);

	leaf = path->nodes[0];

	i = path->slots[0];
	last_item = btrfs_header_nritems(leaf) - 1;
	if (i > last_item)
		return -ENOENT;	/* FIXME: Is errno suitable? */

	next = item;
	INIT_LIST_HEAD(&head);
	btrfs_item_key_to_cpu(leaf, &key, i);
	nitems = 0;
	/*
	 * count the number of the dir index items that we can delete in batch
	 */
	while (btrfs_comp_cpu_keys(&next->key, &key) == 0) {
		list_add_tail(&next->tree_list, &head);
		nitems++;

		curr = next;
		next = __btrfs_next_delayed_item(curr);
		if (!next)
			break;

		if (!btrfs_is_continuous_delayed_item(curr, next))
			break;

		i++;
		if (i > last_item)
			break;
		btrfs_item_key_to_cpu(leaf, &key, i);
	}

	if (!nitems)
		return 0;

	ret = btrfs_del_items(trans, root, path, path->slots[0], nitems);
	if (ret)
		goto out;

	list_for_each_entry_safe(curr, next, &head, tree_list) {
		btrfs_delayed_item_release_metadata(root, curr);
		list_del(&curr->tree_list);
		btrfs_release_delayed_item(curr);
	}

out:
	return ret;
}

static int btrfs_delete_delayed_items(struct btrfs_trans_handle *trans,
				      struct btrfs_path *path,
				      struct btrfs_root *root,
				      struct btrfs_delayed_node *node)
{
	struct btrfs_delayed_item *curr, *prev;
	int ret = 0;

do_again:
	mutex_lock(&node->mutex);
	curr = __btrfs_first_delayed_deletion_item(node);
	if (!curr)
		goto delete_fail;

	ret = btrfs_search_slot(trans, root, &curr->key, path, -1, 1);
	if (ret < 0)
		goto delete_fail;
	else if (ret > 0) {
		/*
		 * can't find the item which the node points to, so this node
		 * is invalid, just drop it.
		 */
		prev = curr;
		curr = __btrfs_next_delayed_item(prev);
		btrfs_release_delayed_item(prev);
		ret = 0;
		btrfs_release_path(path);
		if (curr) {
			mutex_unlock(&node->mutex);
			goto do_again;
		} else
			goto delete_fail;
	}

	btrfs_batch_delete_items(trans, root, path, curr);
	btrfs_release_path(path);
	mutex_unlock(&node->mutex);
	goto do_again;

delete_fail:
	btrfs_release_path(path);
	mutex_unlock(&node->mutex);
	return ret;
}

static void btrfs_release_delayed_inode(struct btrfs_delayed_node *delayed_node)
{
	struct btrfs_delayed_root *delayed_root;

	if (delayed_node &&
	    test_bit(BTRFS_DELAYED_NODE_INODE_DIRTY, &delayed_node->flags)) {
		BUG_ON(!delayed_node->root);
		clear_bit(BTRFS_DELAYED_NODE_INODE_DIRTY, &delayed_node->flags);
		delayed_node->count--;

		delayed_root = delayed_node->root->fs_info->delayed_root;
		finish_one_item(delayed_root);
	}
}

static void btrfs_release_delayed_iref(struct btrfs_delayed_node *delayed_node)
{
	struct btrfs_delayed_root *delayed_root;

	ASSERT(delayed_node->root);
	clear_bit(BTRFS_DELAYED_NODE_DEL_IREF, &delayed_node->flags);
	delayed_node->count--;

	delayed_root = delayed_node->root->fs_info->delayed_root;
	finish_one_item(delayed_root);
}

static int __btrfs_update_delayed_inode(struct btrfs_trans_handle *trans,
					struct btrfs_root *root,
					struct btrfs_path *path,
					struct btrfs_delayed_node *node)
{
	struct btrfs_key key;
	struct btrfs_inode_item *inode_item;
	struct extent_buffer *leaf;
	int mod;
	int ret;

	key.objectid = node->inode_id;
	key.type = BTRFS_INODE_ITEM_KEY;
	key.offset = 0;

	if (test_bit(BTRFS_DELAYED_NODE_DEL_IREF, &node->flags))
		mod = -1;
	else
		mod = 1;

	ret = btrfs_lookup_inode(trans, root, path, &key, mod);
	if (ret > 0) {
		btrfs_release_path(path);
		return -ENOENT;
	} else if (ret < 0) {
		return ret;
	}

	leaf = path->nodes[0];
	inode_item = btrfs_item_ptr(leaf, path->slots[0],
				    struct btrfs_inode_item);
	write_extent_buffer(leaf, &node->inode_item, (unsigned long)inode_item,
			    sizeof(struct btrfs_inode_item));
	btrfs_mark_buffer_dirty(leaf);

	if (!test_bit(BTRFS_DELAYED_NODE_DEL_IREF, &node->flags))
		goto no_iref;

	path->slots[0]++;
	if (path->slots[0] >= btrfs_header_nritems(leaf))
		goto search;
again:
	btrfs_item_key_to_cpu(leaf, &key, path->slots[0]);
	if (key.objectid != node->inode_id)
		goto out;

	if (key.type != BTRFS_INODE_REF_KEY &&
	    key.type != BTRFS_INODE_EXTREF_KEY)
		goto out;

	/*
	 * Delayed iref deletion is for the inode who has only one link,
	 * so there is only one iref. The case that several irefs are
	 * in the same item doesn't exist.
	 */
	btrfs_del_item(trans, root, path);
out:
	btrfs_release_delayed_iref(node);
no_iref:
	btrfs_release_path(path);
err_out:
	btrfs_delayed_inode_release_metadata(root, node);
	btrfs_release_delayed_inode(node);

	return ret;

search:
	btrfs_release_path(path);

	key.type = BTRFS_INODE_EXTREF_KEY;
	key.offset = -1;
	ret = btrfs_search_slot(trans, root, &key, path, -1, 1);
	if (ret < 0)
		goto err_out;
	ASSERT(ret);

	ret = 0;
	leaf = path->nodes[0];
	path->slots[0]--;
	goto again;
}

static inline int btrfs_update_delayed_inode(struct btrfs_trans_handle *trans,
					     struct btrfs_root *root,
					     struct btrfs_path *path,
					     struct btrfs_delayed_node *node)
{
	int ret;

	mutex_lock(&node->mutex);
	if (!test_bit(BTRFS_DELAYED_NODE_INODE_DIRTY, &node->flags)) {
		mutex_unlock(&node->mutex);
		return 0;
	}

	ret = __btrfs_update_delayed_inode(trans, root, path, node);
	mutex_unlock(&node->mutex);
	return ret;
}

static inline int
__btrfs_commit_inode_delayed_items(struct btrfs_trans_handle *trans,
				   struct btrfs_path *path,
				   struct btrfs_delayed_node *node)
{
	int ret;

	ret = btrfs_insert_delayed_items(trans, path, node->root, node);
	if (ret)
		return ret;

	ret = btrfs_delete_delayed_items(trans, path, node->root, node);
	if (ret)
		return ret;

	ret = btrfs_update_delayed_inode(trans, node->root, path, node);
	return ret;
}

/*
 * Called when committing the transaction.
 * Returns 0 on success.
 * Returns < 0 on error and returns with an aborted transaction with any
 * outstanding delayed items cleaned up.
 */
static int __btrfs_run_delayed_items(struct btrfs_trans_handle *trans,
				     struct btrfs_root *root, int nr)
{
	struct btrfs_delayed_root *delayed_root;
	struct btrfs_delayed_node *curr_node, *prev_node;
	struct btrfs_path *path;
	struct btrfs_block_rsv *block_rsv;
	int ret = 0;
	bool count = (nr > 0);

	if (trans->aborted)
		return -EIO;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;
	path->leave_spinning = 1;

	block_rsv = trans->block_rsv;
	trans->block_rsv = &root->fs_info->delayed_block_rsv;

	delayed_root = btrfs_get_delayed_root(root);

	curr_node = btrfs_first_delayed_node(delayed_root);
	while (curr_node && (!count || (count && nr--))) {
		ret = __btrfs_commit_inode_delayed_items(trans, path,
							 curr_node);
		if (ret) {
			btrfs_abort_transaction(trans, root, ret);
			break;
		}

		prev_node = curr_node;
		curr_node = btrfs_next_delayed_node(curr_node);
		/*
		 * See the comment below about releasing path before releasing
		 * node. If the commit of delayed items was successful the path
		 * should always be released, but in case of an error, it may
		 * point to locked extent buffers (a leaf at the very least).
		 */
		ASSERT(path->nodes[0] == NULL);
		btrfs_release_delayed_node(prev_node);
	}

	/*
	 * Release the path to avoid a potential deadlock and lockdep splat when
	 * releasing the delayed node, as that requires taking the delayed node's
	 * mutex. If another task starts running delayed items before we take
	 * the mutex, it will first lock the mutex and then it may try to lock
	 * the same btree path (leaf).
	 */
	btrfs_free_path(path);

	if (curr_node)
		btrfs_release_delayed_node(curr_node);
	trans->block_rsv = block_rsv;

	return ret;
}

int btrfs_run_delayed_items(struct btrfs_trans_handle *trans,
			    struct btrfs_root *root)
{
	return __btrfs_run_delayed_items(trans, root, -1);
}

int btrfs_run_delayed_items_nr(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root, int nr)
{
	return __btrfs_run_delayed_items(trans, root, nr);
}

int btrfs_commit_inode_delayed_items(struct btrfs_trans_handle *trans,
				     struct inode *inode)
{
	struct btrfs_delayed_node *delayed_node = btrfs_get_delayed_node(inode);
	struct btrfs_path *path;
	struct btrfs_block_rsv *block_rsv;
	int ret;

	if (!delayed_node)
		return 0;

	mutex_lock(&delayed_node->mutex);
	if (!delayed_node->count) {
		mutex_unlock(&delayed_node->mutex);
		btrfs_release_delayed_node(delayed_node);
		return 0;
	}
	mutex_unlock(&delayed_node->mutex);

	path = btrfs_alloc_path();
	if (!path) {
		btrfs_release_delayed_node(delayed_node);
		return -ENOMEM;
	}
	path->leave_spinning = 1;

	block_rsv = trans->block_rsv;
	trans->block_rsv = &delayed_node->root->fs_info->delayed_block_rsv;

	ret = __btrfs_commit_inode_delayed_items(trans, path, delayed_node);

	btrfs_release_delayed_node(delayed_node);
	btrfs_free_path(path);
	trans->block_rsv = block_rsv;

	return ret;
}

int btrfs_commit_inode_delayed_inode(struct inode *inode)
{
	struct btrfs_trans_handle *trans;
	struct btrfs_delayed_node *delayed_node = btrfs_get_delayed_node(inode);
	struct btrfs_path *path;
	struct btrfs_block_rsv *block_rsv;
	int ret;

	if (!delayed_node)
		return 0;

	mutex_lock(&delayed_node->mutex);
	if (!test_bit(BTRFS_DELAYED_NODE_INODE_DIRTY, &delayed_node->flags)) {
		mutex_unlock(&delayed_node->mutex);
		btrfs_release_delayed_node(delayed_node);
		return 0;
	}
	mutex_unlock(&delayed_node->mutex);

	trans = btrfs_join_transaction(delayed_node->root);
	if (IS_ERR(trans)) {
		ret = PTR_ERR(trans);
		goto out;
	}

	path = btrfs_alloc_path();
	if (!path) {
		ret = -ENOMEM;
		goto trans_out;
	}
	path->leave_spinning = 1;

	block_rsv = trans->block_rsv;
	trans->block_rsv = &delayed_node->root->fs_info->delayed_block_rsv;

	mutex_lock(&delayed_node->mutex);
	if (test_bit(BTRFS_DELAYED_NODE_INODE_DIRTY, &delayed_node->flags))
		ret = __btrfs_update_delayed_inode(trans, delayed_node->root,
						   path, delayed_node);
	else
		ret = 0;
	mutex_unlock(&delayed_node->mutex);

	btrfs_free_path(path);
	trans->block_rsv = block_rsv;
trans_out:
	btrfs_end_transaction(trans, delayed_node->root);
	btrfs_btree_balance_dirty(delayed_node->root);
out:
	btrfs_release_delayed_node(delayed_node);

	return ret;
}

void btrfs_remove_delayed_node(struct inode *inode)
{
	struct btrfs_delayed_node *delayed_node;

	delayed_node = ACCESS_ONCE(BTRFS_I(inode)->delayed_node);
	if (!delayed_node)
		return;

	BTRFS_I(inode)->delayed_node = NULL;
	btrfs_release_delayed_node(delayed_node);
}

struct btrfs_async_delayed_work {
	struct btrfs_delayed_root *delayed_root;
	int nr;
	struct btrfs_work work;
};

static void btrfs_async_run_delayed_root(struct btrfs_work *work)
{
	struct btrfs_async_delayed_work *async_work;
	struct btrfs_delayed_root *delayed_root;
	struct btrfs_trans_handle *trans;
	struct btrfs_path *path;
	struct btrfs_delayed_node *delayed_node = NULL;
	struct btrfs_root *root;
	struct btrfs_block_rsv *block_rsv;
	int total_done = 0;

	async_work = container_of(work, struct btrfs_async_delayed_work, work);
	delayed_root = async_work->delayed_root;

	path = btrfs_alloc_path();
	if (!path)
		goto out;

again:
	if (atomic_read(&delayed_root->items) < BTRFS_DELAYED_BACKGROUND / 2)
		goto free_path;

	delayed_node = btrfs_first_prepared_delayed_node(delayed_root);
	if (!delayed_node)
		goto free_path;

	path->leave_spinning = 1;
	root = delayed_node->root;

	trans = btrfs_join_transaction(root);
	if (IS_ERR(trans))
		goto release_path;

	block_rsv = trans->block_rsv;
	trans->block_rsv = &root->fs_info->delayed_block_rsv;

	__btrfs_commit_inode_delayed_items(trans, path, delayed_node);

	trans->block_rsv = block_rsv;
	btrfs_end_transaction(trans, root);
	btrfs_btree_balance_dirty_nodelay(root);

release_path:
	btrfs_release_path(path);
	total_done++;

	btrfs_release_prepared_delayed_node(delayed_node);
	if ((async_work->nr == 0 && total_done < BTRFS_DELAYED_WRITEBACK) ||
	    total_done < async_work->nr)
		goto again;

free_path:
	btrfs_free_path(path);
out:
	wake_up(&delayed_root->wait);
	kfree(async_work);
}


static int btrfs_wq_run_delayed_node(struct btrfs_delayed_root *delayed_root,
				     struct btrfs_fs_info *fs_info, int nr)
{
	struct btrfs_async_delayed_work *async_work;

	if (atomic_read(&delayed_root->items) < BTRFS_DELAYED_BACKGROUND ||
	    btrfs_workqueue_normal_congested(fs_info->delayed_workers))
		return 0;

	async_work = kmalloc(sizeof(*async_work), GFP_NOFS);
	if (!async_work)
		return -ENOMEM;

	async_work->delayed_root = delayed_root;
	btrfs_init_work(&async_work->work, btrfs_delayed_meta_helper,
			btrfs_async_run_delayed_root, NULL, NULL);
	async_work->nr = nr;

	btrfs_queue_work(fs_info->delayed_workers, &async_work->work);
	return 0;
}

void btrfs_assert_delayed_root_empty(struct btrfs_root *root)
{
	struct btrfs_delayed_root *delayed_root;
	delayed_root = btrfs_get_delayed_root(root);
	WARN_ON(btrfs_first_delayed_node(delayed_root));
}

static int could_end_wait(struct btrfs_delayed_root *delayed_root, int seq)
{
	int val = atomic_read(&delayed_root->items_seq);

	if (val < seq || val >= seq + BTRFS_DELAYED_BATCH)
		return 1;

	if (atomic_read(&delayed_root->items) < BTRFS_DELAYED_BACKGROUND)
		return 1;

	return 0;
}

void btrfs_balance_delayed_items(struct btrfs_root *root)
{
	struct btrfs_delayed_root *delayed_root;
	struct btrfs_fs_info *fs_info = root->fs_info;

	delayed_root = btrfs_get_delayed_root(root);

	if (atomic_read(&delayed_root->items) < BTRFS_DELAYED_BACKGROUND)
		return;

	if (atomic_read(&delayed_root->items) >= BTRFS_DELAYED_WRITEBACK) {
		int seq;
		int ret;

		seq = atomic_read(&delayed_root->items_seq);

		ret = btrfs_wq_run_delayed_node(delayed_root, fs_info, 0);
		if (ret)
			return;

		wait_event_interruptible(delayed_root->wait,
					 could_end_wait(delayed_root, seq));
		return;
	}

	btrfs_wq_run_delayed_node(delayed_root, fs_info, BTRFS_DELAYED_BATCH);
}

/* Will return 0 or -ENOMEM */
int btrfs_insert_delayed_dir_index(struct btrfs_trans_handle *trans,
				   struct btrfs_root *root, const char *name,
				   int name_len, struct inode *dir,
				   struct btrfs_disk_key *disk_key, u8 type,
				   u64 index)
{
	struct btrfs_delayed_node *delayed_node;
	struct btrfs_delayed_item *delayed_item;
	struct btrfs_dir_item *dir_item;
	int ret;

	delayed_node = btrfs_get_or_create_delayed_node(dir);
	if (IS_ERR(delayed_node))
		return PTR_ERR(delayed_node);

	delayed_item = btrfs_alloc_delayed_item(sizeof(*dir_item) + name_len);
	if (!delayed_item) {
		ret = -ENOMEM;
		goto release_node;
	}

	delayed_item->key.objectid = btrfs_ino(dir);
	delayed_item->key.type = BTRFS_DIR_INDEX_KEY;
	delayed_item->key.offset = index;

	dir_item = (struct btrfs_dir_item *)delayed_item->data;
	dir_item->location = *disk_key;
	btrfs_set_stack_dir_transid(dir_item, trans->transid);
	btrfs_set_stack_dir_data_len(dir_item, 0);
	btrfs_set_stack_dir_name_len(dir_item, name_len);
	btrfs_set_stack_dir_type(dir_item, type);
	memcpy((char *)(dir_item + 1), name, name_len);

	ret = btrfs_delayed_item_reserve_metadata(trans, root, delayed_item);
	/*
	 * we have reserved enough space when we start a new transaction,
	 * so reserving metadata failure is impossible
	 */
	BUG_ON(ret);


	mutex_lock(&delayed_node->mutex);
	ret = __btrfs_add_delayed_insertion_item(delayed_node, delayed_item);
	if (unlikely(ret)) {
		btrfs_err(root->fs_info, "err add delayed dir index item(name: %.*s) "
				"into the insertion tree of the delayed node"
				"(root id: %llu, inode id: %llu, errno: %d)",
				name_len, name, delayed_node->root->objectid,
				delayed_node->inode_id, ret);
		BUG();
	}
	mutex_unlock(&delayed_node->mutex);

release_node:
	btrfs_release_delayed_node(delayed_node);
	return ret;
}

static int btrfs_delete_delayed_insertion_item(struct btrfs_root *root,
					       struct btrfs_delayed_node *node,
					       struct btrfs_key *key)
{
	struct btrfs_delayed_item *item;

	mutex_lock(&node->mutex);
	item = __btrfs_lookup_delayed_insertion_item(node, key);
	if (!item) {
		mutex_unlock(&node->mutex);
		return 1;
	}

	btrfs_delayed_item_release_metadata(root, item);
	btrfs_release_delayed_item(item);
	mutex_unlock(&node->mutex);
	return 0;
}

int btrfs_delete_delayed_dir_index(struct btrfs_trans_handle *trans,
				   struct btrfs_root *root, struct inode *dir,
				   u64 index)
{
	struct btrfs_delayed_node *node;
	struct btrfs_delayed_item *item;
	struct btrfs_key item_key;
	int ret;

	node = btrfs_get_or_create_delayed_node(dir);
	if (IS_ERR(node))
		return PTR_ERR(node);

	item_key.objectid = btrfs_ino(dir);
	item_key.type = BTRFS_DIR_INDEX_KEY;
	item_key.offset = index;

	ret = btrfs_delete_delayed_insertion_item(root, node, &item_key);
	if (!ret)
		goto end;

	item = btrfs_alloc_delayed_item(0);
	if (!item) {
		ret = -ENOMEM;
		goto end;
	}

	item->key = item_key;

	ret = btrfs_delayed_item_reserve_metadata(trans, root, item);
	/*
	 * we have reserved enough space when we start a new transaction,
	 * so reserving metadata failure is impossible.
	 */
	BUG_ON(ret);

	mutex_lock(&node->mutex);
	ret = __btrfs_add_delayed_deletion_item(node, item);
	if (unlikely(ret)) {
		btrfs_err(root->fs_info, "err add delayed dir index item(index: %llu) "
				"into the deletion tree of the delayed node"
				"(root id: %llu, inode id: %llu, errno: %d)",
				index, node->root->objectid, node->inode_id,
				ret);
		BUG();
	}
	mutex_unlock(&node->mutex);
end:
	btrfs_release_delayed_node(node);
	return ret;
}

int btrfs_inode_delayed_dir_index_count(struct inode *inode)
{
	struct btrfs_delayed_node *delayed_node = btrfs_get_delayed_node(inode);

	if (!delayed_node)
		return -ENOENT;

	/*
	 * Since we have held i_mutex of this directory, it is impossible that
	 * a new directory index is added into the delayed node and index_cnt
	 * is updated now. So we needn't lock the delayed node.
	 */
	if (!delayed_node->index_cnt) {
		btrfs_release_delayed_node(delayed_node);
		return -EINVAL;
	}

	BTRFS_I(inode)->index_cnt = delayed_node->index_cnt;
	btrfs_release_delayed_node(delayed_node);
	return 0;
}

void btrfs_get_delayed_items(struct inode *inode, struct list_head *ins_list,
			     struct list_head *del_list)
{
	struct btrfs_delayed_node *delayed_node;
	struct btrfs_delayed_item *item;

	delayed_node = btrfs_get_delayed_node(inode);
	if (!delayed_node)
		return;

	mutex_lock(&delayed_node->mutex);
	item = __btrfs_first_delayed_insertion_item(delayed_node);
	while (item) {
		atomic_inc(&item->refs);
		list_add_tail(&item->readdir_list, ins_list);
		item = __btrfs_next_delayed_item(item);
	}

	item = __btrfs_first_delayed_deletion_item(delayed_node);
	while (item) {
		atomic_inc(&item->refs);
		list_add_tail(&item->readdir_list, del_list);
		item = __btrfs_next_delayed_item(item);
	}
	mutex_unlock(&delayed_node->mutex);
	/*
	 * This delayed node is still cached in the btrfs inode, so refs
	 * must be > 1 now, and we needn't check it is going to be freed
	 * or not.
	 *
	 * Besides that, this function is used to read dir, we do not
	 * insert/delete delayed items in this period. So we also needn't
	 * requeue or dequeue this delayed node.
	 */
	atomic_dec(&delayed_node->refs);
}

void btrfs_put_delayed_items(struct list_head *ins_list,
			     struct list_head *del_list)
{
	struct btrfs_delayed_item *curr, *next;

	list_for_each_entry_safe(curr, next, ins_list, readdir_list) {
		list_del(&curr->readdir_list);
		if (atomic_dec_and_test(&curr->refs))
			kfree(curr);
	}

	list_for_each_entry_safe(curr, next, del_list, readdir_list) {
		list_del(&curr->readdir_list);
		if (atomic_dec_and_test(&curr->refs))
			kfree(curr);
	}
}

int btrfs_should_delete_dir_index(struct list_head *del_list,
				  u64 index)
{
	struct btrfs_delayed_item *curr, *next;
	int ret;

	if (list_empty(del_list))
		return 0;

	list_for_each_entry_safe(curr, next, del_list, readdir_list) {
		if (curr->key.offset > index)
			break;

		list_del(&curr->readdir_list);
		ret = (curr->key.offset == index);

		if (atomic_dec_and_test(&curr->refs))
			kfree(curr);

		if (ret)
			return 1;
		else
			continue;
	}
	return 0;
}

/*
 * btrfs_readdir_delayed_dir_index - read dir info stored in the delayed tree
 *
 */
int btrfs_readdir_delayed_dir_index(struct dir_context *ctx,
				    struct list_head *ins_list, bool *emitted)
{
	struct btrfs_dir_item *di;
	struct btrfs_delayed_item *curr, *next;
	struct btrfs_key location;
	char *name;
	int name_len;
	int over = 0;
	unsigned char d_type;

	if (list_empty(ins_list))
		return 0;

	/*
	 * Changing the data of the delayed item is impossible. So
	 * we needn't lock them. And we have held i_mutex of the
	 * directory, nobody can delete any directory indexes now.
	 */
	list_for_each_entry_safe(curr, next, ins_list, readdir_list) {
		list_del(&curr->readdir_list);

		if (curr->key.offset < ctx->pos) {
			if (atomic_dec_and_test(&curr->refs))
				kfree(curr);
			continue;
		}

		ctx->pos = curr->key.offset;

		di = (struct btrfs_dir_item *)curr->data;
		name = (char *)(di + 1);
		name_len = btrfs_stack_dir_name_len(di);

		d_type = btrfs_filetype_table[di->type];
		btrfs_disk_key_to_cpu(&location, &di->location);

		over = !dir_emit(ctx, name, name_len,
			       location.objectid, d_type);

		if (atomic_dec_and_test(&curr->refs))
			kfree(curr);

		if (over)
			return 1;
		*emitted = true;
	}
	return 0;
}

static void fill_stack_inode_item(struct btrfs_trans_handle *trans,
				  struct btrfs_inode_item *inode_item,
				  struct inode *inode)
{
	btrfs_set_stack_inode_uid(inode_item, i_uid_read(inode));
	btrfs_set_stack_inode_gid(inode_item, i_gid_read(inode));
	btrfs_set_stack_inode_size(inode_item, BTRFS_I(inode)->disk_i_size);
	btrfs_set_stack_inode_mode(inode_item, inode->i_mode);
	btrfs_set_stack_inode_nlink(inode_item, inode->i_nlink);
	btrfs_set_stack_inode_nbytes(inode_item, inode_get_bytes(inode));
	btrfs_set_stack_inode_generation(inode_item,
					 BTRFS_I(inode)->generation);
	btrfs_set_stack_inode_sequence(inode_item, inode->i_version);
	btrfs_set_stack_inode_transid(inode_item, trans->transid);
	btrfs_set_stack_inode_rdev(inode_item, inode->i_rdev);
	btrfs_set_stack_inode_flags(inode_item, BTRFS_I(inode)->flags);
	btrfs_set_stack_inode_block_group(inode_item, 0);

	btrfs_set_stack_timespec_sec(&inode_item->atime,
				     inode->i_atime.tv_sec);
	btrfs_set_stack_timespec_nsec(&inode_item->atime,
				      inode->i_atime.tv_nsec);

	btrfs_set_stack_timespec_sec(&inode_item->mtime,
				     inode->i_mtime.tv_sec);
	btrfs_set_stack_timespec_nsec(&inode_item->mtime,
				      inode->i_mtime.tv_nsec);

	btrfs_set_stack_timespec_sec(&inode_item->ctime,
				     inode->i_ctime.tv_sec);
	btrfs_set_stack_timespec_nsec(&inode_item->ctime,
				      inode->i_ctime.tv_nsec);

	btrfs_set_stack_timespec_sec(&inode_item->otime,
				     BTRFS_I(inode)->i_otime.tv_sec);
	btrfs_set_stack_timespec_nsec(&inode_item->otime,
				     BTRFS_I(inode)->i_otime.tv_nsec);
}

int btrfs_fill_inode(struct inode *inode, u32 *rdev)
{
	struct btrfs_delayed_node *delayed_node;
	struct btrfs_inode_item *inode_item;

	delayed_node = btrfs_get_delayed_node(inode);
	if (!delayed_node)
		return -ENOENT;

	mutex_lock(&delayed_node->mutex);
	if (!test_bit(BTRFS_DELAYED_NODE_INODE_DIRTY, &delayed_node->flags)) {
		mutex_unlock(&delayed_node->mutex);
		btrfs_release_delayed_node(delayed_node);
		return -ENOENT;
	}

	inode_item = &delayed_node->inode_item;

	i_uid_write(inode, btrfs_stack_inode_uid(inode_item));
	i_gid_write(inode, btrfs_stack_inode_gid(inode_item));
	btrfs_i_size_write(inode, btrfs_stack_inode_size(inode_item));
	inode->i_mode = btrfs_stack_inode_mode(inode_item);
	set_nlink(inode, btrfs_stack_inode_nlink(inode_item));
	inode_set_bytes(inode, btrfs_stack_inode_nbytes(inode_item));
	BTRFS_I(inode)->generation = btrfs_stack_inode_generation(inode_item);
        BTRFS_I(inode)->last_trans = btrfs_stack_inode_transid(inode_item);

	inode->i_version = btrfs_stack_inode_sequence(inode_item);
	inode->i_rdev = 0;
	*rdev = btrfs_stack_inode_rdev(inode_item);
	BTRFS_I(inode)->flags = btrfs_stack_inode_flags(inode_item);

	inode->i_atime.tv_sec = btrfs_stack_timespec_sec(&inode_item->atime);
	inode->i_atime.tv_nsec = btrfs_stack_timespec_nsec(&inode_item->atime);

	inode->i_mtime.tv_sec = btrfs_stack_timespec_sec(&inode_item->mtime);
	inode->i_mtime.tv_nsec = btrfs_stack_timespec_nsec(&inode_item->mtime);

	inode->i_ctime.tv_sec = btrfs_stack_timespec_sec(&inode_item->ctime);
	inode->i_ctime.tv_nsec = btrfs_stack_timespec_nsec(&inode_item->ctime);

	BTRFS_I(inode)->i_otime.tv_sec =
		btrfs_stack_timespec_sec(&inode_item->otime);
	BTRFS_I(inode)->i_otime.tv_nsec =
		btrfs_stack_timespec_nsec(&inode_item->otime);

	inode->i_generation = BTRFS_I(inode)->generation;
	BTRFS_I(inode)->index_cnt = (u64)-1;

	mutex_unlock(&delayed_node->mutex);
	btrfs_release_delayed_node(delayed_node);
	return 0;
}

int btrfs_delayed_update_inode(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root, struct inode *inode)
{
	struct btrfs_delayed_node *delayed_node;
	int ret = 0;

	delayed_node = btrfs_get_or_create_delayed_node(inode);
	if (IS_ERR(delayed_node))
		return PTR_ERR(delayed_node);

	mutex_lock(&delayed_node->mutex);
	if (test_bit(BTRFS_DELAYED_NODE_INODE_DIRTY, &delayed_node->flags)) {
		fill_stack_inode_item(trans, &delayed_node->inode_item, inode);
		goto release_node;
	}

	ret = btrfs_delayed_inode_reserve_metadata(trans, root, inode,
						   delayed_node);
	if (ret)
		goto release_node;

	fill_stack_inode_item(trans, &delayed_node->inode_item, inode);
	set_bit(BTRFS_DELAYED_NODE_INODE_DIRTY, &delayed_node->flags);
	delayed_node->count++;
	atomic_inc(&root->fs_info->delayed_root->items);
release_node:
	mutex_unlock(&delayed_node->mutex);
	btrfs_release_delayed_node(delayed_node);
	return ret;
}

int btrfs_delayed_delete_inode_ref(struct inode *inode)
{
	struct btrfs_delayed_node *delayed_node;

	/*
	 * we don't do delayed inode updates during log recovery because it
	 * leads to enospc problems.  This means we also can't do
	 * delayed inode refs
	 */
	if (BTRFS_I(inode)->root->fs_info->log_root_recovering)
		return -EAGAIN;

	delayed_node = btrfs_get_or_create_delayed_node(inode);
	if (IS_ERR(delayed_node))
		return PTR_ERR(delayed_node);

	/*
	 * We don't reserve space for inode ref deletion is because:
	 * - We ONLY do async inode ref deletion for the inode who has only
	 *   one link(i_nlink == 1), it means there is only one inode ref.
	 *   And in most case, the inode ref and the inode item are in the
	 *   same leaf, and we will deal with them at the same time.
	 *   Since we are sure we will reserve the space for the inode item,
	 *   it is unnecessary to reserve space for inode ref deletion.
	 * - If the inode ref and the inode item are not in the same leaf,
	 *   We also needn't worry about enospc problem, because we reserve
	 *   much more space for the inode update than it needs.
	 * - At the worst, we can steal some space from the global reservation.
	 *   It is very rare.
	 */
	mutex_lock(&delayed_node->mutex);
	if (test_bit(BTRFS_DELAYED_NODE_DEL_IREF, &delayed_node->flags))
		goto release_node;

	set_bit(BTRFS_DELAYED_NODE_DEL_IREF, &delayed_node->flags);
	delayed_node->count++;
	atomic_inc(&BTRFS_I(inode)->root->fs_info->delayed_root->items);
release_node:
	mutex_unlock(&delayed_node->mutex);
	btrfs_release_delayed_node(delayed_node);
	return 0;
}

static void __btrfs_kill_delayed_node(struct btrfs_delayed_node *delayed_node)
{
	struct btrfs_root *root = delayed_node->root;
	struct btrfs_delayed_item *curr_item, *prev_item;

	mutex_lock(&delayed_node->mutex);
	curr_item = __btrfs_first_delayed_insertion_item(delayed_node);
	while (curr_item) {
		btrfs_delayed_item_release_metadata(root, curr_item);
		prev_item = curr_item;
		curr_item = __btrfs_next_delayed_item(prev_item);
		btrfs_release_delayed_item(prev_item);
	}

	curr_item = __btrfs_first_delayed_deletion_item(delayed_node);
	while (curr_item) {
		btrfs_delayed_item_release_metadata(root, curr_item);
		prev_item = curr_item;
		curr_item = __btrfs_next_delayed_item(prev_item);
		btrfs_release_delayed_item(prev_item);
	}

	if (test_bit(BTRFS_DELAYED_NODE_DEL_IREF, &delayed_node->flags))
		btrfs_release_delayed_iref(delayed_node);

	if (test_bit(BTRFS_DELAYED_NODE_INODE_DIRTY, &delayed_node->flags)) {
		btrfs_delayed_inode_release_metadata(root, delayed_node);
		btrfs_release_delayed_inode(delayed_node);
	}
	mutex_unlock(&delayed_node->mutex);
}

void btrfs_kill_delayed_inode_items(struct inode *inode)
{
	struct btrfs_delayed_node *delayed_node;

	delayed_node = btrfs_get_delayed_node(inode);
	if (!delayed_node)
		return;

	__btrfs_kill_delayed_node(delayed_node);
	btrfs_release_delayed_node(delayed_node);
}

void btrfs_kill_all_delayed_nodes(struct btrfs_root *root)
{
	u64 inode_id = 0;
	struct btrfs_delayed_node *delayed_nodes[8];
	int i, n;

	while (1) {
		spin_lock(&root->inode_lock);
		n = radix_tree_gang_lookup(&root->delayed_nodes_tree,
					   (void **)delayed_nodes, inode_id,
					   ARRAY_SIZE(delayed_nodes));
		if (!n) {
			spin_unlock(&root->inode_lock);
			break;
		}

		inode_id = delayed_nodes[n - 1]->inode_id + 1;

		for (i = 0; i < n; i++)
			atomic_inc(&delayed_nodes[i]->refs);
		spin_unlock(&root->inode_lock);

		for (i = 0; i < n; i++) {
			__btrfs_kill_delayed_node(delayed_nodes[i]);
			btrfs_release_delayed_node(delayed_nodes[i]);
		}
	}
}

void btrfs_destroy_delayed_inodes(struct btrfs_root *root)
{
	struct btrfs_delayed_root *delayed_root;
	struct btrfs_delayed_node *curr_node, *prev_node;

	delayed_root = btrfs_get_delayed_root(root);

	curr_node = btrfs_first_delayed_node(delayed_root);
	while (curr_node) {
		__btrfs_kill_delayed_node(curr_node);

		prev_node = curr_node;
		curr_node = btrfs_next_delayed_node(curr_node);
		btrfs_release_delayed_node(prev_node);
	}
}

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /*
 * Copyright (C) 2011 Fujitsu.  All rights reserved.
 * Written by Miao Xie <miaox@cn.fujitsu.com>
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

#ifndef __DELAYED_TREE_OPERATION_H
#define __DELAYED_TREE_OPERATION_H

#include <linux/rbtree.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/atomic.h>

#include "ctree.h"

/* types of the delayed item */
#define BTRFS_DELAYED_INSERTION_ITEM	1
#define BTRFS_DELAYED_DELETION_ITEM	2

struct btrfs_delayed_root {
	spinlock_t lock;
	struct list_head node_list;
	/*
	 * Used for delayed nodes which is waiting to be dealt with by the
	 * worker. If the delayed node is inserted into the work queue, we
	 * drop it from this list.
	 */
	struct list_head prepare_list;
	atomic_t items;		/* for delayed items */
	atomic_t items_seq;	/* for delayed items */
	int nodes;		/* for delayed nodes */
	wait_queue_head_t wait;
};

#define BTRFS_DELAYED_NODE_IN_LIST	0
#define BTRFS_DELAYED_NODE_INODE_DIRTY	1
#define BTRFS_DELAYED_NODE_DEL_IREF	2

struct btrfs_delayed_node {
	u64 inode_id;
	u64 bytes_reserved;
	struct btrfs_root *root;
	/* Used to add the node into the delayed root's node list. */
	struct list_head n_list;
	/*
	 * Used to add the node into the prepare list, the nodes in this list
	 * is waiting to be dealt with by the async worker.
	 */
	struct list_head p_list;
	struct rb_root ins_root;
	struct rb_root del_root;
	struct mutex mutex;
	struct btrfs_inode_item inode_item;
	atomic_t refs;
	u64 index_cnt;
	unsigned long flags;
	int count;
};

struct btrfs_delayed_item {
	struct rb_node rb_node;
	struct btrfs_key key;
	struct list_head tree_list;	/* used for batch insert/delete items */
	struct list_head readdir_list;	/* used for readdir items */
	u64 bytes_reserved;
	struct btrfs_delayed_node *delayed_node;
	atomic_t refs;
	int ins_or_del;
	u32 data_len;
	char data[0];
};

static inline void btrfs_init_delayed_root(
				struct btrfs_delayed_root *delayed_root)
{
	atomic_set(&delayed_root->items, 0);
	atomic_set(&delayed_root->items_seq, 0);
	delayed_root->nodes = 0;
	spin_lock_init(&delayed_root->lock);
	init_waitqueue_head(&delayed_root->wait);
	INIT_LIST_HEAD(&delayed_root->node_list);
	INIT_LIST_HEAD(&delayed_root->prepare_list);
}

int btrfs_insert_delayed_dir_index(struct btrfs_trans_handle *trans,
				   struct btrfs_root *root, const char *name,
				   int name_len, struct inode *dir,
				   struct btrfs_disk_key *disk_key, u8 type,
				   u64 index);

int btrfs_delete_delayed_dir_index(struct btrfs_trans_handle *trans,
				   struct btrfs_root *root, struct inode *dir,
				   u64 index);

int btrfs_inode_delayed_dir_index_count(struct inode *inode);

int btrfs_run_delayed_items(struct btrfs_trans_handle *trans,
			    struct btrfs_root *root);
int btrfs_run_delayed_items_nr(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root, int nr);

void btrfs_balance_delayed_items(struct btrfs_root *root);

int btrfs_commit_inode_delayed_items(struct btrfs_trans_handle *trans,
				     struct inode *inode);
/* Used for evicting the inode. */
void btrfs_remove_delayed_node(struct inode *inode);
void btrfs_kill_delayed_inode_items(struct inode *inode);
int btrfs_commit_inode_delayed_inode(struct inode *inode);


int btrfs_delayed_update_inode(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root, struct inode *inode);
int btrfs_fill_inode(struct inode *inode, u32 *rdev);
int btrfs_delayed_delete_inode_ref(struct inode *inode);

/* Used for drop dead root */
void btrfs_kill_all_delayed_nodes(struct btrfs_root *root);

/* Used for clean the transaction */
void btrfs_destroy_delayed_inodes(struct btrfs_root *root);

/* Used for readdir() */
void btrfs_get_delayed_items(struct inode *inode, struct list_head *ins_list,
			     struct list_head *del_list);
void btrfs_put_delayed_items(struct list_head *ins_list,
			     struct list_head *del_list);
int btrfs_should_delete_dir_index(struct list_head *del_list,
				  u64 index);
int btrfs_readdir_delayed_dir_index(struct dir_context *ctx,
				    struct list_head *ins_list, bool *emitted);

/* for init */
int __init btrfs_delayed_inode_init(void);
void btrfs_delayed_inode_exit(void);

/* for debugging */
void btrfs_assert_delayed_root_empty(struct btrfs_root *root);

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
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

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include "ctree.h"
#include "delayed-ref.h"
#include "transaction.h"
#include "qgroup.h"

struct kmem_cache *btrfs_delayed_ref_head_cachep;
struct kmem_cache *btrfs_delayed_tree_ref_cachep;
struct kmem_cache *btrfs_delayed_data_ref_cachep;
struct kmem_cache *btrfs_delayed_extent_op_cachep;
/*
 * delayed back reference update tracking.  For subvolume trees
 * we queue up extent allocations and backref maintenance for
 * delayed processing.   This avoids deep call chains where we
 * add extents in the middle of btrfs_search_slot, and it allows
 * us to buffer up frequently modified backrefs in an rb tree instead
 * of hammering updates on the extent allocation tree.
 */

/*
 * compare two delayed tree backrefs with same bytenr and type
 */
static int comp_tree_refs(struct btrfs_delayed_tree_ref *ref2,
			  struct btrfs_delayed_tree_ref *ref1, int type)
{
	if (type == BTRFS_TREE_BLOCK_REF_KEY) {
		if (ref1->root < ref2->root)
			return -1;
		if (ref1->root > ref2->root)
			return 1;
	} else {
		if (ref1->parent < ref2->parent)
			return -1;
		if (ref1->parent > ref2->parent)
			return 1;
	}
	return 0;
}

/*
 * compare two delayed data backrefs with same bytenr and type
 */
static int comp_data_refs(struct btrfs_delayed_data_ref *ref2,
			  struct btrfs_delayed_data_ref *ref1)
{
	if (ref1->node.type == BTRFS_EXTENT_DATA_REF_KEY) {
		if (ref1->root < ref2->root)
			return -1;
		if (ref1->root > ref2->root)
			return 1;
		if (ref1->objectid < ref2->objectid)
			return -1;
		if (ref1->objectid > ref2->objectid)
			return 1;
		if (ref1->offset < ref2->offset)
			return -1;
		if (ref1->offset > ref2->offset)
			return 1;
	} else {
		if (ref1->parent < ref2->parent)
			return -1;
		if (ref1->parent > ref2->parent)
			return 1;
	}
	return 0;
}

/* insert a new ref to head ref rbtree */
static struct btrfs_delayed_ref_head *htree_insert(struct rb_root *root,
						   struct rb_node *node)
{
	struct rb_node **p = &root->rb_node;
	struct rb_node *parent_node = NULL;
	struct btrfs_delayed_ref_head *entry;
	struct btrfs_delayed_ref_head *ins;
	u64 bytenr;

	ins = rb_entry(node, struct btrfs_delayed_ref_head, href_node);
	bytenr = ins->node.bytenr;
	while (*p) {
		parent_node = *p;
		entry = rb_entry(parent_node, struct btrfs_delayed_ref_head,
				 href_node);

		if (bytenr < entry->node.bytenr)
			p = &(*p)->rb_left;
		else if (bytenr > entry->node.bytenr)
			p = &(*p)->rb_right;
		else
			return entry;
	}

	rb_link_node(node, parent_node, p);
	rb_insert_color(node, root);
	return NULL;
}

/*
 * find an head entry based on bytenr. This returns the delayed ref
 * head if it was able to find one, or NULL if nothing was in that spot.
 * If return_bigger is given, the next bigger entry is returned if no exact
 * match is found.
 */
static struct btrfs_delayed_ref_head *
find_ref_head(struct rb_root *root, u64 bytenr,
	      int return_bigger)
{
	struct rb_node *n;
	struct btrfs_delayed_ref_head *entry;

	n = root->rb_node;
	entry = NULL;
	while (n) {
		entry = rb_entry(n, struct btrfs_delayed_ref_head, href_node);

		if (bytenr < entry->node.bytenr)
			n = n->rb_left;
		else if (bytenr > entry->node.bytenr)
			n = n->rb_right;
		else
			return entry;
	}
	if (entry && return_bigger) {
		if (bytenr > entry->node.bytenr) {
			n = rb_next(&entry->href_node);
			if (!n)
				n = rb_first(root);
			entry = rb_entry(n, struct btrfs_delayed_ref_head,
					 href_node);
			return entry;
		}
		return entry;
	}
	return NULL;
}

int btrfs_delayed_ref_lock(struct btrfs_trans_handle *trans,
			   struct btrfs_delayed_ref_head *head)
{
	struct btrfs_delayed_ref_root *delayed_refs;

	delayed_refs = &trans->transaction->delayed_refs;
	assert_spin_locked(&delayed_refs->lock);
	if (mutex_trylock(&head->mutex))
		return 0;

	atomic_inc(&head->node.refs);
	spin_unlock(&delayed_refs->lock);

	mutex_lock(&head->mutex);
	spin_lock(&delayed_refs->lock);
	if (!head->node.in_tree) {
		mutex_unlock(&head->mutex);
		btrfs_put_delayed_ref(&head->node);
		return -EAGAIN;
	}
	btrfs_put_delayed_ref(&head->node);
	return 0;
}

static inline void drop_delayed_ref(struct btrfs_trans_handle *trans,
				    struct btrfs_delayed_ref_root *delayed_refs,
				    struct btrfs_delayed_ref_head *head,
				    struct btrfs_delayed_ref_node *ref)
{
	if (btrfs_delayed_ref_is_head(ref)) {
		head = btrfs_delayed_node_to_head(ref);
		rb_erase(&head->href_node, &delayed_refs->href_root);
	} else {
		assert_spin_locked(&head->lock);
		list_del(&ref->list);
	}
	ref->in_tree = 0;
	btrfs_put_delayed_ref(ref);
	atomic_dec(&delayed_refs->num_entries);
}

static bool merge_ref(struct btrfs_trans_handle *trans,
		      struct btrfs_delayed_ref_root *delayed_refs,
		      struct btrfs_delayed_ref_head *head,
		      struct btrfs_delayed_ref_node *ref,
		      u64 seq)
{
	struct btrfs_delayed_ref_node *next;
	bool done = false;

	next = list_first_entry(&head->ref_list, struct btrfs_delayed_ref_node,
				list);
	while (!done && &next->list != &head->ref_list) {
		int mod;
		struct btrfs_delayed_ref_node *next2;

		next2 = list_next_entry(next, list);

		if (next == ref)
			goto next;

		if (seq && next->seq >= seq)
			goto next;

		if (next->type != ref->type)
			goto next;

		if ((ref->type == BTRFS_TREE_BLOCK_REF_KEY ||
		     ref->type == BTRFS_SHARED_BLOCK_REF_KEY) &&
		    comp_tree_refs(btrfs_delayed_node_to_tree_ref(ref),
				   btrfs_delayed_node_to_tree_ref(next),
				   ref->type))
			goto next;
		if ((ref->type == BTRFS_EXTENT_DATA_REF_KEY ||
		     ref->type == BTRFS_SHARED_DATA_REF_KEY) &&
		    comp_data_refs(btrfs_delayed_node_to_data_ref(ref),
				   btrfs_delayed_node_to_data_ref(next)))
			goto next;

		if (ref->action == next->action) {
			mod = next->ref_mod;
		} else {
			if (ref->ref_mod < next->ref_mod) {
				swap(ref, next);
				done = true;
			}
			mod = -next->ref_mod;
		}

		drop_delayed_ref(trans, delayed_refs, head, next);
		ref->ref_mod += mod;
		if (ref->ref_mod == 0) {
			drop_delayed_ref(trans, delayed_refs, head, ref);
			done = true;
		} else {
			/*
			 * Can't have multiples of the same ref on a tree block.
			 */
			WARN_ON(ref->type == BTRFS_TREE_BLOCK_REF_KEY ||
				ref->type == BTRFS_SHARED_BLOCK_REF_KEY);
		}
next:
		next = next2;
	}

	return done;
}

void btrfs_merge_delayed_refs(struct btrfs_trans_handle *trans,
			      struct btrfs_fs_info *fs_info,
			      struct btrfs_delayed_ref_root *delayed_refs,
			      struct btrfs_delayed_ref_head *head)
{
	struct btrfs_delayed_ref_node *ref;
	u64 seq = 0;

	assert_spin_locked(&head->lock);

	if (list_empty(&head->ref_list))
		return;

	/* We don't have too many refs to merge for data. */
	if (head->is_data)
		return;

	read_lock(&fs_info->tree_mod_log_lock);
	if (!list_empty(&fs_info->tree_mod_seq_list)) {
		struct seq_list *elem;

		elem = list_first_entry(&fs_info->tree_mod_seq_list,
					struct seq_list, list);
		seq = elem->seq;
	}
	read_unlock(&fs_info->tree_mod_log_lock);

	ref = list_first_entry(&head->ref_list, struct btrfs_delayed_ref_node,
			       list);
	while (&ref->list != &head->ref_list) {
		if (seq && ref->seq >= seq)
			goto next;

		if (merge_ref(trans, delayed_refs, head, ref, seq)) {
			if (list_empty(&head->ref_list))
				break;
			ref = list_first_entry(&head->ref_list,
					       struct btrfs_delayed_ref_node,
					       list);
			continue;
		}
next:
		ref = list_next_entry(ref, list);
	}
}

int btrfs_check_delayed_seq(struct btrfs_fs_info *fs_info,
			    struct btrfs_delayed_ref_root *delayed_refs,
			    u64 seq)
{
	struct seq_list *elem;
	int ret = 0;

	read_lock(&fs_info->tree_mod_log_lock);
	if (!list_empty(&fs_info->tree_mod_seq_list)) {
		elem = list_first_entry(&fs_info->tree_mod_seq_list,
					struct seq_list, list);
		if (seq >= elem->seq) {
			pr_debug("holding back delayed_ref %#x.%x, lowest is %#x.%x (%p)\n",
				 (u32)(seq >> 32), (u32)seq,
				 (u32)(elem->seq >> 32), (u32)elem->seq,
				 delayed_refs);
			ret = 1;
		}
	}

	read_unlock(&fs_info->tree_mod_log_lock);
	return ret;
}

struct btrfs_delayed_ref_head *
btrfs_select_ref_head(struct btrfs_trans_handle *trans)
{
	struct btrfs_delayed_ref_root *delayed_refs;
	struct btrfs_delayed_ref_head *head;
	u64 start;
	bool loop = false;

	delayed_refs = &trans->transaction->delayed_refs;

again:
	start = delayed_refs->run_delayed_start;
	head = find_ref_head(&delayed_refs->href_root, start, 1);
	if (!head && !loop) {
		delayed_refs->run_delayed_start = 0;
		start = 0;
		loop = true;
		head = find_ref_head(&delayed_refs->href_root, start, 1);
		if (!head)
			return NULL;
	} else if (!head && loop) {
		return NULL;
	}

	while (head->processing) {
		struct rb_node *node;

		node = rb_next(&head->href_node);
		if (!node) {
			if (loop)
				return NULL;
			delayed_refs->run_delayed_start = 0;
			start = 0;
			loop = true;
			goto again;
		}
		head = rb_entry(node, struct btrfs_delayed_ref_head,
				href_node);
	}

	head->processing = 1;
	WARN_ON(delayed_refs->num_heads_ready == 0);
	delayed_refs->num_heads_ready--;
	delayed_refs->run_delayed_start = head->node.bytenr +
		head->node.num_bytes;
	return head;
}

/*
 * Helper to insert the ref_node to the tail or merge with tail.
 *
 * Return 0 for insert.
 * Return >0 for merge.
 */
static int
add_delayed_ref_tail_merge(struct btrfs_trans_handle *trans,
			   struct btrfs_delayed_ref_root *root,
			   struct btrfs_delayed_ref_head *href,
			   struct btrfs_delayed_ref_node *ref)
{
	struct btrfs_delayed_ref_node *exist;
	int mod;
	int ret = 0;

	spin_lock(&href->lock);
	/* Check whether we can merge the tail node with ref */
	if (list_empty(&href->ref_list))
		goto add_tail;
	exist = list_entry(href->ref_list.prev, struct btrfs_delayed_ref_node,
			   list);
	/* No need to compare bytenr nor is_head */
	if (exist->type != ref->type || exist->seq != ref->seq)
		goto add_tail;

	if ((exist->type == BTRFS_TREE_BLOCK_REF_KEY ||
	     exist->type == BTRFS_SHARED_BLOCK_REF_KEY) &&
	    comp_tree_refs(btrfs_delayed_node_to_tree_ref(exist),
			   btrfs_delayed_node_to_tree_ref(ref),
			   ref->type))
		goto add_tail;
	if ((exist->type == BTRFS_EXTENT_DATA_REF_KEY ||
	     exist->type == BTRFS_SHARED_DATA_REF_KEY) &&
	    comp_data_refs(btrfs_delayed_node_to_data_ref(exist),
			   btrfs_delayed_node_to_data_ref(ref)))
		goto add_tail;

	/* Now we are sure we can merge */
	ret = 1;
	if (exist->action == ref->action) {
		mod = ref->ref_mod;
	} else {
		/* Need to change action */
		if (exist->ref_mod < ref->ref_mod) {
			exist->action = ref->action;
			mod = -exist->ref_mod;
			exist->ref_mod = ref->ref_mod;
		} else
			mod = -ref->ref_mod;
	}
	exist->ref_mod += mod;

	/* remove existing tail if its ref_mod is zero */
	if (exist->ref_mod == 0)
		drop_delayed_ref(trans, root, href, exist);
	spin_unlock(&href->lock);
	return ret;

add_tail:
	list_add_tail(&ref->list, &href->ref_list);
	atomic_inc(&root->num_entries);
	spin_unlock(&href->lock);
	return ret;
}

/*
 * helper function to update the accounting in the head ref
 * existing and update must have the same bytenr
 */
static noinline void
update_existing_head_ref(struct btrfs_delayed_ref_root *delayed_refs,
			 struct btrfs_delayed_ref_node *existing,
			 struct btrfs_delayed_ref_node *update)
{
	struct btrfs_delayed_ref_head *existing_ref;
	struct btrfs_delayed_ref_head *ref;
	int old_ref_mod;

	existing_ref = btrfs_delayed_node_to_head(existing);
	ref = btrfs_delayed_node_to_head(update);
	BUG_ON(existing_ref->is_data != ref->is_data);

	spin_lock(&existing_ref->lock);
	if (ref->must_insert_reserved) {
		/* if the extent was freed and then
		 * reallocated before the delayed ref
		 * entries were processed, we can end up
		 * with an existing head ref without
		 * the must_insert_reserved flag set.
		 * Set it again here
		 */
		existing_ref->must_insert_reserved = ref->must_insert_reserved;

		/*
		 * update the num_bytes so we make sure the accounting
		 * is done correctly
		 */
		existing->num_bytes = update->num_bytes;

	}

	if (ref->extent_op) {
		if (!existing_ref->extent_op) {
			existing_ref->extent_op = ref->extent_op;
		} else {
			if (ref->extent_op->update_key) {
				memcpy(&existing_ref->extent_op->key,
				       &ref->extent_op->key,
				       sizeof(ref->extent_op->key));
				existing_ref->extent_op->update_key = 1;
			}
			if (ref->extent_op->update_flags) {
				existing_ref->extent_op->flags_to_set |=
					ref->extent_op->flags_to_set;
				existing_ref->extent_op->update_flags = 1;
			}
			btrfs_free_delayed_extent_op(ref->extent_op);
		}
	}
	/*
	 * update the reference mod on the head to reflect this new operation,
	 * only need the lock for this case cause we could be processing it
	 * currently, for refs we just added we know we're a-ok.
	 */
	old_ref_mod = existing_ref->total_ref_mod;
	existing->ref_mod += update->ref_mod;
	existing_ref->total_ref_mod += update->ref_mod;

	/*
	 * If we are going to from a positive ref mod to a negative or vice
	 * versa we need to make sure to adjust pending_csums accordingly.
	 */
	if (existing_ref->is_data) {
		if (existing_ref->total_ref_mod >= 0 && old_ref_mod < 0)
			delayed_refs->pending_csums -= existing->num_bytes;
		if (existing_ref->total_ref_mod < 0 && old_ref_mod >= 0)
			delayed_refs->pending_csums += existing->num_bytes;
	}
	spin_unlock(&existing_ref->lock);
}

/*
 * helper function to actually insert a head node into the rbtree.
 * this does all the dirty work in terms of maintaining the correct
 * overall modification count.
 */
static noinline struct btrfs_delayed_ref_head *
add_delayed_ref_head(struct btrfs_fs_info *fs_info,
		     struct btrfs_trans_handle *trans,
		     struct btrfs_delayed_ref_node *ref,
		     struct btrfs_qgroup_extent_record *qrecord,
		     u64 bytenr, u64 num_bytes, u64 ref_root, u64 reserved,
		     int action, int is_data)
{
	struct btrfs_delayed_ref_head *existing;
	struct btrfs_delayed_ref_head *head_ref = NULL;
	struct btrfs_delayed_ref_root *delayed_refs;
	struct btrfs_qgroup_extent_record *qexisting;
	int count_mod = 1;
	int must_insert_reserved = 0;

	/* If reserved is provided, it must be a data extent. */
	BUG_ON(!is_data && reserved);

	/*
	 * the head node stores the sum of all the mods, so dropping a ref
	 * should drop the sum in the head node by one.
	 */
	if (action == BTRFS_UPDATE_DELAYED_HEAD)
		count_mod = 0;
	else if (action == BTRFS_DROP_DELAYED_REF)
		count_mod = -1;

	/*
	 * BTRFS_ADD_DELAYED_EXTENT means that we need to update
	 * the reserved accounting when the extent is finally added, or
	 * if a later modification deletes the delayed ref without ever
	 * inserting the extent into the extent allocation tree.
	 * ref->must_insert_reserved is the flag used to record
	 * that accounting mods are required.
	 *
	 * Once we record must_insert_reserved, switch the action to
	 * BTRFS_ADD_DELAYED_REF because other special casing is not required.
	 */
	if (action == BTRFS_ADD_DELAYED_EXTENT)
		must_insert_reserved = 1;
	else
		must_insert_reserved = 0;

	delayed_refs = &trans->transaction->delayed_refs;

	/* first set the basic ref node struct up */
	atomic_set(&ref->refs, 1);
	ref->bytenr = bytenr;
	ref->num_bytes = num_bytes;
	ref->ref_mod = count_mod;
	ref->type  = 0;
	ref->action  = 0;
	ref->is_head = 1;
	ref->in_tree = 1;
	ref->seq = 0;

	head_ref = btrfs_delayed_node_to_head(ref);
	head_ref->must_insert_reserved = must_insert_reserved;
	head_ref->is_data = is_data;
	INIT_LIST_HEAD(&head_ref->ref_list);
	head_ref->processing = 0;
	head_ref->total_ref_mod = count_mod;
	head_ref->qgroup_reserved = 0;
	head_ref->qgroup_ref_root = 0;

	/* Record qgroup extent info if provided */
	if (qrecord) {
		if (ref_root && reserved) {
			head_ref->qgroup_ref_root = ref_root;
			head_ref->qgroup_reserved = reserved;
		}

		qrecord->bytenr = bytenr;
		qrecord->num_bytes = num_bytes;
		qrecord->old_roots = NULL;

		qexisting = btrfs_qgroup_insert_dirty_extent(delayed_refs,
							     qrecord);
		if (qexisting)
			kfree(qrecord);
	}

	spin_lock_init(&head_ref->lock);
	mutex_init(&head_ref->mutex);

	trace_add_delayed_ref_head(ref, head_ref, action);

	existing = htree_insert(&delayed_refs->href_root,
				&head_ref->href_node);
	if (existing) {
		WARN_ON(ref_root && reserved && existing->qgroup_ref_root
			&& existing->qgroup_reserved);
		update_existing_head_ref(delayed_refs, &existing->node, ref);
		/*
		 * we've updated the existing ref, free the newly
		 * allocated ref
		 */
		kmem_cache_free(btrfs_delayed_ref_head_cachep, head_ref);
		head_ref = existing;
	} else {
		if (is_data && count_mod < 0)
			delayed_refs->pending_csums += num_bytes;
		delayed_refs->num_heads++;
		delayed_refs->num_heads_ready++;
		atomic_inc(&delayed_refs->num_entries);
		trans->delayed_ref_updates++;
	}
	return head_ref;
}

/*
 * helper to insert a delayed tree ref into the rbtree.
 */
static noinline void
add_delayed_tree_ref(struct btrfs_fs_info *fs_info,
		     struct btrfs_trans_handle *trans,
		     struct btrfs_delayed_ref_head *head_ref,
		     struct btrfs_delayed_ref_node *ref, u64 bytenr,
		     u64 num_bytes, u64 parent, u64 ref_root, int level,
		     int action)
{
	struct btrfs_delayed_tree_ref *full_ref;
	struct btrfs_delayed_ref_root *delayed_refs;
	u64 seq = 0;
	int ret;

	if (action == BTRFS_ADD_DELAYED_EXTENT)
		action = BTRFS_ADD_DELAYED_REF;

	if (is_fstree(ref_root))
		seq = atomic64_read(&fs_info->tree_mod_seq);
	delayed_refs = &trans->transaction->delayed_refs;

	/* first set the basic ref node struct up */
	atomic_set(&ref->refs, 1);
	ref->bytenr = bytenr;
	ref->num_bytes = num_bytes;
	ref->ref_mod = 1;
	ref->action = action;
	ref->is_head = 0;
	ref->in_tree = 1;
	ref->seq = seq;

	full_ref = btrfs_delayed_node_to_tree_ref(ref);
	full_ref->parent = parent;
	full_ref->root = ref_root;
	if (parent)
		ref->type = BTRFS_SHARED_BLOCK_REF_KEY;
	else
		ref->type = BTRFS_TREE_BLOCK_REF_KEY;
	full_ref->level = level;

	trace_add_delayed_tree_ref(ref, full_ref, action);

	ret = add_delayed_ref_tail_merge(trans, delayed_refs, head_ref, ref);

	/*
	 * XXX: memory should be freed at the same level allocated.
	 * But bad practice is anywhere... Follow it now. Need cleanup.
	 */
	if (ret > 0)
		kmem_cache_free(btrfs_delayed_tree_ref_cachep, full_ref);
}

/*
 * helper to insert a delayed data ref into the rbtree.
 */
static noinline void
add_delayed_data_ref(struct btrfs_fs_info *fs_info,
		     struct btrfs_trans_handle *trans,
		     struct btrfs_delayed_ref_head *head_ref,
		     struct btrfs_delayed_ref_node *ref, u64 bytenr,
		     u64 num_bytes, u64 parent, u64 ref_root, u64 owner,
		     u64 offset, int action)
{
	struct btrfs_delayed_data_ref *full_ref;
	struct btrfs_delayed_ref_root *delayed_refs;
	u64 seq = 0;
	int ret;

	if (action == BTRFS_ADD_DELAYED_EXTENT)
		action = BTRFS_ADD_DELAYED_REF;

	delayed_refs = &trans->transaction->delayed_refs;

	if (is_fstree(ref_root))
		seq = atomic64_read(&fs_info->tree_mod_seq);

	/* first set the basic ref node struct up */
	atomic_set(&ref->refs, 1);
	ref->bytenr = bytenr;
	ref->num_bytes = num_bytes;
	ref->ref_mod = 1;
	ref->action = action;
	ref->is_head = 0;
	ref->in_tree = 1;
	ref->seq = seq;

	full_ref = btrfs_delayed_node_to_data_ref(ref);
	full_ref->parent = parent;
	full_ref->root = ref_root;
	if (parent)
		ref->type = BTRFS_SHARED_DATA_REF_KEY;
	else
		ref->type = BTRFS_EXTENT_DATA_REF_KEY;

	full_ref->objectid = owner;
	full_ref->offset = offset;

	trace_add_delayed_data_ref(ref, full_ref, action);

	ret = add_delayed_ref_tail_merge(trans, delayed_refs, head_ref, ref);

	if (ret > 0)
		kmem_cache_free(btrfs_delayed_data_ref_cachep, full_ref);
}

/*
 * add a delayed tree ref.  This does all of the accounting required
 * to make sure the delayed ref is eventually processed before this
 * transaction commits.
 */
int btrfs_add_delayed_tree_ref(struct btrfs_fs_info *fs_info,
			       struct btrfs_trans_handle *trans,
			       u64 bytenr, u64 num_bytes, u64 parent,
			       u64 ref_root,  int level, int action,
			       struct btrfs_delayed_extent_op *extent_op)
{
	struct btrfs_delayed_tree_ref *ref;
	struct btrfs_delayed_ref_head *head_ref;
	struct btrfs_delayed_ref_root *delayed_refs;
	struct btrfs_qgroup_extent_record *record = NULL;

	BUG_ON(extent_op && extent_op->is_data);
	ref = kmem_cache_alloc(btrfs_delayed_tree_ref_cachep, GFP_NOFS);
	if (!ref)
		return -ENOMEM;

	head_ref = kmem_cache_alloc(btrfs_delayed_ref_head_cachep, GFP_NOFS);
	if (!head_ref)
		goto free_ref;

	if (fs_info->quota_enabled && is_fstree(ref_root)) {
		record = kmalloc(sizeof(*record), GFP_NOFS);
		if (!record)
			goto free_head_ref;
	}

	head_ref->extent_op = extent_op;

	delayed_refs = &trans->transaction->delayed_refs;
	spin_lock(&delayed_refs->lock);

	/*
	 * insert both the head node and the new ref without dropping
	 * the spin lock
	 */
	head_ref = add_delayed_ref_head(fs_info, trans, &head_ref->node, record,
					bytenr, num_bytes, 0, 0, action, 0);

	add_delayed_tree_ref(fs_info, trans, head_ref, &ref->node, bytenr,
			     num_bytes, parent, ref_root, level, action);
	spin_unlock(&delayed_refs->lock);

	return 0;

free_head_ref:
	kmem_cache_free(btrfs_delayed_ref_head_cachep, head_ref);
free_ref:
	kmem_cache_free(btrfs_delayed_tree_ref_cachep, ref);

	return -ENOMEM;
}

/*
 * add a delayed data ref. it's similar to btrfs_add_delayed_tree_ref.
 */
int btrfs_add_delayed_data_ref(struct btrfs_fs_info *fs_info,
			       struct btrfs_trans_handle *trans,
			       u64 bytenr, u64 num_bytes,
			       u64 parent, u64 ref_root,
			       u64 owner, u64 offset, u64 reserved, int action,
			       struct btrfs_delayed_extent_op *extent_op)
{
	struct btrfs_delayed_data_ref *ref;
	struct btrfs_delayed_ref_head *head_ref;
	struct btrfs_delayed_ref_root *delayed_refs;
	struct btrfs_qgroup_extent_record *record = NULL;

	BUG_ON(extent_op && !extent_op->is_data);
	ref = kmem_cache_alloc(btrfs_delayed_data_ref_cachep, GFP_NOFS);
	if (!ref)
		return -ENOMEM;

	head_ref = kmem_cache_alloc(btrfs_delayed_ref_head_cachep, GFP_NOFS);
	if (!head_ref) {
		kmem_cache_free(btrfs_delayed_data_ref_cachep, ref);
		return -ENOMEM;
	}

	if (fs_info->quota_enabled && is_fstree(ref_root)) {
		record = kmalloc(sizeof(*record), GFP_NOFS);
		if (!record) {
			kmem_cache_free(btrfs_delayed_data_ref_cachep, ref);
			kmem_cache_free(btrfs_delayed_ref_head_cachep,
					head_ref);
			return -ENOMEM;
		}
	}

	head_ref->extent_op = extent_op;

	delayed_refs = &trans->transaction->delayed_refs;
	spin_lock(&delayed_refs->lock);

	/*
	 * insert both the head node and the new ref without dropping
	 * the spin lock
	 */
	head_ref = add_delayed_ref_head(fs_info, trans, &head_ref->node, record,
					bytenr, num_bytes, ref_root, reserved,
					action, 1);

	add_delayed_data_ref(fs_info, trans, head_ref, &ref->node, bytenr,
				   num_bytes, parent, ref_root, owner, offset,
				   action);
	spin_unlock(&delayed_refs->lock);

	return 0;
}

int btrfs_add_delayed_qgroup_reserve(struct btrfs_fs_info *fs_info,
				     struct btrfs_trans_handle *trans,
				     u64 ref_root, u64 bytenr, u64 num_bytes)
{
	struct btrfs_delayed_ref_root *delayed_refs;
	struct btrfs_delayed_ref_head *ref_head;
	int ret = 0;

	if (!fs_info->quota_enabled || !is_fstree(ref_root))
		return 0;

	delayed_refs = &trans->transaction->delayed_refs;

	spin_lock(&delayed_refs->lock);
	ref_head = find_ref_head(&delayed_refs->href_root, bytenr, 0);
	if (!ref_head) {
		ret = -ENOENT;
		goto out;
	}
	WARN_ON(ref_head->qgroup_reserved || ref_head->qgroup_ref_root);
	ref_head->qgroup_ref_root = ref_root;
	ref_head->qgroup_reserved = num_bytes;
out:
	spin_unlock(&delayed_refs->lock);
	return ret;
}

int btrfs_add_delayed_extent_op(struct btrfs_fs_info *fs_info,
				struct btrfs_trans_handle *trans,
				u64 bytenr, u64 num_bytes,
				struct btrfs_delayed_extent_op *extent_op)
{
	struct btrfs_delayed_ref_head *head_ref;
	struct btrfs_delayed_ref_root *delayed_refs;

	head_ref = kmem_cache_alloc(btrfs_delayed_ref_head_cachep, GFP_NOFS);
	if (!head_ref)
		return -ENOMEM;

	head_ref->extent_op = extent_op;

	delayed_refs = &trans->transaction->delayed_refs;
	spin_lock(&delayed_refs->lock);

	add_delayed_ref_head(fs_info, trans, &head_ref->node, NULL, bytenr,
			     num_bytes, 0, 0, BTRFS_UPDATE_DELAYED_HEAD,
			     extent_op->is_data);

	spin_unlock(&delayed_refs->lock);
	return 0;
}

/*
 * this does a simple search for the head node for a given extent.
 * It must be called with the delayed ref spinlock held, and it returns
 * the head node if any where found, or NULL if not.
 */
struct btrfs_delayed_ref_head *
btrfs_find_delayed_ref_head(struct btrfs_trans_handle *trans, u64 bytenr)
{
	struct btrfs_delayed_ref_root *delayed_refs;

	delayed_refs = &trans->transaction->delayed_refs;
	return find_ref_head(&delayed_refs->href_root, bytenr, 0);
}

void btrfs_delayed_ref_exit(void)
{
	if (btrfs_delayed_ref_head_cachep)
		kmem_cache_destroy(btrfs_delayed_ref_head_cachep);
	if (btrfs_delayed_tree_ref_cachep)
		kmem_cache_destroy(btrfs_delayed_tree_ref_cachep);
	if (btrfs_delayed_data_ref_cachep)
		kmem_cache_destroy(btrfs_delayed_data_ref_cachep);
	if (btrfs_delayed_extent_op_cachep)
		kmem_cache_destroy(btrfs_delayed_extent_op_cachep);
}

int btrfs_delayed_ref_init(void)
{
	btrfs_delayed_ref_head_cachep = kmem_cache_create(
				"btrfs_delayed_ref_head",
				sizeof(struct btrfs_delayed_ref_head), 0,
				SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD, NULL);
	if (!btrfs_delayed_ref_head_cachep)
		goto fail;

	btrfs_delayed_tree_ref_cachep = kmem_cache_create(
				"btrfs_delayed_tree_ref",
				sizeof(struct btrfs_delayed_tree_ref), 0,
				SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD, NULL);
	if (!btrfs_delayed_tree_ref_cachep)
		goto fail;

	btrfs_delayed_data_ref_cachep = kmem_cache_create(
				"btrfs_delayed_data_ref",
				sizeof(struct btrfs_delayed_data_ref), 0,
				SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD, NULL);
	if (!btrfs_delayed_data_ref_cachep)
		goto fail;

	btrfs_delayed_extent_op_cachep = kmem_cache_create(
				"btrfs_delayed_extent_op",
				sizeof(struct btrfs_delayed_extent_op), 0,
				SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD, NULL);
	if (!btrfs_delayed_extent_op_cachep)
		goto fail;

	return 0;
fail:
	btrfs_delayed_ref_exit();
	return -ENOMEM;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*
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
#ifndef __DELAYED_REF__
#define __DELAYED_REF__

/* these are the possible values of struct btrfs_delayed_ref_node->action */
#define BTRFS_ADD_DELAYED_REF    1 /* add one backref to the tree */
#define BTRFS_DROP_DELAYED_REF   2 /* delete one backref from the tree */
#define BTRFS_ADD_DELAYED_EXTENT 3 /* record a full extent allocation */
#define BTRFS_UPDATE_DELAYED_HEAD 4 /* not changing ref count on head ref */

/*
 * XXX: Qu: I really hate the design that ref_head and tree/data ref shares the
 * same ref_node structure.
 * Ref_head is in a higher logic level than tree/data ref, and duplicated
 * bytenr/num_bytes in ref_node is really a waste or memory, they should be
 * referred from ref_head.
 * This gets more disgusting after we use list to store tree/data ref in
 * ref_head. Must clean this mess up later.
 */
struct btrfs_delayed_ref_node {
	/*
	 * ref_head use rb tree, stored in ref_root->href.
	 * indexed by bytenr
	 */
	struct rb_node rb_node;

	/*data/tree ref use list, stored in ref_head->ref_list. */
	struct list_head list;

	/* the starting bytenr of the extent */
	u64 bytenr;

	/* the size of the extent */
	u64 num_bytes;

	/* seq number to keep track of insertion order */
	u64 seq;

	/* ref count on this data structure */
	atomic_t refs;

	/*
	 * how many refs is this entry adding or deleting.  For
	 * head refs, this may be a negative number because it is keeping
	 * track of the total mods done to the reference count.
	 * For individual refs, this will always be a positive number
	 *
	 * It may be more than one, since it is possible for a single
	 * parent to have more than one ref on an extent
	 */
	int ref_mod;

	unsigned int action:8;
	unsigned int type:8;
	/* is this node still in the rbtree? */
	unsigned int is_head:1;
	unsigned int in_tree:1;
};

struct btrfs_delayed_extent_op {
	struct btrfs_disk_key key;
	u64 flags_to_set;
	int level;
	unsigned int update_key:1;
	unsigned int update_flags:1;
	unsigned int is_data:1;
};

/*
 * the head refs are used to hold a lock on a given extent, which allows us
 * to make sure that only one process is running the delayed refs
 * at a time for a single extent.  They also store the sum of all the
 * reference count modifications we've queued up.
 */
struct btrfs_delayed_ref_head {
	struct btrfs_delayed_ref_node node;

	/*
	 * the mutex is held while running the refs, and it is also
	 * held when checking the sum of reference modifications.
	 */
	struct mutex mutex;

	spinlock_t lock;
	struct list_head ref_list;

	struct rb_node href_node;

	struct btrfs_delayed_extent_op *extent_op;

	/*
	 * This is used to track the final ref_mod from all the refs associated
	 * with this head ref, this is not adjusted as delayed refs are run,
	 * this is meant to track if we need to do the csum accounting or not.
	 */
	int total_ref_mod;

	/*
	 * For qgroup reserved space freeing.
	 *
	 * ref_root and reserved will be recorded after
	 * BTRFS_ADD_DELAYED_EXTENT is called.
	 * And will be used to free reserved qgroup space at
	 * run_delayed_refs() time.
	 */
	u64 qgroup_ref_root;
	u64 qgroup_reserved;

	/*
	 * when a new extent is allocated, it is just reserved in memory
	 * The actual extent isn't inserted into the extent allocation tree
	 * until the delayed ref is processed.  must_insert_reserved is
	 * used to flag a delayed ref so the accounting can be updated
	 * when a full insert is done.
	 *
	 * It is possible the extent will be freed before it is ever
	 * inserted into the extent allocation tree.  In this case
	 * we need to update the in ram accounting to properly reflect
	 * the free has happened.
	 */
	unsigned int must_insert_reserved:1;
	unsigned int is_data:1;
	unsigned int processing:1;
};

struct btrfs_delayed_tree_ref {
	struct btrfs_delayed_ref_node node;
	u64 root;
	u64 parent;
	int level;
};

struct btrfs_delayed_data_ref {
	struct btrfs_delayed_ref_node node;
	u64 root;
	u64 parent;
	u64 objectid;
	u64 offset;
};

struct btrfs_delayed_ref_root {
	/* head ref rbtree */
	struct rb_root href_root;

	/* dirty extent records */
	struct rb_root dirty_extent_root;

	/* this spin lock protects the rbtree and the entries inside */
	spinlock_t lock;

	/* how many delayed ref updates we've queued, used by the
	 * throttling code
	 */
	atomic_t num_entries;

	/* total number of head nodes in tree */
	unsigned long num_heads;

	/* total number of head nodes ready for processing */
	unsigned long num_heads_ready;

	u64 pending_csums;

	/*
	 * set when the tree is flushing before a transaction commit,
	 * used by the throttling code to decide if new updates need
	 * to be run right away
	 */
	int flushing;

	u64 run_delayed_start;

	/*
	 * To make qgroup to skip given root.
	 * This is for snapshot, as btrfs_qgroup_inherit() will manully
	 * modify counters for snapshot and its source, so we should skip
	 * the snapshot in new_root/old_roots or it will get calculated twice
	 */
	u64 qgroup_to_skip;
};

extern struct kmem_cache *btrfs_delayed_ref_head_cachep;
extern struct kmem_cache *btrfs_delayed_tree_ref_cachep;
extern struct kmem_cache *btrfs_delayed_data_ref_cachep;
extern struct kmem_cache *btrfs_delayed_extent_op_cachep;

int btrfs_delayed_ref_init(void);
void btrfs_delayed_ref_exit(void);

static inline struct btrfs_delayed_extent_op *
btrfs_alloc_delayed_extent_op(void)
{
	return kmem_cache_alloc(btrfs_delayed_extent_op_cachep, GFP_NOFS);
}

static inline void
btrfs_free_delayed_extent_op(struct btrfs_delayed_extent_op *op)
{
	if (op)
		kmem_cache_free(btrfs_delayed_extent_op_cachep, op);
}

static inline void btrfs_put_delayed_ref(struct btrfs_delayed_ref_node *ref)
{
	WARN_ON(atomic_read(&ref->refs) == 0);
	if (atomic_dec_and_test(&ref->refs)) {
		WARN_ON(ref->in_tree);
		switch (ref->type) {
		case BTRFS_TREE_BLOCK_REF_KEY:
		case BTRFS_SHARED_BLOCK_REF_KEY:
			kmem_cache_free(btrfs_delayed_tree_ref_cachep, ref);
			break;
		case BTRFS_EXTENT_DATA_REF_KEY:
		case BTRFS_SHARED_DATA_REF_KEY:
			kmem_cache_free(btrfs_delayed_data_ref_cachep, ref);
			break;
		case 0:
			kmem_cache_free(btrfs_delayed_ref_head_cachep, ref);
			break;
		default:
			BUG();
		}
	}
}

int btrfs_add_delayed_tree_ref(struct btrfs_fs_info *fs_info,
			       struct btrfs_trans_handle *trans,
			       u64 bytenr, u64 num_bytes, u64 parent,
			       u64 ref_root, int level, int action,
			       struct btrfs_delayed_extent_op *extent_op);
int btrfs_add_delayed_data_ref(struct btrfs_fs_info *fs_info,
			       struct btrfs_trans_handle *trans,
			       u64 bytenr, u64 num_bytes,
			       u64 parent, u64 ref_root,
			       u64 owner, u64 offset, u64 reserved, int action,
			       struct btrfs_delayed_extent_op *extent_op);
int btrfs_add_delayed_qgroup_reserve(struct btrfs_fs_info *fs_info,
				     struct btrfs_trans_handle *trans,
				     u64 ref_root, u64 bytenr, u64 num_bytes);
int btrfs_add_delayed_extent_op(struct btrfs_fs_info *fs_info,
				struct btrfs_trans_handle *trans,
				u64 bytenr, u64 num_bytes,
				struct btrfs_delayed_extent_op *extent_op);
void btrfs_merge_delayed_refs(struct btrfs_trans_handle *trans,
			      struct btrfs_fs_info *fs_info,
			      struct btrfs_delayed_ref_root *delayed_refs,
			      struct btrfs_delayed_ref_head *head);

struct btrfs_delayed_ref_head *
btrfs_find_delayed_ref_head(struct btrfs_trans_handle *trans, u64 bytenr);
int btrfs_delayed_ref_lock(struct btrfs_trans_handle *trans,
			   struct btrfs_delayed_ref_head *head);
static inline void btrfs_delayed_ref_unlock(struct btrfs_delayed_ref_head *head)
{
	mutex_unlock(&head->mutex);
}


struct btrfs_delayed_ref_head *
btrfs_select_ref_head(struct btrfs_trans_handle *trans);

int btrfs_check_delayed_seq(struct btrfs_fs_info *fs_info,
			    struct btrfs_delayed_ref_root *delayed_refs,
			    u64 seq);

/*
 * a node might live in a head or a regular ref, this lets you
 * test for the proper type to use.
 */
static int btrfs_delayed_ref_is_head(struct btrfs_delayed_ref_node *node)
{
	return node->is_head;
}

/*
 * helper functions to cast a node into its container
 */
static inline struct btrfs_delayed_tree_ref *
btrfs_delayed_node_to_tree_ref(struct btrfs_delayed_ref_node *node)
{
	WARN_ON(btrfs_delayed_ref_is_head(node));
	return container_of(node, struct btrfs_delayed_tree_ref, node);
}

static inline struct btrfs_delayed_data_ref *
btrfs_delayed_node_to_data_ref(struct btrfs_delayed_ref_node *node)
{
	WARN_ON(btrfs_delayed_ref_is_head(node));
	return container_of(node, struct btrfs_delayed_data_ref, node);
}

static inline struct btrfs_delayed_ref_head *
btrfs_delayed_node_to_head(struct btrfs_delayed_ref_node *node)
{
	WARN_ON(!btrfs_delayed_ref_is_head(node));
	return container_of(node, struct btrfs_delayed_ref_head, node);
}
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*
 * Copyright (C) STRATO AG 2012.  All rights reserved.
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
#include <linux/bio.h>
#include <linux/slab.h>
#include <linux/buffer_head.h>
#include <linux/blkdev.h>
#include <linux/random.h>
#include <linux/iocontext.h>
#include <linux/capability.h>
#include <linux/kthread.h>
#include <linux/math64.h>
#include <asm/div64.h>
#include "ctree.h"
#include "extent_map.h"
#include "disk-io.h"
#include "transaction.h"
#include "print-tree.h"
#include "volumes.h"
#include "async-thread.h"
#include "check-integrity.h"
#include "rcu-string.h"
#include "dev-replace.h"
#include "sysfs.h"

static int btrfs_dev_replace_finishing(struct btrfs_fs_info *fs_info,
				       int scrub_ret);
static void btrfs_dev_replace_update_device_in_mapping_tree(
						struct btrfs_fs_info *fs_info,
						struct btrfs_device *srcdev,
						struct btrfs_device *tgtdev);
static int btrfs_dev_replace_find_srcdev(struct btrfs_root *root, u64 srcdevid,
					 char *srcdev_name,
					 struct btrfs_device **device);
static u64 __btrfs_dev_replace_cancel(struct btrfs_fs_info *fs_info);
static int btrfs_dev_replace_kthread(void *data);
static int btrfs_dev_replace_continue_on_mount(struct btrfs_fs_info *fs_info);


int btrfs_init_dev_replace(struct btrfs_fs_info *fs_info)
{
	struct btrfs_key key;
	struct btrfs_root *dev_root = fs_info->dev_root;
	struct btrfs_dev_replace *dev_replace = &fs_info->dev_replace;
	struct extent_buffer *eb;
	int slot;
	int ret = 0;
	struct btrfs_path *path = NULL;
	int item_size;
	struct btrfs_dev_replace_item *ptr;
	u64 src_devid;

	path = btrfs_alloc_path();
	if (!path) {
		ret = -ENOMEM;
		goto out;
	}

	key.objectid = 0;
	key.type = BTRFS_DEV_REPLACE_KEY;
	key.offset = 0;
	ret = btrfs_search_slot(NULL, dev_root, &key, path, 0, 0);
	if (ret) {
no_valid_dev_replace_entry_found:
		ret = 0;
		dev_replace->replace_state =
			BTRFS_DEV_REPLACE_ITEM_STATE_NEVER_STARTED;
		dev_replace->cont_reading_from_srcdev_mode =
		    BTRFS_DEV_REPLACE_ITEM_CONT_READING_FROM_SRCDEV_MODE_ALWAYS;
		dev_replace->replace_state = 0;
		dev_replace->time_started = 0;
		dev_replace->time_stopped = 0;
		atomic64_set(&dev_replace->num_write_errors, 0);
		atomic64_set(&dev_replace->num_uncorrectable_read_errors, 0);
		dev_replace->cursor_left = 0;
		dev_replace->committed_cursor_left = 0;
		dev_replace->cursor_left_last_write_of_item = 0;
		dev_replace->cursor_right = 0;
		dev_replace->srcdev = NULL;
		dev_replace->tgtdev = NULL;
		dev_replace->is_valid = 0;
		dev_replace->item_needs_writeback = 0;
		goto out;
	}
	slot = path->slots[0];
	eb = path->nodes[0];
	item_size = btrfs_item_size_nr(eb, slot);
	ptr = btrfs_item_ptr(eb, slot, struct btrfs_dev_replace_item);

	if (item_size != sizeof(struct btrfs_dev_replace_item)) {
		btrfs_warn(fs_info,
			"dev_replace entry found has unexpected size, ignore entry");
		goto no_valid_dev_replace_entry_found;
	}

	src_devid = btrfs_dev_replace_src_devid(eb, ptr);
	dev_replace->cont_reading_from_srcdev_mode =
		btrfs_dev_replace_cont_reading_from_srcdev_mode(eb, ptr);
	dev_replace->replace_state = btrfs_dev_replace_replace_state(eb, ptr);
	dev_replace->time_started = btrfs_dev_replace_time_started(eb, ptr);
	dev_replace->time_stopped =
		btrfs_dev_replace_time_stopped(eb, ptr);
	atomic64_set(&dev_replace->num_write_errors,
		     btrfs_dev_replace_num_write_errors(eb, ptr));
	atomic64_set(&dev_replace->num_uncorrectable_read_errors,
		     btrfs_dev_replace_num_uncorrectable_read_errors(eb, ptr));
	dev_replace->cursor_left = btrfs_dev_replace_cursor_left(eb, ptr);
	dev_replace->committed_cursor_left = dev_replace->cursor_left;
	dev_replace->cursor_left_last_write_of_item = dev_replace->cursor_left;
	dev_replace->cursor_right = btrfs_dev_replace_cursor_right(eb, ptr);
	dev_replace->is_valid = 1;

	dev_replace->item_needs_writeback = 0;
	switch (dev_replace->replace_state) {
	case BTRFS_IOCTL_DEV_REPLACE_STATE_NEVER_STARTED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_FINISHED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_CANCELED:
		dev_replace->srcdev = NULL;
		dev_replace->tgtdev = NULL;
		break;
	case BTRFS_IOCTL_DEV_REPLACE_STATE_STARTED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_SUSPENDED:
		dev_replace->srcdev = btrfs_find_device(fs_info, src_devid,
							NULL, NULL);
		dev_replace->tgtdev = btrfs_find_device(fs_info,
							BTRFS_DEV_REPLACE_DEVID,
							NULL, NULL);
		/*
		 * allow 'btrfs dev replace_cancel' if src/tgt device is
		 * missing
		 */
		if (!dev_replace->srcdev &&
		    !btrfs_test_opt(dev_root, DEGRADED)) {
			ret = -EIO;
			btrfs_warn(fs_info,
			   "cannot mount because device replace operation is ongoing and");
			btrfs_warn(fs_info,
			   "srcdev (devid %llu) is missing, need to run 'btrfs dev scan'?",
			   src_devid);
		}
		if (!dev_replace->tgtdev &&
		    !btrfs_test_opt(dev_root, DEGRADED)) {
			ret = -EIO;
			btrfs_warn(fs_info,
			   "cannot mount because device replace operation is ongoing and");
			btrfs_warn(fs_info,
			   "tgtdev (devid %llu) is missing, need to run 'btrfs dev scan'?",
				BTRFS_DEV_REPLACE_DEVID);
		}
		if (dev_replace->tgtdev) {
			if (dev_replace->srcdev) {
				dev_replace->tgtdev->total_bytes =
					dev_replace->srcdev->total_bytes;
				dev_replace->tgtdev->disk_total_bytes =
					dev_replace->srcdev->disk_total_bytes;
				dev_replace->tgtdev->commit_total_bytes =
					dev_replace->srcdev->commit_total_bytes;
				dev_replace->tgtdev->bytes_used =
					dev_replace->srcdev->bytes_used;
				dev_replace->tgtdev->commit_bytes_used =
					dev_replace->srcdev->commit_bytes_used;
			}
			dev_replace->tgtdev->is_tgtdev_for_dev_replace = 1;
			btrfs_init_dev_replace_tgtdev_for_resume(fs_info,
				dev_replace->tgtdev);
		}
		break;
	}

out:
	btrfs_free_path(path);
	return ret;
}

/*
 * called from commit_transaction. Writes changed device replace state to
 * disk.
 */
int btrfs_run_dev_replace(struct btrfs_trans_handle *trans,
			  struct btrfs_fs_info *fs_info)
{
	int ret;
	struct btrfs_root *dev_root = fs_info->dev_root;
	struct btrfs_path *path;
	struct btrfs_key key;
	struct extent_buffer *eb;
	struct btrfs_dev_replace_item *ptr;
	struct btrfs_dev_replace *dev_replace = &fs_info->dev_replace;

	btrfs_dev_replace_lock(dev_replace);
	if (!dev_replace->is_valid ||
	    !dev_replace->item_needs_writeback) {
		btrfs_dev_replace_unlock(dev_replace);
		return 0;
	}
	btrfs_dev_replace_unlock(dev_replace);

	key.objectid = 0;
	key.type = BTRFS_DEV_REPLACE_KEY;
	key.offset = 0;

	path = btrfs_alloc_path();
	if (!path) {
		ret = -ENOMEM;
		goto out;
	}
	ret = btrfs_search_slot(trans, dev_root, &key, path, -1, 1);
	if (ret < 0) {
		btrfs_warn(fs_info, "error %d while searching for dev_replace item!",
			ret);
		goto out;
	}

	if (ret == 0 &&
	    btrfs_item_size_nr(path->nodes[0], path->slots[0]) < sizeof(*ptr)) {
		/*
		 * need to delete old one and insert a new one.
		 * Since no attempt is made to recover any old state, if the
		 * dev_replace state is 'running', the data on the target
		 * drive is lost.
		 * It would be possible to recover the state: just make sure
		 * that the beginning of the item is never changed and always
		 * contains all the essential information. Then read this
		 * minimal set of information and use it as a base for the
		 * new state.
		 */
		ret = btrfs_del_item(trans, dev_root, path);
		if (ret != 0) {
			btrfs_warn(fs_info, "delete too small dev_replace item failed %d!",
				ret);
			goto out;
		}
		ret = 1;
	}

	if (ret == 1) {
		/* need to insert a new item */
		btrfs_release_path(path);
		ret = btrfs_insert_empty_item(trans, dev_root, path,
					      &key, sizeof(*ptr));
		if (ret < 0) {
			btrfs_warn(fs_info, "insert dev_replace item failed %d!",
				ret);
			goto out;
		}
	}

	eb = path->nodes[0];
	ptr = btrfs_item_ptr(eb, path->slots[0],
			     struct btrfs_dev_replace_item);

	btrfs_dev_replace_lock(dev_replace);
	if (dev_replace->srcdev)
		btrfs_set_dev_replace_src_devid(eb, ptr,
			dev_replace->srcdev->devid);
	else
		btrfs_set_dev_replace_src_devid(eb, ptr, (u64)-1);
	btrfs_set_dev_replace_cont_reading_from_srcdev_mode(eb, ptr,
		dev_replace->cont_reading_from_srcdev_mode);
	btrfs_set_dev_replace_replace_state(eb, ptr,
		dev_replace->replace_state);
	btrfs_set_dev_replace_time_started(eb, ptr, dev_replace->time_started);
	btrfs_set_dev_replace_time_stopped(eb, ptr, dev_replace->time_stopped);
	btrfs_set_dev_replace_num_write_errors(eb, ptr,
		atomic64_read(&dev_replace->num_write_errors));
	btrfs_set_dev_replace_num_uncorrectable_read_errors(eb, ptr,
		atomic64_read(&dev_replace->num_uncorrectable_read_errors));
	dev_replace->cursor_left_last_write_of_item =
		dev_replace->cursor_left;
	btrfs_set_dev_replace_cursor_left(eb, ptr,
		dev_replace->cursor_left_last_write_of_item);
	btrfs_set_dev_replace_cursor_right(eb, ptr,
		dev_replace->cursor_right);
	dev_replace->item_needs_writeback = 0;
	btrfs_dev_replace_unlock(dev_replace);

	btrfs_mark_buffer_dirty(eb);

out:
	btrfs_free_path(path);

	return ret;
}

void btrfs_after_dev_replace_commit(struct btrfs_fs_info *fs_info)
{
	struct btrfs_dev_replace *dev_replace = &fs_info->dev_replace;

	dev_replace->committed_cursor_left =
		dev_replace->cursor_left_last_write_of_item;
}

int btrfs_dev_replace_start(struct btrfs_root *root,
			    struct btrfs_ioctl_dev_replace_args *args)
{
	struct btrfs_trans_handle *trans;
	struct btrfs_fs_info *fs_info = root->fs_info;
	struct btrfs_dev_replace *dev_replace = &fs_info->dev_replace;
	int ret;
	struct btrfs_device *tgt_device = NULL;
	struct btrfs_device *src_device = NULL;

	switch (args->start.cont_reading_from_srcdev_mode) {
	case BTRFS_IOCTL_DEV_REPLACE_CONT_READING_FROM_SRCDEV_MODE_ALWAYS:
	case BTRFS_IOCTL_DEV_REPLACE_CONT_READING_FROM_SRCDEV_MODE_AVOID:
		break;
	default:
		return -EINVAL;
	}

	if ((args->start.srcdevid == 0 && args->start.srcdev_name[0] == '\0') ||
	    args->start.tgtdev_name[0] == '\0')
		return -EINVAL;

	/* the disk copy procedure reuses the scrub code */
	mutex_lock(&fs_info->volume_mutex);
	ret = btrfs_dev_replace_find_srcdev(root, args->start.srcdevid,
					    args->start.srcdev_name,
					    &src_device);
	if (ret) {
		mutex_unlock(&fs_info->volume_mutex);
		return ret;
	}

	ret = btrfs_init_dev_replace_tgtdev(root, args->start.tgtdev_name,
					    src_device, &tgt_device);
	mutex_unlock(&fs_info->volume_mutex);
	if (ret)
		return ret;

	/*
	 * Here we commit the transaction to make sure commit_total_bytes
	 * of all the devices are updated.
	 */
	trans = btrfs_attach_transaction(root);
	if (!IS_ERR(trans)) {
		ret = btrfs_commit_transaction(trans, root);
		if (ret)
			return ret;
	} else if (PTR_ERR(trans) != -ENOENT) {
		return PTR_ERR(trans);
	}

	btrfs_dev_replace_lock(dev_replace);
	switch (dev_replace->replace_state) {
	case BTRFS_IOCTL_DEV_REPLACE_STATE_NEVER_STARTED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_FINISHED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_CANCELED:
		break;
	case BTRFS_IOCTL_DEV_REPLACE_STATE_STARTED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_SUSPENDED:
		args->result = BTRFS_IOCTL_DEV_REPLACE_RESULT_ALREADY_STARTED;
		goto leave;
	}

	dev_replace->cont_reading_from_srcdev_mode =
		args->start.cont_reading_from_srcdev_mode;
	WARN_ON(!src_device);
	dev_replace->srcdev = src_device;
	WARN_ON(!tgt_device);
	dev_replace->tgtdev = tgt_device;

	btrfs_info_in_rcu(root->fs_info,
		      "dev_replace from %s (devid %llu) to %s started",
		      src_device->missing ? "<missing disk>" :
		        rcu_str_deref(src_device->name),
		      src_device->devid,
		      rcu_str_deref(tgt_device->name));

	/*
	 * from now on, the writes to the srcdev are all duplicated to
	 * go to the tgtdev as well (refer to btrfs_map_block()).
	 */
	dev_replace->replace_state = BTRFS_IOCTL_DEV_REPLACE_STATE_STARTED;
	dev_replace->time_started = get_seconds();
	dev_replace->cursor_left = 0;
	dev_replace->committed_cursor_left = 0;
	dev_replace->cursor_left_last_write_of_item = 0;
	dev_replace->cursor_right = 0;
	dev_replace->is_valid = 1;
	dev_replace->item_needs_writeback = 1;
	args->result = BTRFS_IOCTL_DEV_REPLACE_RESULT_NO_ERROR;
	btrfs_dev_replace_unlock(dev_replace);

	ret = btrfs_sysfs_add_device_link(tgt_device->fs_devices, tgt_device);
	if (ret)
		btrfs_err(root->fs_info, "kobj add dev failed %d\n", ret);

	btrfs_wait_ordered_roots(root->fs_info, -1);

	/* force writing the updated state information to disk */
	trans = btrfs_start_transaction(root, 0);
	if (IS_ERR(trans)) {
		ret = PTR_ERR(trans);
		btrfs_dev_replace_lock(dev_replace);
		goto leave;
	}

	ret = btrfs_commit_transaction(trans, root);
	WARN_ON(ret);

	/* the disk copy procedure reuses the scrub code */
	ret = btrfs_scrub_dev(fs_info, src_device->devid, 0,
			      btrfs_device_get_total_bytes(src_device),
			      &dev_replace->scrub_progress, 0, 1);

	ret = btrfs_dev_replace_finishing(root->fs_info, ret);
	/* don't warn if EINPROGRESS, someone else might be running scrub */
	if (ret == -EINPROGRESS) {
		args->result = BTRFS_IOCTL_DEV_REPLACE_RESULT_SCRUB_INPROGRESS;
		ret = 0;
	} else {
		WARN_ON(ret);
	}

	return ret;

leave:
	dev_replace->srcdev = NULL;
	dev_replace->tgtdev = NULL;
	btrfs_dev_replace_unlock(dev_replace);
	btrfs_destroy_dev_replace_tgtdev(fs_info, tgt_device);
	return ret;
}

/*
 * blocked until all flighting bios are finished.
 */
static void btrfs_rm_dev_replace_blocked(struct btrfs_fs_info *fs_info)
{
	set_bit(BTRFS_FS_STATE_DEV_REPLACING, &fs_info->fs_state);
	wait_event(fs_info->replace_wait, !percpu_counter_sum(
		   &fs_info->bio_counter));
}

/*
 * we have removed target device, it is safe to allow new bios request.
 */
static void btrfs_rm_dev_replace_unblocked(struct btrfs_fs_info *fs_info)
{
	clear_bit(BTRFS_FS_STATE_DEV_REPLACING, &fs_info->fs_state);
	wake_up(&fs_info->replace_wait);
}

static int btrfs_dev_replace_finishing(struct btrfs_fs_info *fs_info,
				       int scrub_ret)
{
	struct btrfs_dev_replace *dev_replace = &fs_info->dev_replace;
	struct btrfs_device *tgt_device;
	struct btrfs_device *src_device;
	struct btrfs_root *root = fs_info->tree_root;
	u8 uuid_tmp[BTRFS_UUID_SIZE];
	struct btrfs_trans_handle *trans;
	int ret = 0;

	/* don't allow cancel or unmount to disturb the finishing procedure */
	mutex_lock(&dev_replace->lock_finishing_cancel_unmount);

	btrfs_dev_replace_lock(dev_replace);
	/* was the operation canceled, or is it finished? */
	if (dev_replace->replace_state !=
	    BTRFS_IOCTL_DEV_REPLACE_STATE_STARTED) {
		btrfs_dev_replace_unlock(dev_replace);
		mutex_unlock(&dev_replace->lock_finishing_cancel_unmount);
		return 0;
	}

	tgt_device = dev_replace->tgtdev;
	src_device = dev_replace->srcdev;
	btrfs_dev_replace_unlock(dev_replace);

	/*
	 * flush all outstanding I/O and inode extent mappings before the
	 * copy operation is declared as being finished
	 */
	ret = btrfs_start_delalloc_roots(root->fs_info, 0, -1);
	if (ret) {
		mutex_unlock(&dev_replace->lock_finishing_cancel_unmount);
		return ret;
	}
	btrfs_wait_ordered_roots(root->fs_info, -1);

	while (1) {
		trans = btrfs_start_transaction(root, 0);
		if (IS_ERR(trans)) {
			mutex_unlock(&dev_replace->lock_finishing_cancel_unmount);
			return PTR_ERR(trans);
		}
		ret = btrfs_commit_transaction(trans, root);
		WARN_ON(ret);
		mutex_lock(&uuid_mutex);
		/* keep away write_all_supers() during the finishing procedure */
		mutex_lock(&root->fs_info->fs_devices->device_list_mutex);
		mutex_lock(&root->fs_info->chunk_mutex);
		if (src_device->has_pending_chunks) {
			mutex_unlock(&root->fs_info->chunk_mutex);
			mutex_unlock(&root->fs_info->fs_devices->device_list_mutex);
			mutex_unlock(&uuid_mutex);
		} else {
			break;
		}
	}

	btrfs_dev_replace_lock(dev_replace);
	dev_replace->replace_state =
		scrub_ret ? BTRFS_IOCTL_DEV_REPLACE_STATE_CANCELED
			  : BTRFS_IOCTL_DEV_REPLACE_STATE_FINISHED;
	dev_replace->tgtdev = NULL;
	dev_replace->srcdev = NULL;
	dev_replace->time_stopped = get_seconds();
	dev_replace->item_needs_writeback = 1;

	/* replace old device with new one in mapping tree */
	if (!scrub_ret) {
		btrfs_dev_replace_update_device_in_mapping_tree(fs_info,
								src_device,
								tgt_device);
	} else {
		btrfs_err_in_rcu(root->fs_info,
			      "btrfs_scrub_dev(%s, %llu, %s) failed %d",
			      src_device->missing ? "<missing disk>" :
			        rcu_str_deref(src_device->name),
			      src_device->devid,
			      rcu_str_deref(tgt_device->name), scrub_ret);
		btrfs_dev_replace_unlock(dev_replace);
		mutex_unlock(&root->fs_info->chunk_mutex);
		mutex_unlock(&root->fs_info->fs_devices->device_list_mutex);
		mutex_unlock(&uuid_mutex);
		if (tgt_device)
			btrfs_destroy_dev_replace_tgtdev(fs_info, tgt_device);
		mutex_unlock(&dev_replace->lock_finishing_cancel_unmount);

		return scrub_ret;
	}

	btrfs_info_in_rcu(root->fs_info,
		      "dev_replace from %s (devid %llu) to %s finished",
		      src_device->missing ? "<missing disk>" :
		        rcu_str_deref(src_device->name),
		      src_device->devid,
		      rcu_str_deref(tgt_device->name));
	tgt_device->is_tgtdev_for_dev_replace = 0;
	tgt_device->devid = src_device->devid;
	src_device->devid = BTRFS_DEV_REPLACE_DEVID;
	memcpy(uuid_tmp, tgt_device->uuid, sizeof(uuid_tmp));
	memcpy(tgt_device->uuid, src_device->uuid, sizeof(tgt_device->uuid));
	memcpy(src_device->uuid, uuid_tmp, sizeof(src_device->uuid));
	btrfs_device_set_total_bytes(tgt_device, src_device->total_bytes);
	btrfs_device_set_disk_total_bytes(tgt_device,
					  src_device->disk_total_bytes);
	btrfs_device_set_bytes_used(tgt_device, src_device->bytes_used);
	ASSERT(list_empty(&src_device->resized_list));
	tgt_device->commit_total_bytes = src_device->commit_total_bytes;
	tgt_device->commit_bytes_used = src_device->bytes_used;
	if (fs_info->sb->s_bdev == src_device->bdev)
		fs_info->sb->s_bdev = tgt_device->bdev;
	if (fs_info->fs_devices->latest_bdev == src_device->bdev)
		fs_info->fs_devices->latest_bdev = tgt_device->bdev;
	list_add(&tgt_device->dev_alloc_list, &fs_info->fs_devices->alloc_list);
	fs_info->fs_devices->rw_devices++;

	btrfs_dev_replace_unlock(dev_replace);

	btrfs_rm_dev_replace_blocked(fs_info);

	btrfs_rm_dev_replace_remove_srcdev(fs_info, src_device);

	btrfs_rm_dev_replace_unblocked(fs_info);

	/*
	 * Increment dev_stats_ccnt so that btrfs_run_dev_stats() will
	 * update on-disk dev stats value during commit transaction
	 */
	atomic_inc(&tgt_device->dev_stats_ccnt);

	/*
	 * this is again a consistent state where no dev_replace procedure
	 * is running, the target device is part of the filesystem, the
	 * source device is not part of the filesystem anymore and its 1st
	 * superblock is scratched out so that it is no longer marked to
	 * belong to this filesystem.
	 */
	mutex_unlock(&root->fs_info->chunk_mutex);
	mutex_unlock(&root->fs_info->fs_devices->device_list_mutex);
	mutex_unlock(&uuid_mutex);

	/* replace the sysfs entry */
	btrfs_sysfs_rm_device_link(fs_info->fs_devices, src_device);
	btrfs_rm_dev_replace_free_srcdev(fs_info, src_device);

	/* write back the superblocks */
	trans = btrfs_start_transaction(root, 0);
	if (!IS_ERR(trans))
		btrfs_commit_transaction(trans, root);

	mutex_unlock(&dev_replace->lock_finishing_cancel_unmount);

	return 0;
}

static void btrfs_dev_replace_update_device_in_mapping_tree(
						struct btrfs_fs_info *fs_info,
						struct btrfs_device *srcdev,
						struct btrfs_device *tgtdev)
{
	struct extent_map_tree *em_tree = &fs_info->mapping_tree.map_tree;
	struct extent_map *em;
	struct map_lookup *map;
	u64 start = 0;
	int i;

	write_lock(&em_tree->lock);
	do {
		em = lookup_extent_mapping(em_tree, start, (u64)-1);
		if (!em)
			break;
		map = em->map_lookup;
		for (i = 0; i < map->num_stripes; i++)
			if (srcdev == map->stripes[i].dev)
				map->stripes[i].dev = tgtdev;
		start = em->start + em->len;
		free_extent_map(em);
	} while (start);
	write_unlock(&em_tree->lock);
}

static int btrfs_dev_replace_find_srcdev(struct btrfs_root *root, u64 srcdevid,
					 char *srcdev_name,
					 struct btrfs_device **device)
{
	int ret;

	if (srcdevid) {
		ret = 0;
		*device = btrfs_find_device(root->fs_info, srcdevid, NULL,
					    NULL);
		if (!*device)
			ret = -ENOENT;
	} else {
		ret = btrfs_find_device_missing_or_by_path(root, srcdev_name,
							   device);
	}
	return ret;
}

void btrfs_dev_replace_status(struct btrfs_fs_info *fs_info,
			      struct btrfs_ioctl_dev_replace_args *args)
{
	struct btrfs_dev_replace *dev_replace = &fs_info->dev_replace;
	struct btrfs_device *srcdev;

	btrfs_dev_replace_lock(dev_replace);
	/* even if !dev_replace_is_valid, the values are good enough for
	 * the replace_status ioctl */
	args->result = BTRFS_IOCTL_DEV_REPLACE_RESULT_NO_ERROR;
	args->status.replace_state = dev_replace->replace_state;
	args->status.time_started = dev_replace->time_started;
	args->status.time_stopped = dev_replace->time_stopped;
	args->status.num_write_errors =
		atomic64_read(&dev_replace->num_write_errors);
	args->status.num_uncorrectable_read_errors =
		atomic64_read(&dev_replace->num_uncorrectable_read_errors);
	switch (dev_replace->replace_state) {
	case BTRFS_IOCTL_DEV_REPLACE_STATE_NEVER_STARTED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_CANCELED:
		args->status.progress_1000 = 0;
		break;
	case BTRFS_IOCTL_DEV_REPLACE_STATE_FINISHED:
		args->status.progress_1000 = 1000;
		break;
	case BTRFS_IOCTL_DEV_REPLACE_STATE_STARTED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_SUSPENDED:
		srcdev = dev_replace->srcdev;
		args->status.progress_1000 = div_u64(dev_replace->cursor_left,
			div_u64(btrfs_device_get_total_bytes(srcdev), 1000));
		break;
	}
	btrfs_dev_replace_unlock(dev_replace);
}

int btrfs_dev_replace_cancel(struct btrfs_fs_info *fs_info,
			     struct btrfs_ioctl_dev_replace_args *args)
{
	args->result = __btrfs_dev_replace_cancel(fs_info);
	return 0;
}

static u64 __btrfs_dev_replace_cancel(struct btrfs_fs_info *fs_info)
{
	struct btrfs_dev_replace *dev_replace = &fs_info->dev_replace;
	struct btrfs_device *tgt_device = NULL;
	struct btrfs_trans_handle *trans;
	struct btrfs_root *root = fs_info->tree_root;
	u64 result;
	int ret;

	if (fs_info->sb->s_flags & MS_RDONLY)
		return -EROFS;

	mutex_lock(&dev_replace->lock_finishing_cancel_unmount);
	btrfs_dev_replace_lock(dev_replace);
	switch (dev_replace->replace_state) {
	case BTRFS_IOCTL_DEV_REPLACE_STATE_NEVER_STARTED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_FINISHED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_CANCELED:
		result = BTRFS_IOCTL_DEV_REPLACE_RESULT_NOT_STARTED;
		btrfs_dev_replace_unlock(dev_replace);
		goto leave;
	case BTRFS_IOCTL_DEV_REPLACE_STATE_STARTED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_SUSPENDED:
		result = BTRFS_IOCTL_DEV_REPLACE_RESULT_NO_ERROR;
		tgt_device = dev_replace->tgtdev;
		dev_replace->tgtdev = NULL;
		dev_replace->srcdev = NULL;
		break;
	}
	dev_replace->replace_state = BTRFS_IOCTL_DEV_REPLACE_STATE_CANCELED;
	dev_replace->time_stopped = get_seconds();
	dev_replace->item_needs_writeback = 1;
	btrfs_dev_replace_unlock(dev_replace);
	btrfs_scrub_cancel(fs_info);

	trans = btrfs_start_transaction(root, 0);
	if (IS_ERR(trans)) {
		mutex_unlock(&dev_replace->lock_finishing_cancel_unmount);
		return PTR_ERR(trans);
	}
	ret = btrfs_commit_transaction(trans, root);
	WARN_ON(ret);
	if (tgt_device)
		btrfs_destroy_dev_replace_tgtdev(fs_info, tgt_device);

leave:
	mutex_unlock(&dev_replace->lock_finishing_cancel_unmount);
	return result;
}

void btrfs_dev_replace_suspend_for_unmount(struct btrfs_fs_info *fs_info)
{
	struct btrfs_dev_replace *dev_replace = &fs_info->dev_replace;

	mutex_lock(&dev_replace->lock_finishing_cancel_unmount);
	btrfs_dev_replace_lock(dev_replace);
	switch (dev_replace->replace_state) {
	case BTRFS_IOCTL_DEV_REPLACE_STATE_NEVER_STARTED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_FINISHED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_CANCELED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_SUSPENDED:
		break;
	case BTRFS_IOCTL_DEV_REPLACE_STATE_STARTED:
		dev_replace->replace_state =
			BTRFS_IOCTL_DEV_REPLACE_STATE_SUSPENDED;
		dev_replace->time_stopped = get_seconds();
		dev_replace->item_needs_writeback = 1;
		btrfs_info(fs_info, "suspending dev_replace for unmount");
		break;
	}

	btrfs_dev_replace_unlock(dev_replace);
	mutex_unlock(&dev_replace->lock_finishing_cancel_unmount);
}

/* resume dev_replace procedure that was interrupted by unmount */
int btrfs_resume_dev_replace_async(struct btrfs_fs_info *fs_info)
{
	struct task_struct *task;
	struct btrfs_dev_replace *dev_replace = &fs_info->dev_replace;

	btrfs_dev_replace_lock(dev_replace);
	switch (dev_replace->replace_state) {
	case BTRFS_IOCTL_DEV_REPLACE_STATE_NEVER_STARTED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_FINISHED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_CANCELED:
		btrfs_dev_replace_unlock(dev_replace);
		return 0;
	case BTRFS_IOCTL_DEV_REPLACE_STATE_STARTED:
		break;
	case BTRFS_IOCTL_DEV_REPLACE_STATE_SUSPENDED:
		dev_replace->replace_state =
			BTRFS_IOCTL_DEV_REPLACE_STATE_STARTED;
		break;
	}
	if (!dev_replace->tgtdev || !dev_replace->tgtdev->bdev) {
		btrfs_info(fs_info, "cannot continue dev_replace, tgtdev is missing");
		btrfs_info(fs_info,
			"you may cancel the operation after 'mount -o degraded'");
		btrfs_dev_replace_unlock(dev_replace);
		return 0;
	}
	btrfs_dev_replace_unlock(dev_replace);

	WARN_ON(atomic_xchg(
		&fs_info->mutually_exclusive_operation_running, 1));
	task = kthread_run(btrfs_dev_replace_kthread, fs_info, "btrfs-devrepl");
	return PTR_ERR_OR_ZERO(task);
}

static int btrfs_dev_replace_kthread(void *data)
{
	struct btrfs_fs_info *fs_info = data;
	struct btrfs_dev_replace *dev_replace = &fs_info->dev_replace;
	struct btrfs_ioctl_dev_replace_args *status_args;
	u64 progress;

	status_args = kzalloc(sizeof(*status_args), GFP_NOFS);
	if (status_args) {
		btrfs_dev_replace_status(fs_info, status_args);
		progress = status_args->status.progress_1000;
		kfree(status_args);
		progress = div_u64(progress, 10);
		btrfs_info_in_rcu(fs_info,
			"continuing dev_replace from %s (devid %llu) to %s @%u%%",
			dev_replace->srcdev->missing ? "<missing disk>" :
			rcu_str_deref(dev_replace->srcdev->name),
			dev_replace->srcdev->devid,
			dev_replace->tgtdev ?
			rcu_str_deref(dev_replace->tgtdev->name) :
			"<missing target disk>",
			(unsigned int)progress);
	}
	btrfs_dev_replace_continue_on_mount(fs_info);
	atomic_set(&fs_info->mutually_exclusive_operation_running, 0);

	return 0;
}

static int btrfs_dev_replace_continue_on_mount(struct btrfs_fs_info *fs_info)
{
	struct btrfs_dev_replace *dev_replace = &fs_info->dev_replace;
	int ret;

	ret = btrfs_scrub_dev(fs_info, dev_replace->srcdev->devid,
			      dev_replace->committed_cursor_left,
			      btrfs_device_get_total_bytes(dev_replace->srcdev),
			      &dev_replace->scrub_progress, 0, 1);
	ret = btrfs_dev_replace_finishing(fs_info, ret);
	WARN_ON(ret);
	return 0;
}

int btrfs_dev_replace_is_ongoing(struct btrfs_dev_replace *dev_replace)
{
	if (!dev_replace->is_valid)
		return 0;

	switch (dev_replace->replace_state) {
	case BTRFS_IOCTL_DEV_REPLACE_STATE_NEVER_STARTED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_FINISHED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_CANCELED:
		return 0;
	case BTRFS_IOCTL_DEV_REPLACE_STATE_STARTED:
	case BTRFS_IOCTL_DEV_REPLACE_STATE_SUSPENDED:
		/*
		 * return true even if tgtdev is missing (this is
		 * something that can happen if the dev_replace
		 * procedure is suspended by an umount and then
		 * the tgtdev is missing (or "btrfs dev scan") was
		 * not called and the the filesystem is remounted
		 * in degraded state. This does not stop the
		 * dev_replace procedure. It needs to be canceled
		 * manually if the cancelation is wanted.
		 */
		break;
	}
	return 1;
}

void btrfs_dev_replace_lock(struct btrfs_dev_replace *dev_replace)
{
	/* the beginning is just an optimization for the typical case */
	if (atomic_read(&dev_replace->nesting_level) == 0) {
acquire_lock:
		/* this is not a nested case where the same thread
		 * is trying to acqurire the same lock twice */
		mutex_lock(&dev_replace->lock);
		mutex_lock(&dev_replace->lock_management_lock);
		dev_replace->lock_owner = current->pid;
		atomic_inc(&dev_replace->nesting_level);
		mutex_unlock(&dev_replace->lock_management_lock);
		return;
	}

	mutex_lock(&dev_replace->lock_management_lock);
	if (atomic_read(&dev_replace->nesting_level) > 0 &&
	    dev_replace->lock_owner == current->pid) {
		WARN_ON(!mutex_is_locked(&dev_replace->lock));
		atomic_inc(&dev_replace->nesting_level);
		mutex_unlock(&dev_replace->lock_management_lock);
		return;
	}

	mutex_unlock(&dev_replace->lock_management_lock);
	goto acquire_lock;
}

void btrfs_dev_replace_unlock(struct btrfs_dev_replace *dev_replace)
{
	WARN_ON(!mutex_is_locked(&dev_replace->lock));
	mutex_lock(&dev_replace->lock_management_lock);
	WARN_ON(atomic_read(&dev_replace->nesting_level) < 1);
	WARN_ON(dev_replace->lock_owner != current->pid);
	atomic_dec(&dev_replace->nesting_level);
	if (atomic_read(&dev_replace->nesting_level) == 0) {
		dev_replace->lock_owner = 0;
		mutex_unlock(&dev_replace->lock_management_lock);
		mutex_unlock(&dev_replace->lock);
	} else {
		mutex_unlock(&dev_replace->lock_management_lock);
	}
}

void btrfs_bio_counter_inc_noblocked(struct btrfs_fs_info *fs_info)
{
	percpu_counter_inc(&fs_info->bio_counter);
}

void btrfs_bio_counter_sub(struct btrfs_fs_info *fs_info, s64 amount)
{
	percpu_counter_sub(&fs_info->bio_counter, amount);

	if (waitqueue_active(&fs_info->replace_wait))
		wake_up(&fs_info->replace_wait);
}

void btrfs_bio_counter_inc_blocked(struct btrfs_fs_info *fs_info)
{
	while (1) {
		percpu_counter_inc(&fs_info->bio_counter);
		if (likely(!test_bit(BTRFS_FS_STATE_DEV_REPLACING,
				     &fs_info->fs_state)))
			break;

		btrfs_bio_counter_dec(fs_info);
		wait_event(fs_info->replace_wait,
			   !test_bit(BTRFS_FS_STATE_DEV_REPLACING,
				     &fs_info->fs_state));
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 * Copyright (C) STRATO AG 2012.  All rights reserved.
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

#if !defined(__BTRFS_DEV_REPLACE__)
#define __BTRFS_DEV_REPLACE__

struct btrfs_ioctl_dev_replace_args;

int btrfs_init_dev_replace(struct btrfs_fs_info *fs_info);
int btrfs_run_dev_replace(struct btrfs_trans_handle *trans,
			  struct btrfs_fs_info *fs_info);
void btrfs_after_dev_replace_commit(struct btrfs_fs_info *fs_info);
int btrfs_dev_replace_start(struct btrfs_root *root,
			    struct btrfs_ioctl_dev_replace_args *args);
void btrfs_dev_replace_status(struct btrfs_fs_info *fs_info,
			      struct btrfs_ioctl_dev_replace_args *args);
int btrfs_dev_replace_cancel(struct btrfs_fs_info *fs_info,
			     struct btrfs_ioctl_dev_replace_args *args);
void btrfs_dev_replace_suspend_for_unmount(struct btrfs_fs_info *fs_info);
int btrfs_resume_dev_replace_async(struct btrfs_fs_info *fs_info);
int btrfs_dev_replace_is_ongoing(struct btrfs_dev_replace *dev_replace);
void btrfs_dev_replace_lock(struct btrfs_dev_replace *dev_replace);
void btrfs_dev_replace_unlock(struct btrfs_dev_replace *dev_replace);

static inline void btrfs_dev_replace_stats_inc(atomic64_t *stat_value)
{
	atomic64_inc(stat_value);
}
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
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

/*
 * insert a name into a directory, doing overflow properly if there is a hash
 * collision.  data_size indicates how big the item inserted should be.  On
 * success a struct btrfs_dir_item pointer is returned, otherwise it is
 * an ERR_PTR.
 *
 * The name is not copied into the dir item, you have to do that yourself.
 */
static struct btrfs_dir_item *insert_with_overflow(struct btrfs_trans_handle
						   *trans,
						   struct btrfs_root *root,
						   struct btrfs_path *path,
						   struct btrfs_key *cpu_key,
						   u32 data_size,
						   const char *name,
						   int name_len)
{
	int ret;
	char *ptr;
	struct btrfs_item *item;
	struct extent_buffer *leaf;

	ret = btrfs_insert_empty_item(trans, root, path, cpu_key, data_size);
	if (ret == -EEXIST) {
		struct btrfs_dir_item *di;
		di = btrfs_match_dir_item_name(root, path, name, name_len);
		if (di)
			return ERR_PTR(-EEXIST);
		btrfs_extend_item(root, path, data_size);
	} else if (ret < 0)
		return ERR_PTR(ret);
	WARN_ON(ret > 0);
	leaf = path->nodes[0];
	item = btrfs_item_nr(path->slots[0]);
	ptr = btrfs_item_ptr(leaf, path->slots[0], char);
	BUG_ON(data_size > btrfs_item_size(leaf, item));
	ptr += btrfs_item_size(leaf, item) - data_size;
	return (struct btrfs_dir_item *)ptr;
}

/*
 * xattrs work a lot like directories, this inserts an xattr item
 * into the tree
 */
int btrfs_insert_xattr_item(struct btrfs_trans_handle *trans,
			    struct btrfs_root *root,
			    struct btrfs_path *path, u64 objectid,
			    const char *name, u16 name_len,
			    const void *data, u16 data_len)
{
	int ret = 0;
	struct btrfs_dir_item *dir_item;
	unsigned long name_ptr, data_ptr;
	struct btrfs_key key, location;
	struct btrfs_disk_key disk_key;
	struct extent_buffer *leaf;
	u32 data_size;

	BUG_ON(name_len + data_len > BTRFS_MAX_XATTR_SIZE(root));

	key.objectid = objectid;
	key.type = BTRFS_XATTR_ITEM_KEY;
	key.offset = btrfs_name_hash(name, name_len);

	data_size = sizeof(*dir_item) + name_len + data_len;
	dir_item = insert_with_overflow(trans, root, path, &key, data_size,
					name, name_len);
	if (IS_ERR(dir_item))
		return PTR_ERR(dir_item);
	memset(&location, 0, sizeof(location));

	leaf = path->nodes[0];
	btrfs_cpu_key_to_disk(&disk_key, &location);
	btrfs_set_dir_item_key(leaf, dir_item, &disk_key);
	btrfs_set_dir_type(leaf, dir_item, BTRFS_FT_XATTR);
	btrfs_set_dir_name_len(leaf, dir_item, name_len);
	btrfs_set_dir_transid(leaf, dir_item, trans->transid);
	btrfs_set_dir_data_len(leaf, dir_item, data_len);
	name_ptr = (unsigned long)(dir_item + 1);
	data_ptr = (unsigned long)((char *)name_ptr + name_len);

	write_extent_buffer(leaf, name, name_ptr, name_len);
	write_extent_buffer(leaf, data, data_ptr, data_len);
	btrfs_mark_buffer_dirty(path->nodes[0]);

	return ret;
}

/*
 * insert a directory item in the tree, doing all the magic for
 * both indexes. 'dir' indicates which objectid to insert it into,
 * 'location' is the key to stuff into the directory item, 'type' is the
 * type of the inode we're pointing to, and 'index' is the sequence number
 * to use for the second index (if one is created).
 * Will return 0 or -ENOMEM
 */
int btrfs_insert_dir_item(struct btrfs_trans_handle *trans, struct btrfs_root
			  *root, const char *name, int name_len,
			  struct inode *dir, struct btrfs_key *location,
			  u8 type, u64 index)
{
	int ret = 0;
	int ret2 = 0;
	struct btrfs_path *path;
	struct btrfs_dir_item *dir_item;
	struct extent_buffer *leaf;
	unsigned long name_ptr;
	struct btrfs_key key;
	struct btrfs_disk_key disk_key;
	u32 data_size;

	key.objectid = btrfs_ino(dir);
	key.type = BTRFS_DIR_ITEM_KEY;
	key.offset = btrfs_name_hash(name, name_len);

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;
	path->leave_spinning = 1;

	btrfs_cpu_key_to_disk(&disk_key, location);

	data_size = sizeof(*dir_item) + name_len;
	dir_item = insert_with_overflow(trans, root, path, &key, data_size,
					name, name_len);
	if (IS_ERR(dir_item)) {
		ret = PTR_ERR(dir_item);
		if (ret == -EEXIST)
			goto second_insert;
		goto out_free;
	}

	leaf = path->nodes[0];
	btrfs_set_dir_item_key(leaf, dir_item, &disk_key);
	btrfs_set_dir_type(leaf, dir_item, type);
	btrfs_set_dir_data_len(leaf, dir_item, 0);
	btrfs_set_dir_name_len(leaf, dir_item, name_len);
	btrfs_set_dir_transid(leaf, dir_item, trans->transid);
	name_ptr = (unsigned long)(dir_item + 1);

	write_extent_buffer(leaf, name, name_ptr, name_len);
	btrfs_mark_buffer_dirty(leaf);

second_insert:
	/* FIXME, use some real flag for selecting the extra index */
	if (root == root->fs_info->tree_root) {
		ret = 0;
		goto out_free;
	}
	btrfs_release_path(path);

	ret2 = btrfs_insert_delayed_dir_index(trans, root, name, name_len, dir,
					      &disk_key, type, index);
out_free:
	btrfs_free_path(path);
	if (ret)
		return ret;
	if (ret2)
		return ret2;
	return 0;
}

/*
 * lookup a directory item based on name.  'dir' is the objectid
 * we're searching in, and 'mod' tells us if you plan on deleting the
 * item (use mod < 0) or changing the options (use mod > 0)
 */
struct btrfs_dir_item *btrfs_lookup_dir_item(struct btrfs_trans_handle *trans,
					     struct btrfs_root *root,
					     struct btrfs_path *path, u64 dir,
					     const char *name, int name_len,
					     int mod)
{
	int ret;
	struct btrfs_key key;
	int ins_len = mod < 0 ? -1 : 0;
	int cow = mod != 0;

	key.objectid = dir;
	key.type = BTRFS_DIR_ITEM_KEY;

	key.offset = btrfs_name_hash(name, name_len);

	ret = btrfs_search_slot(trans, root, &key, path, ins_len, cow);
	if (ret < 0)
		return ERR_PTR(ret);
	if (ret > 0)
		return NULL;

	return btrfs_match_dir_item_name(root, path, name, name_len);
}

int btrfs_check_dir_item_collision(struct btrfs_root *root, u64 dir,
				   const char *name, int name_len)
{
	int ret;
	struct btrfs_key key;
	struct btrfs_dir_item *di;
	int data_size;
	struct extent_buffer *leaf;
	int slot;
	struct btrfs_path *path;


	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	key.objectid = dir;
	key.type = BTRFS_DIR_ITEM_KEY;
	key.offset = btrfs_name_hash(name, name_len);

	ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);

	/* return back any errors */
	if (ret < 0)
		goto out;

	/* nothing found, we're safe */
	if (ret > 0) {
		ret = 0;
		goto out;
	}

	/* we found an item, look for our name in the item */
	di = btrfs_match_dir_item_name(root, path, name, name_len);
	if (di) {
		/* our exact name was found */
		ret = -EEXIST;
		goto out;
	}

	/*
	 * see if there is room in the item to insert this
	 * name
	 */
	data_size = sizeof(*di) + name_len;
	leaf = path->nodes[0];
	slot = path->slots[0];
	if (data_size + btrfs_item_size_nr(leaf, slot) +
	    sizeof(struct btrfs_item) > BTRFS_LEAF_DATA_SIZE(root)) {
		ret = -EOVERFLOW;
	} else {
		/* plenty of insertion room */
		ret = 0;
	}
out:
	btrfs_free_path(path);
	return ret;
}

/*
 * lookup a directory item based on index.  'dir' is the objectid
 * we're searching in, and 'mod' tells us if you plan on deleting the
 * item (use mod < 0) or changing the options (use mod > 0)
 *
 * The name is used to make sure the index really points to the name you were
 * looking for.
 */
struct btrfs_dir_item *
btrfs_lookup_dir_index_item(struct btrfs_trans_handle *trans,
			    struct btrfs_root *root,
			    struct btrfs_path *path, u64 dir,
			    u64 objectid, const char *name, int name_len,
			    int mod)
{
	int ret;
	struct btrfs_key key;
	int ins_len = mod < 0 ? -1 : 0;
	int cow = mod != 0;

	key.objectid = dir;
	key.type = BTRFS_DIR_INDEX_KEY;
	key.offset = objectid;

	ret = btrfs_search_slot(trans, root, &key, path, ins_len, cow);
	if (ret < 0)
		return ERR_PTR(ret);
	if (ret > 0)
		return ERR_PTR(-ENOENT);
	return btrfs_match_dir_item_name(root, path, name, name_len);
}

struct btrfs_dir_item *
btrfs_search_dir_index_item(struct btrfs_root *root,
			    struct btrfs_path *path, u64 dirid,
			    const char *name, int name_len)
{
	struct extent_buffer *leaf;
	struct btrfs_dir_item *di;
	struct btrfs_key key;
	u32 nritems;
	int ret;

	key.objectid = dirid;
	key.type = BTRFS_DIR_INDEX_KEY;
	key.offset = 0;

	ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
	if (ret < 0)
		return ERR_PTR(ret);

	leaf = path->nodes[0];
	nritems = btrfs_header_nritems(leaf);

	while (1) {
		if (path->slots[0] >= nritems) {
			ret = btrfs_next_leaf(root, path);
			if (ret < 0)
				return ERR_PTR(ret);
			if (ret > 0)
				break;
			leaf = path->nodes[0];
			nritems = btrfs_header_nritems(leaf);
			continue;
		}

		btrfs_item_key_to_cpu(leaf, &key, path->slots[0]);
		if (key.objectid != dirid || key.type != BTRFS_DIR_INDEX_KEY)
			break;

		di = btrfs_match_dir_item_name(root, path, name, name_len);
		if (di)
			return di;

		path->slots[0]++;
	}
	return NULL;
}

struct btrfs_dir_item *btrfs_lookup_xattr(struct btrfs_trans_handle *trans,
					  struct btrfs_root *root,
					  struct btrfs_path *path, u64 dir,
					  const char *name, u16 name_len,
					  int mod)
{
	int ret;
	struct btrfs_key key;
	int ins_len = mod < 0 ? -1 : 0;
	int cow = mod != 0;

	key.objectid = dir;
	key.type = BTRFS_XATTR_ITEM_KEY;
	key.offset = btrfs_name_hash(name, name_len);
	ret = btrfs_search_slot(trans, root, &key, path, ins_len, cow);
	if (ret < 0)
		return ERR_PTR(ret);
	if (ret > 0)
		return NULL;

	return btrfs_match_dir_item_name(root, path, name, name_len);
}

/*
 * helper function to look at the directory item pointed to by 'path'
 * this walks through all the entries in a dir item and finds one
 * for a specific name.
 */
struct btrfs_dir_item *btrfs_match_dir_item_name(struct btrfs_root *root,
						 struct btrfs_path *path,
						 const char *name, int name_len)
{
	struct btrfs_dir_item *dir_item;
	unsigned long name_ptr;
	u32 total_len;
	u32 cur = 0;
	u32 this_len;
	struct extent_buffer *leaf;

	leaf = path->nodes[0];
	dir_item = btrfs_item_ptr(leaf, path->slots[0], struct btrfs_dir_item);
	if (verify_dir_item(root, leaf, dir_item))
		return NULL;

	total_len = btrfs_item_size_nr(leaf, path->slots[0]);
	while (cur < total_len) {
		this_len = sizeof(*dir_item) +
			btrfs_dir_name_len(leaf, dir_item) +
			btrfs_dir_data_len(leaf, dir_item);
		name_ptr = (unsigned long)(dir_item + 1);

		if (btrfs_dir_name_len(leaf, dir_item) == name_len &&
		    memcmp_extent_buffer(leaf, name, name_ptr, name_len) == 0)
			return dir_item;

		cur += this_len;
		dir_item = (struct btrfs_dir_item *)((char *)dir_item +
						     this_len);
	}
	return NULL;
}

/*
 * given a pointer into a directory item, delete it.  This
 * handles items that have more than one entry in them.
 */
int btrfs_delete_one_dir_name(struct btrfs_trans_handle *trans,
			      struct btrfs_root *root,
			      struct btrfs_path *path,
			      struct btrfs_dir_item *di)
{

	struct extent_buffer *leaf;
	u32 sub_item_len;
	u32 item_len;
	int ret = 0;

	leaf = path->nodes[0];
	sub_item_len = sizeof(*di) + btrfs_dir_name_len(leaf, di) +
		btrfs_dir_data_len(leaf, di);
	item_len = btrfs_item_size_nr(leaf, path->slots[0]);
	if (sub_item_len == item_len) {
		ret = btrfs_del_item(trans, root, path);
	} else {
		/* MARKER */
		unsigned long ptr = (unsigned long)di;
		unsigned long start;

		start = btrfs_item_ptr_offset(leaf, path->slots[0]);
		memmove_extent_buffer(leaf, ptr, ptr + sub_item_len,
			item_len - (ptr + sub_item_len - start));
		btrfs_truncate_item(root, path, item_len - sub_item_len, 1);
	}
	return ret;
}

int verify_dir_item(struct btrfs_root *root,
		    struct extent_buffer *leaf,
		    struct btrfs_dir_item *dir_item)
{
	u16 namelen = BTRFS_NAME_LEN;
	u8 type = btrfs_dir_type(leaf, dir_item);

	if (type >= BTRFS_FT_MAX) {
		btrfs_crit(root->fs_info, "invalid dir item type: %d",
		       (int)type);
		return 1;
	}

	if (type == BTRFS_FT_XATTR)
		namelen = XATTR_NAME_MAX;

	if (btrfs_dir_name_len(leaf, dir_item) > namelen) {
		btrfs_crit(root->fs_info, "invalid dir item name len: %u",
		       (unsigned)btrfs_dir_data_len(leaf, dir_item));
		return 1;
	}

	/* BTRFS_MAX_XATTR_SIZE is the same for all dir items */
	if ((btrfs_dir_data_len(leaf, dir_item) +
	     btrfs_dir_name_len(leaf, dir_item)) > BTRFS_MAX_XATTR_SIZE(root)) {
		btrfs_crit(root->fs_info, "invalid dir item name + data len: %u + %u",
		       (unsigned)btrfs_dir_name_len(leaf, dir_item),
		       (unsigned)btrfs_dir_data_len(leaf, dir_item));
		return 1;
	}

	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*
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

#ifndef __DISKIO__
#define __DISKIO__

#define BTRFS_SUPER_INFO_OFFSET (64 * 1024)
#define BTRFS_SUPER_INFO_SIZE 4096

#define BTRFS_SUPER_MIRROR_MAX	 3
#define BTRFS_SUPER_MIRROR_SHIFT 12

enum btrfs_wq_endio_type {
	BTRFS_WQ_ENDIO_DATA = 0,
	BTRFS_WQ_ENDIO_METADATA = 1,
	BTRFS_WQ_ENDIO_FREE_SPACE = 2,
	BTRFS_WQ_ENDIO_RAID56 = 3,
	BTRFS_WQ_ENDIO_DIO_REPAIR = 4,
};

static inline u64 btrfs_sb_offset(int mirror)
{
	u64 start = 16 * 1024;
	if (mirror)
		return start << (BTRFS_SUPER_MIRROR_SHIFT * mirror);
	return BTRFS_SUPER_INFO_OFFSET;
}

struct btrfs_device;
struct btrfs_fs_devices;

struct extent_buffer *read_tree_block(struct btrfs_root *root, u64 bytenr,
				      u64 parent_transid);
void readahead_tree_block(struct btrfs_root *root, u64 bytenr);
int reada_tree_block_flagged(struct btrfs_root *root, u64 bytenr,
			 int mirror_num, struct extent_buffer **eb);
struct extent_buffer *btrfs_find_create_tree_block(struct btrfs_root *root,
						   u64 bytenr);
void clean_tree_block(struct btrfs_trans_handle *trans,
		      struct btrfs_fs_info *fs_info, struct extent_buffer *buf);
int open_ctree(struct super_block *sb,
	       struct btrfs_fs_devices *fs_devices,
	       char *options);
void close_ctree(struct btrfs_root *root);
int write_ctree_super(struct btrfs_trans_handle *trans,
		      struct btrfs_root *root, int max_mirrors);
struct buffer_head *btrfs_read_dev_super(struct block_device *bdev);
int btrfs_read_dev_one_super(struct block_device *bdev, int copy_num,
			struct buffer_head **bh_ret);
int btrfs_commit_super(struct btrfs_root *root);
struct extent_buffer *btrfs_find_tree_block(struct btrfs_fs_info *fs_info,
					    u64 bytenr);
struct btrfs_root *btrfs_read_fs_root(struct btrfs_root *tree_root,
				      struct btrfs_key *location);
int btrfs_init_fs_root(struct btrfs_root *root);
struct btrfs_root *btrfs_lookup_fs_root(struct btrfs_fs_info *fs_info,
					u64 root_id);
int btrfs_insert_fs_root(struct btrfs_fs_info *fs_info,
			 struct btrfs_root *root);
void btrfs_free_fs_roots(struct btrfs_fs_info *fs_info);

struct btrfs_root *btrfs_get_fs_root(struct btrfs_fs_info *fs_info,
				     struct btrfs_key *key,
				     bool check_ref);
static inline struct btrfs_root *
btrfs_read_fs_root_no_name(struct btrfs_fs_info *fs_info,
			   struct btrfs_key *location)
{
	return btrfs_get_fs_root(fs_info, location, true);
}

int btrfs_cleanup_fs_roots(struct btrfs_fs_info *fs_info);
void btrfs_btree_balance_dirty(struct btrfs_root *root);
void btrfs_btree_balance_dirty_nodelay(struct btrfs_root *root);
void btrfs_drop_and_free_fs_root(struct btrfs_fs_info *fs_info,
				 struct btrfs_root *root);
void btrfs_free_fs_root(struct btrfs_root *root);

#ifdef CONFIG_BTRFS_FS_RUN_SANITY_TESTS
struct btrfs_root *btrfs_alloc_dummy_root(void);
#endif

/*
 * This function is used to grab the root, and avoid it is freed when we
 * access it. But it doesn't ensure that the tree is not dropped.
 *
 * If you want to ensure the whole tree is safe, you should use
 * 	fs_info->subvol_srcu
 */
static inline struct btrfs_root *btrfs_grab_fs_root(struct btrfs_root *root)
{
	if (atomic_inc_not_zero(&root->refs))
		return root;
	return NULL;
}

static inline void btrfs_put_fs_root(struct btrfs_root *root)
{
	if (atomic_dec_and_test(&root->refs))
		kfree(root);
}

void btrfs_mark_buffer_dirty(struct extent_buffer *buf);
int btrfs_buffer_uptodate(struct extent_buffer *buf, u64 parent_transid,
			  int atomic);
int btrfs_set_buffer_uptodate(struct extent_buffer *buf);
int btrfs_read_buffer(struct extent_buffer *buf, u64 parent_transid);
u32 btrfs_csum_data(char *data, u32 seed, size_t len);
void btrfs_csum_final(u32 crc, char *result);
int btrfs_bio_wq_end_io(struct btrfs_fs_info *info, struct bio *bio,
			enum btrfs_wq_endio_type metadata);
int btrfs_wq_submit_bio(struct btrfs_fs_info *fs_info, struct inode *inode,
			int rw, struct bio *bio, int mirror_num,
			unsigned long bio_flags, u64 bio_offset,
			extent_submit_bio_hook_t *submit_bio_start,
			extent_submit_bio_hook_t *submit_bio_done);
unsigned long btrfs_async_submit_limit(struct btrfs_fs_info *info);
int btrfs_write_tree_block(struct extent_buffer *buf);
int btrfs_wait_tree_block_writeback(struct extent_buffer *buf);
int btrfs_init_log_root_tree(struct btrfs_trans_handle *trans,
			     struct btrfs_fs_info *fs_info);
int btrfs_add_log_tree(struct btrfs_trans_handle *trans,
		       struct btrfs_root *root);
void btrfs_cleanup_one_transaction(struct btrfs_transaction *trans,
				  struct btrfs_root *root);
struct btrfs_root *btrfs_create_tree(struct btrfs_trans_handle *trans,
				     struct btrfs_fs_info *fs_info,
				     u64 objectid);
int btree_lock_page_hook(struct page *page, void *data,
				void (*flush_fn)(void *));
int btrfs_get_num_tolerated_disk_barrier_failures(u64 flags);
int btrfs_calc_num_tolerated_disk_barrier_failures(
	struct btrfs_fs_info *fs_info);
int __init btrfs_end_io_wq_init(void);
void btrfs_end_io_wq_exit(void);

#ifdef CONFIG_DEBUG_LOCK_ALLOC
void btrfs_init_lockdep(void);
void btrfs_set_buffer_lockdep_class(u64 objectid,
			            struct extent_buffer *eb, int level);
#else
static inline void btrfs_init_lockdep(void)
{ }
static inline void btrfs_set_buffer_lockdep_class(u64 objectid,
					struct extent_buffer *eb, int level)
{
}
#endif
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        #include <linux/fs.h>
#include <linux/types.h>
#include "ctree.h"
#include "disk-io.h"
#include "btrfs_inode.h"
#include "print-tree.h"
#include "export.h"

#define BTRFS_FID_SIZE_NON_CONNECTABLE (offsetof(struct btrfs_fid, \
						 parent_objectid) / 4)
#define BTRFS_FID_SIZE_CONNECTABLE (offsetof(struct btrfs_fid, \
					     parent_root_objectid) / 4)
#define BTRFS_FID_SIZE_CONNECTABLE_ROOT (sizeof(struct btrfs_fid) / 4)

static int btrfs_encode_fh(struct inode *inode, u32 *fh, int *max_len,
			   struct inode *parent)
{
	struct btrfs_fid *fid = (struct btrfs_fid *)fh;
	int len = *max_len;
	int type;

	if (parent && (len < BTRFS_FID_SIZE_CONNECTABLE)) {
		*max_len = BTRFS_FID_SIZE_CONNECTABLE;
		return FILEID_INVALID;
	} else if (len < BTRFS_FID_SIZE_NON_CONNECTABLE) {
		*max_len = BTRFS_FID_SIZE_NON_CONNECTABLE;
		return FILEID_INVALID;
	}

	len  = BTRFS_FID_SIZE_NON_CONNECTABLE;
	type = FILEID_BTRFS_WITHOUT_PARENT;

	fid->objectid = btrfs_ino(inode);
	fid->root_objectid = BTRFS_I(inode)->root->objectid;
	fid->gen = inode->i_generation;

	if (parent) {
		u64 parent_root_id;

		fid->parent_objectid = BTRFS_I(parent)->location.objectid;
		fid->parent_gen = parent->i_generation;
		parent_root_id = BTRFS_I(parent)->root->objectid;

		if (parent_root_id != fid->root_objectid) {
			fid->parent_root_objectid = parent_root_id;
			len = BTRFS_FID_SIZE_CONNECTABLE_ROOT;
			type = FILEID_BTRFS_WITH_PARENT_ROOT;
		} else {
			len = BTRFS_FID_SIZE_CONNECTABLE;
			type = FILEID_BTRFS_WITH_PARENT;
		}
	}

	*max_len = len;
	return type;
}

struct dentry *btrfs_get_dentry(struct super_block *sb, u64 objectid,
				u64 root_objectid, u64 generation,
				int check_generation)
{
	struct btrfs_fs_info *fs_info = btrfs_sb(sb);
	struct btrfs_root *root;
	struct inode *inode;
	struct btrfs_key key;
	int index;
	int err = 0;

	if (objectid < BTRFS_FIRST_FREE_OBJECTID)
		return ERR_PTR(-ESTALE);

	key.objectid = root_objectid;
	key.type = BTRFS_ROOT_ITEM_KEY;
	key.offset = (u64)-1;

	index = srcu_read_lock(&fs_info->subvol_srcu);

	root = btrfs_read_fs_root_no_name(fs_info, &key);
	if (IS_ERR(root)) {
		err = PTR_ERR(root);
		goto fail;
	}

	key.objectid = objectid;
	key.type = BTRFS_INODE_ITEM_KEY;
	key.offset = 0;

	inode = btrfs_iget(sb, &key, root, NULL);
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		goto fail;
	}

	srcu_read_unlock(&fs_info->subvol_srcu, index);

	if (check_generation && generation != inode->i_generation) {
		iput(inode);
		return ERR_PTR(-ESTALE);
	}

	return d_obtain_alias(inode);
fail:
	srcu_read_unlock(&fs_info->subvol_srcu, index);
	return ERR_PTR(err);
}

static struct dentry *btrfs_fh_to_parent(struct super_block *sb, struct fid *fh,
					 int fh_len, int fh_type)
{
	struct btrfs_fid *fid = (struct btrfs_fid *) fh;
	u64 objectid, root_objectid;
	u32 generation;

	if (fh_type == FILEID_BTRFS_WITH_PARENT) {
		if (fh_len <  BTRFS_FID_SIZE_CONNECTABLE)
			return NULL;
		root_objectid = fid->root_objectid;
	} else if (fh_type == FILEID_BTRFS_WITH_PARENT_ROOT) {
		if (fh_len < BTRFS_FID_SIZE_CONNECTABLE_ROOT)
			return NULL;
		root_objectid = fid->parent_root_objectid;
	} else
		return NULL;

	objectid = fid->parent_objectid;
	generation = fid->parent_gen;

	return btrfs_get_dentry(sb, objectid, root_objectid, generation, 1);
}

static struct dentry *btrfs_fh_to_dentry(struct super_block *sb, struct fid *fh,
					 int fh_len, int fh_type)
{
	struct btrfs_fid *fid = (struct btrfs_fid *) fh;
	u64 objectid, root_objectid;
	u32 generation;

	if ((fh_type != FILEID_BTRFS_WITH_PARENT ||
	     fh_len < BTRFS_FID_SIZE_CONNECTABLE) &&
	    (fh_type != FILEID_BTRFS_WITH_PARENT_ROOT ||
	     fh_len < BTRFS_FID_SIZE_CONNECTABLE_ROOT) &&
	    (fh_type != FILEID_BTRFS_WITHOUT_PARENT ||
	     fh_len < BTRFS_FID_SIZE_NON_CONNECTABLE))
		return NULL;

	objectid = fid->objectid;
	root_objectid = fid->root_objectid;
	generation = fid->gen;

	return btrfs_get_dentry(sb, objectid, root_objectid, generation, 1);
}

struct dentry *btrfs_get_parent(struct dentry *child)
{
	struct inode *dir = d_inode(child);
	struct btrfs_root *root = BTRFS_I(dir)->root;
	struct btrfs_path *path;
	struct extent_buffer *leaf;
	struct btrfs_root_ref *ref;
	struct btrfs_key key;
	struct btrfs_key found_key;
	int ret;

	path = btrfs_alloc_path();
	if (!path)
		return ERR_PTR(-ENOMEM);

	if (btrfs_ino(dir) == BTRFS_FIRST_FREE_OBJECTID) {
		key.objectid = root->root_key.objectid;
		key.type = BTRFS_ROOT_BACKREF_KEY;
		key.offset = (u64)-1;
		root = root->fs_info->tree_root;
	} else {
		key.objectid = btrfs_ino(dir);
		key.type = BTRFS_INODE_REF_KEY;
		key.offset = (u64)-1;
	}

	ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
	if (ret < 0)
		goto fail;

	BUG_ON(ret == 0); /* Key with offset of -1 found */
	if (path->slots[0] == 0) {
		ret = -ENOENT;
		goto fail;
	}

	path->slots[0]--;
	leaf = path->nodes[0];

	btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
	if (found_key.objectid != key.objectid || found_key.type != key.type) {
		ret = -ENOENT;
		goto fail;
	}

	if (found_key.type == BTRFS_ROOT_BACKREF