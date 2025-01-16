* extent entries, both for clustered and non-clustered allocation requests.
 */
static int
test_steal_space_from_bitmap_to_extent(struct btrfs_block_group_cache *cache)
{
	int ret;
	u64 offset;
	u64 max_extent_size;

	bool (*use_bitmap_op)(struct btrfs_free_space_ctl *,
			      struct btrfs_free_space *);

	test_msg("Running space stealing from bitmap to extent\n");

	/*
	 * For this test, we want to ensure we end up with an extent entry
	 * immediately adjacent to a bitmap entry, where the bitmap starts
	 * at an offset where the extent entry ends. We keep adding and
	 * removing free space to reach into this state, but to get there
	 * we need to reach a point where marking new free space doesn't
	 * result in adding new extent entries or merging the new space
	 * with existing extent entries - the space ends up being marked
	 * in an existing bitmap that covers the new free space range.
	 *
	 * To get there, we need to reach the threshold defined set at
	 * cache->free_space_ctl->extents_thresh, which currently is
	 * 256 extents on a x86_64 system at least, and a few other
	 * conditions (check free_space_cache.c). Instead of making the
	 * test much longer and complicated, use a "use_bitmap" operation
	 * that forces use of bitmaps as soon as we have at least 1
	 * extent entry.
	 */
	use_bitmap_op = cache->free_space_ctl->op->use_bitmap;
	cache->free_space_ctl->op->use_bitmap = test_use_bitmap;

	/*
	 * Extent entry covering free space range [128Mb - 256Kb, 128Mb - 128Kb[
	 */
	ret = test_add_free_space_entry(cache, 128 * 1024 * 1024 - 256 * 1024,
					128 * 1024, 0);
	if (ret) {
		test_msg("Couldn't add extent entry %d\n", ret);
		return ret;
	}

	/* Bitmap entry covering free space range [128Mb + 512Kb, 256Mb[ */
	ret = test_add_free_space_entry(cache, 128 * 1024 * 1024 + 512 * 1024,
					128 * 1024 * 1024 - 512 * 1024, 1);
	if (ret) {
		test_msg("Couldn't add bitmap entry %d\n", ret);
		return ret;
	}

	ret = check_num_extents_and_bitmaps(cache, 2, 1);
	if (ret)
		return ret;

	/*
	 * Now make only the first 256Kb of the bitmap marked as free, so that
	 * we end up with only the following ranges marked as free space:
	 *
	 * [128Mb - 256Kb, 128Mb - 128Kb[
	 * [128Mb + 512Kb, 128Mb + 768Kb[
	 */
	ret = btrfs_remove_free_space(cache,
				      128 * 1024 * 1024 + 768 * 1024,
				      128 * 1024 * 1024 - 768 * 1024);
	if (ret) {
		test_msg("Failed to free part of bitmap space %d\n", ret);
		return ret;
	}

	/* Confirm that only those 2 ranges are marked as free. */
	if (!test_check_exists(cache, 128 * 1024 * 1024 - 256 * 1024,
			       128 * 1024)) {
		test_msg("Free space range missing\n");
		return -ENOENT;
	}
	if (!test_check_exists(cache, 128 * 1024 * 1024 + 512 * 1024,
			       256 * 1024)) {
		test_msg("Free space range missing\n");
		return -ENOENT;
	}

	/*
	 * Confirm that the bitmap range [128Mb + 768Kb, 256Mb[ isn't marked
	 * as free anymore.
	 */
	if (test_check_exists(cache, 128 * 1024 * 1024 + 768 * 1024,
			      128 * 1024 * 1024 - 768 * 1024)) {
		test_msg("Bitmap region not removed from space cache\n");
		return -EINVAL;
	}

	/*
	 * Confirm that the region [128Mb + 256Kb, 128Mb + 512Kb[, which is
	 * covered by the bitmap, isn't marked as free.
	 */
	if (test_check_exists(cache, 128 * 1024 * 1024 + 256 * 1024,
			      256 * 1024)) {
		test_msg("Invalid bitmap region marked as free\n");
		return -EINVAL;
	}

	/*
	 * Confirm that the region [128Mb, 128Mb + 256Kb[, which is covered
	 * by the bitmap too, isn't marked as free either.
	 */
	if (test_check_exists(cache, 128 * 1024 * 1024,
			      256 * 1024)) {
		test_msg("Invalid bitmap region marked as free\n");
		return -EINVAL;
	}

	/*
	 * Now lets mark the region [128Mb, 128Mb + 512Kb[ as free too. But,
	 * lets make sure the free space cache marks it as free in the bitmap,
	 * and doesn't insert a new extent entry to represent this region.
	 */
	ret = btrfs_add_free_space(cache, 128 * 1024 * 1024, 512 * 1024);
	if (ret) {
		test_msg("Error adding free space: %d\n", ret);
		return ret;
	}
	/* Confirm the region is marked as free. */
	if (!test_check_exists(cache, 128 * 1024 * 1024, 512 * 1024)) {
		test_msg("Bitmap region not marked as free\n");
		return -ENOENT;
	}

	/*
	 * Confirm that no new extent entries or bitmap entries were added to
	 * the cache after adding that free space region.
	 */
	ret = check_num_extents_and_bitmaps(cache, 2, 1);
	if (ret)
		return ret;

	/*
	 * Now lets add a small free space region to the right of the previous
	 * one, which is not contiguous with it and is part of the bitmap too.
	 * The goal is to test that the bitmap entry space stealing doesn't
	 * steal this space region.
	 */
	ret = btrfs_add_free_space(cache, 128 * 1024 * 1024 + 16 * 1024 * 1024,
				   4096);
	if (ret) {
		test_msg("Error adding free space: %d\n", ret);
		return ret;
	}

	/*
	 * Confirm that no new extent entries or bitmap entries were added to
	 * the cache after adding that free space region.
	 */
	ret = check_num_extents_and_bitmaps(cache, 2, 1);
	if (ret)
		return ret;

	/*
	 * Now mark the region [128Mb - 128Kb, 128Mb[ as free too. This will
	 * expand the range covered by the existing extent entry that represents
	 * the free space [128Mb - 256Kb, 128Mb - 128Kb[.
	 */
	ret = btrfs_add_free_space(cache, 128 * 1024 * 1024 - 128 * 1024,
				   128 * 1024);
	if (ret) {
		test_msg("Error adding free space: %d\n", ret);
		return ret;
	}
	/* Confirm the region is marked as free. */
	if (!test_check_exists(cache, 128 * 1024 * 1024 - 128 * 1024,
			       128 * 1024)) {
		test_msg("Extent region not marked as free\n");
		return -ENOENT;
	}

	/*
	 * Confirm that our extent entry didn't stole all free space from the
	 * bitmap, because of the small 4Kb free space region.
	 */
	ret = check_num_extents_and_bitmaps(cache, 2, 1);
	if (ret)
		return ret;

	/*
	 * So now we have the range [128Mb - 256Kb, 128Mb + 768Kb[ as free
	 * space. Without stealing bitmap free space into extent entry space,
	 * we would have all this free space represented by 2 entries in the
	 * cache:
	 *
	 * extent entry covering range: [128Mb - 256Kb, 128Mb[
	 * bitmap entry covering range: [128Mb, 128Mb + 768Kb[
	 *
	 * Attempting to allocate the whole free space (1Mb) would fail, because
	 * we can't allocate from multiple entries.
	 * With the bitmap free space stealing, we get a single extent entry
	 * that represents the 1Mb free space, and therefore we're able to
	 * allocate the whole free space at once.
	 */
	if (!test_check_exists(cache, 128 * 1024 * 1024 - 256 * 1024,
			       1 * 1024 * 1024)) {
		test_msg("Expected region not marked as free\n");
		return -ENOENT;
	}

	if (cache->free_space_ctl->free_space != (1 * 1024 * 1024 + 4096)) {
		test_msg("Cache free space is not 1Mb + 4Kb\n");
		return -EINVAL;
	}

	offset = btrfs_find_space_for_alloc(cache,
					    0, 1 * 1024 * 1024, 0,
					    &max_extent_size);
	if (offset != (128 * 1024 * 1024 - 256 * 1024)) {
		test_msg("Failed to allocate 1Mb from space cache, returned offset is: %llu\n",
			 offset);
		return -EINVAL;
	}

	/* All that remains is a 4Kb free space region in a bitmap. Confirm. */
	ret = check_num_extents_and_bitmaps(cache, 1, 1);
	if (ret)
		return ret;

	if (cache->free_space_ctl->free_space != 4096) {
		test_msg("Cache free space is not 4Kb\n");
		return -EINVAL;
	}

	offset = btrfs_find_space_for_alloc(cache,
					    0, 4096, 0,
					    &max_extent_size);
	if (offset != (128 * 1024 * 1024 + 16 * 1024 * 1024)) {
		test_msg("Failed to allocate 4Kb from space cache, returned offset is: %llu\n",
			 offset);
		return -EINVAL;
	}

	ret = check_cache_empty(cache);
	if (ret)
		return ret;

	__btrfs_remove_free_space_cache(cache->free_space_ctl);

	/*
	 * Now test a similar scenario, but where our extent entry is located
	 * to the right of the bitmap entry, so that we can check that stealing
	 * space from a bitmap to the front of an extent entry works.
	 */

	/*
	 * Extent entry covering free space range [128Mb + 128Kb, 128Mb + 256Kb[
	 */
	ret = test_add_free_space_entry(cache, 128 * 1024 * 1024 + 128 * 1024,
					128 * 1024, 0);
	if (ret) {
		test_msg("Couldn't add extent entry %d\n", ret);
		return ret;
	}

	/* Bitmap entry covering free space range [0, 128Mb - 512Kb[ */
	ret = test_add_free_space_entry(cache, 0,
					128 * 1024 * 1024 - 512 * 1024, 1);
	if (ret) {
		test_msg("Couldn't add bitmap entry %d\n", ret);
		return ret;
	}

	ret = check_num_extents_and_bitmaps(cache, 2, 1);
	if (ret)
		return ret;

	/*
	 * Now make only the last 256Kb of the bitmap marked as free, so that
	 * we end up with only the following ranges marked as free space:
	 *
	 * [128Mb + 128b, 128Mb + 256Kb[
	 * [128Mb - 768Kb, 128Mb - 512Kb[
	 */
	ret = btrfs_remove_free_space(cache,
				      0,
				      128 * 1024 * 1024 - 768 * 1024);
	if (ret) {
		test_msg("Failed to free part of bitmap space %d\n", ret);
		return ret;
	}

	/* Confirm that only those 2 ranges are marked as free. */
	if (!test_check_exists(cache, 128 * 1024 * 1024 + 128 * 1024,
			       128 * 1024)) {
		test_msg("Free space range missing\n");
		return -ENOENT;
	}
	if (!test_check_exists(cache, 128 * 1024 * 1024 - 768 * 1024,
			       256 * 1024)) {
		test_msg("Free space range missing\n");
		return -ENOENT;
	}

	/*
	 * Confirm that the bitmap range [0, 128Mb - 768Kb[ isn't marked
	 * as free anymore.
	 */
	if (test_check_exists(cache, 0,
			      128 * 1024 * 1024 - 768 * 1024)) {
		test_msg("Bitmap region not removed from space cache\n");
		return -EINVAL;
	}

	/*
	 * Confirm that the region [128Mb - 512Kb, 128Mb[, which is
	 * covered by the bitmap, isn't marked as free.
	 */
	if (test_check_exists(cache, 128 * 1024 * 1024 - 512 * 1024,
			      512 * 1024)) {
		test_msg("Invalid bitmap region marked as free\n");
		return -EINVAL;
	}

	/*
	 * Now lets mark the region [128Mb - 512Kb, 128Mb[ as free too. But,
	 * lets make sure the free space cache marks it as free in the bitmap,
	 * and doesn't insert a new extent entry to represent this region.
	 */
	ret = btrfs_add_free_space(cache, 128 * 1024 * 1024 - 512 * 1024,
				   512 * 1024);
	if (ret) {
		test_msg("Error adding free space: %d\n", ret);
		return ret;
	}
	/* Confirm the region is marked as free. */
	if (!test_check_exists(cache, 128 * 1024 * 1024 - 512 * 1024,
			       512 * 1024)) {
		test_msg("Bitmap region not marked as free\n");
		return -ENOENT;
	}

	/*
	 * Confirm that no new extent entries or bitmap entries were added to
	 * the cache after adding that free space region.
	 */
	ret = check_num_extents_and_bitmaps(cache, 2, 1);
	if (ret)
		return ret;

	/*
	 * Now lets add a small free space region to the left of the previous
	 * one, which is not contiguous with it and is part of the bitmap too.
	 * The goal is to test that the bitmap entry space stealing doesn't
	 * steal this space region.
	 */
	ret = btrfs_add_free_space(cache, 32 * 1024 * 1024, 8192);
	if (ret) {
		test_msg("Error adding free space: %d\n", ret);
		return ret;
	}

	/*
	 * Now mark the region [128Mb, 128Mb + 128Kb[ as free too. This will
	 * expand the range covered by the existing extent entry that represents
	 * the free space [128Mb + 128Kb, 128Mb + 256Kb[.
	 */
	ret = btrfs_add_free_space(cache, 128 * 1024 * 1024, 128 * 1024);
	if (ret) {
		test_msg("Error adding free space: %d\n", ret);
		return ret;
	}
	/* Confirm the region is marked as free. */
	if (!test_check_exists(cache, 128 * 1024 * 1024, 128 * 1024)) {
		test_msg("Extent region not marked as free\n");
		return -ENOENT;
	}

	/*
	 * Confirm that our extent entry didn't stole all free space from the
	 * bitmap, because of the small 8Kb free space region.
	 */
	ret = check_num_extents_and_bitmaps(cache, 2, 1);
	if (ret)
		return ret;

	/*
	 * So now we have the range [128Mb - 768Kb, 128Mb + 256Kb[ as free
	 * space. Without stealing bitmap free space into extent entry space,
	 * we would have all this free space represented by 2 entries in the
	 * cache:
	 *
	 * extent entry covering range: [128Mb, 128Mb + 256Kb[
	 * bitmap entry covering range: [128Mb - 768Kb, 128Mb[
	 *
	 * Attempting to allocate the whole free space (1Mb) would fail, because
	 * we can't allocate from multiple entries.
	 * With the bitmap free space stealing, we get a single extent entry
	 * that represents the 1Mb free space, and therefore we're able to
	 * allocate the whole free space at once.
	 */
	if (!test_check_exists(cache, 128 * 1024 * 1024 - 768 * 1024,
			       1 * 1024 * 1024)) {
		test_msg("Expected region not marked as free\n");
		return -ENOENT;
	}

	if (cache->free_space_ctl->free_space != (1 * 1024 * 1024 + 8192)) {
		test_msg("Cache free space is not 1Mb + 8Kb\n");
		return -EINVAL;
	}

	offset = btrfs_find_space_for_alloc(cache,
					    0, 1 * 1024 * 1024, 0,
					    &max_extent_size);
	if (offset != (128 * 1024 * 1024 - 768 * 1024)) {
		test_msg("Failed to allocate 1Mb from space cache, returned offset is: %llu\n",
			 offset);
		return -EINVAL;
	}

	/* All that remains is a 8Kb free space region in a bitmap. Confirm. */
	ret = check_num_extents_and_bitmaps(cache, 1, 1);
	if (ret)
		return ret;

	if (cache->free_space_ctl->free_space != 8192) {
		test_msg("Cache free space is not 8Kb\n");
		return -EINVAL;
	}

	offset = btrfs_find_space_for_alloc(cache,
					    0, 8192, 0,
					    &max_extent_size);
	if (offset != (32 * 1024 * 1024)) {
		test_msg("Failed to allocate 8Kb from space cache, returned offset is: %llu\n",
			 offset);
		return -EINVAL;
	}

	ret = check_cache_empty(cache);
	if (ret)
		return ret;

	cache->free_space_ctl->op->use_bitmap = use_bitmap_op;
	__btrfs_remove_free_space_cache(cache->free_space_ctl);

	return 0;
}

int btrfs_test_free_space_cache(void)
{
	struct btrfs_block_group_cache *cache;
	struct btrfs_root *root = NULL;
	int ret = -ENOMEM;

	test_msg("Running btrfs free space cache tests\n");

	cache = init_test_block_group();
	if (!cache) {
		test_msg("Couldn't run the tests\n");
		return 0;
	}

	root = btrfs_alloc_dummy_root();
	if (IS_ERR(root)) {
		ret = PTR_ERR(root);
		goto out;
	}

	root->fs_info = btrfs_alloc_dummy_fs_info();
	if (!root->fs_info)
		goto out;

	root->fs_info->extent_root = root;
	cache->fs_info = root->fs_info;

	ret = test_extents(cache);
	if (ret)
		goto out;
	ret = test_bitmaps(cache);
	if (ret)
		goto out;
	ret = test_bitmaps_and_extents(cache);
	if (ret)
		goto out;

	ret = test_steal_space_from_bitmap_to_extent(cache);
out:
	__btrfs_remove_free_space_cache(cache->free_space_ctl);
	kfree(cache->free_space_ctl);
	kfree(cache);
	btrfs_free_dummy_root(root);
	test_msg("Free space cache tests finished\n");
	return ret;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * Copyright (C) 2013 Fusion IO.  All rights reserved.
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

#include "btrfs-tests.h"
#include "../ctree.h"
#include "../btrfs_inode.h"
#include "../disk-io.h"
#include "../extent_io.h"
#include "../volumes.h"

static void insert_extent(struct btrfs_root *root, u64 start, u64 len,
			  u64 ram_bytes, u64 offset, u64 disk_bytenr,
			  u64 disk_len, u32 type, u8 compression, int slot)
{
	struct btrfs_path path;
	struct btrfs_file_extent_item *fi;
	struct extent_buffer *leaf = root->node;
	struct btrfs_key key;
	u32 value_len = sizeof(struct btrfs_file_extent_item);

	if (type == BTRFS_FILE_EXTENT_INLINE)
		value_len += len;
	memset(&path, 0, sizeof(path));

	path.nodes[0] = leaf;
	path.slots[0] = slot;

	key.objectid = BTRFS_FIRST_FREE_OBJECTID;
	key.type = BTRFS_EXTENT_DATA_KEY;
	key.offset = start;

	setup_items_for_insert(root, &path, &key, &value_len, value_len,
			       value_len + sizeof(struct btrfs_item), 1);
	fi = btrfs_item_ptr(leaf, slot, struct btrfs_file_extent_item);
	btrfs_set_file_extent_generation(leaf, fi, 1);
	btrfs_set_file_extent_type(leaf, fi, type);
	btrfs_set_file_extent_disk_bytenr(leaf, fi, disk_bytenr);
	btrfs_set_file_extent_disk_num_bytes(leaf, fi, disk_len);
	btrfs_set_file_extent_offset(leaf, fi, offset);
	btrfs_set_file_extent_num_bytes(leaf, fi, len);
	btrfs_set_file_extent_ram_bytes(leaf, fi, ram_bytes);
	btrfs_set_file_extent_compression(leaf, fi, compression);
	btrfs_set_file_extent_encryption(leaf, fi, 0);
	btrfs_set_file_extent_other_encoding(leaf, fi, 0);
}

static void insert_inode_item_key(struct btrfs_root *root)
{
	struct btrfs_path path;
	struct extent_buffer *leaf = root->node;
	struct btrfs_key key;
	u32 value_len = 0;

	memset(&path, 0, sizeof(path));

	path.nodes[0] = leaf;
	path.slots[0] = 0;

	key.objectid = BTRFS_INODE_ITEM_KEY;
	key.type = BTRFS_INODE_ITEM_KEY;
	key.offset = 0;

	setup_items_for_insert(root, &path, &key, &value_len, value_len,
			       value_len + sizeof(struct btrfs_item), 1);
}

/*
 * Build the most complicated map of extents the earth has ever seen.  We want
 * this so we can test all of the corner cases of btrfs_get_extent.  Here is a
 * diagram of how the extents will look though this may not be possible we still
 * want to make sure everything acts normally (the last number is not inclusive)
 *
 * [0 - 5][5 -  6][6 - 10][10 - 4096][  4096 - 8192 ][8192 - 12288]
 * [hole ][inline][ hole ][ regular ][regular1 split][    hole    ]
 *
 * [ 12288 - 20480][20480 - 24576][  24576 - 28672  ][28672 - 36864][36864 - 45056]
 * [regular1 split][   prealloc1 ][prealloc1 written][   prealloc1 ][ compressed  ]
 *
 * [45056 - 49152][49152-53248][53248-61440][61440-65536][     65536+81920   ]
 * [ compressed1 ][  regular  ][compressed1][  regular  ][ hole but no extent]
 *
 * [81920-86016]
 * [  regular  ]
 */
static void setup_file_extents(struct btrfs_root *root)
{
	int slot = 0;
	u64 disk_bytenr = 1 * 1024 * 1024;
	u64 offset = 0;

	/* First we want a hole */
	insert_extent(root, offset, 5, 5, 0, 0, 0, BTRFS_FILE_EXTENT_REG, 0,
		      slot);
	slot++;
	offset += 5;

	/*
	 * Now we want an inline extent, I don't think this is possible but hey
	 * why not?  Also keep in mind if we have an inline extent it counts as
	 * the whole first page.  If we were to expand it we would have to cow
	 * and we wouldn't have an inline extent anymore.
	 */
	insert_extent(root, offset, 1, 1, 0, 0, 0, BTRFS_FILE_EXTENT_INLINE, 0,
		      slot);
	slot++;
	offset = 4096;

	/* Now another hole */
	insert_extent(root, offset, 4, 4, 0, 0, 0, BTRFS_FILE_EXTENT_REG, 0,
		      slot);
	slot++;
	offset += 4;

	/* Now for a regular extent */
	insert_extent(root, offset, 4095, 4095, 0, disk_bytenr, 4096,
		      BTRFS_FILE_EXTENT_REG, 0, slot);
	slot++;
	disk_bytenr += 4096;
	offset += 4095;

	/*
	 * Now for 3 extents that were split from a hole punch so we test
	 * offsets properly.
	 */
	insert_extent(root, offset, 4096, 16384, 0, disk_bytenr, 16384,
		      BTRFS_FILE_EXTENT_REG, 0, slot);
	slot++;
	offset += 4096;
	insert_extent(root, offset, 4096, 4096, 0, 0, 0, BTRFS_FILE_EXTENT_REG,
		      0, slot);
	slot++;
	offset += 4096;
	insert_extent(root, offset, 8192, 16384, 8192, disk_bytenr, 16384,
		      BTRFS_FILE_EXTENT_REG, 0, slot);
	slot++;
	offset += 8192;
	disk_bytenr += 16384;

	/* Now for a unwritten prealloc extent */
	insert_extent(root, offset, 4096, 4096, 0, disk_bytenr, 4096,
		      BTRFS_FILE_EXTENT_PREALLOC, 0, slot);
	slot++;
	offset += 4096;

	/*
	 * We want to jack up disk_bytenr a little more so the em stuff doesn't
	 * merge our records.
	 */
	disk_bytenr += 8192;

	/*
	 * Now for a partially written prealloc extent, basically the same as
	 * the hole punch example above.  Ram_bytes never changes when you mark
	 * extents written btw.
	 */
	insert_extent(root, offset, 4096, 16384, 0, disk_bytenr, 16384,
		      BTRFS_FILE_EXTENT_PREALLOC, 0, slot);
	slot++;
	offset += 4096;
	insert_extent(root, offset, 4096, 16384, 4096, disk_bytenr, 16384,
		      BTRFS_FILE_EXTENT_REG, 0, slot);
	slot++;
	offset += 4096;
	insert_extent(root, offset, 8192, 16384, 8192, disk_bytenr, 16384,
		      BTRFS_FILE_EXTENT_PREALLOC, 0, slot);
	slot++;
	offset += 8192;
	disk_bytenr += 16384;

	/* Now a normal compressed extent */
	insert_extent(root, offset, 8192, 8192, 0, disk_bytenr, 4096,
		      BTRFS_FILE_EXTENT_REG, BTRFS_COMPRESS_ZLIB, slot);
	slot++;
	offset += 8192;
	/* No merges */
	disk_bytenr += 8192;

	/* Now a split compressed extent */
	insert_extent(root, offset, 4096, 16384, 0, disk_bytenr, 4096,
		      BTRFS_FILE_EXTENT_REG, BTRFS_COMPRESS_ZLIB, slot);
	slot++;
	offset += 4096;
	insert_extent(root, offset, 4096, 4096, 0, disk_bytenr + 4096, 4096,
		      BTRFS_FILE_EXTENT_REG, 0, slot);
	slot++;
	offset += 4096;
	insert_extent(root, offset, 8192, 16384, 8192, disk_bytenr, 4096,
		      BTRFS_FILE_EXTENT_REG, BTRFS_COMPRESS_ZLIB, slot);
	slot++;
	offset += 8192;
	disk_bytenr += 8192;

	/* Now extents that have a hole but no hole extent */
	insert_extent(root, offset, 4096, 4096, 0, disk_bytenr, 4096,
		      BTRFS_FILE_EXTENT_REG, 0, slot);
	slot++;
	offset += 16384;
	disk_bytenr += 4096;
	insert_extent(root, offset, 4096, 4096, 0, disk_bytenr, 4096,
		      BTRFS_FILE_EXTENT_REG, 0, slot);
}

static unsigned long prealloc_only = 0;
static unsigned long compressed_only = 0;
static unsigned long vacancy_only = 0;

static noinline int test_btrfs_get_extent(void)
{
	struct inode *inode = NULL;
	struct btrfs_root *root = NULL;
	struct extent_map *em = NULL;
	u64 orig_start;
	u64 disk_bytenr;
	u64 offset;
	int ret = -ENOMEM;

	inode = btrfs_new_test_inode();
	if (!inode) {
		test_msg("Couldn't allocate inode\n");
		return ret;
	}

	inode->i_mode = S_IFREG;
	BTRFS_I(inode)->location.type = BTRFS_INODE_ITEM_KEY;
	BTRFS_I(inode)->location.objectid = BTRFS_FIRST_FREE_OBJECTID;
	BTRFS_I(inode)->location.offset = 0;

	root = btrfs_alloc_dummy_root();
	if (IS_ERR(root)) {
		test_msg("Couldn't allocate root\n");
		goto out;
	}

	/*
	 * We do this since btrfs_get_extent wants to assign em->bdev to
	 * root->fs_info->fs_devices->latest_bdev.
	 */
	root->fs_info = btrfs_alloc_dummy_fs_info();
	if (!root->fs_info) {
		test_msg("Couldn't allocate dummy fs info\n");
		goto out;
	}

	root->node = alloc_dummy_extent_buffer(NULL, 4096);
	if (!root->node) {
		test_msg("Couldn't allocate dummy buffer\n");
		goto out;
	}

	/*
	 * We will just free a dummy node if it's ref count is 2 so we need an
	 * extra ref so our searches don't accidently release our page.
	 */
	extent_buffer_get(root->node);
	btrfs_set_header_nritems(root->node, 0);
	btrfs_set_header_level(root->node, 0);
	ret = -EINVAL;

	/* First with no extents */
	BTRFS_I(inode)->root = root;
	em = btrfs_get_extent(inode, NULL, 0, 0, 4096, 0);
	if (IS_ERR(em)) {
		em = NULL;
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start != EXTENT_MAP_HOLE) {
		test_msg("Expected a hole, got %llu\n", em->block_start);
		goto out;
	}
	if (!test_bit(EXTENT_FLAG_VACANCY, &em->flags)) {
		test_msg("Vacancy flag wasn't set properly\n");
		goto out;
	}
	free_extent_map(em);
	btrfs_drop_extent_cache(inode, 0, (u64)-1, 0);

	/*
	 * All of the magic numbers are based on the mapping setup in
	 * setup_file_extents, so if you change anything there you need to
	 * update the comment and update the expected values below.
	 */
	setup_file_extents(root);

	em = btrfs_get_extent(inode, NULL, 0, 0, (u64)-1, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start != EXTENT_MAP_HOLE) {
		test_msg("Expected a hole, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != 0 || em->len != 5) {
		test_msg("Unexpected extent wanted start 0 len 5, got start "
			 "%llu len %llu\n", em->start, em->len);
		goto out;
	}
	if (em->flags != 0) {
		test_msg("Unexpected flags set, want 0 have %lu\n", em->flags);
		goto out;
	}
	offset = em->start + em->len;
	free_extent_map(em);

	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start != EXTENT_MAP_INLINE) {
		test_msg("Expected an inline, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 4091) {
		test_msg("Unexpected extent wanted start %llu len 1, got start "
			 "%llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != 0) {
		test_msg("Unexpected flags set, want 0 have %lu\n", em->flags);
		goto out;
	}
	/*
	 * We don't test anything else for inline since it doesn't get set
	 * unless we have a page for it to write into.  Maybe we should change
	 * this?
	 */
	offset = em->start + em->len;
	free_extent_map(em);

	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start != EXTENT_MAP_HOLE) {
		test_msg("Expected a hole, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 4) {
		test_msg("Unexpected extent wanted start %llu len 4, got start "
			 "%llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != 0) {
		test_msg("Unexpected flags set, want 0 have %lu\n", em->flags);
		goto out;
	}
	offset = em->start + em->len;
	free_extent_map(em);

	/* Regular extent */
	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start >= EXTENT_MAP_LAST_BYTE) {
		test_msg("Expected a real extent, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 4095) {
		test_msg("Unexpected extent wanted start %llu len 4095, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != 0) {
		test_msg("Unexpected flags set, want 0 have %lu\n", em->flags);
		goto out;
	}
	if (em->orig_start != em->start) {
		test_msg("Wrong orig offset, want %llu, have %llu\n", em->start,
			 em->orig_start);
		goto out;
	}
	offset = em->start + em->len;
	free_extent_map(em);

	/* The next 3 are split extents */
	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start >= EXTENT_MAP_LAST_BYTE) {
		test_msg("Expected a real extent, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 4096) {
		test_msg("Unexpected extent wanted start %llu len 4096, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != 0) {
		test_msg("Unexpected flags set, want 0 have %lu\n", em->flags);
		goto out;
	}
	if (em->orig_start != em->start) {
		test_msg("Wrong orig offset, want %llu, have %llu\n", em->start,
			 em->orig_start);
		goto out;
	}
	disk_bytenr = em->block_start;
	orig_start = em->start;
	offset = em->start + em->len;
	free_extent_map(em);

	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start != EXTENT_MAP_HOLE) {
		test_msg("Expected a hole, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 4096) {
		test_msg("Unexpected extent wanted start %llu len 4096, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != 0) {
		test_msg("Unexpected flags set, want 0 have %lu\n", em->flags);
		goto out;
	}
	offset = em->start + em->len;
	free_extent_map(em);

	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start >= EXTENT_MAP_LAST_BYTE) {
		test_msg("Expected a real extent, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 8192) {
		test_msg("Unexpected extent wanted start %llu len 8192, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != 0) {
		test_msg("Unexpected flags set, want 0 have %lu\n", em->flags);
		goto out;
	}
	if (em->orig_start != orig_start) {
		test_msg("Wrong orig offset, want %llu, have %llu\n",
			 orig_start, em->orig_start);
		goto out;
	}
	disk_bytenr += (em->start - orig_start);
	if (em->block_start != disk_bytenr) {
		test_msg("Wrong block start, want %llu, have %llu\n",
			 disk_bytenr, em->block_start);
		goto out;
	}
	offset = em->start + em->len;
	free_extent_map(em);

	/* Prealloc extent */
	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start >= EXTENT_MAP_LAST_BYTE) {
		test_msg("Expected a real extent, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 4096) {
		test_msg("Unexpected extent wanted start %llu len 4096, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != prealloc_only) {
		test_msg("Unexpected flags set, want %lu have %lu\n",
			 prealloc_only, em->flags);
		goto out;
	}
	if (em->orig_start != em->start) {
		test_msg("Wrong orig offset, want %llu, have %llu\n", em->start,
			 em->orig_start);
		goto out;
	}
	offset = em->start + em->len;
	free_extent_map(em);

	/* The next 3 are a half written prealloc extent */
	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start >= EXTENT_MAP_LAST_BYTE) {
		test_msg("Expected a real extent, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 4096) {
		test_msg("Unexpected extent wanted start %llu len 4096, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != prealloc_only) {
		test_msg("Unexpected flags set, want %lu have %lu\n",
			 prealloc_only, em->flags);
		goto out;
	}
	if (em->orig_start != em->start) {
		test_msg("Wrong orig offset, want %llu, have %llu\n", em->start,
			 em->orig_start);
		goto out;
	}
	disk_bytenr = em->block_start;
	orig_start = em->start;
	offset = em->start + em->len;
	free_extent_map(em);

	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start >= EXTENT_MAP_HOLE) {
		test_msg("Expected a real extent, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 4096) {
		test_msg("Unexpected extent wanted start %llu len 4096, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != 0) {
		test_msg("Unexpected flags set, want 0 have %lu\n", em->flags);
		goto out;
	}
	if (em->orig_start != orig_start) {
		test_msg("Unexpected orig offset, wanted %llu, have %llu\n",
			 orig_start, em->orig_start);
		goto out;
	}
	if (em->block_start != (disk_bytenr + (em->start - em->orig_start))) {
		test_msg("Unexpected block start, wanted %llu, have %llu\n",
			 disk_bytenr + (em->start - em->orig_start),
			 em->block_start);
		goto out;
	}
	offset = em->start + em->len;
	free_extent_map(em);

	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start >= EXTENT_MAP_LAST_BYTE) {
		test_msg("Expected a real extent, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 8192) {
		test_msg("Unexpected extent wanted start %llu len 8192, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != prealloc_only) {
		test_msg("Unexpected flags set, want %lu have %lu\n",
			 prealloc_only, em->flags);
		goto out;
	}
	if (em->orig_start != orig_start) {
		test_msg("Wrong orig offset, want %llu, have %llu\n", orig_start,
			 em->orig_start);
		goto out;
	}
	if (em->block_start != (disk_bytenr + (em->start - em->orig_start))) {
		test_msg("Unexpected block start, wanted %llu, have %llu\n",
			 disk_bytenr + (em->start - em->orig_start),
			 em->block_start);
		goto out;
	}
	offset = em->start + em->len;
	free_extent_map(em);

	/* Now for the compressed extent */
	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start >= EXTENT_MAP_LAST_BYTE) {
		test_msg("Expected a real extent, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 8192) {
		test_msg("Unexpected extent wanted start %llu len 8192, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != compressed_only) {
		test_msg("Unexpected flags set, want %lu have %lu\n",
			 compressed_only, em->flags);
		goto out;
	}
	if (em->orig_start != em->start) {
		test_msg("Wrong orig offset, want %llu, have %llu\n",
			 em->start, em->orig_start);
		goto out;
	}
	if (em->compress_type != BTRFS_COMPRESS_ZLIB) {
		test_msg("Unexpected compress type, wanted %d, got %d\n",
			 BTRFS_COMPRESS_ZLIB, em->compress_type);
		goto out;
	}
	offset = em->start + em->len;
	free_extent_map(em);

	/* Split compressed extent */
	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start >= EXTENT_MAP_LAST_BYTE) {
		test_msg("Expected a real extent, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 4096) {
		test_msg("Unexpected extent wanted start %llu len 4096, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != compressed_only) {
		test_msg("Unexpected flags set, want %lu have %lu\n",
			 compressed_only, em->flags);
		goto out;
	}
	if (em->orig_start != em->start) {
		test_msg("Wrong orig offset, want %llu, have %llu\n",
			 em->start, em->orig_start);
		goto out;
	}
	if (em->compress_type != BTRFS_COMPRESS_ZLIB) {
		test_msg("Unexpected compress type, wanted %d, got %d\n",
			 BTRFS_COMPRESS_ZLIB, em->compress_type);
		goto out;
	}
	disk_bytenr = em->block_start;
	orig_start = em->start;
	offset = em->start + em->len;
	free_extent_map(em);

	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start >= EXTENT_MAP_LAST_BYTE) {
		test_msg("Expected a real extent, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 4096) {
		test_msg("Unexpected extent wanted start %llu len 4096, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != 0) {
		test_msg("Unexpected flags set, want 0 have %lu\n", em->flags);
		goto out;
	}
	if (em->orig_start != em->start) {
		test_msg("Wrong orig offset, want %llu, have %llu\n", em->start,
			 em->orig_start);
		goto out;
	}
	offset = em->start + em->len;
	free_extent_map(em);

	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start != disk_bytenr) {
		test_msg("Block start does not match, want %llu got %llu\n",
			 disk_bytenr, em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 8192) {
		test_msg("Unexpected extent wanted start %llu len 8192, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != compressed_only) {
		test_msg("Unexpected flags set, want %lu have %lu\n",
			 compressed_only, em->flags);
		goto out;
	}
	if (em->orig_start != orig_start) {
		test_msg("Wrong orig offset, want %llu, have %llu\n",
			 em->start, orig_start);
		goto out;
	}
	if (em->compress_type != BTRFS_COMPRESS_ZLIB) {
		test_msg("Unexpected compress type, wanted %d, got %d\n",
			 BTRFS_COMPRESS_ZLIB, em->compress_type);
		goto out;
	}
	offset = em->start + em->len;
	free_extent_map(em);

	/* A hole between regular extents but no hole extent */
	em = btrfs_get_extent(inode, NULL, 0, offset + 6, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start >= EXTENT_MAP_LAST_BYTE) {
		test_msg("Expected a real extent, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 4096) {
		test_msg("Unexpected extent wanted start %llu len 4096, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != 0) {
		test_msg("Unexpected flags set, want 0 have %lu\n", em->flags);
		goto out;
	}
	if (em->orig_start != em->start) {
		test_msg("Wrong orig offset, want %llu, have %llu\n", em->start,
			 em->orig_start);
		goto out;
	}
	offset = em->start + em->len;
	free_extent_map(em);

	em = btrfs_get_extent(inode, NULL, 0, offset, 4096 * 1024, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start != EXTENT_MAP_HOLE) {
		test_msg("Expected a hole extent, got %llu\n", em->block_start);
		goto out;
	}
	/*
	 * Currently we just return a length that we requested rather than the
	 * length of the actual hole, if this changes we'll have to change this
	 * test.
	 */
	if (em->start != offset || em->len != 12288) {
		test_msg("Unexpected extent wanted start %llu len 12288, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != vacancy_only) {
		test_msg("Unexpected flags set, want %lu have %lu\n",
			 vacancy_only, em->flags);
		goto out;
	}
	if (em->orig_start != em->start) {
		test_msg("Wrong orig offset, want %llu, have %llu\n", em->start,
			 em->orig_start);
		goto out;
	}
	offset = em->start + em->len;
	free_extent_map(em);

	em = btrfs_get_extent(inode, NULL, 0, offset, 4096, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start >= EXTENT_MAP_LAST_BYTE) {
		test_msg("Expected a real extent, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != offset || em->len != 4096) {
		test_msg("Unexpected extent wanted start %llu len 4096, got "
			 "start %llu len %llu\n", offset, em->start, em->len);
		goto out;
	}
	if (em->flags != 0) {
		test_msg("Unexpected flags set, want 0 have %lu\n", em->flags);
		goto out;
	}
	if (em->orig_start != em->start) {
		test_msg("Wrong orig offset, want %llu, have %llu\n", em->start,
			 em->orig_start);
		goto out;
	}
	ret = 0;
out:
	if (!IS_ERR(em))
		free_extent_map(em);
	iput(inode);
	btrfs_free_dummy_root(root);
	return ret;
}

static int test_hole_first(void)
{
	struct inode *inode = NULL;
	struct btrfs_root *root = NULL;
	struct extent_map *em = NULL;
	int ret = -ENOMEM;

	inode = btrfs_new_test_inode();
	if (!inode) {
		test_msg("Couldn't allocate inode\n");
		return ret;
	}

	BTRFS_I(inode)->location.type = BTRFS_INODE_ITEM_KEY;
	BTRFS_I(inode)->location.objectid = BTRFS_FIRST_FREE_OBJECTID;
	BTRFS_I(inode)->location.offset = 0;

	root = btrfs_alloc_dummy_root();
	if (IS_ERR(root)) {
		test_msg("Couldn't allocate root\n");
		goto out;
	}

	root->fs_info = btrfs_alloc_dummy_fs_info();
	if (!root->fs_info) {
		test_msg("Couldn't allocate dummy fs info\n");
		goto out;
	}

	root->node = alloc_dummy_extent_buffer(NULL, 4096);
	if (!root->node) {
		test_msg("Couldn't allocate dummy buffer\n");
		goto out;
	}

	extent_buffer_get(root->node);
	btrfs_set_header_nritems(root->node, 0);
	btrfs_set_header_level(root->node, 0);
	BTRFS_I(inode)->root = root;
	ret = -EINVAL;

	/*
	 * Need a blank inode item here just so we don't confuse
	 * btrfs_get_extent.
	 */
	insert_inode_item_key(root);
	insert_extent(root, 4096, 4096, 4096, 0, 4096, 4096,
		      BTRFS_FILE_EXTENT_REG, 0, 1);
	em = btrfs_get_extent(inode, NULL, 0, 0, 8192, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start != EXTENT_MAP_HOLE) {
		test_msg("Expected a hole, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != 0 || em->len != 4096) {
		test_msg("Unexpected extent wanted start 0 len 4096, got start "
			 "%llu len %llu\n", em->start, em->len);
		goto out;
	}
	if (em->flags != vacancy_only) {
		test_msg("Wrong flags, wanted %lu, have %lu\n", vacancy_only,
			 em->flags);
		goto out;
	}
	free_extent_map(em);

	em = btrfs_get_extent(inode, NULL, 0, 4096, 8192, 0);
	if (IS_ERR(em)) {
		test_msg("Got an error when we shouldn't have\n");
		goto out;
	}
	if (em->block_start != 4096) {
		test_msg("Expected a real extent, got %llu\n", em->block_start);
		goto out;
	}
	if (em->start != 4096 || em->len != 4096) {
		test_msg("Unexpected extent wanted start 4096 len 4096, got "
			 "start %llu len %llu\n", em->start, em->len);
		goto out;
	}
	if (em->flags != 0) {
		test_msg("Unexpected flags set, wanted 0 got %lu\n",
			 em->flags);
		goto out;
	}
	ret = 0;
out:
	if (!IS_ERR(em))
		free_extent_map(em);
	iput(inode);
	btrfs_free_dummy_root(root);
	return ret;
}

static int test_extent_accounting(void)
{
	struct inode *inode = NULL;
	struct btrfs_root *root = NULL;
	int ret = -ENOMEM;

	inode = btrfs_new_test_inode();
	if (!inode) {
		test_msg("Couldn't allocate inode\n");
		return ret;
	}

	root = btrfs_alloc_dummy_root();
	if (IS_ERR(root)) {
		test_msg("Couldn't allocate root\n");
		goto out;
	}

	root->fs_info = btrfs_alloc_dummy_fs_info();
	if (!root->fs_info) {
		test_msg("Couldn't allocate dummy fs info\n");
		goto out;
	}

	BTRFS_I(inode)->root = root;
	btrfs_test_inode_set_ops(inode);

	/* [BTRFS_MAX_EXTENT_SIZE] */
	BTRFS_I(inode)->outstanding_extents++;
	ret = btrfs_set_extent_delalloc(inode, 0, BTRFS_MAX_EXTENT_SIZE - 1,
					NULL);
	if (ret) {
		test_msg("btrfs_set_extent_delalloc returned %d\n", ret);
		goto out;
	}
	if (BTRFS_I(inode)->outstanding_extents != 1) {
		ret = -EINVAL;
		test_msg("Miscount, wanted 1, got %u\n",
			 BTRFS_I(inode)->outstanding_extents);
		goto out;
	}

	/* [BTRFS_MAX_EXTENT_SIZE][4k] */
	BTRFS_I(inode)->outstanding_extents++;
	ret = btrfs_set_extent_delalloc(inode, BTRFS_MAX_EXTENT_SIZE,
					BTRFS_MAX_EXTENT_SIZE + 4095, NULL);
	if (ret) {
		test_msg("btrfs_set_extent_delalloc returned %d\n", ret);
		goto out;
	}
	if (BTRFS_I(inode)->outstanding_extents != 2) {
		ret = -EINVAL;
		test_msg("Miscount, wanted 2, got %u\n",
			 BTRFS_I(inode)->outstanding_extents);
		goto out;
	}

	/* [BTRFS_MAX_EXTENT_SIZE/2][4K HOLE][the rest] */
	ret = clear_extent_bit(&BTRFS_I(inode)->io_tree,
			       BTRFS_MAX_EXTENT_SIZE >> 1,
			       (BTRFS_MAX_EXTENT_SIZE >> 1) + 4095,
			       EXTENT_DELALLOC | EXTENT_DIRTY |
			       EXTENT_UPTODATE | EXTENT_DO_ACCOUNTING, 0, 0,
			       NULL, GFP_NOFS);
	if (ret) {
		test_msg("clear_extent_bit returned %d\n", ret);
		goto out;
	}
	if (BTRFS_I(inode)->outstanding_extents != 2) {
		ret = -EINVAL;
		test_msg("Miscount, wanted 2, got %u\n",
			 BTRFS_I(inode)->outstanding_extents);
		goto out;
	}

	/* [BTRFS_MAX_EXTENT_SIZE][4K] */
	BTRFS_I(inode)->outstanding_extents++;
	ret = btrfs_set_extent_delalloc(inode, BTRFS_MAX_EXTENT_SIZE >> 1,
					(BTRFS_MAX_EXTENT_SIZE >> 1) + 4095,
					NULL);
	if (ret) {
		test_msg("btrfs_set_extent_delalloc returned %d\n", ret);
		goto out;
	}
	if (BTRFS_I(inode)->outstanding_extents != 2) {
		ret = -EINVAL;
		test_msg("Miscount, wanted 2, got %u\n",
			 BTRFS_I(inode)->outstanding_extents);
		goto out;
	}

	/*
	 * [BTRFS_MAX_EXTENT_SIZE+4K][4K HOLE][BTRFS_MAX_EXTENT_SIZE+4K]
	 *
	 * I'm artificially adding 2 to outstanding_extents because in the
	 * buffered IO case we'd add things up as we go, but I don't feel like
	 * doing that here, this isn't the interesting case we want to test.
	 */
	BTRFS_I(inode)->outstanding_extents += 2;
	ret = btrfs_set_extent_delalloc(inode, BTRFS_MAX_EXTENT_SIZE + 8192,
					(BTRFS_MAX_EXTENT_SIZE << 1) + 12287,
					NULL);
	if (ret) {
		test_msg("btrfs_set_extent_delalloc returned %d\n", ret);
		goto out;
	}
	if (BTRFS_I(inode)->outstanding_extents != 4) {
		ret = -EINVAL;
		test_msg("Miscount, wanted 4, got %u\n",
			 BTRFS_I(inode)->outstanding_extents);
		goto out;
	}

	/* [BTRFS_MAX_EXTENT_SIZE+4k][4k][BTRFS_MAX_EXTENT_SIZE+4k] */
	BTRFS_I(inode)->outstanding_extents++;
	ret = btrfs_set_extent_delalloc(inode, BTRFS_MAX_EXTENT_SIZE+4096,
					BTRFS_MAX_EXTENT_SIZE+8191, NULL);
	if (ret) {
		test_msg("btrfs_set_extent_delalloc returned %d\n", ret);
		goto out;
	}
	if (BTRFS_I(inode)->outstanding_extents != 3) {
		ret = -EINVAL;
		test_msg("Miscount, wanted 3, got %u\n",
			 BTRFS_I(inode)->outstanding_extents);
		goto out;
	}

	/* [BTRFS_MAX_EXTENT_SIZE+4k][4K HOLE][BTRFS_MAX_EXTENT_SIZE+4k] */
	ret = clear_extent_bit(&BTRFS_I(inode)->io_tree,
			       BTRFS_MAX_EXTENT_SIZE+4096,
			       BTRFS_MAX_EXTENT_SIZE+8191,
			       EXTENT_DIRTY | EXTENT_DELALLOC |
			       EXTENT_DO_ACCOUNTING | EXTENT_UPTODATE, 0, 0,
			       NULL, GFP_NOFS);
	if (ret) {
		test_msg("clear_extent_bit returned %d\n", ret);
		goto out;
	}
	if (BTRFS_I(inode)->outstanding_extents != 4) {
		ret = -EINVAL;
		test_msg("Miscount, wanted 4, got %u\n",
			 BTRFS_I(inode)->outstanding_extents);
		goto out;
	}

	/*
	 * Refill the hole again just for good measure, because I thought it
	 * might fail and I'd rather satisfy my paranoia at this point.
	 */
	BTRFS_I(inode)->outstanding_extents++;
	ret = btrfs_set_extent_delalloc(inode, BTRFS_MAX_EXTENT_SIZE+4096,
					BTRFS_MAX_EXTENT_SIZE+8191, NULL);
	if (ret) {
		test_msg("btrfs_set_extent_delalloc returned %d\n", ret);
		goto out;
	}
	if (BTRFS_I(inode)->outstanding_extents != 3) {
		ret = -EINVAL;
		test_msg("Miscount, wanted 3, got %u\n",
			 BTRFS_I(inode)->outstanding_extents);
		goto out;
	}

	/* Empty */
	ret = clear_extent_bit(&BTRFS_I(inode)->io_tree, 0, (u64)-1,
			       EXTENT_DIRTY | EXTENT_DELALLOC |
			       EXTENT_DO_ACCOUNTING | EXTENT_UPTODATE, 0, 0,
			       NULL, GFP_NOFS);
	if (ret) {
		test_msg("clear_extent_bit returned %d\n", ret);
		goto out;
	}
	if (BTRFS_I(inode)->outstanding_extents) {
		ret = -EINVAL;
		test_msg("Miscount, wanted 0, got %u\n",
			 BTRFS_I(inode)->outstanding_extents);
		goto out;
	}
	ret = 0;
out:
	if (ret)
		clear_extent_bit(&BTRFS_I(inode)->io_tree, 0, (u64)-1,
				 EXTENT_DIRTY | EXTENT_DELALLOC |
				 EXTENT_DO_ACCOUNTING | EXTENT_UPTODATE, 0, 0,
				 NULL, GFP_NOFS);
	iput(inode);
	btrfs_free_dummy_root(root);
	return ret;
}

int btrfs_test_inodes(void)
{
	int ret;

	set_bit(EXTENT_FLAG_COMPRESSED, &compressed_only);
	set_bit(EXTENT_FLAG_VACANCY, &vacancy_only);
	set_bit(EXTENT_FLAG_PREALLOC, &prealloc_only);

	test_msg("Running btrfs_get_extent tests\n");
	ret = test_btrfs_get_extent();
	if (ret)
		return ret;
	test_msg("Running hole first btrfs_get_extent test\n");
	ret = test_hole_first();
	if (ret)
		return ret;
	test_msg("Running outstanding_extents tests\n");
	return test_extent_accounting();
}
                                                                                                                                                                                                                                                                              /*
 * Copyright (C) 2013 Facebook.  All rights reserved.
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

#include "btrfs-tests.h"
#include "../ctree.h"
#include "../transaction.h"
#include "../disk-io.h"
#include "../qgroup.h"
#include "../backref.h"

static void init_dummy_trans(struct btrfs_trans_handle *trans)
{
	memset(trans, 0, sizeof(*trans));
	trans->transid = 1;
	INIT_LIST_HEAD(&trans->qgroup_ref_list);
	trans->type = __TRANS_DUMMY;
}

static int insert_normal_tree_ref(struct btrfs_root *root, u64 bytenr,
				  u64 num_bytes, u64 parent, u64 root_objectid)
{
	struct btrfs_trans_handle trans;
	struct btrfs_extent_item *item;
	struct btrfs_extent_inline_ref *iref;
	struct btrfs_tree_block_info *block_info;
	struct btrfs_path *path;
	struct extent_buffer *leaf;
	struct btrfs_key ins;
	u32 size = sizeof(*item) + sizeof(*iref) + sizeof(*block_info);
	int ret;

	init_dummy_trans(&trans);

	ins.objectid = bytenr;
	ins.type = BTRFS_EXTENT_ITEM_KEY;
	ins.offset = num_bytes;

	path = btrfs_alloc_path();
	if (!path) {
		test_msg("Couldn't allocate path\n");
		return -ENOMEM;
	}

	path->leave_spinning = 1;
	ret = btrfs_insert_empty_item(&trans, root, path, &ins, size);
	if (ret) {
		test_msg("Couldn't insert ref %d\n", ret);
		btrfs_free_path(path);
		return ret;
	}

	leaf = path->nodes[0];
	item = btrfs_item_ptr(leaf, path->slots[0], struct btrfs_extent_item);
	btrfs_set_extent_refs(leaf, item, 1);
	btrfs_set_extent_generation(leaf, item, 1);
	btrfs_set_extent_flags(leaf, item, BTRFS_EXTENT_FLAG_TREE_BLOCK);
	block_info = (struct btrfs_tree_block_info *)(item + 1);
	btrfs_set_tree_block_level(leaf, block_info, 0);
	iref = (struct btrfs_extent_inline_ref *)(block_info + 1);
	if (parent > 0) {
		btrfs_set_extent_inline_ref_type(leaf, iref,
						 BTRFS_SHARED_BLOCK_REF_KEY);
		btrfs_set_extent_inline_ref_offset(leaf, iref, parent);
	} else {
		btrfs_set_extent_inline_ref_type(leaf, iref, BTRFS_TREE_BLOCK_REF_KEY);
		btrfs_set_extent_inline_ref_offset(leaf, iref, root_objectid);
	}
	btrfs_free_path(path);
	return 0;
}

static int add_tree_ref(struct btrfs_root *root, u64 bytenr, u64 num_bytes,
			u64 parent, u64 root_objectid)
{
	struct btrfs_trans_handle trans;
	struct btrfs_extent_item *item;
	struct btrfs_path *path;
	struct btrfs_key key;
	u64 refs;
	int ret;

	init_dummy_trans(&trans);

	key.objectid = bytenr;
	key.type = BTRFS_EXTENT_ITEM_KEY;
	key.offset = num_bytes;

	path = btrfs_alloc_path();
	if (!path) {
		test_msg("Couldn't allocate path\n");
		return -ENOMEM;
	}

	path->leave_spinning = 1;
	ret = btrfs_search_slot(&trans, root, &key, path, 0, 1);
	if (ret) {
		test_msg("Couldn't find extent ref\n");
		btrfs_free_path(path);
		return ret;
	}

	item = btrfs_item_ptr(path->nodes[0], path->slots[0],
			      struct btrfs_extent_item);
	refs = btrfs_extent_refs(path->nodes[0], item);
	btrfs_set_extent_refs(path->nodes[0], item, refs + 1);
	btrfs_release_path(path);

	key.objectid = bytenr;
	if (parent) {
		key.type = BTRFS_SHARED_BLOCK_REF_KEY;
		key.offset = parent;
	} else {
		key.type = BTRFS_TREE_BLOCK_REF_KEY;
		key.offset = root_objectid;
	}

	ret = btrfs_insert_empty_item(&trans, root, path, &key, 0);
	if (ret)
		test_msg("Failed to insert backref\n");
	btrfs_free_path(path);
	return ret;
}

static int remove_extent_item(struct btrfs_root *root, u64 bytenr,
			      u64 num_bytes)
{
	struct btrfs_trans_handle trans;
	struct btrfs_key key;
	struct btrfs_path *path;
	int ret;

	init_dummy_trans(&trans);

	key.objectid = bytenr;
	key.type = BTRFS_EXTENT_ITEM_KEY;
	key.offset = num_bytes;

	path = btrfs_alloc_path();
	if (!path) {
		test_msg("Couldn't allocate path\n");
		return -ENOMEM;
	}
	path->leave_spinning = 1;

	ret = btrfs_search_slot(&trans, root, &key, path, -1, 1);
	if (ret) {
		test_msg("Didn't find our key %d\n", ret);
		btrfs_free_path(path);
		return ret;
	}
	btrfs_del_item(&trans, root, path);
	btrfs_free_path(path);
	return 0;
}

static int remove_extent_ref(struct btrfs_root *root, u64 bytenr,
			     u64 num_bytes, u64 parent, u64 root_objectid)
{
	struct btrfs_trans_handle trans;
	struct btrfs_extent_item *item;
	struct btrfs_path *path;
	struct btrfs_key key;
	u64 refs;
	int ret;

	init_dummy_trans(&trans);

	key.objectid = bytenr;
	key.type = BTRFS_EXTENT_ITEM_KEY;
	key.offset = num_bytes;

	path = btrfs_alloc_path();
	if (!path) {
		test_msg("Couldn't allocate path\n");
		return -ENOMEM;
	}

	path->leave_spinning = 1;
	ret = btrfs_search_slot(&trans, root, &key, path, 0, 1);
	if (ret) {
		test_msg("Couldn't find extent ref\n");
		btrfs_free_path(path);
		return ret;
	}

	item = btrfs_item_ptr(path->nodes[0], path->slots[0],
			      struct btrfs_extent_item);
	refs = btrfs_extent_refs(path->nodes[0], item);
	btrfs_set_extent_refs(path->nodes[0], item, refs - 1);
	btrfs_release_path(path);

	key.objectid = bytenr;
	if (parent) {
		key.type = BTRFS_SHARED_BLOCK_REF_KEY;
		key.offset = parent;
	} else {
		key.type = BTRFS_TREE_BLOCK_REF_KEY;
		key.offset = root_objectid;
	}

	ret = btrfs_search_slot(&trans, root, &key, path, -1, 1);
	if (ret) {
		test_msg("Couldn't find backref %d\n", ret);
		btrfs_free_path(path);
		return ret;
	}
	btrfs_del_item(&trans, root, path);
	btrfs_free_path(path);
	return ret;
}

static int test_no_shared_qgroup(struct btrfs_root *root)
{
	struct btrfs_trans_handle trans;
	struct btrfs_fs_info *fs_info = root->fs_info;
	struct ulist *old_roots = NULL;
	struct ulist *new_roots = NULL;
	int ret;

	init_dummy_trans(&trans);

	test_msg("Qgroup basic add\n");
	ret = btrfs_create_qgroup(NULL, fs_info, 5);
	if (ret) {
		test_msg("Couldn't create a qgroup %d\n", ret);
		return ret;
	}

	/*
	 * Since the test trans doesn't havee the complicated delayed refs,
	 * we can only call btrfs_qgroup_account_extent() directly to test
	 * quota.
	 */
	ret = btrfs_find_all_roots(&trans, fs_info, 4096, 0, &old_roots);
	if (ret) {
		ulist_free(old_roots);
		test_msg("Couldn't find old roots: %d\n", ret);
		return ret;
	}

	ret = insert_normal_tree_ref(root, 4096, 4096, 0, 5);
	if (ret) {
		ulist_free(old_roots);
		return ret;
	}

	ret = btrfs_find_all_roots(&trans, fs_info, 4096, 0, &new_roots);
	if (ret) {
		ulist_free(old_roots);
		ulist_free(new_roots);
		test_msg("Couldn't find old roots: %d\n", ret);
		return ret;
	}

	ret = btrfs_qgroup_account_extent(&trans, fs_info, 4096, 4096,
					  old_roots, new_roots);
	if (ret) {
		test_msg("Couldn't account space for a qgroup %d\n", ret);
		return ret;
	}

	if (btrfs_verify_qgroup_counts(fs_info, 5, 4096, 4096)) {
		test_msg("Qgroup counts didn't match expected values\n");
		return -EINVAL;
	}
	old_roots = NULL;
	new_roots = NULL;

	ret = btrfs_find_all_roots(&trans, fs_info, 4096, 0, &old_roots);
	if (ret) {
		ulist_free(old_roots);
		test_msg("Couldn't find old roots: %d\n", ret);
		return ret;
	}

	ret = remove_extent_item(root, 4096, 4096);
	if (ret) {
		ulist_free(old_roots);
		return -EINVAL;
	}

	ret = btrfs_find_all_roots(&trans, fs_info, 4096, 0, &new_roots);
	if (ret) {
		ulist_free(old_roots);
		ulist_free(new_roots);
		test_msg("Couldn't find old roots: %d\n", ret);
		return ret;
	}

	ret = btrfs_qgroup_account_extent(&trans, fs_info, 4096, 4096,
					  old_roots, new_roots);
	if (ret) {
		test_msg("Couldn't account space for a qgroup %d\n", ret);
		return -EINVAL;
	}

	if (btrfs_verify_qgroup_counts(fs_info, 5, 0, 0)) {
		test_msg("Qgroup counts didn't match expected values\n");
		return -EINVAL;
	}

	return 0;
}

/*
 * Add a ref for two different roots to make sure the shared value comes out
 * right, also remove one of the roots and make sure the exclusive count is
 * adjusted properly.
 */
static int test_multiple_refs(struct btrfs_root *root)
{
	struct btrfs_trans_handle trans;
	struct btrfs_fs_info *fs_info = root->fs_info;
	struct ulist *old_roots = NULL;
	struct ulist *new_roots = NULL;
	int ret;

	init_dummy_trans(&trans);

	test_msg("Qgroup multiple refs test\n");

	/* We have 5 created already from the previous test */
	ret = btrfs_create_qgroup(NULL, fs_info, 256);
	if (ret) {
		test_msg("Couldn't create a qgroup %d\n", ret);
		return ret;
	}

	ret = btrfs_find_all_roots(&trans, fs_info, 4096, 0, &old_roots);
	if (ret) {
		ulist_free(old_roots);
		test_msg("Couldn't find old roots: %d\n", ret);
		return ret;
	}

	ret = insert_normal_tree_ref(root, 4096, 4096, 0, 5);
	if (ret) {
		ulist_free(old_roots);
		return ret;
	}

	ret = btrfs_find_all_roots(&trans, fs_info, 4096, 0, &new_roots);
	if (ret) {
		ulist_free(old_roots);
		ulist_free(new_roots);
		test_msg("Couldn't find old roots: %d\n", ret);
		return ret;
	}

	ret = btrfs_qgroup_account_extent(&trans, fs_info, 4096, 4096,
					  old_roots, new_roots);
	if (ret) {
		test_msg("Couldn't account space for a qgroup %d\n", ret);
		return ret;
	}

	if (btrfs_verify_qgroup_counts(fs_info, 5, 4096, 4096)) {
		test_msg("Qgroup counts didn't match expected values\n");
		return -EINVAL;
	}

	ret = btrfs_find_all_roots(&trans, fs_info, 4096, 0, &old_roots);
	if (ret) {
		ulist_free(old_roots);
		test_msg("Couldn't find old roots: %d\n", ret);
		return ret;
	}

	ret = add_tree_ref(root, 4096, 4096, 0, 256);
	if (ret) {
		ulist_free(old_roots);
		return ret;
	}

	ret = btrfs_find_all_roots(&trans, fs_info, 4096, 0, &new_roots);
	if (ret) {
		ulist_free(old_roots);
		ulist_free(new_roots);
		test_msg("Couldn't find old roots: %d\n", ret);
		return ret;
	}

	ret = btrfs_qgroup_account_extent(&trans, fs_info, 4096, 4096,
					  old_roots, new_roots);
	if (ret) {
		test_msg("Couldn't account space for a qgroup %d\n", ret);
		return ret;
	}

	if (btrfs_verify_qgroup_counts(fs_info, 5, 4096, 0)) {
		test_msg("Qgroup counts didn't match expected values\n");
		return -EINVAL;
	}

	if (btrfs_verify_qgroup_counts(fs_info, 256, 4096, 0)) {
		test_msg("Qgroup counts didn't match expected values\n");
		return -EINVAL;
	}

	ret = btrfs_find_all_roots(&trans, fs_info, 4096, 0, &old_roots);
	if (ret) {
		ulist_free(old_roots);
		test_msg("Couldn't find old roots: %d\n", ret);
		return ret;
	}

	ret = remove_extent_ref(root, 4096, 4096, 0, 256);
	if (ret) {
		ulist_free(old_roots);
		return ret;
	}

	ret = btrfs_find_all_roots(&trans, fs_info, 4096, 0, &new_roots);
	if (ret) {
		ulist_free(old_roots);
		ulist_free(new_roots);
		test_msg("Couldn't find old roots: %d\n", ret);
		return ret;
	}

	ret = btrfs_qgroup_account_extent(&trans, fs_info, 4096, 4096,
					  old_roots, new_roots);
	if (ret) {
		test_msg("Couldn't account space for a qgroup %d\n", ret);
		return ret;
	}

	if (btrfs_verify_qgroup_counts(fs_info, 256, 0, 0)) {
		test_msg("Qgroup counts didn't match expected values\n");
		return -EINVAL;
	}

	if (btrfs_verify_qgroup_counts(fs_info, 5, 4096, 4096)) {
		test_msg("Qgroup counts didn't match expected values\n");
		return -EINVAL;
	}

	return 0;
}

int btrfs_test_qgroups(void)
{
	struct btrfs_root *root;
	struct btrfs_root *tmp_root;
	int ret = 0;

	root = btrfs_alloc_dummy_root();
	if (IS_ERR(root)) {
		test_msg("Couldn't allocate root\n");
		return PTR_ERR(root);
	}

	root->fs_info = btrfs_alloc_dummy_fs_info();
	if (!root->fs_info) {
		test_msg("Couldn't allocate dummy fs info\n");
		ret = -ENOMEM;
		goto out;
	}
	/* We are using this root as our extent root */
	root->fs_info->extent_root = root;

	/*
	 * Some of the paths we test assume we have a filled out fs_info, so we
	 * just need to add the root in there so we don't panic.
	 */
	root->fs_info->tree_root = root;
	root->fs_info->quota_root = root;
	root->fs_info->quota_enabled = 1;

	/*
	 * Can't use bytenr 0, some things freak out
	 * *cough*backref walking code*cough*
	 */
	root->node = alloc_test_extent_buffer(root->fs_info, 4096);
	if (IS_ERR(root->node)) {
		test_msg("Couldn't allocate dummy buffer\n");
		ret = PTR_ERR(root->node);
		goto out;
	}
	btrfs_set_header_level(root->node, 0);
	btrfs_set_header_nritems(root->node, 0);
	root->alloc_bytenr += 8192;

	tmp_root = btrfs_alloc_dummy_root();
	if (IS_ERR(tmp_root)) {
		test_msg("Couldn't allocate a fs root\n");
		ret = PTR_ERR(tmp_root);
		goto out;
	}

	tmp_root->root_key.objectid = 5;
	root->fs_info->fs_root = tmp_root;
	ret = btrfs_insert_fs_root(root->fs_info, tmp_root);
	if (ret) {
		test_msg("Couldn't insert fs root %d\n", ret);
		goto out;
	}

	tmp_root = btrfs_alloc_dummy_root();
	if (IS_ERR(tmp_root)) {
		test_msg("Couldn't allocate a fs root\n");
		ret = PTR_ERR(tmp_root);
		goto out;
	}

	tmp_root->root_key.objectid = 256;
	ret = btrfs_insert_fs_root(root->fs_info, tmp_root);
	if (ret) {
		test_msg("Couldn't insert fs root %d\n", ret);
		goto out;
	}

	test_msg("Running qgroup tests\n");
	ret = test_no_shared_qgroup(root);
	if (ret)
		goto out;
	ret = test_multiple_refs(root);
out:
	btrfs_free_dummy_root(root);
	return ret;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
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

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <linux/pagemap.h>
#include <linux/blkdev.h>
#include <linux/uuid.h>
#include "ctree.h"
#include "disk-io.h"
#include "transaction.h"
#include "locking.h"
#include "tree-log.h"
#include "inode-map.h"
#include "volumes.h"
#include "dev-replace.h"
#include "qgroup.h"

#define BTRFS_ROOT_TRANS_TAG 0

static const unsigned int btrfs_blocked_trans_types[TRANS_STATE_MAX] = {
	[TRANS_STATE_RUNNING]		= 0U,
	[TRANS_STATE_BLOCKED]		= (__TRANS_USERSPACE |
					   __TRANS_START),
	[TRANS_STATE_COMMIT_START]	= (__TRANS_USERSPACE |
					   __TRANS_START |
					   __TRANS_ATTACH),
	[TRANS_STATE_COMMIT_DOING]	= (__TRANS_USERSPACE |
					   __TRANS_START |
					   __TRANS_ATTACH |
					   __TRANS_JOIN),
	[TRANS_STATE_UNBLOCKED]		= (__TRANS_USERSPACE |
					   __TRANS_START |
					   __TRANS_ATTACH |
					   __TRANS_JOIN |
					   __TRANS_JOIN_NOLOCK),
	[TRANS_STATE_COMPLETED]		= (__TRANS_USERSPACE |
					   __TRANS_START |
					   __TRANS_ATTACH |
					   __TRANS_JOIN |
					   __TRANS_JOIN_NOLOCK),
};

void btrfs_put_transaction(struct btrfs_transaction *transaction)
{
	WARN_ON(atomic_read(&transaction->use_count) == 0);
	if (atomic_dec_and_test(&transaction->use_count)) {
		BUG_ON(!list_empty(&transaction->list));
		WARN_ON(!RB_EMPTY_ROOT(&transaction->delayed_refs.href_root));
		if (transaction->delayed_refs.pending_csums)
			printk(KERN_ERR "pending csums is %llu\n",
			       transaction->delayed_refs.pending_csums);
		while (!list_empty(&transaction->pending_chunks)) {
			struct extent_map *em;

			em = list_first_entry(&transaction->pending_chunks,
					      struct extent_map, list);
			list_del_init(&em->list);
			free_extent_map(em);
		}
		kmem_cache_free(btrfs_transaction_cachep, transaction);
	}
}

static void clear_btree_io_tree(struct extent_io_tree *tree)
{
	spin_lock(&tree->lock);
	/*
	 * Do a single barrier for the waitqueue_active check here, the state
	 * of the waitqueue should not change once clear_btree_io_tree is
	 * called.
	 */
	smp_mb();
	while (!RB_EMPTY_ROOT(&tree->state)) {
		struct rb_node *node;
		struct extent_state *state;

		node = rb_first(&tree->state);
		state = rb_entry(node, struct extent_state, rb_node);
		rb_erase(&state->rb_node, &tree->state);
		RB_CLEAR_NODE(&state->rb_node);
		/*
		 * btree io trees aren't supposed to have tasks waiting for
		 * changes in the flags of extent states ever.
		 */
		ASSERT(!waitqueue_active(&state->wq));
		free_extent_state(state);

		cond_resched_lock(&tree->lock);
	}
	spin_unlock(&tree->lock);
}

static noinline void switch_commit_roots(struct btrfs_transaction *trans,
					 struct btrfs_fs_info *fs_info)
{
	struct btrfs_root *root, *tmp;

	down_write(&fs_info->commit_root_sem);
	list_for_each_entry_safe(root, tmp, &trans->switch_commits,
				 dirty_list) {
		list_del_init(&root->dirty_list);
		free_extent_buffer(root->commit_root);
		root->commit_root = btrfs_root_node(root);
		if (is_fstree(root->objectid))
			btrfs_unpin_free_ino(root);
		clear_btree_io_tree(&root->dirty_log_pages);
	}

	/* We can free old roots now. */
	spin_lock(&trans->dropped_roots_lock);
	while (!list_empty(&trans->dropped_roots)) {
		root = list_first_entry(&trans->dropped_roots,
					struct btrfs_root, root_list);
		list_del_init(&root->root_list);
		spin_unlock(&trans->dropped_roots_lock);
		btrfs_drop_and_free_fs_root(fs_info, root);
		spin_lock(&trans->dropped_roots_lock);
	}
	spin_unlock(&trans->dropped_roots_lock);
	up_write(&fs_info->commit_root_sem);
}

static inline void extwriter_counter_inc(struct btrfs_transaction *trans,
					 unsigned int type)
{
	if (type & TRANS_EXTWRITERS)
		atomic_inc(&trans->num_extwriters);
}

static inline void extwriter_counter_dec(struct btrfs_transaction *trans,
					 unsigned int type)
{
	if (type & TRANS_EXTWRITERS)
		atomic_dec(&trans->num_extwriters);
}

static inline void extwriter_counter_init(struct btrfs_transaction *trans,
					  unsigned int type)
{
	atomic_set(&trans->num_extwriters, ((type & TRANS_EXTWRITERS) ? 1 : 0));
}

static inline int extwriter_counter_read(struct btrfs_transaction *trans)
{
	return atomic_read(&trans->num_extwriters);
}

/*
 * either allocate a new transaction or hop into the existing one
 */
static noinline int join_transaction(struct btrfs_root *root, unsigned int type)
{
	struct btrfs_transaction *cur_trans;
	struct btrfs_fs_info *fs_info = root->fs_info;

	spin_lock(&fs_info->trans_lock);
loop:
	/* The file system has been taken offline. No new transactions. */
	if (test_bit(BTRFS_FS_STATE_ERROR, &fs_info->fs_state)) {
		spin_unlock(&fs_info->trans_lock);
		return -EROFS;
	}

	cur_trans = fs_info->running_transaction;
	if (cur_trans) {
		if (cur_trans->aborted) {
			spin_unlock(&fs_info->trans_lock);
			return cur_trans->aborted;
		}
		if (btrfs_blocked_trans_types[cur_trans->state] & type) {
			spin_unlock(&fs_info->trans_lock);
			return -EBUSY;
		}
		atomic_inc(&cur_trans->use_count);
		atomic_inc(&cur_trans->num_writers);
		extwriter_counter_inc(cur_trans, type);
		spin_unlock(&fs_info->trans_lock);
		return 0;
	}
	spin_unlock(&fs_info->trans_lock);

	/*
	 * If we are ATTACH, we just want to catch the current transaction,
	 * and commit it. If there is no transaction, just return ENOENT.
	 */
	if (type == TRANS_ATTACH)
		return -ENOENT;

	/*
	 * JOIN_NOLOCK only happens during the transaction commit, so
	 * it is impossible that ->running_transaction is NULL
	 */
	BUG_ON(type == TRANS_JOIN_NOLOCK);

	cur_trans = kmem_cache_alloc(btrfs_transaction_cachep, GFP_NOFS);
	if (!cur_trans)
		return -ENOMEM;

	spin_lock(&fs_info->trans_lock);
	if (fs_info->running_transaction) {
		/*
		 * someone started a transaction after we unlocked.  Make sure
		 * to redo the checks above
		 */
		kmem_cache_free(btrfs_transaction_cachep, cur_trans);
		goto loop;
	} else if (test_bit(BTRFS_FS_STATE_ERROR, &fs_info->fs_state)) {
		spin_unlock(&fs_info->trans_lock);
		kmem_cache_free(btrfs_transaction_cachep, cur_trans);
		return -EROFS;
	}

	atomic_set(&cur_trans->num_writers, 1);
	extwriter_counter_init(cur_trans, type);
	init_waitqueue_head(&cur_trans->writer_wait);
	init_waitqueue_head(&cur_trans->commit_wait);
	init_waitqueue_head(&cur_trans->pending_wait);
	cur_trans->state = TRANS_STATE_RUNNING;
	/*
	 * One for this trans handle, one so it will live on until we
	 * commit the transaction.
	 */
	atomic_set(&cur_trans->use_count, 2);
	atomic_set(&cur_trans->pending_ordered, 0);
	cur_trans->flags = 0;
	cur_trans->start_time = get_seconds();

	memset(&cur_trans->delayed_refs, 0, sizeof(cur_trans->delayed_refs));

	cur_trans->delayed_refs.href_root = RB_ROOT;
	cur_trans->delayed_refs.dirty_extent_root = RB_ROOT;
	atomic_set(&cur_trans->delayed_refs.num_entries, 0);

	/*
	 * although the tree mod log is per file system and not per transaction,
	 * the log must never go across transaction boundaries.
	 */
	smp_mb();
	if (!list_empty(&fs_info->tree_mod_seq_list))
		WARN(1, KERN_ERR "BTRFS: tree_mod_seq_list not empty when "
			"creating a fresh transaction\n");
	if (!RB_EMPTY_ROOT(&fs_info->tree_mod_log))
		WARN(1, KERN_ERR "BTRFS: tree_mod_log rb tree not empty when "
			"creating a fresh transaction\n");
	atomic64_set(&fs_info->tree_mod_seq, 0);

	spin_lock_init(&cur_trans->delayed_refs.lock);

	INIT_LIST_HEAD(&cur_trans->pending_snapshots);
	INIT_LIST_HEAD(&cur_trans->pending_chunks);
	INIT_LIST_HEAD(&cur_trans->switch_commits);
	INIT_LIST_HEAD(&cur_trans->dirty_bgs);
	INIT_LIST_HEAD(&cur_trans->io_bgs);
	INIT_LIST_HEAD(&cur_trans->dropped_roots);
	mutex_init(&cur_trans->cache_write_mutex);
	cur_trans->num_dirty_bgs = 0;
	spin_lock_init(&cur_trans->dirty_bgs_lock);
	INIT_LIST_HEAD(&cur_trans->deleted_bgs);
	spin_lock_init(&cur_trans->dropped_roots_lock);
	list_add_tail(&cur_trans->list, &fs_info->trans_list);
	extent_io_tree_init(&cur_trans->dirty_pages,
			     fs_info->btree_inode->i_mapping);
	fs_info->generation++;
	cur_trans->transid = fs_info->generation;
	fs_info->running_transaction = cur_trans;
	cur_trans->aborted = 0;
	spin_unlock(&fs_info->trans_lock);

	return 0;
}

/*
 * this does all the record keeping required to make sure that a reference
 * counted root is properly recorded in a given transaction.  This is required
 * to make sure the old root from before we joined the transaction is deleted
 * when the transaction commits
 */
static int record_root_in_trans(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root)
{
	if (test_bit(BTRFS_ROOT_REF_COWS, &root->state) &&
	    root->last_trans < trans->transid) {
		WARN_ON(root == root->fs_info->extent_root);
		WARN_ON(root->commit_root != root->node);

		/*
		 * see below for IN_TRANS_SETUP usage rules
		 * we have the reloc mutex held now, so there
		 * is only one writer in this function
		 */
		set_bit(BTRFS_ROOT_IN_TRANS_SETUP, &root->state);

		/* make sure readers find IN_TRANS_SETUP before
		 * they find our root->last_trans update
		 */
		smp_wmb();

		spin_lock(&root->fs_info->fs_roots_radix_lock);
		if (root->last_trans == trans->transid) {
			spin_unlock(&root->fs_info->fs_roots_radix_lock);
			return 0;
		}
		radix_tree_tag_set(&root->fs_info->fs_roots_radix,
			   (unsigned long)root->root_key.objectid,
			   BTRFS_ROOT_TRANS_TAG);
		spin_unlock(&root->fs_info->fs_roots_radix_lock);
		root->last_trans = trans->transid;

		/* this is pretty tricky.  We don't want to
		 * take the relocation lock in btrfs_record_root_in_trans
		 * unless we're really doing the first setup for this root in
		 * this transaction.
		 *
		 * Normally we'd use root->last_trans as a flag to decide
		 * if we want to take the expensive mutex.
		 *
		 * But, we have to set root->last_trans before we
		 * init the relocation root, otherwise, we trip over warnings
		 * in ctree.c.  The solution used here is to flag ourselves
		 * with root IN_TRANS_SETUP.  When this is 1, we're still
		 * fixing up the reloc trees and everyone must wait.
		 *
		 * When this is zero, they can trust root->last_trans and fly
		 * through btrfs_record_root_in_trans without having to take the
		 * lock.  smp_wmb() makes sure that all the writes above are
		 * done before we pop in the zero below
		 */
		btrfs_init_reloc_root(trans, root);
		smp_mb__before_atomic();
		clear_bit(BTRFS_ROOT_IN_TRANS_SETUP, &root->state);
	}
	return 0;
}


void btrfs_add_dropped_root(struct btrfs_trans_handle *trans,
			    struct btrfs_root *root)
{
	struct btrfs_transaction *cur_trans = trans->transaction;

	/* Add ourselves to the transaction dropped list */
	spin_lock(&cur_trans->dropped_roots_lock);
	list_add_tail(&root->root_list, &cur_trans->dropped_roots);
	spin_unlock(&cur_trans->dropped_roots_lock);

	/* Make sure we don't try to update the root at commit time */
	spin_lock(&root->fs_info->fs_roots_radix_lock);
	radix_tree_tag_clear(&root->fs_info->fs_roots_radix,
			     (unsigned long)root->root_key.objectid,
			     BTRFS_ROOT_TRANS_TAG);
	spin_unlock(&root->fs_info->fs_roots_radix_lock);
}

int btrfs_record_root_in_trans(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root)
{
	if (!test_bit(BTRFS_ROOT_REF_COWS, &root->state))
		return 0;

	/*
	 * see record_root_in_trans for comments about IN_TRANS_SETUP usage
	 * and barriers
	 */
	smp_rmb();
	if (root->last_trans == trans->transid &&
	    !test_bit(BTRFS_ROOT_IN_TRANS_SETUP, &root->state))
		return 0;

	mutex_lock(&root->fs_info->reloc_mutex);
	record_root_in_trans(trans, root);
	mutex_unlock(&root->fs_info->reloc_mutex);

	return 0;
}

static inline int is_transaction_blocked(struct btrfs_transaction *trans)
{
	return (trans->state >= TRANS_STATE_BLOCKED &&
		trans->state < TRANS_STATE_UNBLOCKED &&
		!trans->aborted);
}

/* wait for commit against the current transaction to become unblocked
 * when this is done, it is safe to start a new transaction, but the current
 * transaction might not be fully on disk.
 */
static void wait_current_trans(struct btrfs_root *root)
{
	struct btrfs_transaction *cur_trans;

	spin_lock(&root->fs_info->trans_lock);
	cur_trans = root->fs_info->running_transaction;
	if (cur_trans && is_transaction_blocked(cur_trans)) {
		atomic_inc(&cur_trans->use_count);
		spin_unlock(&root->fs_info->trans_lock);

		wait_event(root->fs_info->transaction_wait,
			   cur_trans->state >= TRANS_STATE_UNBLOCKED ||
			   cur_trans->aborted);
		btrfs_put_transaction(cur_trans);
	} else {
		spin_unlock(&root->fs_info->trans_lock);
	}
}

static int may_wait_transaction(struct btrfs_root *root, int type)
{
	if (root->fs_info->log_root_recovering)
		return 0;

	if (type == TRANS_USERSPACE)
		return 1;

	if (type == TRANS_START &&
	    !atomic_read(&root->fs_info->open_ioctl_trans))
		return 1;

	return 0;
}

static inline bool need_reserve_reloc_root(struct btrfs_root *root)
{
	if (!root->fs_info->reloc_ctl ||
	    !test_bit(BTRFS_ROOT_REF_COWS, &root->state) ||
	    root->root_key.objectid == BTRFS_TREE_RELOC_OBJECTID ||
	    root->reloc_root)
		return false;

	return true;
}

static struct btrfs_trans_handle *
start_transaction(struct btrfs_root *root, unsigned int num_items,
		  unsigned int type, enum btrfs_reserve_flush_enum flush)
{
	struct btrfs_trans_handle *h;
	struct btrfs_transaction *cur_trans;
	u64 num_bytes = 0;
	u64 qgroup_reserved = 0;
	bool reloc_reserved = false;
	int ret;

	/* Send isn't supposed to start transactions. */
	ASSERT(current->journal_info != BTRFS_SEND_TRANS_STUB);

	if (test_bit(BTRFS_FS_STATE_ERROR, &root->fs_info->fs_state))
		return ERR_PTR(-EROFS);

	if (current->journal_info) {
		WARN_ON(type & TRANS_EXTWRITERS);
		h = current->journal_info;
		h->use_count++;
		WARN_ON(h->use_count > 2);
		h->orig_rsv = h->block_rsv;
		h->block_rsv = NULL;
		goto got_it;
	}

	/*
	 * Do the reservation before we join the transaction so we can do all
	 * the appropriate flushing if need be.
	 */
	if (num_items > 0 && root != root->fs_info->chunk_root) {
		qgroup_reserved = num_items * root->nodesize;
		ret = btrfs_qgroup_reserve_meta(root, qgroup_reserved);
		if (ret)
			return ERR_PTR(ret);

		num_bytes = btrfs_calc_trans_metadata_size(root, num_items);
		/*
		 * Do the reservation for the relocation root creation
		 */
		if (need_reserve_reloc_root(root)) {
			num_bytes += root->nodesize;
			reloc_reserved = true;
		}

		ret = btrfs_block_rsv_add(root,
					  &root->fs_info->trans_block_rsv,
					  num_bytes, flush);
		if (ret)
			goto reserve_fail;
	}
again:
	h = kmem_cache_zalloc(btrfs_trans_handle_cachep, GFP_NOFS);
	if (!h) {
		ret = -ENOMEM;
		goto alloc_fail;
	}

	/*
	 * If we are JOIN_NOLOCK we're already committing a transaction and
	 * waiting on this guy, so we don't need to do the sb_start_intwrite
	 * because we're already holding a ref.  We need this because we could
	 * have raced in and did an fsync() on a file which can kick a commit
	 * and then we deadlock with somebody doing a freeze.
	 *
	 * If we are ATTACH, it means we just want to catch the current
	 * transaction and commit it, so we needn't do sb_start_intwrite(). 
	 */
	if (type & __TRANS_FREEZABLE)
		sb_start_intwrite(root->fs_info->sb);

	if (may_wait_transaction(root, type))
		wait_current_trans(root);

	do {
		ret = join_transaction(root, type);
		if (ret == -EBUSY) {
			wait_current_trans(root);
			if (unlikely(type == TRANS_ATTACH))
				ret = -ENOENT;
		}
	} while (ret == -EBUSY);

	if (ret < 0) {
		/* We must get the transaction if we are JOIN_NOLOCK. */
		BUG_ON(type == TRANS_JOIN_NOLOCK);
		goto join_fail;
	}

	cur_trans = root->fs_info->running_transaction;

	h->transid = cur_trans->transid;
	h->transaction = cur_trans;
	h->root = root;
	h->use_count = 1;

	h->type = type;
	h->can_flush_pending_bgs = true;
	INIT_LIST_HEAD(&h->qgroup_ref_list);
	INIT_LIST_HEAD(&h->new_bgs);

	smp_mb();
	if (cur_trans->state >= TRANS_STATE_BLOCKED &&
	    may_wait_transaction(root, type)) {
		current->journal_info = h;
		btrfs_commit_transaction(h, root);
		goto again;
	}

	if (num_bytes) {
		trace_btrfs_spa