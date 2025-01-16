{
	struct btrfs_transaction *cur_trans = NULL, *t;
	int ret = 0;

	if (transid) {
		if (transid <= root->fs_info->last_trans_committed)
			goto out;

		/* find specified transaction */
		spin_lock(&root->fs_info->trans_lock);
		list_for_each_entry(t, &root->fs_info->trans_list, list) {
			if (t->transid == transid) {
				cur_trans = t;
				atomic_inc(&cur_trans->use_count);
				ret = 0;
				break;
			}
			if (t->transid > transid) {
				ret = 0;
				break;
			}
		}
		spin_unlock(&root->fs_info->trans_lock);

		/*
		 * The specified transaction doesn't exist, or we
		 * raced with btrfs_commit_transaction
		 */
		if (!cur_trans) {
			if (transid > root->fs_info->last_trans_committed)
				ret = -EINVAL;
			goto out;
		}
	} else {
		/* find newest transaction that is committing | committed */
		spin_lock(&root->fs_info->trans_lock);
		list_for_each_entry_reverse(t, &root->fs_info->trans_list,
					    list) {
			if (t->state >= TRANS_STATE_COMMIT_START) {
				if (t->state == TRANS_STATE_COMPLETED)
					break;
				cur_trans = t;
				atomic_inc(&cur_trans->use_count);
				break;
			}
		}
		spin_unlock(&root->fs_info->trans_lock);
		if (!cur_trans)
			goto out;  /* nothing committing|committed */
	}

	wait_for_commit(root, cur_trans);
	btrfs_put_transaction(cur_trans);
out:
	return ret;
}

void btrfs_throttle(struct btrfs_root *root)
{
	if (!atomic_read(&root->fs_info->open_ioctl_trans))
		wait_current_trans(root);
}

static int should_end_transaction(struct btrfs_trans_handle *trans,
				  struct btrfs_root *root)
{
	if (root->fs_info->global_block_rsv.space_info->full &&
	    btrfs_check_space_for_delayed_refs(trans, root))
		return 1;

	return !!btrfs_block_rsv_check(root, &root->fs_info->global_block_rsv, 5);
}

int btrfs_should_end_transaction(struct btrfs_trans_handle *trans,
				 struct btrfs_root *root)
{
	struct btrfs_transaction *cur_trans = trans->transaction;
	int updates;
	int err;

	smp_mb();
	if (cur_trans->state >= TRANS_STATE_BLOCKED ||
	    cur_trans->delayed_refs.flushing)
		return 1;

	updates = trans->delayed_ref_updates;
	trans->delayed_ref_updates = 0;
	if (updates) {
		err = btrfs_run_delayed_refs(trans, root, updates * 2);
		if (err) /* Error code will also eval true */
			return err;
	}

	return should_end_transaction(trans, root);
}

static int __btrfs_end_transaction(struct btrfs_trans_handle *trans,
			  struct btrfs_root *root, int throttle)
{
	struct btrfs_transaction *cur_trans = trans->transaction;
	struct btrfs_fs_info *info = root->fs_info;
	unsigned long cur = trans->delayed_ref_updates;
	int lock = (trans->type != TRANS_JOIN_NOLOCK);
	int err = 0;
	int must_run_delayed_refs = 0;

	if (trans->use_count > 1) {
		trans->use_count--;
		trans->block_rsv = trans->orig_rsv;
		return 0;
	}

	btrfs_trans_release_metadata(trans, root);
	trans->block_rsv = NULL;

	if (!list_empty(&trans->new_bgs))
		btrfs_create_pending_block_groups(trans, root);

	trans->delayed_ref_updates = 0;
	if (!trans->sync) {
		must_run_delayed_refs =
			btrfs_should_throttle_delayed_refs(trans, root);
		cur = max_t(unsigned long, cur, 32);

		/*
		 * don't make the caller wait if they are from a NOLOCK
		 * or ATTACH transaction, it will deadlock with commit
		 */
		if (must_run_delayed_refs == 1 &&
		    (trans->type & (__TRANS_JOIN_NOLOCK | __TRANS_ATTACH)))
			must_run_delayed_refs = 2;
	}

	btrfs_trans_release_metadata(trans, root);
	trans->block_rsv = NULL;

	if (!list_empty(&trans->new_bgs))
		btrfs_create_pending_block_groups(trans, root);

	btrfs_trans_release_chunk_metadata(trans);

	if (lock && !atomic_read(&root->fs_info->open_ioctl_trans) &&
	    should_end_transaction(trans, root) &&
	    ACCESS_ONCE(cur_trans->state) == TRANS_STATE_RUNNING) {
		spin_lock(&info->trans_lock);
		if (cur_trans->state == TRANS_STATE_RUNNING)
			cur_trans->state = TRANS_STATE_BLOCKED;
		spin_unlock(&info->trans_lock);
	}

	if (lock && ACCESS_ONCE(cur_trans->state) == TRANS_STATE_BLOCKED) {
		if (throttle)
			return btrfs_commit_transaction(trans, root);
		else
			wake_up_process(info->transaction_kthread);
	}

	if (trans->type & __TRANS_FREEZABLE)
		sb_end_intwrite(root->fs_info->sb);

	WARN_ON(cur_trans != info->running_transaction);
	WARN_ON(atomic_read(&cur_trans->num_writers) < 1);
	atomic_dec(&cur_trans->num_writers);
	extwriter_counter_dec(cur_trans, trans->type);

	/*
	 * Make sure counter is updated before we wake up waiters.
	 */
	smp_mb();
	if (waitqueue_active(&cur_trans->writer_wait))
		wake_up(&cur_trans->writer_wait);
	btrfs_put_transaction(cur_trans);

	if (current->journal_info == trans)
		current->journal_info = NULL;

	if (throttle)
		btrfs_run_delayed_iputs(root);

	if (trans->aborted ||
	    test_bit(BTRFS_FS_STATE_ERROR, &root->fs_info->fs_state)) {
		wake_up_process(info->transaction_kthread);
		err = -EIO;
	}
	assert_qgroups_uptodate(trans);

	kmem_cache_free(btrfs_trans_handle_cachep, trans);
	if (must_run_delayed_refs) {
		btrfs_async_run_delayed_refs(root, cur,
					     must_run_delayed_refs == 1);
	}
	return err;
}

int btrfs_end_transaction(struct btrfs_trans_handle *trans,
			  struct btrfs_root *root)
{
	return __btrfs_end_transaction(trans, root, 0);
}

int btrfs_end_transaction_throttle(struct btrfs_trans_handle *trans,
				   struct btrfs_root *root)
{
	return __btrfs_end_transaction(trans, root, 1);
}

/*
 * when btree blocks are allocated, they have some corresponding bits set for
 * them in one of two extent_io trees.  This is used to make sure all of
 * those extents are sent to disk but does not wait on them
 */
int btrfs_write_marked_extents(struct btrfs_root *root,
			       struct extent_io_tree *dirty_pages, int mark)
{
	int err = 0;
	int werr = 0;
	struct address_space *mapping = root->fs_info->btree_inode->i_mapping;
	struct extent_state *cached_state = NULL;
	u64 start = 0;
	u64 end;

	while (!find_first_extent_bit(dirty_pages, start, &start, &end,
				      mark, &cached_state)) {
		bool wait_writeback = false;

		err = convert_extent_bit(dirty_pages, start, end,
					 EXTENT_NEED_WAIT,
					 mark, &cached_state, GFP_NOFS);
		/*
		 * convert_extent_bit can return -ENOMEM, which is most of the
		 * time a temporary error. So when it happens, ignore the error
		 * and wait for writeback of this range to finish - because we
		 * failed to set the bit EXTENT_NEED_WAIT for the range, a call
		 * to btrfs_wait_marked_extents() would not know that writeback
		 * for this range started and therefore wouldn't wait for it to
		 * finish - we don't want to commit a superblock that points to
		 * btree nodes/leafs for which writeback hasn't finished yet
		 * (and without errors).
		 * We cleanup any entries left in the io tree when committing
		 * the transaction (through clear_btree_io_tree()).
		 */
		if (err == -ENOMEM) {
			err = 0;
			wait_writeback = true;
		}
		if (!err)
			err = filemap_fdatawrite_range(mapping, start, end);
		if (err)
			werr = err;
		else if (wait_writeback)
			werr = filemap_fdatawait_range(mapping, start, end);
		free_extent_state(cached_state);
		cached_state = NULL;
		cond_resched();
		start = end + 1;
	}
	return werr;
}

/*
 * when btree blocks are allocated, they have some corresponding bits set for
 * them in one of two extent_io trees.  This is used to make sure all of
 * those extents are on disk for transaction or log commit.  We wait
 * on all the pages and clear them from the dirty pages state tree
 */
int btrfs_wait_marked_extents(struct btrfs_root *root,
			      struct extent_io_tree *dirty_pages, int mark)
{
	int err = 0;
	int werr = 0;
	struct address_space *mapping = root->fs_info->btree_inode->i_mapping;
	struct extent_state *cached_state = NULL;
	u64 start = 0;
	u64 end;
	struct btrfs_inode *btree_ino = BTRFS_I(root->fs_info->btree_inode);
	bool errors = false;

	while (!find_first_extent_bit(dirty_pages, start, &start, &end,
				      EXTENT_NEED_WAIT, &cached_state)) {
		/*
		 * Ignore -ENOMEM errors returned by clear_extent_bit().
		 * When committing the transaction, we'll remove any entries
		 * left in the io tree. For a log commit, we don't remove them
		 * after committing the log because the tree can be accessed
		 * concurrently - we do it only at transaction commit time when
		 * it's safe to do it (through clear_btree_io_tree()).
		 */
		err = clear_extent_bit(dirty_pages, start, end,
				       EXTENT_NEED_WAIT,
				       0, 0, &cached_state, GFP_NOFS);
		if (err == -ENOMEM)
			err = 0;
		if (!err)
			err = filemap_fdatawait_range(mapping, start, end);
		if (err)
			werr = err;
		free_extent_state(cached_state);
		cached_state = NULL;
		cond_resched();
		start = end + 1;
	}
	if (err)
		werr = err;

	if (root->root_key.objectid == BTRFS_TREE_LOG_OBJECTID) {
		if ((mark & EXTENT_DIRTY) &&
		    test_and_clear_bit(BTRFS_INODE_BTREE_LOG1_ERR,
				       &btree_ino->runtime_flags))
			errors = true;

		if ((mark & EXTENT_NEW) &&
		    test_and_clear_bit(BTRFS_INODE_BTREE_LOG2_ERR,
				       &btree_ino->runtime_flags))
			errors = true;
	} else {
		if (test_and_clear_bit(BTRFS_INODE_BTREE_ERR,
				       &btree_ino->runtime_flags))
			errors = true;
	}

	if (errors && !werr)
		werr = -EIO;

	return werr;
}

/*
 * when btree blocks are allocated, they have some corresponding bits set for
 * them in one of two extent_io trees.  This is used to make sure all of
 * those extents are on disk for transaction or log commit
 */
static int btrfs_write_and_wait_marked_extents(struct btrfs_root *root,
				struct extent_io_tree *dirty_pages, int mark)
{
	int ret;
	int ret2;
	struct blk_plug plug;

	blk_start_plug(&plug);
	ret = btrfs_write_marked_extents(root, dirty_pages, mark);
	blk_finish_plug(&plug);
	ret2 = btrfs_wait_marked_extents(root, dirty_pages, mark);

	if (ret)
		return ret;
	if (ret2)
		return ret2;
	return 0;
}

static int btrfs_write_and_wait_transaction(struct btrfs_trans_handle *trans,
				     struct btrfs_root *root)
{
	int ret;

	ret = btrfs_write_and_wait_marked_extents(root,
					   &trans->transaction->dirty_pages,
					   EXTENT_DIRTY);
	clear_btree_io_tree(&trans->transaction->dirty_pages);

	return ret;
}

/*
 * this is used to update the root pointer in the tree of tree roots.
 *
 * But, in the case of the extent allocation tree, updating the root
 * pointer may allocate blocks which may change the root of the extent
 * allocation tree.
 *
 * So, this loops and repeats and makes sure the cowonly root didn't
 * change while the root pointer was being updated in the metadata.
 */
static int update_cowonly_root(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root)
{
	int ret;
	u64 old_root_bytenr;
	u64 old_root_used;
	struct btrfs_root *tree_root = root->fs_info->tree_root;

	old_root_used = btrfs_root_used(&root->root_item);

	while (1) {
		old_root_bytenr = btrfs_root_bytenr(&root->root_item);
		if (old_root_bytenr == root->node->start &&
		    old_root_used == btrfs_root_used(&root->root_item))
			break;

		btrfs_set_root_node(&root->root_item, root->node);
		ret = btrfs_update_root(trans, tree_root,
					&root->root_key,
					&root->root_item);
		if (ret)
			return ret;

		old_root_used = btrfs_root_used(&root->root_item);
	}

	return 0;
}

/*
 * update all the cowonly tree roots on disk
 *
 * The error handling in this function may not be obvious. Any of the
 * failures will cause the file system to go offline. We still need
 * to clean up the delayed refs.
 */
static noinline int commit_cowonly_roots(struct btrfs_trans_handle *trans,
					 struct btrfs_root *root)
{
	struct btrfs_fs_info *fs_info = root->fs_info;
	struct list_head *dirty_bgs = &trans->transaction->dirty_bgs;
	struct list_head *io_bgs = &trans->transaction->io_bgs;
	struct list_head *next;
	struct extent_buffer *eb;
	int ret;

	eb = btrfs_lock_root_node(fs_info->tree_root);
	ret = btrfs_cow_block(trans, fs_info->tree_root, eb, NULL,
			      0, &eb);
	btrfs_tree_unlock(eb);
	free_extent_buffer(eb);

	if (ret)
		return ret;

	ret = btrfs_run_delayed_refs(trans, root, (unsigned long)-1);
	if (ret)
		return ret;

	ret = btrfs_run_dev_stats(trans, root->fs_info);
	if (ret)
		return ret;
	ret = btrfs_run_dev_replace(trans, root->fs_info);
	if (ret)
		return ret;
	ret = btrfs_run_qgroups(trans, root->fs_info);
	if (ret)
		return ret;

	ret = btrfs_setup_space_cache(trans, root);
	if (ret)
		return ret;

	/* run_qgroups might have added some more refs */
	ret = btrfs_run_delayed_refs(trans, root, (unsigned long)-1);
	if (ret)
		return ret;
again:
	while (!list_empty(&fs_info->dirty_cowonly_roots)) {
		next = fs_info->dirty_cowonly_roots.next;
		list_del_init(next);
		root = list_entry(next, struct btrfs_root, dirty_list);
		clear_bit(BTRFS_ROOT_DIRTY, &root->state);

		if (root != fs_info->extent_root)
			list_add_tail(&root->dirty_list,
				      &trans->transaction->switch_commits);
		ret = update_cowonly_root(trans, root);
		if (ret)
			return ret;
		ret = btrfs_run_delayed_refs(trans, root, (unsigned long)-1);
		if (ret)
			return ret;
	}

	while (!list_empty(dirty_bgs) || !list_empty(io_bgs)) {
		ret = btrfs_write_dirty_block_groups(trans, root);
		if (ret)
			return ret;
		ret = btrfs_run_delayed_refs(trans, root, (unsigned long)-1);
		if (ret)
			return ret;
	}

	if (!list_empty(&fs_info->dirty_cowonly_roots))
		goto again;

	list_add_tail(&fs_info->extent_root->dirty_list,
		      &trans->transaction->switch_commits);
	btrfs_after_dev_replace_commit(fs_info);

	return 0;
}

/*
 * dead roots are old snapshots that need to be deleted.  This allocates
 * a dirty root struct and adds it into the list of dead roots that need to
 * be deleted
 */
void btrfs_add_dead_root(struct btrfs_root *root)
{
	spin_lock(&root->fs_info->trans_lock);
	if (list_empty(&root->root_list))
		list_add_tail(&root->root_list, &root->fs_info->dead_roots);
	spin_unlock(&root->fs_info->trans_lock);
}

/*
 * update all the cowonly tree roots on disk
 */
static noinline int commit_fs_roots(struct btrfs_trans_handle *trans,
				    struct btrfs_root *root)
{
	struct btrfs_root *gang[8];
	struct btrfs_fs_info *fs_info = root->fs_info;
	int i;
	int ret;
	int err = 0;

	spin_lock(&fs_info->fs_roots_radix_lock);
	while (1) {
		ret = radix_tree_gang_lookup_tag(&fs_info->fs_roots_radix,
						 (void **)gang, 0,
						 ARRAY_SIZE(gang),
						 BTRFS_ROOT_TRANS_TAG);
		if (ret == 0)
			break;
		for (i = 0; i < ret; i++) {
			root = gang[i];
			radix_tree_tag_clear(&fs_info->fs_roots_radix,
					(unsigned long)root->root_key.objectid,
					BTRFS_ROOT_TRANS_TAG);
			spin_unlock(&fs_info->fs_roots_radix_lock);

			btrfs_free_log(trans, root);
			btrfs_update_reloc_root(trans, root);
			btrfs_orphan_commit_root(trans, root);

			btrfs_save_ino_cache(root, trans);

			/* see comments in should_cow_block() */
			clear_bit(BTRFS_ROOT_FORCE_COW, &root->state);
			smp_mb__after_atomic();

			if (root->commit_root != root->node) {
				list_add_tail(&root->dirty_list,
					&trans->transaction->switch_commits);
				btrfs_set_root_node(&root->root_item,
						    root->node);
			}

			err = btrfs_update_root(trans, fs_info->tree_root,
						&root->root_key,
						&root->root_item);
			spin_lock(&fs_info->fs_roots_radix_lock);
			if (err)
				break;
			btrfs_qgroup_free_meta_all(root);
		}
	}
	spin_unlock(&fs_info->fs_roots_radix_lock);
	return err;
}

/*
 * defrag a given btree.
 * Every leaf in the btree is read and defragged.
 */
int btrfs_defrag_root(struct btrfs_root *root)
{
	struct btrfs_fs_info *info = root->fs_info;
	struct btrfs_trans_handle *trans;
	int ret;

	if (test_and_set_bit(BTRFS_ROOT_DEFRAG_RUNNING, &root->state))
		return 0;

	while (1) {
		trans = btrfs_start_transaction(root, 0);
		if (IS_ERR(trans)) {
			ret = PTR_ERR(trans);
			break;
		}

		ret = btrfs_defrag_leaves(trans, root);

		btrfs_end_transaction(trans, root);
		btrfs_btree_balance_dirty(info->tree_root);
		cond_resched();

		if (btrfs_fs_closing(root->fs_info) || ret != -EAGAIN)
			break;

		if (btrfs_defrag_cancelled(root->fs_info)) {
			pr_debug("BTRFS: defrag_root cancelled\n");
			ret = -EAGAIN;
			break;
		}
	}
	clear_bit(BTRFS_ROOT_DEFRAG_RUNNING, &root->state);
	return ret;
}

/*
 * new snapshots need to be created at a very specific time in the
 * transaction commit.  This does the actual creation.
 *
 * Note:
 * If the error which may affect the commitment of the current transaction
 * happens, we should return the error number. If the error which just affect
 * the creation of the pending snapshots, just return 0.
 */
static noinline int create_pending_snapshot(struct btrfs_trans_handle *trans,
				   struct btrfs_fs_info *fs_info,
				   struct btrfs_pending_snapshot *pending)
{
	struct btrfs_key key;
	struct btrfs_root_item *new_root_item;
	struct btrfs_root *tree_root = fs_info->tree_root;
	struct btrfs_root *root = pending->root;
	struct btrfs_root *parent_root;
	struct btrfs_block_rsv *rsv;
	struct inode *parent_inode;
	struct btrfs_path *path;
	struct btrfs_dir_item *dir_item;
	struct dentry *dentry;
	struct extent_buffer *tmp;
	struct extent_buffer *old;
	struct timespec cur_time = CURRENT_TIME;
	int ret = 0;
	u64 to_reserve = 0;
	u64 index = 0;
	u64 objectid;
	u64 root_flags;
	uuid_le new_uuid;

	path = btrfs_alloc_path();
	if (!path) {
		pending->error = -ENOMEM;
		return 0;
	}

	new_root_item = kmalloc(sizeof(*new_root_item), GFP_NOFS);
	if (!new_root_item) {
		pending->error = -ENOMEM;
		goto root_item_alloc_fail;
	}

	pending->error = btrfs_find_free_objectid(tree_root, &objectid);
	if (pending->error)
		goto no_free_objectid;

	/*
	 * Make qgroup to skip current new snapshot's qgroupid, as it is
	 * accounted by later btrfs_qgroup_inherit().
	 */
	btrfs_set_skip_qgroup(trans, objectid);

	btrfs_reloc_pre_snapshot(pending, &to_reserve);

	if (to_reserve > 0) {
		pending->error = btrfs_block_rsv_add(root,
						     &pending->block_rsv,
						     to_reserve,
						     BTRFS_RESERVE_NO_FLUSH);
		if (pending->error)
			goto clear_skip_qgroup;
	}

	key.objectid = objectid;
	key.offset = (u64)-1;
	key.type = BTRFS_ROOT_ITEM_KEY;

	rsv = trans->block_rsv;
	trans->block_rsv = &pending->block_rsv;
	trans->bytes_reserved = trans->block_rsv->reserved;

	dentry = pending->dentry;
	parent_inode = pending->dir;
	parent_root = BTRFS_I(parent_inode)->root;
	record_root_in_trans(trans, parent_root);

	/*
	 * insert the directory item
	 */
	ret = btrfs_set_inode_index(parent_inode, &index);
	BUG_ON(ret); /* -ENOMEM */

	/* check if there is a file/dir which has the same name. */
	dir_item = btrfs_lookup_dir_item(NULL, parent_root, path,
					 btrfs_ino(parent_inode),
					 dentry->d_name.name,
					 dentry->d_name.len, 0);
	if (dir_item != NULL && !IS_ERR(dir_item)) {
		pending->error = -EEXIST;
		goto dir_item_existed;
	} else if (IS_ERR(dir_item)) {
		ret = PTR_ERR(dir_item);
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}
	btrfs_release_path(path);

	/*
	 * pull in the delayed directory update
	 * and the delayed inode item
	 * otherwise we corrupt the FS during
	 * snapshot
	 */
	ret = btrfs_run_delayed_items(trans, root);
	if (ret) {	/* Transaction aborted */
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}

	record_root_in_trans(trans, root);
	btrfs_set_root_last_snapshot(&root->root_item, trans->transid);
	memcpy(new_root_item, &root->root_item, sizeof(*new_root_item));
	btrfs_check_and_init_root_item(new_root_item);

	root_flags = btrfs_root_flags(new_root_item);
	if (pending->readonly)
		root_flags |= BTRFS_ROOT_SUBVOL_RDONLY;
	else
		root_flags &= ~BTRFS_ROOT_SUBVOL_RDONLY;
	btrfs_set_root_flags(new_root_item, root_flags);

	btrfs_set_root_generation_v2(new_root_item,
			trans->transid);
	uuid_le_gen(&new_uuid);
	memcpy(new_root_item->uuid, new_uuid.b, BTRFS_UUID_SIZE);
	memcpy(new_root_item->parent_uuid, root->root_item.uuid,
			BTRFS_UUID_SIZE);
	if (!(root_flags & BTRFS_ROOT_SUBVOL_RDONLY)) {
		memset(new_root_item->received_uuid, 0,
		       sizeof(new_root_item->received_uuid));
		memset(&new_root_item->stime, 0, sizeof(new_root_item->stime));
		memset(&new_root_item->rtime, 0, sizeof(new_root_item->rtime));
		btrfs_set_root_stransid(new_root_item, 0);
		btrfs_set_root_rtransid(new_root_item, 0);
	}
	btrfs_set_stack_timespec_sec(&new_root_item->otime, cur_time.tv_sec);
	btrfs_set_stack_timespec_nsec(&new_root_item->otime, cur_time.tv_nsec);
	btrfs_set_root_otransid(new_root_item, trans->transid);

	old = btrfs_lock_root_node(root);
	ret = btrfs_cow_block(trans, root, old, NULL, 0, &old);
	if (ret) {
		btrfs_tree_unlock(old);
		free_extent_buffer(old);
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}

	btrfs_set_lock_blocking(old);

	ret = btrfs_copy_root(trans, root, old, &tmp, objectid);
	/* clean up in any case */
	btrfs_tree_unlock(old);
	free_extent_buffer(old);
	if (ret) {
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}
	/* see comments in should_cow_block() */
	set_bit(BTRFS_ROOT_FORCE_COW, &root->state);
	smp_wmb();

	btrfs_set_root_node(new_root_item, tmp);
	/* record when the snapshot was created in key.offset */
	key.offset = trans->transid;
	ret = btrfs_insert_root(trans, tree_root, &key, new_root_item);
	btrfs_tree_unlock(tmp);
	free_extent_buffer(tmp);
	if (ret) {
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}

	/*
	 * insert root back/forward references
	 */
	ret = btrfs_add_root_ref(trans, tree_root, objectid,
				 parent_root->root_key.objectid,
				 btrfs_ino(parent_inode), index,
				 dentry->d_name.name, dentry->d_name.len);
	if (ret) {
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}

	key.offset = (u64)-1;
	pending->snap = btrfs_read_fs_root_no_name(root->fs_info, &key);
	if (IS_ERR(pending->snap)) {
		ret = PTR_ERR(pending->snap);
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}

	ret = btrfs_reloc_post_snapshot(trans, pending);
	if (ret) {
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}

	ret = btrfs_run_delayed_refs(trans, root, (unsigned long)-1);
	if (ret) {
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}

	ret = btrfs_insert_dir_item(trans, parent_root,
				    dentry->d_name.name, dentry->d_name.len,
				    parent_inode, &key,
				    BTRFS_FT_DIR, index);
	/* We have check then name at the beginning, so it is impossible. */
	BUG_ON(ret == -EEXIST || ret == -EOVERFLOW);
	if (ret) {
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}

	btrfs_i_size_write(parent_inode, parent_inode->i_size +
					 dentry->d_name.len * 2);
	parent_inode->i_mtime = parent_inode->i_ctime = CURRENT_TIME;
	ret = btrfs_update_inode_fallback(trans, parent_root, parent_inode);
	if (ret) {
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}
	ret = btrfs_uuid_tree_add(trans, fs_info->uuid_root, new_uuid.b,
				  BTRFS_UUID_KEY_SUBVOL, objectid);
	if (ret) {
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}
	if (!btrfs_is_empty_uuid(new_root_item->received_uuid)) {
		ret = btrfs_uuid_tree_add(trans, fs_info->uuid_root,
					  new_root_item->received_uuid,
					  BTRFS_UUID_KEY_RECEIVED_SUBVOL,
					  objectid);
		if (ret && ret != -EEXIST) {
			btrfs_abort_transaction(trans, root, ret);
			goto fail;
		}
	}

	ret = btrfs_run_delayed_refs(trans, root, (unsigned long)-1);
	if (ret) {
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}

	/*
	 * account qgroup counters before qgroup_inherit()
	 */
	ret = btrfs_qgroup_prepare_account_extents(trans, fs_info);
	if (ret)
		goto fail;
	ret = btrfs_qgroup_account_extents(trans, fs_info);
	if (ret)
		goto fail;
	ret = btrfs_qgroup_inherit(trans, fs_info,
				   root->root_key.objectid,
				   objectid, pending->inherit);
	if (ret) {
		btrfs_abort_transaction(trans, root, ret);
		goto fail;
	}

fail:
	pending->error = ret;
dir_item_existed:
	trans->block_rsv = rsv;
	trans->bytes_reserved = 0;
clear_skip_qgroup:
	btrfs_clear_skip_qgroup(trans);
no_free_objectid:
	kfree(new_root_item);
root_item_alloc_fail:
	btrfs_free_path(path);
	return ret;
}

/*
 * create all the snapshots we've scheduled for creation
 */
static noinline int create_pending_snapshots(struct btrfs_trans_handle *trans,
					     struct btrfs_fs_info *fs_info)
{
	struct btrfs_pending_snapshot *pending, *next;
	struct list_head *head = &trans->transaction->pending_snapshots;
	int ret = 0;

	list_for_each_entry_safe(pending, next, head, list) {
		list_del(&pending->list);
		ret = create_pending_snapshot(trans, fs_info, pending);
		if (ret)
			break;
	}
	return ret;
}

static void update_super_roots(struct btrfs_root *root)
{
	struct btrfs_root_item *root_item;
	struct btrfs_super_block *super;

	super = root->fs_info->super_copy;

	root_item = &root->fs_info->chunk_root->root_item;
	super->chunk_root = root_item->bytenr;
	super->chunk_root_generation = root_item->generation;
	super->chunk_root_level = root_item->level;

	root_item = &root->fs_info->tree_root->root_item;
	super->root = root_item->bytenr;
	super->generation = root_item->generation;
	super->root_level = root_item->level;
	if (btrfs_test_opt(root, SPACE_CACHE))
		super->cache_generation = root_item->generation;
	if (root->fs_info->update_uuid_tree_gen)
		super->uuid_tree_generation = root_item->generation;
}

int btrfs_transaction_in_commit(struct btrfs_fs_info *info)
{
	struct btrfs_transaction *trans;
	int ret = 0;

	spin_lock(&info->trans_lock);
	trans = info->running_transaction;
	if (trans)
		ret = (trans->state >= TRANS_STATE_COMMIT_START);
	spin_unlock(&info->trans_lock);
	return ret;
}

int btrfs_transaction_blocked(struct btrfs_fs_info *info)
{
	struct btrfs_transaction *trans;
	int ret = 0;

	spin_lock(&info->trans_lock);
	trans = info->running_transaction;
	if (trans)
		ret = is_transaction_blocked(trans);
	spin_unlock(&info->trans_lock);
	return ret;
}

/*
 * wait for the current transaction commit to start and block subsequent
 * transaction joins
 */
static void wait_current_trans_commit_start(struct btrfs_root *root,
					    struct btrfs_transaction *trans)
{
	wait_event(root->fs_info->transaction_blocked_wait,
		   trans->state >= TRANS_STATE_COMMIT_START ||
		   trans->aborted);
}

/*
 * wait for the current transaction to start and then become unblocked.
 * caller holds ref.
 */
static void wait_current_trans_commit_start_and_unblock(struct btrfs_root *root,
					 struct btrfs_transaction *trans)
{
	wait_event(root->fs_info->transaction_wait,
		   trans->state >= TRANS_STATE_UNBLOCKED ||
		   trans->aborted);
}

/*
 * commit transactions asynchronously. once btrfs_commit_transaction_async
 * returns, any subsequent transaction will not be allowed to join.
 */
struct btrfs_async_commit {
	struct btrfs_trans_handle *newtrans;
	struct btrfs_root *root;
	struct work_struct work;
};

static void do_async_commit(struct work_struct *work)
{
	struct btrfs_async_commit *ac =
		container_of(work, struct btrfs_async_commit, work);

	/*
	 * We've got freeze protection passed with the transaction.
	 * Tell lockdep about it.
	 */
	if (ac->newtrans->type & __TRANS_FREEZABLE)
		__sb_writers_acquired(ac->root->fs_info->sb, SB_FREEZE_FS);

	current->journal_info = ac->newtrans;

	btrfs_commit_transaction(ac->newtrans, ac->root);
	kfree(ac);
}

int btrfs_commit_transaction_async(struct btrfs_trans_handle *trans,
				   struct btrfs_root *root,
				   int wait_for_unblock)
{
	struct btrfs_async_commit *ac;
	struct btrfs_transaction *cur_trans;

	ac = kmalloc(sizeof(*ac), GFP_NOFS);
	if (!ac)
		return -ENOMEM;

	INIT_WORK(&ac->work, do_async_commit);
	ac->root = root;
	ac->newtrans = btrfs_join_transaction(root);
	if (IS_ERR(ac->newtrans)) {
		int err = PTR_ERR(ac->newtrans);
		kfree(ac);
		return err;
	}

	/* take transaction reference */
	cur_trans = trans->transaction;
	atomic_inc(&cur_trans->use_count);

	btrfs_end_transaction(trans, root);

	/*
	 * Tell lockdep we've released the freeze rwsem, since the
	 * async commit thread will be the one to unlock it.
	 */
	if (ac->newtrans->type & __TRANS_FREEZABLE)
		__sb_writers_release(root->fs_info->sb, SB_FREEZE_FS);

	schedule_work(&ac->work);

	/* wait for transaction to start and unblock */
	if (wait_for_unblock)
		wait_current_trans_commit_start_and_unblock(root, cur_trans);
	else
		wait_current_trans_commit_start(root, cur_trans);

	if (current->journal_info == trans)
		current->journal_info = NULL;

	btrfs_put_transaction(cur_trans);
	return 0;
}


static void cleanup_transaction(struct btrfs_trans_handle *trans,
				struct btrfs_root *root, int err)
{
	struct btrfs_transaction *cur_trans = trans->transaction;
	DEFINE_WAIT(wait);

	WARN_ON(trans->use_count > 1);

	btrfs_abort_transaction(trans, root, err);

	spin_lock(&root->fs_info->trans_lock);

	/*
	 * If the transaction is removed from the list, it means this
	 * transaction has been committed successfully, so it is impossible
	 * to call the cleanup function.
	 */
	BUG_ON(list_empty(&cur_trans->list));

	list_del_init(&cur_trans->list);
	if (cur_trans == root->fs_info->running_transaction) {
		cur_trans->state = TRANS_STATE_COMMIT_DOING;
		spin_unlock(&root->fs_info->trans_lock);
		wait_event(cur_trans->writer_wait,
			   atomic_read(&cur_trans->num_writers) == 1);

		spin_lock(&root->fs_info->trans_lock);
	}
	spin_unlock(&root->fs_info->trans_lock);

	btrfs_cleanup_one_transaction(trans->transaction, root);

	spin_lock(&root->fs_info->trans_lock);
	if (cur_trans == root->fs_info->running_transaction)
		root->fs_info->running_transaction = NULL;
	spin_unlock(&root->fs_info->trans_lock);

	if (trans->type & __TRANS_FREEZABLE)
		sb_end_intwrite(root->fs_info->sb);
	btrfs_put_transaction(cur_trans);
	btrfs_put_transaction(cur_trans);

	trace_btrfs_transaction_commit(root);

	if (current->journal_info == trans)
		current->journal_info = NULL;
	btrfs_scrub_cancel(root->fs_info);

	kmem_cache_free(btrfs_trans_handle_cachep, trans);
}

static inline int btrfs_start_delalloc_flush(struct btrfs_fs_info *fs_info)
{
	if (btrfs_test_opt(fs_info->tree_root, FLUSHONCOMMIT))
		return btrfs_start_delalloc_roots(fs_info, 1, -1);
	return 0;
}

static inline void btrfs_wait_delalloc_flush(struct btrfs_fs_info *fs_info)
{
	if (btrfs_test_opt(fs_info->tree_root, FLUSHONCOMMIT))
		btrfs_wait_ordered_roots(fs_info, -1);
}

static inline void
btrfs_wait_pending_ordered(struct btrfs_transaction *cur_trans)
{
	wait_event(cur_trans->pending_wait,
		   atomic_read(&cur_trans->pending_ordered) == 0);
}

int btrfs_commit_transaction(struct btrfs_trans_handle *trans,
			     struct btrfs_root *root)
{
	struct btrfs_transaction *cur_trans = trans->transaction;
	struct btrfs_transaction *prev_trans = NULL;
	struct btrfs_inode *btree_ino = BTRFS_I(root->fs_info->btree_inode);
	int ret;

	/*
	 * Some places just start a transaction to commit it.  We need to make
	 * sure that if this commit fails that the abort code actually marks the
	 * transaction as failed, so set trans->dirty to make the abort code do
	 * the right thing.
	 */
	trans->dirty = true;

	/* Stop the commit early if ->aborted is set */
	if (unlikely(ACCESS_ONCE(cur_trans->aborted))) {
		ret = cur_trans->aborted;
		btrfs_end_transaction(trans, root);
		return ret;
	}

	btrfs_trans_release_metadata(trans, root);
	trans->block_rsv = NULL;

	/* make a pass through all the delayed refs we have so far
	 * any runnings procs may add more while we are here
	 */
	ret = btrfs_run_delayed_refs(trans, root, 0);
	if (ret) {
		btrfs_end_transaction(trans, root);
		return ret;
	}

	cur_trans = trans->transaction;

	/*
	 * set the flushing flag so procs in this transaction have to
	 * start sending their work down.
	 */
	cur_trans->delayed_refs.flushing = 1;
	smp_wmb();

	if (!list_empty(&trans->new_bgs))
		btrfs_create_pending_block_groups(trans, root);

	ret = btrfs_run_delayed_refs(trans, root, 0);
	if (ret) {
		btrfs_end_transaction(trans, root);
		return ret;
	}

	if (!test_bit(BTRFS_TRANS_DIRTY_BG_RUN, &cur_trans->flags)) {
		int run_it = 0;

		/* this mutex is also taken before trying to set
		 * block groups readonly.  We need to make sure
		 * that nobody has set a block group readonly
		 * after a extents from that block group have been
		 * allocated for cache files.  btrfs_set_block_group_ro
		 * will wait for the transaction to commit if it
		 * finds BTRFS_TRANS_DIRTY_BG_RUN set.
		 *
		 * The BTRFS_TRANS_DIRTY_BG_RUN flag is also used to make sure
		 * only one process starts all the block group IO.  It wouldn't
		 * hurt to have more than one go through, but there's no
		 * real advantage to it either.
		 */
		mutex_lock(&root->fs_info->ro_block_group_mutex);
		if (!test_and_set_bit(BTRFS_TRANS_DIRTY_BG_RUN,
				      &cur_trans->flags))
			run_it = 1;
		mutex_unlock(&root->fs_info->ro_block_group_mutex);

		if (run_it)
			ret = btrfs_start_dirty_block_groups(trans, root);
	}
	if (ret) {
		btrfs_end_transaction(trans, root);
		return ret;
	}

	spin_lock(&root->fs_info->trans_lock);
	if (cur_trans->state >= TRANS_STATE_COMMIT_START) {
		spin_unlock(&root->fs_info->trans_lock);
		atomic_inc(&cur_trans->use_count);
		ret = btrfs_end_transaction(trans, root);

		wait_for_commit(root, cur_trans);

		if (unlikely(cur_trans->aborted))
			ret = cur_trans->aborted;

		btrfs_put_transaction(cur_trans);

		return ret;
	}

	cur_trans->state = TRANS_STATE_COMMIT_START;
	wake_up(&root->fs_info->transaction_blocked_wait);

	if (cur_trans->list.prev != &root->fs_info->trans_list) {
		prev_trans = list_entry(cur_trans->list.prev,
					struct btrfs_transaction, list);
		if (prev_trans->state != TRANS_STATE_COMPLETED) {
			atomic_inc(&prev_trans->use_count);
			spin_unlock(&root->fs_info->trans_lock);

			wait_for_commit(root, prev_trans);
			ret = prev_trans->aborted;

			btrfs_put_transaction(prev_trans);
			if (ret)
				goto cleanup_transaction;
		} else {
			spin_unlock(&root->fs_info->trans_lock);
		}
	} else {
		spin_unlock(&root->fs_info->trans_lock);
	}

	extwriter_counter_dec(cur_trans, trans->type);

	ret = btrfs_start_delalloc_flush(root->fs_info);
	if (ret)
		goto cleanup_transaction;

	ret = btrfs_run_delayed_items(trans, root);
	if (ret)
		goto cleanup_transaction;

	wait_event(cur_trans->writer_wait,
		   extwriter_counter_read(cur_trans) == 0);

	/* some pending stuffs might be added after the previous flush. */
	ret = btrfs_run_delayed_items(trans, root);
	if (ret)
		goto cleanup_transaction;

	btrfs_wait_delalloc_flush(root->fs_info);

	btrfs_wait_pending_ordered(cur_trans);

	btrfs_scrub_pause(root);
	/*
	 * Ok now we need to make sure to block out any other joins while we
	 * commit the transaction.  We could have started a join before setting
	 * COMMIT_DOING so make sure to wait for num_writers to == 1 again.
	 */
	spin_lock(&root->fs_info->trans_lock);
	cur_trans->state = TRANS_STATE_COMMIT_DOING;
	spin_unlock(&root->fs_info->trans_lock);
	wait_event(cur_trans->writer_wait,
		   atomic_read(&cur_trans->num_writers) == 1);

	/* ->aborted might be set after the previous check, so check it */
	if (unlikely(ACCESS_ONCE(cur_trans->aborted))) {
		ret = cur_trans->aborted;
		goto scrub_continue;
	}
	/*
	 * the reloc mutex makes sure that we stop
	 * the balancing code from coming in and moving
	 * extents around in the middle of the commit
	 */
	mutex_lock(&root->fs_info->reloc_mutex);

	/*
	 * We needn't worry about the delayed items because we will
	 * deal with them in create_pending_snapshot(), which is the
	 * core function of the snapshot creation.
	 */
	ret = create_pending_snapshots(trans, root->fs_info);
	if (ret) {
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	/*
	 * We insert the dir indexes of the snapshots and update the inode
	 * of the snapshots' parents after the snapshot creation, so there
	 * are some delayed items which are not dealt with. Now deal with
	 * them.
	 *
	 * We needn't worry that this operation will corrupt the snapshots,
	 * because all the tree which are snapshoted will be forced to COW
	 * the nodes and leaves.
	 */
	ret = btrfs_run_delayed_items(trans, root);
	if (ret) {
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	ret = btrfs_run_delayed_refs(trans, root, (unsigned long)-1);
	if (ret) {
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	/* Reocrd old roots for later qgroup accounting */
	ret = btrfs_qgroup_prepare_account_extents(trans, root->fs_info);
	if (ret) {
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	/*
	 * make sure none of the code above managed to slip in a
	 * delayed item
	 */
	btrfs_assert_delayed_root_empty(root);

	WARN_ON(cur_trans != trans->transaction);

	/* btrfs_commit_tree_roots is responsible for getting the
	 * various roots consistent with each other.  Every pointer
	 * in the tree of tree roots has to point to the most up to date
	 * root for every subvolume and other tree.  So, we have to keep
	 * the tree logging code from jumping in and changing any
	 * of the trees.
	 *
	 * At this point in the commit, there can't be any tree-log
	 * writers, but a little lower down we drop the trans mutex
	 * and let new people in.  By holding the tree_log_mutex
	 * from now until after the super is written, we avoid races
	 * with the tree-log code.
	 */
	mutex_lock(&root->fs_info->tree_log_mutex);

	ret = commit_fs_roots(trans, root);
	if (ret) {
		mutex_unlock(&root->fs_info->tree_log_mutex);
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	/*
	 * Since the transaction is done, we can apply the pending changes
	 * before the next transaction.
	 */
	btrfs_apply_pending_changes(root->fs_info);

	/* commit_fs_roots gets rid of all the tree log roots, it is now
	 * safe to free the root of tree log roots
	 */
	btrfs_free_log_root_tree(trans, root->fs_info);

	/*
	 * Since fs roots are all committed, we can get a quite accurate
	 * new_roots. So let's do quota accounting.
	 */
	ret = btrfs_qgroup_account_extents(trans, root->fs_info);
	if (ret < 0) {
		mutex_unlock(&root->fs_info->tree_log_mutex);
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	ret = commit_cowonly_roots(trans, root);
	if (ret) {
		mutex_unlock(&root->fs_info->tree_log_mutex);
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	/*
	 * The tasks which save the space cache and inode cache may also
	 * update ->aborted, check it.
	 */
	if (unlikely(ACCESS_ONCE(cur_trans->aborted))) {
		ret = cur_trans->aborted;
		mutex_unlock(&root->fs_info->tree_log_mutex);
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	btrfs_prepare_extent_commit(trans, root);

	cur_trans = root->fs_info->running_transaction;

	btrfs_set_root_node(&root->fs_info->tree_root->root_item,
			    root->fs_info->tree_root->node);
	list_add_tail(&root->fs_info->tree_root->dirty_list,
		      &cur_trans->switch_commits);

	btrfs_set_root_node(&root->fs_info->chunk_root->root_item,
			    root->fs_info->chunk_root->node);
	list_add_tail(&root->fs_info->chunk_root->dirty_list,
		      &cur_trans->switch_commits);

	switch_commit_roots(cur_trans, root->fs_info);

	assert_qgroups_uptodate(trans);
	ASSERT(list_empty(&cur_trans->dirty_bgs));
	ASSERT(list_empty(&cur_trans->io_bgs));
	update_super_roots(root);

	btrfs_set_super_log_root(root->fs_info->super_copy, 0);
	btrfs_set_super_log_root_level(root->fs_info->super_copy, 0);
	memcpy(root->fs_info->super_for_commit, root->fs_info->super_copy,
	       sizeof(*root->fs_info->super_copy));

	btrfs_update_commit_device_size(root->fs_info);
	btrfs_update_commit_device_bytes_used(root, cur_trans);

	clear_bit(BTRFS_INODE_BTREE_LOG1_ERR, &btree_ino->runtime_flags);
	clear_bit(BTRFS_INODE_BTREE_LOG2_ERR, &btree_ino->runtime_flags);

	btrfs_trans_release_chunk_metadata(trans);

	spin_lock(&root->fs_info->trans_lock);
	cur_trans->state = TRANS_STATE_UNBLOCKED;
	root->fs_info->running_transaction = NULL;
	spin_unlock(&root->fs_info->trans_lock);
	mutex_unlock(&root->fs_info->reloc_mutex);

	wake_up(&root->fs_info->transaction_wait);

	ret = btrfs_write_and_wait_transaction(trans, root);
	if (ret) {
		btrfs_std_error(root->fs_info, ret,
			    "Error while writing out transaction");
		mutex_unlock(&root->fs_info->tree_log_mutex);
		goto scrub_continue;
	}

	ret = write_ctree_super(trans, root, 0);
	if (ret) {
		mutex_unlock(&root->fs_info->tree_log_mutex);
		goto scrub_continue;
	}

	/*
	 * the super is written, we can safely allow the tree-loggers
	 * to go about their business
	 */
	mutex_unlock(&root->fs_info->tree_log_mutex);

	btrfs_finish_extent_commit(trans, root);

	if (test_bit(BTRFS_TRANS_HAVE_FREE_BGS, &cur_trans->flags))
		btrfs_clear_space_info_full(root->fs_info);

	root->fs_info->last_trans_committed = cur_trans->transid;
	/*
	 * We needn't acquire the lock here because there is no other task
	 * which can change it.
	 */
	cur_trans->state = TRANS_STATE_COMPLETED;
	wake_up(&cur_trans->commit_wait);

	spin_lock(&root->fs_info->trans_lock);
	list_del_init(&cur_trans->list);
	spin_unlock(&root->fs_info->trans_lock);

	btrfs_put_transaction(cur_trans);
	btrfs_put_transaction(cur_trans);

	if (trans->type & __TRANS_FREEZABLE)
		sb_end_intwrite(root->fs_info->sb);

	trace_btrfs_transaction_commit(root);

	btrfs_scrub_continue(root);

	if (current->journal_info == trans)
		current->journal_info = NULL;

	kmem_cache_free(btrfs_trans_handle_cachep, trans);

	if (current != root->fs_info->transaction_kthread &&
	    current != root->fs_info->cleaner_kthread)
		btrfs_run_delayed_iputs(root);

	return ret;

scrub_continue:
	btrfs_scrub_continue(root);
cleanup_transaction:
	btrfs_trans_release_metadata(trans, root);
	btrfs_trans_release_chunk_metadata(trans);
	trans->block_rsv = NULL;
	btrfs_warn(root->fs_info, "Skipping commit of aborted transaction.");
	if (current->journal_info == trans)
		current->journal_info = NULL;
	cleanup_transaction(trans, root, ret);

	return ret;
}

/*
 * return < 0 if error
 * 0 if there are no more dead_roots at the time of call
 * 1 there are more to be processed, call me again
 *
 * The return value indicates there are certainly more snapshots to delete, but
 * if there comes a new one during processing, it may return 0. We don't mind,
 * because btrfs_commit_super will poke cleaner thread and it will process it a
 * few seconds later.
 */
int btrfs_clean_one_deleted_snapshot(struct btrfs_root *root)
{
	int ret;
	struct btrfs_fs_info *fs_info = root->fs_info;

	spin_lock(&fs_info->trans_lock);
	if (list_empty(&fs_info->dead_roots)) {
		spin_unlock(&fs_info->trans_lock);
		return 0;
	}
	root = list_first_entry(&fs_info->dead_roots,
			struct btrfs_root, root_list);
	list_del_init(&root->root_list);
	spin_unlock(&fs_info->trans_lock);

	pr_debug("BTRFS: cleaner removing %llu\n", root->objectid);

	btrfs_kill_all_delayed_nodes(root);

	if (btrfs_header_backref_rev(root->node) <
			BTRFS_MIXED_BACKREF_REV)
		ret = btrfs_drop_snapshot(root, NULL, 0, 0);
	else
		ret = btrfs_drop_snapshot(root, NULL, 1, 0);

	return (ret < 0) ? 0 : 1;
}

void btrfs_apply_pending_changes(struct btrfs_fs_info *fs_info)
{
	unsigned long prev;
	unsigned long bit;

	prev = xchg(&fs_info->pending_changes, 0);
	if (!prev)
		return;

	bit = 1 << BTRFS_PENDING_SET_INODE_MAP_CACHE;
	if (prev & bit)
		btrfs_set_opt(fs_info->mount_opt, INODE_MAP_CACHE);
	prev &= ~bit;

	bit = 1 << BTRFS_PENDING_CLEAR_INODE_MAP_CACHE;
	if (prev & bit)
		btrfs_clear_opt(fs_info->mount_opt, INODE_MAP_CACHE);
	prev &= ~bit;

	bit = 1 << BTRFS_PENDING_COMMIT;
	if (prev & bit)
		btrfs_debug(fs_info, "pending commit done");
	prev &= ~bit;

	if (prev)
		btrfs_warn(fs_info,
			"unknown pending changes left 0x%lx, ignoring", prev);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /*
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

#ifndef __BTRFS_TRANSACTION__
#define __BTRFS_TRANSACTION__
#include "btrfs_inode.h"
#include "delayed-ref.h"
#include "ctree.h"

enum btrfs_trans_state {
	TRANS_STATE_RUNNING		= 0,
	TRANS_STATE_BLOCKED		= 1,
	TRANS_STATE_COMMIT_START	= 2,
	TRANS_STATE_COMMIT_DOING	= 3,
	TRANS_STATE_UNBLOCKED		= 4,
	TRANS_STATE_COMPLETED		= 5,
	TRANS_STATE_MAX			= 6,
};

#define BTRFS_TRANS_HAVE_FREE_BGS	0
#define BTRFS_TRANS_DIRTY_BG_RUN	1
#define BTRFS_TRANS_CACHE_ENOSPC	2

struct btrfs_transaction {
	u64 transid;
	/*
	 * total external writers(USERSPACE/START/ATTACH) in this
	 * transaction, it must be zero before the transaction is
	 * being committed
	 */
	atomic_t num_extwriters;
	/*
	 * total writers in this transaction, it must be zero before the
	 * transaction can end
	 */
	atomic_t num_writers;
	atomic_t use_count;
	atomic_t pending_ordered;

	unsigned long flags;

	/* Be protected by fs_info->trans_lock when we want to change it. */
	enum btrfs_trans_state state;
	struct list_head list;
	struct extent_io_tree dirty_pages;
	unsigned long start_time;
	wait_queue_head_t writer_wait;
	wait_queue_head_t commit_wait;
	wait_queue_head_t pending_wait;
	struct list_head pending_snapshots;
	struct list_head pending_chunks;
	struct list_head switch_commits;
	struct list_head dirty_bgs;
	struct list_head io_bgs;
	struct list_head dropped_roots;
	u64 num_dirty_bgs;

	/*
	 * we need to make sure block group deletion doesn't race with
	 * free space cache writeout.  This mutex keeps them from stomping
	 * on each other
	 */
	struct mutex cache_write_mutex;
	spinlock_t dirty_bgs_lock;
	/* Protected by spin lock fs_info->unused_bgs_lock. */
	struct list_head deleted_bgs;
	spinlock_t dropped_roots_lock;
	struct btrfs_delayed_ref_root delayed_refs;
	int aborted;
};

#define __TRANS_FREEZABLE	(1U << 0)

#define __TRANS_USERSPACE	(1U << 8)
#define __TRANS_START		(1U << 9)
#define __TRANS_ATTACH		(1U << 10)
#define __TRANS_JOIN		(1U << 11)
#define __TRANS_JOIN_NOLOCK	(1U << 12)
#define __TRANS_DUMMY		(1U << 13)

#define TRANS_USERSPACE		(__TRANS_USERSPACE | __TRANS_FREEZABLE)
#define TRANS_START		(__TRANS_START | __TRANS_FREEZABLE)
#define TRANS_ATTACH		(__TRANS_ATTACH)
#define TRANS_JOIN		(__TRANS_JOIN | __TRANS_FREEZABLE)
#define TRANS_JOIN_NOLOCK	(__TRANS_JOIN_NOLOCK)

#define TRANS_EXTWRITERS	(__TRANS_USERSPACE | __TRANS_START |	\
				 __TRANS_ATTACH)

#define BTRFS_SEND_TRANS_STUB	((void *)1)

struct btrfs_trans_handle {
	u64 transid;
	u64 bytes_reserved;
	u64 chunk_bytes_reserved;
	unsigned long use_count;
	unsigned long blocks_reserved;
	unsigned long delayed_ref_updates;
	struct btrfs_transaction *transaction;
	struct btrfs_block_rsv *block_rsv;
	struct btrfs_block_rsv *orig_rsv;
	short aborted;
	short adding_csums;
	bool allocating_chunk;
	bool can_flush_pending_bgs;
	bool reloc_reserved;
	bool sync;
	bool dirty;
	unsigned int type;
	/*
	 * this root is only needed to validate that the root passed to
	 * start_transaction is the same as the one passed to end_transaction.
	 * Subvolume quota depends on this
	 */
	struct btrfs_root *root;
	struct seq_list delayed_ref_elem;
	struct list_head qgroup_ref_list;
	struct list_head new_bgs;
};

struct btrfs_pending_snapshot {
	struct dentry *dentry;
	struct inode *dir;
	struct btrfs_root *root;
	struct btrfs_root *snap;
	struct btrfs_qgroup_inherit *inherit;
	/* block reservation for the operation */
	struct btrfs_block_rsv block_rsv;
	u64 qgroup_reserved;
	/* extra metadata reseration for relocation */
	int error;
	bool readonly;
	struct list_head list;
};

static inline void btrfs_set_inode_last_trans(struct btrfs_trans_handle *trans,
					      struct inode *inode)
{
	spin_lock(&BTRFS_I(inode)->lock);
	BTRFS_I(inode)->last_trans = trans->transaction->transid;
	BTRFS_I(inode)->last_sub_trans = BTRFS_I(inode)->root->log_transid;
	BTRFS_I(inode)->last_log_commit = BTRFS_I(inode)->root->last_log_commit;
	spin_unlock(&BTRFS_I(inode)->lock);
}

/*
 * Make qgroup codes to skip given qgroupid, means the old/new_roots for
 * qgroup won't contain the qgroupid in it.
 */
static inline void btrfs_set_skip_qgroup(struct btrfs_trans_handle *trans,
					 u64 qgroupid)
{
	struct btrfs_delayed_ref_root *delayed_refs;

	delayed_refs = &trans->transaction->delayed_refs;
	WARN_ON(delayed_refs->qgroup_to_skip);
	delayed_refs->qgroup_to_skip = qgroupid;
}

static inline void btrfs_clear_skip_qgroup(struct btrfs_trans_handle *trans)
{
	struct btrfs_delayed_ref_root *delayed_refs;

	delayed_refs = &trans->transaction->delayed_refs;
	WARN_ON(!delayed_refs->qgroup_to_skip);
	delayed_refs->qgroup_to_skip = 0;
}

int btrfs_end_transaction(struct btrfs_trans_handle *trans,
			  struct btrfs_root *root);
struct btrfs_trans_handle *btrfs_start_transaction(struct btrfs_root *root,
						   unsigned int num_items);
struct btrfs_trans_handle *btrfs_start_transaction_fallback_global_rsv(
					struct btrfs_root *root,
					unsigned int num_items,
					int min_factor);
struct btrfs_trans_handle *btrfs_start_transaction_lflush(
					struct btrfs_root *root,
					unsigned int num_items);
struct btrfs_trans_handle *btrfs_join_transaction(struct btrfs_root *root);
struct btrfs_trans_handle *btrfs_join_transaction_nolock(struct btrfs_root *root);
struct btrfs_trans_handle *btrfs_attach_transaction(struct btrfs_root *root);
struct btrfs_trans_handle *btrfs_attach_transaction_barrier(
					struct btrfs_root *root);
struct btrfs_trans_handle *btrfs_start_ioctl_transaction(struct btrfs_root *root);
int btrfs_wait_for_commit(struct btrfs_root *root, u64 transid);

void btrfs_add_dead_root(struct btrfs_root *root);
int btrfs_defrag_root(struct btrfs_root *root);
int btrfs_clean_one_deleted_snapshot(struct btrfs_root *root);
int btrfs_commit_transaction(struct btrfs_trans_handle *trans,
			     struct btrfs_root *root);
int btrfs_commit_transaction_async(struct btrfs_trans_handle *trans,
				   struct btrfs_root *root,
				   int wait_for_unblock);
int btrfs_end_transaction_throttle(struct btrfs_trans_handle *trans,
				   struct btrfs_root *root);
int btrfs_should_end_transaction(struct btrfs_trans_handle *trans,
				 struct btrfs_root *root);
void btrfs_throttle(struct btrfs_root *root);
int btrfs_record_root_in_trans(struct btrfs_trans_handle *trans,
				struct btrfs_root *root);
int btrfs_write_marked_extents(struct btrfs_root *root,
				struct extent_io_tree *dirty_pages, int mark);
int btrfs_wait_marked_extents(struct btrfs_root *root,
				struct extent_io_tree *dirty_pages, int mark);
int btrfs_transaction_blocked(struct btrfs_fs_info *info);
int btrfs_transaction_in_commit(struct btrfs_fs_info *info);
void btrfs_put_transaction(struct btrfs_transaction *transaction);
void btrfs_apply_pending_changes(struct btrfs_fs_info *fs_info);
void btrfs_add_dropped_root(struct btrfs_trans_handle *trans,
			    struct btrfs_root *root);
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*
 * Copyright (C) Qu Wenruo 2017.  All rights reserved.
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
 * License along with this program.
 */

/*
 * The module is used to catch unexpected/corrupted tree block data.
 * Such behavior can be caused either by a fuzzed image or bugs.
 *
 * The objective is to do leaf/node validation checks when tree block is read
 * from disk, and check *every* possible member, so other code won't
 * need to checking them again.
 *
 * Due to the potential and unwanted damage, every checker needs to be
 * carefully reviewed otherwise so it does not prevent mount of valid images.
 */

#include "ctree.h"
#include "tree-checker.h"
#include "disk-io.h"
#include "compression.h"
#include "hash.h"
#include "volumes.h"

#define CORRUPT(reason, eb, root, slot)					\
	btrfs_crit(root->fs_info,					\
		   "corrupt %s, %s: block=%llu, root=%llu, slot=%d",	\
		   btrfs_header_level(eb) == 0 ? "leaf" : "node",	\
		   reason, btrfs_header_bytenr(eb), root->objectid, slot)

/*
 * Error message should follow the following format:
 * corrupt <type>: <identifier>, <reason>[, <bad_value>]
 *
 * @type:	leaf or node
 * @identifier:	the necessary info to locate the leaf/node.
 * 		It's recommened to decode key.objecitd/offset if it's
 * 		meaningful.
 * @reason:	describe the error
 * @bad_value:	optional, it's recommened to output bad value and its
 *		expected value (range).
 *
 * Since comma is used to separate the components, only space is allowed
 * inside each component.
 */

/*
 * Append generic "corrupt leaf/node root=%llu block=%llu slot=%d: " to @fmt.
 * Allows callers to customize the output.
 */
__printf(4, 5)
static void generic_err(const struct btrfs_root *root,
			const struct extent_buffer *eb, int slot,
			const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	btrfs_crit(root->fs_info,
		"corrupt %s: root=%llu block=%llu slot=%d, %pV",
		btrfs_header_level(eb) == 0 ? "leaf" : "node",
		root->objectid, btrfs_header_bytenr(eb), slot, &vaf);
	va_end(args);
}

static int check_extent_data_item(struct btrfs_root *root,
				  struct extent_buffer *leaf,
				  struct btrfs_key *key, int slot)
{
	struct btrfs_file_extent_item *fi;
	u32 sectorsize = root->sectorsize;
	u32 item_size = btrfs_item_size_nr(leaf, slot);

	if (!IS_ALIGNED(key->offset, sectorsize)) {
		CORRUPT("unaligned key offset for file extent",
			leaf, root, slot);
		return -EUCLEAN;
	}

	fi = btrfs_item_ptr(leaf, slot, struct btrfs_file_extent_item);

	if (btrfs_file_extent_type(leaf, fi) > BTRFS_FILE_EXTENT_TYPES) {
		CORRUPT("invalid file extent type", leaf, root, slot);
		return -EUCLEAN;
	}

	/*
	 * Support for new compression/encrption must introduce incompat flag,
	 * and must be caught in open_ctree().
	 */
	if (btrfs_file_extent_compression(leaf, fi) > BTRFS_COMPRESS_TYPES) {
		CORRUPT("invalid file extent compression", leaf, root, slot);
		return -EUCLEAN;
	}
	if (btrfs_file_extent_encryption(leaf, fi)) {
		CORRUPT("invalid file extent encryption", leaf, root, slot);
		return -EUCLEAN;
	}
	if (btrfs_file_extent_type(leaf, fi) == BTRFS_FILE_EXTENT_INLINE) {
		/* Inline extent must have 0 as key offset */
		if (key->offset) {
			CORRUPT("inline extent has non-zero key offset",
				leaf, root, slot);
			return -EUCLEAN;
		}

		/* Compressed inline extent has no on-disk size, skip it */
		if (btrfs_file_extent_compression(leaf, fi) !=
		    BTRFS_COMPRESS_NONE)
			return 0;

		/* Uncompressed inline extent size must match item size */
		if (item_size != BTRFS_FILE_EXTENT_INLINE_DATA_START +
		    btrfs_file_extent_ram_bytes(leaf, fi)) {
			CORRUPT("plaintext inline extent has invalid size",
				leaf, root, slot);
			return -EUCLEAN;
		}
		return 0;
	}

	/* Regular or preallocated extent has fixed item size */
	if (item_size != sizeof(*fi)) {
		CORRUPT(
		"regluar or preallocated extent data item size is invalid",
			leaf, root, slot);
		return -EUCLEAN;
	}
	if (!IS_ALIGNED(btrfs_file_extent_ram_bytes(leaf, fi), sectorsize) ||
	    !IS_ALIGNED(btrfs_file_extent_disk_bytenr(leaf, fi), sectorsize) ||
	    !IS_ALIGNED(btrfs_file_extent_disk_num_bytes(leaf, fi), sectorsize) ||
	    !IS_ALIGNED(btrfs_file_extent_offset(leaf, fi), sectorsize) ||
	    !IS_ALIGNED(btrfs_file_extent_num_bytes(leaf, fi), sectorsize)) {
		CORRUPT(
		"regular or preallocated extent data item has unaligned value",
			leaf, root, slot);
		return -EUCLEAN;
	}

	return 0;
}

static int check_csum_item(struct btrfs_root *root, struct extent_buffer *leaf,
			   struct btrfs_key *key, int slot)
{
	u32 sectorsize = root->sectorsize;
	u32 csumsize = btrfs_super_csum_size(root->fs_info->super_copy);

	if (key->objectid != BTRFS_EXTENT_CSUM_OBJECTID) {
		CORRUPT("invalid objectid for csum item", leaf, root, slot);
		return -EUCLEAN;
	}
	if (!IS_ALIGNED(key->offset, sectorsize)) {
		CORRUPT("unaligned key offset for csum item", leaf, root, slot);
		return -EUCLEAN;
	}
	if (!IS_ALIGNED(btrfs_item_size_nr(leaf, slot), csumsize)) {
		CORRUPT("unaligned csum item size", leaf, root, slot);
		return -EUCLEAN;
	}
	return 0;
}

/*
 * Customized reported for dir_item, only important new info is key->objectid,
 * which represents inode number
 */
__printf(4, 5)
static void dir_item_err(const struct btrfs_root *root,
			 const struct extent_buffer *eb, int slot,
			 const char *fmt, ...)
{
	struct btrfs_key key;
	struct va_format vaf;
	va_list args;

	btrfs_item_key_to_cpu(eb, &key, slot);
	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	btrfs_crit(root->fs_info,
	"corrupt %s: root=%llu block=%llu slot=%d ino=%llu, %pV",
		btrfs_header_level(eb) == 0 ? "leaf" : "node", root->objectid,
		btrfs_header_bytenr(eb), slot, key.objectid, &vaf);
	va_end(args);
}

static int check_dir_item(struct btrfs_root *root,
			  struct extent_buffer *leaf,
			  struct btrfs_key *key, int slot)
{
	struct btrfs_dir_item *di;
	u32 item_size = btrfs_item_size_nr(leaf, slot);
	u32 cur = 0;

	di = btrfs_item_ptr(leaf, slot, struct btrfs_dir_item);
	while (cur < item_size) {
		u32 name_len;
		u32 data_len;
		u32 max_name_len;
		u32 total_size;
		u32 name_hash;
		u8 dir_type;

		/* header itself should not cross item boundary */
		if (cur + sizeof(*di) > item_size) {
			dir_item_err(root, leaf, slot,
		"dir item header crosses item boundary, have %zu boundary %u",
				cur + sizeof(*di), item_size);
			return -EUCLEAN;
		}

		/* dir type check */
		dir_type = btrfs_dir_type(leaf, di);
		if (dir_type >= BTRFS_FT_MAX) {
			dir_item_err(root, leaf, slot,
			"invalid dir item type, have %u expect [0, %u)",
				dir_type, BTRFS_FT_MAX);
			return -EUCLEAN;
		}

		if (key->type == BTRFS_XATTR_ITEM_KEY &&
		    dir_type != BTRFS_FT_XATTR) {
			dir_item_err(root, leaf, slot,
		"invalid dir item type for XATTR key, have %u expect %u",
				dir_type, BTRFS_FT_XATTR);
			return -EUCLEAN;
		}
		if (dir_type == BTRFS_FT_XATTR &&
		    key->type != BTRFS_XATTR_ITEM_KEY) {
			dir_item_err(root, leaf, slot,
			"xattr dir type found for non-XATTR key");
			return -EUCLEAN;
		}
		if (dir_type == BTRFS_FT_XATTR)
			max_name_len = XATTR_NAME_MAX;
		else
			max_name_len = BTRFS_NAME_LEN;

		/* Name/data length check */
		name_len = btrfs_dir_name_len(leaf, di);
		data_len = btrfs_dir_data_len(leaf, di);
		if (name_len > max_name_len) {
			dir_item_err(root, leaf, slot,
			"dir item name len too long, have %u max %u",
				name_len, max_name_len);
			return -EUCLEAN;
		}
		if (name_len + data_len > BTRFS_MAX_XATTR_SIZE(root)) {
			dir_item_err(root, leaf, slot,
			"dir item name and data len too long, have %u max %zu",
				name_len + data_len,
				BTRFS_MAX_XATTR_SIZE(root));
			return -EUCLEAN;
		}

		if (data_len && dir_type != BTRFS_FT_XATTR) {
			dir_item_err(root, leaf, slot,
			"dir item with invalid data len, have %u expect 0",
				data_len);
			return -EUCLEAN;
		}

		total_size = sizeof(*di) + name_len + data_len;

		/* header and name/data should not cross item boundary */
		if (cur + total_size > item_size) {
			dir_item_err(root, leaf, slot,
		"dir item data crosses item boundary, have %u boundary %u",
				cur + total_size, item_size);
			return -EUCLEAN;
		}

		/*
		 * Special check for XATTR/DIR_ITEM, as key->offset is name
		 * hash, should match its name
		 */
		if (key->type == BTRFS_DIR_ITEM_KEY ||
		    key->type == BTRFS_XATTR_ITEM_KEY) {
			char namebuf[max(BTRFS_NAME_LEN, XATTR_NAME_MAX)];

			read_extent_buffer(leaf, namebuf,
					(unsigned long)(di + 1), name_len);
			name_hash = btrfs_name_hash(namebuf, name_len);
			if (key->offset != name_hash) {
				dir_item_err(root, leaf, slot,
		"name hash mismatch with key, have 0x%016x expect 0x%016llx",
					name_hash, key->offset);
				return -EUCLEAN;
			}
		}
		cur += total_size;
		di = (struct btrfs_dir_item *)((void *)di + total_size);
	}
	return 0;
}

__printf(4, 5)
__cold
static void block_group_err(const struct btrfs_fs_info *fs_info,
			    const struct extent_buffer *eb, int slot,
			    const char *fmt, ...)
{
	struct btrfs_key key;
	struct va_format vaf;
	va_list args;

	btrfs_item_key_to_cpu(eb, &key, slot);
	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	btrfs_crit(fs_info,
	"corrupt %s: root=%llu block=%llu slot=%d bg_start=%llu bg_len=%llu, %pV",
		btrfs_header_level(eb) == 0 ? "leaf" : "node",
		btrfs_header_owner(eb), btrfs_header_bytenr(eb), slot,
		key.objectid, key.offset, &vaf);
	va_end(args);
}

static int check_block_group_item(struct btrfs_fs_info *fs_info,
				  struct extent_buffer *leaf,
				  struct btrfs_key *key, int slot)
{
	struct btrfs_block_group_item bgi;
	u32 item_size = btrfs_item_size_nr(leaf, slot);
	u64 flags;
	u64 type;

	/*
	 * Here we don't really care about alignment since extent allocator can
	 * handle it.  We care more about the size, as if one block group is
	 * larger than maximum size, it's must be some obvious corruption.
	 */
	if (key->offset > BTRFS_MAX_DATA_CHUNK_SIZE || key->offset == 0) {
		block_group_err(fs_info, leaf, slot,
			"invalid block group size, have %llu expect (0, %llu]",
				key->offset, BTRFS_MAX_DATA_CHUNK_SIZE);
		return -EUCLEAN;
	}

	if (item_size != sizeof(bgi)) {
		block_group_err(fs_info, leaf, slot,
			"invalid item size, have %u expect %zu",
				item_size, sizeof(bgi));
		return -EUCLEAN;
	}

	read_extent_buffer(leaf, &bgi, btrfs_item_ptr_offset(leaf, slot),
			   sizeof(bgi));
	if (btrfs_block_group_chunk_objectid(&bgi) !=
	    BTRFS_FIRST_CHUNK_TREE_OBJECTID) {
		block_group_err(fs_info, leaf, slot,
		"invalid block group chunk objectid, have %llu expect %llu",
				btrfs_block_group_chunk_objectid(&bgi),
				BTRFS_FIRST_CHUNK_TREE_OBJECTID);
		return -EUCLEAN;
	}

	if (btrfs_block_group_used(&bgi) > key->offset) {
		block_group_err(fs_info, leaf, slot,
			"invalid block group used, have %llu expect [0, %llu)",
				btrfs_block_group_used(&bgi), key->offset);
		return -EUCLEAN;
	}

	flags = btrfs_block_group_flags(&bgi);
	if (hweight64(flags & BTRFS_BLOCK_GROUP_PROFILE_MASK) > 1) {
		block_group_err(fs_info, leaf, slot,
"invalid profile flags, have 0x%llx (%lu bits set) expect no more than 1 bit set",
			flags & BTRFS_BLOCK_GROUP_PROFILE_MASK,
			hweight64(flags & BTRFS_BLOCK_GROUP_PROFILE_MASK));
		return -EUCLEAN;
	}

	type = flags & BTRFS_BLOCK_GROUP_TYPE_MASK;
	if (type != BTRFS_BLOCK_GROUP_DATA &&
	    type != BTRFS_BLOCK_GROUP_METADATA &&
	    type != BTRFS_BLOCK_GROUP_SYSTEM &&
	    type != (BTRFS_BLOCK_GROUP_METADATA |
			   BTRFS_BLOCK_GROUP_DATA)) {
		block_group_err(fs_info, leaf, slot,
"invalid type, have 0x%llx (%lu bits set) expect either 0x%llx, 0x%llx, 0x%llx or 0x%llx",
			type, hweight64(type),
			BTRFS_BLOCK_GROUP_DATA, BTRFS_BLOCK_GROUP_METADATA,
			BTRFS_BLOCK_GROUP_SYSTEM,
			BTRFS_BLOCK_GROUP_METADATA | BTRFS_BLOCK_GROUP_DATA);
		return -EUCLEAN;
	}
	return 0;
}

/*
 * Common point to switch the item-specific validation.
 */
static int check_leaf_item(struct btrfs_root *root,
			   struct extent_buffer *leaf,
			   struct btrfs_key *key, int slot)
{
	int ret = 0;

	switch (key->type) {
	case BTRFS_EXTENT_DATA_KEY:
		ret = check_extent_data_item(root, leaf, key, slot);
		break;
	case BTRFS_EXTENT_CSUM_KEY:
		ret = check_csum_item(root, leaf, key, slot);
		break;
	case BTRFS_DIR_ITEM_KEY:
	case BTRFS_DIR_INDEX_KEY:
	case BTRFS_XATTR_ITEM_KEY:
		ret = check_dir_item(root, leaf, key, slot);
		break;
	case BTRFS_BLOCK_GROUP_ITEM_KEY:
		ret = check_block_group_item(root->fs_info, leaf, key, slot);
		break;
	}
	return ret;
}

static int check_leaf(struct btrfs_root *root, struct extent_buffer *leaf,
		      bool check_item_data)
{
	struct btrfs_fs_info *fs_info = root->fs_info;
	/* No valid key type is 0, so all key should be larger than this key */
	struct btrfs_key prev_key = {0, 0, 0};
	struct btrfs_key key;
	u32 nritems = btrfs_header_nritems(leaf);
	int slot;

	if (btrfs_header_level(leaf) != 0) {
		generic_err(root, leaf, 0,
			"invalid level for leaf, have %d expect 0",
			btrfs_header_level(leaf));
		return -EUCLEAN;
	}

	/*
	 * Extent buffers from a relocation tree have a owner field that
	 * corresponds to the subvolume tree they are based on. So just from an
	 * extent buffer alone we can not find out what is the id of the
	 * corresponding subvolume tree, so we can not figure out if the extent
	 * buffer corresponds to the root of the relocation tree or not. So
	 * skip this check for relocation trees.
	 */
	if (nritems == 0 && !btrfs_header_flag(leaf, BTRFS_HEADER_FLAG_RELOC)) {
		u64 owner = btrfs_header_owner(leaf);
		struct btrfs_root *check_root;

		/* These trees must never be empty */
		if (owner == BTRFS_ROOT_TREE_OBJECTID ||
		    owner == BTRFS_CHUNK_TREE_OBJECTID ||
		    owner == BTRFS_EXTENT_TREE_OBJECTID ||
		    owner == BTRFS_DEV_TREE_OBJECTID ||
		    owner == BTRFS_FS_TREE_OBJECTID ||
		    owner == BTRFS_DATA_RELOC_TREE_OBJECTID) {
			generic_err(root, leaf, 0,
			"invalid root, root %llu must never be empty",
				    owner);
			return -EUCLEAN;
		}
		key.objectid = owner;
		key.type = BTRFS_ROOT_ITEM_KEY;
		key.offset = (u64)-1;

		check_root = btrfs_get_fs_root(fs_info, &key, false);
		/*
		 * The only reason we also check NULL here is that during
		 * open_ctree() some roots has not yet been set up.
		 */
		if (!IS_ERR_OR_NULL(check_root)) {
			struct extent_buffer *eb;

			eb = btrfs_root_node(check_root);
			/* if leaf is the root, then it's fine */
			if (leaf != eb) {
				CORRUPT("non-root leaf's nritems is 0",
					leaf, check_root, 0);
				free_extent_buffer(eb);
				return -EUCLEAN;
			}
			free_extent_buffer(eb);
		}
		return 0;
	}

	if (nritems == 0)
		return 0;

	/*
	 * Check the following things to make sure this is a good leaf, and
	 * leaf users won't need to bother with similar sanity checks:
	 *
	 * 1) key ordering
	 * 2) item offset and size
	 *    No overlap, no hole, all inside the leaf.
	 * 3) item content
	 *    If possible, do comprehensive sanity check.
	 *    NOTE: All checks must only rely on the item data itself.
	 */
	for (slot = 0; slot < nritems; slot++) {
		u32 item_end_expected;
		int ret;

		btrfs_item_key_to_cpu(leaf, &key, slot);

		/* Make sure the keys are in the right order */
		if (btrfs_comp_cpu_keys(&prev_key, &key) >= 0) {
			CORRUPT("bad key order", leaf, root, slot);
			return -EUCLEAN;
		}

		/*
		 * Make sure the offset and ends are right, remember that the
		 * item data starts at the end of the leaf and grows towards the
		 * front.
		 */
		if (slot == 0)
			item_end_expected = BTRFS_LEAF_DATA_SIZE(root);
		else
			item_end_expected = btrfs_item_offset_nr(leaf,
								 slot - 1);
		if (btrfs_item_end_nr(leaf, slot) != item_end_expected) {
			CORRUPT("slot offset bad", leaf, root, slot);
			return -EUCLEAN;
		}

		/*
		 * Check to make sure that we don't point outside of the leaf,
		 * just in case all the items are consistent to each other, but
		 * all point outside of the leaf.
		 */
		if (btrfs_item_end_nr(leaf, slot) >
		    BTRFS_LEAF_DATA_SIZE(root)) {
			CORRUPT("slot end outside of leaf", leaf, root, slot);
			return -EUCLEAN;
		}

		/* Also check if the item pointer overlaps with btrfs item. */
		if (btrfs_item_nr_offset(slot) + sizeof(struct btrfs_item) >
		    btrfs_item_ptr_offset(leaf, slot)) {
			CORRUPT("slot overlap with its data", leaf, root, slot);
			return -EUCLEAN;
		}

		if (check_item_data) {
			/*
			 * Check if the item size and content meet other
			 * criteria
			 */
			ret = check_leaf_item(root, leaf, &key, slot);
			if (ret < 0)
				return ret;
		}

		prev_key.objectid = key.o