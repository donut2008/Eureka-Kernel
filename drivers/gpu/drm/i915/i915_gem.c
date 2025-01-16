R_EXCLCREAT_WORD2;

		status = nfsd4_encode_bitmap(xdr, supp[0], supp[1], supp[2]);
		if (status)
			goto out;
	}

	if (bmval2 & FATTR4_WORD2_SECURITY_LABEL) {
		status = nfsd4_encode_security_label(xdr, rqstp, context,
								contextlen);
		if (status)
			goto out;
	}

	attrlen = htonl(xdr->buf->len - attrlen_offset - 4);
	write_bytes_to_xdr_buf(xdr->buf, attrlen_offset, &attrlen, 4);
	status = nfs_ok;

out:
#ifdef CONFIG_NFSD_V4_SECURITY_LABEL
	if (context)
		security_release_secctx(context, contextlen);
#endif /* CONFIG_NFSD_V4_SECURITY_LABEL */
	kfree(acl);
	if (tempfh) {
		fh_put(tempfh);
		kfree(tempfh);
	}
	if (status)
		xdr_truncate_encode(xdr, starting_len);
	return status;
out_nfserr:
	status = nfserrno(err);
	goto out;
out_resource:
	status = nfserr_resource;
	goto out;
}

static void svcxdr_init_encode_from_buffer(struct xdr_stream *xdr,
				struct xdr_buf *buf, __be32 *p, int bytes)
{
	xdr->scratch.iov_len = 0;
	memset(buf, 0, sizeof(struct xdr_buf));
	buf->head[0].iov_base = p;
	buf->head[0].iov_len = 0;
	buf->len = 0;
	xdr->buf = buf;
	xdr->iov = buf->head;
	xdr->p = p;
	xdr->end = (void *)p + bytes;
	buf->buflen = bytes;
}

__be32 nfsd4_encode_fattr_to_buf(__be32 **p, int words,
			struct svc_fh *fhp, struct svc_export *exp,
			struct dentry *dentry, u32 *bmval,
			struct svc_rqst *rqstp, int ignore_crossmnt)
{
	struct xdr_buf dummy;
	struct xdr_stream xdr;
	__be32 ret;

	svcxdr_init_encode_from_buffer(&xdr, &dummy, *p, words << 2);
	ret = nfsd4_encode_fattr(&xdr, fhp, exp, dentry, bmval, rqstp,
							ignore_crossmnt);
	*p = xdr.p;
	return ret;
}

static inline int attributes_need_mount(u32 *bmval)
{
	if (bmval[0] & ~(FATTR4_WORD0_RDATTR_ERROR | FATTR4_WORD0_LEASE_TIME))
		return 1;
	if (bmval[1] & ~FATTR4_WORD1_MOUNTED_ON_FILEID)
		return 1;
	return 0;
}

static __be32
nfsd4_encode_dirent_fattr(struct xdr_stream *xdr, struct nfsd4_readdir *cd,
			const char *name, int namlen)
{
	struct svc_export *exp = cd->rd_fhp->fh_export;
	struct dentry *dentry;
	__be32 nfserr;
	int ignore_crossmnt = 0;

	dentry = lookup_one_len(name, cd->rd_fhp->fh_dentry, namlen);
	if (IS_ERR(dentry))
		return nfserrno(PTR_ERR(dentry));
	if (d_really_is_negative(dentry)) {
		/*
		 * nfsd_buffered_readdir drops the i_mutex between
		 * readdir and calling this callback, leaving a window
		 * where this directory entry could have gone away.
		 */
		dput(dentry);
		return nfserr_noent;
	}

	exp_get(exp);
	/*
	 * In the case of a mountpoint, the client may be asking for
	 * attributes that are only properties of the underlying filesystem
	 * as opposed to the cross-mounted file system. In such a case,
	 * we will not follow the cross mount and will fill the attribtutes
	 * directly from the mountpoint dentry.
	 */
	if (nfsd_mountpoint(dentry, exp)) {
		int err;

		if (!(exp->ex_flags & NFSEXP_V4ROOT)
				&& !attributes_need_mount(cd->rd_bmval)) {
			ignore_crossmnt = 1;
			goto out_encode;
		}
		/*
		 * Why the heck aren't we just using nfsd_lookup??
		 * Different "."/".." handling?  Something else?
		 * At least, add a comment here to explain....
		 */
		err = nfsd_cross_mnt(cd->rd_rqstp, &dentry, &exp);
		if (err) {
			nfserr = nfserrno(err);
			goto out_put;
		}
		nfserr = check_nfsd_access(exp, cd->rd_rqstp);
		if (nfserr)
			goto out_put;

	}
out_encode:
	nfserr = nfsd4_encode_fattr(xdr, NULL, exp, dentry, cd->rd_bmval,
					cd->rd_rqstp, ignore_crossmnt);
out_put:
	dput(dentry);
	exp_put(exp);
	return nfserr;
}

static __be32 *
nfsd4_encode_rdattr_error(struct xdr_stream *xdr, __be32 nfserr)
{
	__be32 *p;

	p = xdr_reserve_space(xdr, 20);
	if (!p)
		return NULL;
	*p++ = htonl(2);
	*p++ = htonl(FATTR4_WORD0_RDATTR_ERROR); /* bmval0 */
	*p++ = htonl(0);			 /* bmval1 */

	*p++ = htonl(4);     /* attribute length */
	*p++ = nfserr;       /* no htonl */
	return p;
}

static int
nfsd4_encode_dirent(void *ccdv, const char *name, int namlen,
		    loff_t offset, u64 ino, unsigned int d_type)
{
	struct readdir_cd *ccd = ccdv;
	struct nfsd4_readdir *cd = container_of(ccd, struct nfsd4_readdir, common);
	struct xdr_stream *xdr = cd->xdr;
	int start_offset = xdr->buf->len;
	int cookie_offset;
	u32 name_and_cookie;
	int entry_bytes;
	__be32 nfserr = nfserr_toosmall;
	__be64 wire_offset;
	__be32 *p;

	/* In nfsv4, "." and ".." never make it onto the wire.. */
	if (name && isdotent(name, namlen)) {
		cd->common.err = nfs_ok;
		return 0;
	}

	if (cd->cookie_offset) {
		wire_offset = cpu_to_be64(offset);
		write_bytes_to_xdr_buf(xdr->buf, cd->cookie_offset,
							&wire_offset, 8);
	}

	p = xdr_reserve_space(xdr, 4);
	if (!p)
		goto fail;
	*p++ = xdr_one;                             /* mark entry present */
	cookie_offset = xdr->buf->len;
	p = xdr_reserve_space(xdr, 3*4 + namlen);
	if (!p)
		goto fail;
	p = xdr_encode_hyper(p, NFS_OFFSET_MAX);    /* offset of next entry */
	p = xdr_encode_array(p, name, namlen);      /* name length & name */

	nfserr = nfsd4_encode_dirent_fattr(xdr, cd, name, namlen);
	switch (nfserr) {
	case nfs_ok:
		break;
	case nfserr_resource:
		nfserr = nfserr_toosmall;
		goto fail;
	case nfserr_noent:
		xdr_truncate_encode(xdr, start_offset);
		goto skip_entry;
	case nfserr_jukebox:
		/*
		 * The pseudoroot should only display dentries that lead to
		 * exports. If we get EJUKEBOX here, then we can't tell whether
		 * this entry should be included. Just fail the whole READDIR
		 * with NFS4ERR_DELAY in that case, and hope that the situation
		 * will resolve itself by the client's next attempt.
		 */
		if (cd->rd_fhp->fh_export->ex_flags & NFSEXP_V4ROOT)
			goto fail;
		/* fallthrough */
	default:
		/*
		 * If the client requested the RDATTR_ERROR attribute,
		 * we stuff the error code into this attribute
		 * and continue.  If this attribute was not requested,
		 * then in accordance with the spec, we fail the
		 * entire READDIR operation(!)
		 */
		if (!(cd->rd_bmval[0] & FATTR4_WORD0_RDATTR_ERROR))
			goto fail;
		p = nfsd4_encode_rdattr_error(xdr, nfserr);
		if (p == NULL) {
			nfserr = nfserr_toosmall;
			goto fail;
		}
	}
	nfserr = nfserr_toosmall;
	entry_bytes = xdr->buf->len - start_offset;
	if (entry_bytes > cd->rd_maxcount)
		goto fail;
	cd->rd_maxcount -= entry_bytes;
	/*
	 * RFC 3530 14.2.24 describes rd_dircount as only a "hint", and
	 * notes that it could be zero. If it is zero, then the server
	 * should enforce only the rd_maxcount value.
	 */
	if (cd->rd_dircount) {
		name_and_cookie = 4 + 4 * XDR_QUADLEN(namlen) + 8;
		if (name_and_cookie > cd->rd_dircount && cd->cookie_offset)
			goto fail;
		cd->rd_dircount -= min(cd->rd_dircount, name_and_cookie);
		if (!cd->rd_dircount)
			cd->rd_maxcount = 0;
	}

	cd->cookie_offset = cookie_offset;
skip_entry:
	cd->common.err = nfs_ok;
	return 0;
fail:
	xdr_truncate_encode(xdr, start_offset);
	cd->common.err = nfserr;
	return -EINVAL;
}

static __be32
nfsd4_encode_stateid(struct xdr_stream *xdr, stateid_t *sid)
{
	__be32 *p;

	p = xdr_reserve_space(xdr, sizeof(stateid_t));
	if (!p)
		return nfserr_resource;
	*p++ = cpu_to_be32(sid->si_generation);
	p = xdr_encode_opaque_fixed(p, &sid->si_opaque,
					sizeof(stateid_opaque_t));
	return 0;
}

static __be32
nfsd4_encode_access(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_access *access)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (!nfserr) {
		p = xdr_reserve_space(xdr, 8);
		if (!p)
			return nfserr_resource;
		*p++ = cpu_to_be32(access->ac_supported);
		*p++ = cpu_to_be32(access->ac_resp_access);
	}
	return nfserr;
}

static __be32 nfsd4_encode_bind_conn_to_session(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_bind_conn_to_session *bcts)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (!nfserr) {
		p = xdr_reserve_space(xdr, NFS4_MAX_SESSIONID_LEN + 8);
		if (!p)
			return nfserr_resource;
		p = xdr_encode_opaque_fixed(p, bcts->sessionid.data,
						NFS4_MAX_SESSIONID_LEN);
		*p++ = cpu_to_be32(bcts->dir);
		/* Sorry, we do not yet support RDMA over 4.1: */
		*p++ = cpu_to_be32(0);
	}
	return nfserr;
}

static __be32
nfsd4_encode_close(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_close *close)
{
	struct xdr_stream *xdr = &resp->xdr;

	if (!nfserr)
		nfserr = nfsd4_encode_stateid(xdr, &close->cl_stateid);

	return nfserr;
}


static __be32
nfsd4_encode_commit(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_commit *commit)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (!nfserr) {
		p = xdr_reserve_space(xdr, NFS4_VERIFIER_SIZE);
		if (!p)
			return nfserr_resource;
		p = xdr_encode_opaque_fixed(p, commit->co_verf.data,
						NFS4_VERIFIER_SIZE);
	}
	return nfserr;
}

static __be32
nfsd4_encode_create(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_create *create)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (!nfserr) {
		p = xdr_reserve_space(xdr, 20);
		if (!p)
			return nfserr_resource;
		encode_cinfo(p, &create->cr_cinfo);
		nfserr = nfsd4_encode_bitmap(xdr, create->cr_bmval[0],
				create->cr_bmval[1], create->cr_bmval[2]);
	}
	return nfserr;
}

static __be32
nfsd4_encode_getattr(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_getattr *getattr)
{
	struct svc_fh *fhp = getattr->ga_fhp;
	struct xdr_stream *xdr = &resp->xdr;

	if (nfserr)
		return nfserr;

	nfserr = nfsd4_encode_fattr(xdr, fhp, fhp->fh_export, fhp->fh_dentry,
				    getattr->ga_bmval,
				    resp->rqstp, 0);
	return nfserr;
}

static __be32
nfsd4_encode_getfh(struct nfsd4_compoundres *resp, __be32 nfserr, struct svc_fh **fhpp)
{
	struct xdr_stream *xdr = &resp->xdr;
	struct svc_fh *fhp = *fhpp;
	unsigned int len;
	__be32 *p;

	if (!nfserr) {
		len = fhp->fh_handle.fh_size;
		p = xdr_reserve_space(xdr, len + 4);
		if (!p)
			return nfserr_resource;
		p = xdr_encode_opaque(p, &fhp->fh_handle.fh_base, len);
	}
	return nfserr;
}

/*
* Including all fields other than the name, a LOCK4denied structure requires
*   8(clientid) + 4(namelen) + 8(offset) + 8(length) + 4(type) = 32 bytes.
*/
static __be32
nfsd4_encode_lock_denied(struct xdr_stream *xdr, struct nfsd4_lock_denied *ld)
{
	struct xdr_netobj *conf = &ld->ld_owner;
	__be32 *p;

again:
	p = xdr_reserve_space(xdr, 32 + XDR_LEN(conf->len));
	if (!p) {
		/*
		 * Don't fail to return the result just because we can't
		 * return the conflicting open:
		 */
		if (conf->len) {
			kfree(conf->data);
			conf->len = 0;
			conf->data = NULL;
			goto again;
		}
		return nfserr_resource;
	}
	p = xdr_encode_hyper(p, ld->ld_start);
	p = xdr_encode_hyper(p, ld->ld_length);
	*p++ = cpu_to_be32(ld->ld_type);
	if (conf->len) {
		p = xdr_encode_opaque_fixed(p, &ld->ld_clientid, 8);
		p = xdr_encode_opaque(p, conf->data, conf->len);
		kfree(conf->data);
	}  else {  /* non - nfsv4 lock in conflict, no clientid nor owner */
		p = xdr_encode_hyper(p, (u64)0); /* clientid */
		*p++ = cpu_to_be32(0); /* length of owner name */
	}
	return nfserr_denied;
}

static __be32
nfsd4_encode_lock(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_lock *lock)
{
	struct xdr_stream *xdr = &resp->xdr;

	if (!nfserr)
		nfserr = nfsd4_encode_stateid(xdr, &lock->lk_resp_stateid);
	else if (nfserr == nfserr_denied)
		nfserr = nfsd4_encode_lock_denied(xdr, &lock->lk_denied);

	return nfserr;
}

static __be32
nfsd4_encode_lockt(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_lockt *lockt)
{
	struct xdr_stream *xdr = &resp->xdr;

	if (nfserr == nfserr_denied)
		nfsd4_encode_lock_denied(xdr, &lockt->lt_denied);
	return nfserr;
}

static __be32
nfsd4_encode_locku(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_locku *locku)
{
	struct xdr_stream *xdr = &resp->xdr;

	if (!nfserr)
		nfserr = nfsd4_encode_stateid(xdr, &locku->lu_stateid);

	return nfserr;
}


static __be32
nfsd4_encode_link(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_link *link)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (!nfserr) {
		p = xdr_reserve_space(xdr, 20);
		if (!p)
			return nfserr_resource;
		p = encode_cinfo(p, &link->li_cinfo);
	}
	return nfserr;
}


static __be32
nfsd4_encode_open(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_open *open)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (nfserr)
		goto out;

	nfserr = nfsd4_encode_stateid(xdr, &open->op_stateid);
	if (nfserr)
		goto out;
	p = xdr_reserve_space(xdr, 24);
	if (!p)
		return nfserr_resource;
	p = encode_cinfo(p, &open->op_cinfo);
	*p++ = cpu_to_be32(open->op_rflags);

	nfserr = nfsd4_encode_bitmap(xdr, open->op_bmval[0], open->op_bmval[1],
					open->op_bmval[2]);
	if (nfserr)
		goto out;

	p = xdr_reserve_space(xdr, 4);
	if (!p)
		return nfserr_resource;

	*p++ = cpu_to_be32(open->op_delegate_type);
	switch (open->op_delegate_type) {
	case NFS4_OPEN_DELEGATE_NONE:
		break;
	case NFS4_OPEN_DELEGATE_READ:
		nfserr = nfsd4_encode_stateid(xdr, &open->op_delegate_stateid);
		if (nfserr)
			return nfserr;
		p = xdr_reserve_space(xdr, 20);
		if (!p)
			return nfserr_resource;
		*p++ = cpu_to_be32(open->op_recall);

		/*
		 * TODO: ACE's in delegations
		 */
		*p++ = cpu_to_be32(NFS4_ACE_ACCESS_ALLOWED_ACE_TYPE);
		*p++ = cpu_to_be32(0);
		*p++ = cpu_to_be32(0);
		*p++ = cpu_to_be32(0);   /* XXX: is NULL principal ok? */
		break;
	case NFS4_OPEN_DELEGATE_WRITE:
		nfserr = nfsd4_encode_stateid(xdr, &open->op_delegate_stateid);
		if (nfserr)
			return nfserr;
		p = xdr_reserve_space(xdr, 32);
		if (!p)
			return nfserr_resource;
		*p++ = cpu_to_be32(open->op_recall);

		/*
		 * TODO: space_limit's in delegations
		 */
		*p++ = cpu_to_be32(NFS4_LIMIT_SIZE);
		*p++ = cpu_to_be32(~(u32)0);
		*p++ = cpu_to_be32(~(u32)0);

		/*
		 * TODO: ACE's in delegations
		 */
		*p++ = cpu_to_be32(NFS4_ACE_ACCESS_ALLOWED_ACE_TYPE);
		*p++ = cpu_to_be32(0);
		*p++ = cpu_to_be32(0);
		*p++ = cpu_to_be32(0);   /* XXX: is NULL principal ok? */
		break;
	case NFS4_OPEN_DELEGATE_NONE_EXT: /* 4.1 */
		switch (open->op_why_no_deleg) {
		case WND4_CONTENTION:
		case WND4_RESOURCE:
			p = xdr_reserve_space(xdr, 8);
			if (!p)
				return nfserr_resource;
			*p++ = cpu_to_be32(open->op_why_no_deleg);
			/* deleg signaling not supported yet: */
			*p++ = cpu_to_be32(0);
			break;
		default:
			p = xdr_reserve_space(xdr, 4);
			if (!p)
				return nfserr_resource;
			*p++ = cpu_to_be32(open->op_why_no_deleg);
		}
		break;
	default:
		BUG();
	}
	/* XXX save filehandle here */
out:
	return nfserr;
}

static __be32
nfsd4_encode_open_confirm(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_open_confirm *oc)
{
	struct xdr_stream *xdr = &resp->xdr;

	if (!nfserr)
		nfserr = nfsd4_encode_stateid(xdr, &oc->oc_resp_stateid);

	return nfserr;
}

static __be32
nfsd4_encode_open_downgrade(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_open_downgrade *od)
{
	struct xdr_stream *xdr = &resp->xdr;

	if (!nfserr)
		nfserr = nfsd4_encode_stateid(xdr, &od->od_stateid);

	return nfserr;
}

static __be32 nfsd4_encode_splice_read(
				struct nfsd4_compoundres *resp,
				struct nfsd4_read *read,
				struct file *file, unsigned long maxcount)
{
	struct xdr_stream *xdr = &resp->xdr;
	struct xdr_buf *buf = xdr->buf;
	u32 eof;
	int space_left;
	__be32 nfserr;
	__be32 *p = xdr->p - 2;

	/* Make sure there will be room for padding if needed */
	if (xdr->end - xdr->p < 1)
		return nfserr_resource;

	nfserr = nfsd_splice_read(read->rd_rqstp, file,
				  read->rd_offset, &maxcount);
	if (nfserr) {
		/*
		 * nfsd_splice_actor may have already messed with the
		 * page length; reset it so as not to confuse
		 * xdr_truncate_encode:
		 */
		buf->page_len = 0;
		return nfserr;
	}

	eof = (read->rd_offset + maxcount >=
	       d_inode(read->rd_fhp->fh_dentry)->i_size);

	*(p++) = htonl(eof);
	*(p++) = htonl(maxcount);

	buf->page_len = maxcount;
	buf->len += maxcount;
	xdr->page_ptr += (buf->page_base + maxcount + PAGE_SIZE - 1)
							/ PAGE_SIZE;

	/* Use rest of head for padding and remaining ops: */
	buf->tail[0].iov_base = xdr->p;
	buf->tail[0].iov_len = 0;
	xdr->iov = buf->tail;
	if (maxcount&3) {
		int pad = 4 - (maxcount&3);

		*(xdr->p++) = 0;

		buf->tail[0].iov_base += maxcount&3;
		buf->tail[0].iov_len = pad;
		buf->len += pad;
	}

	space_left = min_t(int, (void *)xdr->end - (void *)xdr->p,
				buf->buflen - buf->len);
	buf->buflen = buf->len + space_left;
	xdr->end = (__be32 *)((void *)xdr->end + space_left);

	return 0;
}

static __be32 nfsd4_encode_readv(struct nfsd4_compoundres *resp,
				 struct nfsd4_read *read,
				 struct file *file, unsigned long maxcount)
{
	struct xdr_stream *xdr = &resp->xdr;
	u32 eof;
	int v;
	int starting_len = xdr->buf->len - 8;
	long len;
	int thislen;
	__be32 nfserr;
	__be32 tmp;
	__be32 *p;
	u32 zzz = 0;
	int pad;

	len = maxcount;
	v = 0;

	thislen = min_t(long, len, ((void *)xdr->end - (void *)xdr->p));
	p = xdr_reserve_space(xdr, (thislen+3)&~3);
	WARN_ON_ONCE(!p);
	resp->rqstp->rq_vec[v].iov_base = p;
	resp->rqstp->rq_vec[v].iov_len = thislen;
	v++;
	len -= thislen;

	while (len) {
		thislen = min_t(long, len, PAGE_SIZE);
		p = xdr_reserve_space(xdr, (thislen+3)&~3);
		WARN_ON_ONCE(!p);
		resp->rqstp->rq_vec[v].iov_base = p;
		resp->rqstp->rq_vec[v].iov_len = thislen;
		v++;
		len -= thislen;
	}
	read->rd_vlen = v;

	nfserr = nfsd_readv(file, read->rd_offset, resp->rqstp->rq_vec,
			read->rd_vlen, &maxcount);
	if (nfserr)
		return nfserr;
	xdr_truncate_encode(xdr, starting_len + 8 + ((maxcount+3)&~3));

	eof = (read->rd_offset + maxcount >=
	       d_inode(read->rd_fhp->fh_dentry)->i_size);

	tmp = htonl(eof);
	write_bytes_to_xdr_buf(xdr->buf, starting_len    , &tmp, 4);
	tmp = htonl(maxcount);
	write_bytes_to_xdr_buf(xdr->buf, starting_len + 4, &tmp, 4);

	pad = (maxcount&3) ? 4 - (maxcount&3) : 0;
	write_bytes_to_xdr_buf(xdr->buf, starting_len + 8 + maxcount,
								&zzz, pad);
	return 0;

}

static __be32
nfsd4_encode_read(struct nfsd4_compoundres *resp, __be32 nfserr,
		  struct nfsd4_read *read)
{
	unsigned long maxcount;
	struct xdr_stream *xdr = &resp->xdr;
	struct file *file = read->rd_filp;
	int starting_len = xdr->buf->len;
	struct raparms *ra = NULL;
	__be32 *p;

	if (nfserr)
		goto out;

	p = xdr_reserve_space(xdr, 8); /* eof flag and byte count */
	if (!p) {
		WARN_ON_ONCE(test_bit(RQ_SPLICE_OK, &resp->rqstp->rq_flags));
		nfserr = nfserr_resource;
		goto out;
	}
	if (resp->xdr.buf->page_len &&
	    test_bit(RQ_SPLICE_OK, &resp->rqstp->rq_flags)) {
		WARN_ON_ONCE(1);
		nfserr = nfserr_resource;
		goto out;
	}
	xdr_commit_encode(xdr);

	maxcount = svc_max_payload(resp->rqstp);
	maxcount = min_t(unsigned long, maxcount,
			 (xdr->buf->buflen - xdr->buf->len));
	maxcount = min_t(unsigned long, maxcount, read->rd_length);

	if (read->rd_tmp_file)
		ra = nfsd_init_raparms(file);

	if (file->f_op->splice_read &&
	    test_bit(RQ_SPLICE_OK, &resp->rqstp->rq_flags))
		nfserr = nfsd4_encode_splice_read(resp, read, file, maxcount);
	else
		nfserr = nfsd4_encode_readv(resp, read, file, maxcount);

	if (ra)
		nfsd_put_raparams(file, ra);

	if (nfserr)
		xdr_truncate_encode(xdr, starting_len);

out:
	if (file)
		fput(file);
	return nfserr;
}

static __be32
nfsd4_encode_readlink(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_readlink *readlink)
{
	int maxcount;
	__be32 wire_count;
	int zero = 0;
	struct xdr_stream *xdr = &resp->xdr;
	int length_offset = xdr->buf->len;
	__be32 *p;

	if (nfserr)
		return nfserr;

	p = xdr_reserve_space(xdr, 4);
	if (!p)
		return nfserr_resource;
	maxcount = PAGE_SIZE;

	p = xdr_reserve_space(xdr, maxcount);
	if (!p)
		return nfserr_resource;
	/*
	 * XXX: By default, the ->readlink() VFS op will truncate symlinks
	 * if they would overflow the buffer.  Is this kosher in NFSv4?  If
	 * not, one easy fix is: if ->readlink() precisely fills the buffer,
	 * assume that truncation occurred, and return NFS4ERR_RESOURCE.
	 */
	nfserr = nfsd_readlink(readlink->rl_rqstp, readlink->rl_fhp,
						(char *)p, &maxcount);
	if (nfserr == nfserr_isdir)
		nfserr = nfserr_inval;
	if (nfserr) {
		xdr_truncate_encode(xdr, length_offset);
		return nfserr;
	}

	wire_count = htonl(maxcount);
	write_bytes_to_xdr_buf(xdr->buf, length_offset, &wire_count, 4);
	xdr_truncate_encode(xdr, length_offset + 4 + ALIGN(maxcount, 4));
	if (maxcount & 3)
		write_bytes_to_xdr_buf(xdr->buf, length_offset + 4 + maxcount,
						&zero, 4 - (maxcount&3));
	return 0;
}

static __be32
nfsd4_encode_readdir(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_readdir *readdir)
{
	int maxcount;
	int bytes_left;
	loff_t offset;
	__be64 wire_offset;
	struct xdr_stream *xdr = &resp->xdr;
	int starting_len = xdr->buf->len;
	__be32 *p;

	if (nfserr)
		return nfserr;

	p = xdr_reserve_space(xdr, NFS4_VERIFIER_SIZE);
	if (!p)
		return nfserr_resource;

	/* XXX: Following NFSv3, we ignore the READDIR verifier for now. */
	*p++ = cpu_to_be32(0);
	*p++ = cpu_to_be32(0);
	resp->xdr.buf->head[0].iov_len = ((char *)resp->xdr.p)
				- (char *)resp->xdr.buf->head[0].iov_base;

	/*
	 * Number of bytes left for directory entries allowing for the
	 * final 8 bytes of the readdir and a following failed op:
	 */
	bytes_left = xdr->buf->buflen - xdr->buf->len
			- COMPOUND_ERR_SLACK_SPACE - 8;
	if (bytes_left < 0) {
		nfserr = nfserr_resource;
		goto err_no_verf;
	}
	maxcount = svc_max_payload(resp->rqstp);
	maxcount = min_t(u32, readdir->rd_maxcount, maxcount);
	/*
	 * Note the rfc defines rd_maxcount as the size of the
	 * READDIR4resok structure, which includes the verifier above
	 * and the 8 bytes encoded at the end of this function:
	 */
	if (maxcount < 16) {
		nfserr = nfserr_toosmall;
		goto err_no_verf;
	}
	maxcount = min_t(int, maxcount-16, bytes_left);

	/* RFC 3530 14.2.24 allows us to ignore dircount when it's 0: */
	if (!readdir->rd_dircount)
		readdir->rd_dircount = svc_max_payload(resp->rqstp);

	readdir->xdr = xdr;
	readdir->rd_maxcount = maxcount;
	readdir->common.err = 0;
	readdir->cookie_offset = 0;

	offset = readdir->rd_cookie;
	nfserr = nfsd_readdir(readdir->rd_rqstp, readdir->rd_fhp,
			      &offset,
			      &readdir->common, nfsd4_encode_dirent);
	if (nfserr == nfs_ok &&
	    readdir->common.err == nfserr_toosmall &&
	    xdr->buf->len == starting_len + 8) {
		/* nothing encoded; which limit did we hit?: */
		if (maxcount - 16 < bytes_left)
			/* It was the fault of rd_maxcount: */
			nfserr = nfserr_toosmall;
		else
			/* We ran out of buffer space: */
			nfserr = nfserr_resource;
	}
	if (nfserr)
		goto err_no_verf;

	if (readdir->cookie_offset) {
		wire_offset = cpu_to_be64(offset);
		write_bytes_to_xdr_buf(xdr->buf, readdir->cookie_offset,
							&wire_offset, 8);
	}

	p = xdr_reserve_space(xdr, 8);
	if (!p) {
		WARN_ON_ONCE(1);
		goto err_no_verf;
	}
	*p++ = 0;	/* no more entries */
	*p++ = htonl(readdir->common.err == nfserr_eof);

	return 0;
err_no_verf:
	xdr_truncate_encode(xdr, starting_len);
	return nfserr;
}

static __be32
nfsd4_encode_remove(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_remove *remove)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (!nfserr) {
		p = xdr_reserve_space(xdr, 20);
		if (!p)
			return nfserr_resource;
		p = encode_cinfo(p, &remove->rm_cinfo);
	}
	return nfserr;
}

static __be32
nfsd4_encode_rename(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_rename *rename)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (!nfserr) {
		p = xdr_reserve_space(xdr, 40);
		if (!p)
			return nfserr_resource;
		p = encode_cinfo(p, &rename->rn_sinfo);
		p = encode_cinfo(p, &rename->rn_tinfo);
	}
	return nfserr;
}

static __be32
nfsd4_do_encode_secinfo(struct xdr_stream *xdr,
			 __be32 nfserr, struct svc_export *exp)
{
	u32 i, nflavs, supported;
	struct exp_flavor_info *flavs;
	struct exp_flavor_info def_flavs[2];
	__be32 *p, *flavorsp;
	static bool report = true;

	if (nfserr)
		goto out;
	nfserr = nfserr_resource;
	if (exp->ex_nflavors) {
		flavs = exp->ex_flavors;
		nflavs = exp->ex_nflavors;
	} else { /* Handling of some defaults in absence of real secinfo: */
		flavs = def_flavs;
		if (exp->ex_client->flavour->flavour == RPC_AUTH_UNIX) {
			nflavs = 2;
			flavs[0].pseudoflavor = RPC_AUTH_UNIX;
			flavs[1].pseudoflavor = RPC_AUTH_NULL;
		} else if (exp->ex_client->flavour->flavour == RPC_AUTH_GSS) {
			nflavs = 1;
			flavs[0].pseudoflavor
					= svcauth_gss_flavor(exp->ex_client);
		} else {
			nflavs = 1;
			flavs[0].pseudoflavor
					= exp->ex_client->flavour->flavour;
		}
	}

	supported = 0;
	p = xdr_reserve_space(xdr, 4);
	if (!p)
		goto out;
	flavorsp = p++;		/* to be backfilled later */

	for (i = 0; i < nflavs; i++) {
		rpc_authflavor_t pf = flavs[i].pseudoflavor;
		struct rpcsec_gss_info info;

		if (rpcauth_get_gssinfo(pf, &info) == 0) {
			supported++;
			p = xdr_reserve_space(xdr, 4 + 4 +
					      XDR_LEN(info.oid.len) + 4 + 4);
			if (!p)
				goto out;
			*p++ = cpu_to_be32(RPC_AUTH_GSS);
			p = xdr_encode_opaque(p,  info.oid.data, info.oid.len);
			*p++ = cpu_to_be32(info.qop);
			*p++ = cpu_to_be32(info.service);
		} else if (pf < RPC_AUTH_MAXFLAVOR) {
			supported++;
			p = xdr_reserve_space(xdr, 4);
			if (!p)
				goto out;
			*p++ = cpu_to_be32(pf);
		} else {
			if (report)
				pr_warn("NFS: SECINFO: security flavor %u "
					"is not supported\n", pf);
		}
	}

	if (nflavs != supported)
		report = false;
	*flavorsp = htonl(supported);
	nfserr = 0;
out:
	if (exp)
		exp_put(exp);
	return nfserr;
}

static __be32
nfsd4_encode_secinfo(struct nfsd4_compoundres *resp, __be32 nfserr,
		     struct nfsd4_secinfo *secinfo)
{
	struct xdr_stream *xdr = &resp->xdr;

	return nfsd4_do_encode_secinfo(xdr, nfserr, secinfo->si_exp);
}

static __be32
nfsd4_encode_secinfo_no_name(struct nfsd4_compoundres *resp, __be32 nfserr,
		     struct nfsd4_secinfo_no_name *secinfo)
{
	struct xdr_stream *xdr = &resp->xdr;

	return nfsd4_do_encode_secinfo(xdr, nfserr, secinfo->sin_exp);
}

/*
 * The SETATTR encode routine is special -- it always encodes a bitmap,
 * regardless of the error status.
 */
static __be32
nfsd4_encode_setattr(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_setattr *setattr)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	p = xdr_reserve_space(xdr, 16);
	if (!p)
		return nfserr_resource;
	if (nfserr) {
		*p++ = cpu_to_be32(3);
		*p++ = cpu_to_be32(0);
		*p++ = cpu_to_be32(0);
		*p++ = cpu_to_be32(0);
	}
	else {
		*p++ = cpu_to_be32(3);
		*p++ = cpu_to_be32(setattr->sa_bmval[0]);
		*p++ = cpu_to_be32(setattr->sa_bmval[1]);
		*p++ = cpu_to_be32(setattr->sa_bmval[2]);
	}
	return nfserr;
}

static __be32
nfsd4_encode_setclientid(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_setclientid *scd)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (!nfserr) {
		p = xdr_reserve_space(xdr, 8 + NFS4_VERIFIER_SIZE);
		if (!p)
			return nfserr_resource;
		p = xdr_encode_opaque_fixed(p, &scd->se_clientid, 8);
		p = xdr_encode_opaque_fixed(p, &scd->se_confirm,
						NFS4_VERIFIER_SIZE);
	}
	else if (nfserr == nfserr_clid_inuse) {
		p = xdr_reserve_space(xdr, 8);
		if (!p)
			return nfserr_resource;
		*p++ = cpu_to_be32(0);
		*p++ = cpu_to_be32(0);
	}
	return nfserr;
}

static __be32
nfsd4_encode_write(struct nfsd4_compoundres *resp, __be32 nfserr, struct nfsd4_write *write)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (!nfserr) {
		p = xdr_reserve_space(xdr, 16);
		if (!p)
			return nfserr_resource;
		*p++ = cpu_to_be32(write->wr_bytes_written);
		*p++ = cpu_to_be32(write->wr_how_written);
		p = xdr_encode_opaque_fixed(p, write->wr_verifier.data,
							NFS4_VERIFIER_SIZE);
	}
	return nfserr;
}

static const u32 nfs4_minimal_spo_must_enforce[2] = {
	[1] = 1 << (OP_BIND_CONN_TO_SESSION - 32) |
	      1 << (OP_EXCHANGE_ID - 32) |
	      1 << (OP_CREATE_SESSION - 32) |
	      1 << (OP_DESTROY_SESSION - 32) |
	      1 << (OP_DESTROY_CLIENTID - 32)
};

static __be32
nfsd4_encode_exchange_id(struct nfsd4_compoundres *resp, __be32 nfserr,
			 struct nfsd4_exchange_id *exid)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;
	char *major_id;
	char *server_scope;
	int major_id_sz;
	int server_scope_sz;
	uint64_t minor_id = 0;

	if (nfserr)
		return nfserr;

	major_id = utsname()->nodename;
	major_id_sz = strlen(major_id);
	server_scope = utsname()->nodename;
	server_scope_sz = strlen(server_scope);

	p = xdr_reserve_space(xdr,
		8 /* eir_clientid */ +
		4 /* eir_sequenceid */ +
		4 /* eir_flags */ +
		4 /* spr_how */);
	if (!p)
		return nfserr_resource;

	p = xdr_encode_opaque_fixed(p, &exid->clientid, 8);
	*p++ = cpu_to_be32(exid->seqid);
	*p++ = cpu_to_be32(exid->flags);

	*p++ = cpu_to_be32(exid->spa_how);

	switch (exid->spa_how) {
	case SP4_NONE:
		break;
	case SP4_MACH_CRED:
		/* spo_must_enforce, spo_must_allow */
		p = xdr_reserve_space(xdr, 16);
		if (!p)
			return nfserr_resource;

		/* spo_must_enforce bitmap: */
		*p++ = cpu_to_be32(2);
		*p++ = cpu_to_be32(nfs4_minimal_spo_must_enforce[0]);
		*p++ = cpu_to_be32(nfs4_minimal_spo_must_enforce[1]);
		/* empty spo_must_allow bitmap: */
		*p++ = cpu_to_be32(0);

		break;
	default:
		WARN_ON_ONCE(1);
	}

	p = xdr_reserve_space(xdr,
		8 /* so_minor_id */ +
		4 /* so_major_id.len */ +
		(XDR_QUADLEN(major_id_sz) * 4) +
		4 /* eir_server_scope.len */ +
		(XDR_QUADLEN(server_scope_sz) * 4) +
		4 /* eir_server_impl_id.count (0) */);
	if (!p)
		return nfserr_resource;

	/* The server_owner struct */
	p = xdr_encode_hyper(p, minor_id);      /* Minor id */
	/* major id */
	p = xdr_encode_opaque(p, major_id, major_id_sz);

	/* Server scope */
	p = xdr_encode_opaque(p, server_scope, server_scope_sz);

	/* Implementation id */
	*p++ = cpu_to_be32(0);	/* zero length nfs_impl_id4 array */
	return 0;
}

static __be32
nfsd4_encode_create_session(struct nfsd4_compoundres *resp, __be32 nfserr,
			    struct nfsd4_create_session *sess)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (nfserr)
		return nfserr;

	p = xdr_reserve_space(xdr, 24);
	if (!p)
		return nfserr_resource;
	p = xdr_encode_opaque_fixed(p, sess->sessionid.data,
					NFS4_MAX_SESSIONID_LEN);
	*p++ = cpu_to_be32(sess->seqid);
	*p++ = cpu_to_be32(sess->flags);

	p = xdr_reserve_space(xdr, 28);
	if (!p)
		return nfserr_resource;
	*p++ = cpu_to_be32(0); /* headerpadsz */
	*p++ = cpu_to_be32(sess->fore_channel.maxreq_sz);
	*p++ = cpu_to_be32(sess->fore_channel.maxresp_sz);
	*p++ = cpu_to_be32(sess->fore_channel.maxresp_cached);
	*p++ = cpu_to_be32(sess->fore_channel.maxops);
	*p++ = cpu_to_be32(sess->fore_channel.maxreqs);
	*p++ = cpu_to_be32(sess->fore_channel.nr_rdma_attrs);

	if (sess->fore_channel.nr_rdma_attrs) {
		p = xdr_reserve_space(xdr, 4);
		if (!p)
			return nfserr_resource;
		*p++ = cpu_to_be32(sess->fore_channel.rdma_attrs);
	}

	p = xdr_reserve_space(xdr, 28);
	if (!p)
		return nfserr_resource;
	*p++ = cpu_to_be32(0); /* headerpadsz */
	*p++ = cpu_to_be32(sess->back_channel.maxreq_sz);
	*p++ = cpu_to_be32(sess->back_channel.maxresp_sz);
	*p++ = cpu_to_be32(sess->back_channel.maxresp_cached);
	*p++ = cpu_to_be32(sess->back_channel.maxops);
	*p++ = cpu_to_be32(sess->back_channel.maxreqs);
	*p++ = cpu_to_be32(sess->back_channel.nr_rdma_attrs);

	if (sess->back_channel.nr_rdma_attrs) {
		p = xdr_reserve_space(xdr, 4);
		if (!p)
			return nfserr_resource;
		*p++ = cpu_to_be32(sess->back_channel.rdma_attrs);
	}
	return 0;
}

static __be32
nfsd4_encode_sequence(struct nfsd4_compoundres *resp, __be32 nfserr,
		      struct nfsd4_sequence *seq)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (nfserr)
		return nfserr;

	p = xdr_reserve_space(xdr, NFS4_MAX_SESSIONID_LEN + 20);
	if (!p)
		return nfserr_resource;
	p = xdr_encode_opaque_fixed(p, seq->sessionid.data,
					NFS4_MAX_SESSIONID_LEN);
	*p++ = cpu_to_be32(seq->seqid);
	*p++ = cpu_to_be32(seq->slotid);
	/* Note slotid's are numbered from zero: */
	*p++ = cpu_to_be32(seq->maxslots - 1); /* sr_highest_slotid */
	*p++ = cpu_to_be32(seq->maxslots - 1); /* sr_target_highest_slotid */
	*p++ = cpu_to_be32(seq->status_flags);

	resp->cstate.data_offset = xdr->buf->len; /* DRC cache data pointer */
	return 0;
}

static __be32
nfsd4_encode_test_stateid(struct nfsd4_compoundres *resp, __be32 nfserr,
			  struct nfsd4_test_stateid *test_stateid)
{
	struct xdr_stream *xdr = &resp->xdr;
	struct nfsd4_test_stateid_id *stateid, *next;
	__be32 *p;

	if (nfserr)
		return nfserr;

	p = xdr_reserve_space(xdr, 4 + (4 * test_stateid->ts_num_ids));
	if (!p)
		return nfserr_resource;
	*p++ = htonl(test_stateid->ts_num_ids);

	list_for_each_entry_safe(stateid, next, &test_stateid->ts_stateid_list, ts_id_list) {
		*p++ = stateid->ts_id_status;
	}

	return nfserr;
}

#ifdef CONFIG_NFSD_PNFS
static __be32
nfsd4_encode_getdeviceinfo(struct nfsd4_compoundres *resp, __be32 nfserr,
		struct nfsd4_getdeviceinfo *gdev)
{
	struct xdr_stream *xdr = &resp->xdr;
	const struct nfsd4_layout_ops *ops;
	u32 starting_len = xdr->buf->len, needed_len;
	__be32 *p;

	dprintk("%s: err %d\n", __func__, nfserr);
	if (nfserr)
		goto out;

	nfserr = nfserr_resource;
	p = xdr_reserve_space(xdr, 4);
	if (!p)
		goto out;

	*p++ = cpu_to_be32(gdev->gd_layout_type);

	/* If maxcount is 0 then just update notifications */
	if (gdev->gd_maxcount != 0) {
		ops = nfsd4_layout_ops[gdev->gd_layout_type];
		nfserr = ops->encode_getdeviceinfo(xdr, gdev);
		if (nfserr) {
			/*
			 * We don't bother to burden the layout drivers with
			 * enforcing gd_maxcount, just tell the client to
			 * come back with a bigger buffer if it's not enough.
			 */
			if (xdr->buf->len + 4 > gdev->gd_maxcount)
				goto toosmall;
			goto out;
		}
	}

	nfserr = nfserr_resource;
	if (gdev->gd_notify_types) {
		p = xdr_reserve_space(xdr, 4 + 4);
		if (!p)
			goto out;
		*p++ = cpu_to_be32(1);			/* bitmap length */
		*p++ = cpu_to_be32(gdev->gd_notify_types);
	} else {
		p = xdr_reserve_space(xdr, 4);
		if (!p)
			goto out;
		*p++ = 0;
	}

	nfserr = 0;
out:
	kfree(gdev->gd_device);
	dprintk("%s: done: %d\n", __func__, be32_to_cpu(nfserr));
	return nfserr;

toosmall:
	dprintk("%s: maxcount too small\n", __func__);
	needed_len = xdr->buf->len + 4 /* notifications */;
	xdr_truncate_encode(xdr, starting_len);
	p = xdr_reserve_space(xdr, 4);
	if (!p) {
		nfserr = nfserr_resource;
	} else {
		*p++ = cpu_to_be32(needed_len);
		nfserr = nfserr_toosmall;
	}
	goto out;
}

static __be32
nfsd4_encode_layoutget(struct nfsd4_compoundres *resp, __be32 nfserr,
		struct nfsd4_layoutget *lgp)
{
	struct xdr_stream *xdr = &resp->xdr;
	const struct nfsd4_layout_ops *ops;
	__be32 *p;

	dprintk("%s: err %d\n", __func__, nfserr);
	if (nfserr)
		goto out;

	nfserr = nfserr_resource;
	p = xdr_reserve_space(xdr, 36 + sizeof(stateid_opaque_t));
	if (!p)
		goto out;

	*p++ = cpu_to_be32(1);	/* we always set return-on-close */
	*p++ = cpu_to_be32(lgp->lg_sid.si_generation);
	p = xdr_encode_opaque_fixed(p, &lgp->lg_sid.si_opaque,
				    sizeof(stateid_opaque_t));

	*p++ = cpu_to_be32(1);	/* we always return a single layout */
	p = xdr_encode_hyper(p, lgp->lg_seg.offset);
	p = xdr_encode_hyper(p, lgp->lg_seg.length);
	*p++ = cpu_to_be32(lgp->lg_seg.iomode);
	*p++ = cpu_to_be32(lgp->lg_layout_type);

	ops = nfsd4_layout_ops[lgp->lg_layout_type];
	nfserr = ops->encode_layoutget(xdr, lgp);
out:
	kfree(lgp->lg_content);
	return nfserr;
}

static __be32
nfsd4_encode_layoutcommit(struct nfsd4_compoundres *resp, __be32 nfserr,
			  struct nfsd4_layoutcommit *lcp)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (nfserr)
		return nfserr;

	p = xdr_reserve_space(xdr, 4);
	if (!p)
		return nfserr_resource;
	*p++ = cpu_to_be32(lcp->lc_size_chg);
	if (lcp->lc_size_chg) {
		p = xdr_reserve_space(xdr, 8);
		if (!p)
			return nfserr_resource;
		p = xdr_encode_hyper(p, lcp->lc_newsize);
	}

	return nfs_ok;
}

static __be32
nfsd4_encode_layoutreturn(struct nfsd4_compoundres *resp, __be32 nfserr,
		struct nfsd4_layoutreturn *lrp)
{
	struct xdr_stream *xdr = &resp->xdr;
	__be32 *p;

	if (nfserr)
		return nfserr;

	p = xdr_reserve_space(xdr, 4);
	if (!p)
		return nfserr_resource;
	*p++ = cpu_to_be32(lrp->lrs_present);
	if (lrp->lrs_present)
		return nfsd4_encode_stateid(xdr, &lrp->lr_sid);
	return nfs_ok;
}
#endif /* CONFIG_NFSD_PNFS */

static __be32
nfsd4_encode_seek(struct nfsd4_compoundres *resp, __be32 nfserr,
		  struct nfsd4_seek *seek)
{
	__be32 *p;

	if (nfserr)
		return nfserr;

	p = xdr_reserve_space(&resp->xdr, 4 + 8);
	*p++ = cpu_to_be32(seek->seek_eof);
	p = xdr_encode_hyper(p, seek->seek_pos);

	return nfserr;
}

static __be32
nfsd4_encode_noop(struct nfsd4_compoundres *resp, __be32 nfserr, void *p)
{
	return nfserr;
}

typedef __be32(* nfsd4_enc)(struct nfsd4_compoundres *, __be32, void *);

/*
 * Note: nfsd4_enc_ops vector is shared for v4.0 and v4.1
 * since we don't need to filter out obsolete ops as this is
 * done in the decoding phase.
 */
static nfsd4_enc nfsd4_enc_ops[] = {
	[OP_ACCESS]		= (nfsd4_enc)nfsd4_encode_access,
	[OP_CLOSE]		= (nfsd4_enc)nfsd4_encode_close,
	[OP_COMMIT]		= (nfsd4_enc)nfsd4_encode_commit,
	[OP_CREATE]		= (nfsd4_enc)nfsd4_encode_create,
	[OP_DELEGPURGE]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_DELEGRETURN]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_GETATTR]		= (nfsd4_enc)nfsd4_encode_getattr,
	[OP_GETFH]		= (nfsd4_enc)nfsd4_encode_getfh,
	[OP_LINK]		= (nfsd4_enc)nfsd4_encode_link,
	[OP_LOCK]		= (nfsd4_enc)nfsd4_encode_lock,
	[OP_LOCKT]		= (nfsd4_enc)nfsd4_encode_lockt,
	[OP_LOCKU]		= (nfsd4_enc)nfsd4_encode_locku,
	[OP_LOOKUP]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_LOOKUPP]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_NVERIFY]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_OPEN]		= (nfsd4_enc)nfsd4_encode_open,
	[OP_OPENATTR]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_OPEN_CONFIRM]	= (nfsd4_enc)nfsd4_encode_open_confirm,
	[OP_OPEN_DOWNGRADE]	= (nfsd4_enc)nfsd4_encode_open_downgrade,
	[OP_PUTFH]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_PUTPUBFH]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_PUTROOTFH]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_READ]		= (nfsd4_enc)nfsd4_encode_read,
	[OP_READDIR]		= (nfsd4_enc)nfsd4_encode_readdir,
	[OP_READLINK]		= (nfsd4_enc)nfsd4_encode_readlink,
	[OP_REMOVE]		= (nfsd4_enc)nfsd4_encode_remove,
	[OP_RENAME]		= (nfsd4_enc)nfsd4_encode_rename,
	[OP_RENEW]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_RESTOREFH]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_SAVEFH]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_SECINFO]		= (nfsd4_enc)nfsd4_encode_secinfo,
	[OP_SETATTR]		= (nfsd4_enc)nfsd4_encode_setattr,
	[OP_SETCLIENTID]	= (nfsd4_enc)nfsd4_encode_setclientid,
	[OP_SETCLIENTID_CONFIRM] = (nfsd4_enc)nfsd4_encode_noop,
	[OP_VERIFY]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_WRITE]		= (nfsd4_enc)nfsd4_encode_write,
	[OP_RELEASE_LOCKOWNER]	= (nfsd4_enc)nfsd4_encode_noop,

	/* NFSv4.1 operations */
	[OP_BACKCHANNEL_CTL]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_BIND_CONN_TO_SESSION] = (nfsd4_enc)nfsd4_encode_bind_conn_to_session,
	[OP_EXCHANGE_ID]	= (nfsd4_enc)nfsd4_encode_exchange_id,
	[OP_CREATE_SESSION]	= (nfsd4_enc)nfsd4_encode_create_session,
	[OP_DESTROY_SESSION]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_FREE_STATEID]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_GET_DIR_DELEGATION]	= (nfsd4_enc)nfsd4_encode_noop,
#ifdef CONFIG_NFSD_PNFS
	[OP_GETDEVICEINFO]	= (nfsd4_enc)nfsd4_encode_getdeviceinfo,
	[OP_GETDEVICELIST]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_LAYOUTCOMMIT]	= (nfsd4_enc)nfsd4_encode_layoutcommit,
	[OP_LAYOUTGET]		= (nfsd4_enc)nfsd4_encode_layoutget,
	[OP_LAYOUTRETURN]	= (nfsd4_enc)nfsd4_encode_layoutreturn,
#else
	[OP_GETDEVICEINFO]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_GETDEVICELIST]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_LAYOUTCOMMIT]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_LAYOUTGET]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_LAYOUTRETURN]	= (nfsd4_enc)nfsd4_encode_noop,
#endif
	[OP_SECINFO_NO_NAME]	= (nfsd4_enc)nfsd4_encode_secinfo_no_name,
	[OP_SEQUENCE]		= (nfsd4_enc)nfsd4_encode_sequence,
	[OP_SET_SSV]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_TEST_STATEID]	= (nfsd4_enc)nfsd4_encode_test_stateid,
	[OP_WANT_DELEGATION]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_DESTROY_CLIENTID]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_RECLAIM_COMPLETE]	= (nfsd4_enc)nfsd4_encode_noop,

	/* NFSv4.2 operations */
	[OP_ALLOCATE]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_COPY]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_COPY_NOTIFY]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_DEALLOCATE]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_IO_ADVISE]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_LAYOUTERROR]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_LAYOUTSTATS]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_OFFLOAD_CANCEL]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_OFFLOAD_STATUS]	= (nfsd4_enc)nfsd4_encode_noop,
	[OP_READ_PLUS]		= (nfsd4_enc)nfsd4_encode_noop,
	[OP_SEEK]		= (nfsd4_enc)nfsd4_encode_seek,
	[OP_WRITE_SAME]		= (nfsd4_enc)nfsd4_encode_noop,
};

/*
 * Calculate whether we still have space to encode repsize bytes.
 * There are two considerations:
 *     - For NFS versions >=4.1, the size of the reply must stay within
 *       session limits
 *     - For all NFS versions, we must stay within limited preallocated
 *       buffer space.
 *
 * This is called before the operation is processed, so can only provide
 * an upper estimate.  For some nonidempotent operations (such as
 * getattr), it's not necessarily a problem if that estimate is wrong,
 * as we can fail it after processing without significant side effects.
 */
__be32 nfsd4_check_resp_size(struct nfsd4_compoundres *resp, u32 respsize)
{
	struct xdr_buf *buf = &resp->rqstp->rq_res;
	struct nfsd4_slot *slot = resp->cstate.slot;

	if (buf->len + respsize <= buf->buflen)
		return nfs_ok;
	if (!nfsd4_has_session(&resp->cstate))
		return nfserr_resource;
	if (slot->sl_flags & NFSD4_SLOT_CACHETHIS) {
		WARN_ON_ONCE(1);
		return nfserr_rep_too_big_to_cache;
	}
	return nfserr_rep_too_big;
}

void
nfsd4_encode_operation(struct nfsd4_compoundres *resp, struct nfsd4_op *op)
{
	struct xdr_stream *xdr = &resp->xdr;
	struct nfs4_stateowner *so = resp->cstate.replay_owner;
	struct svc_rqst *rqstp = resp->rqstp;
	int post_err_offset;
	nfsd4_enc encoder;
	__be32 *p;

	p = xdr_reserve_space(xdr, 8);
	if (!p) {
		WARN_ON_ONCE(1);
		return;
	}
	*p++ = cpu_to_be32(op->opnum);
	post_err_offset = xdr->buf->len;

	if (op->opnum == OP_ILLEGAL)
		goto status;
	BUG_ON(op->opnum < 0 || op->opnum >= ARRAY_SIZE(nfsd4_enc_ops) ||
	       !nfsd4_enc_ops[op->opnum]);
	encoder = nfsd4_enc_ops[op->opnum];
	op->status = encoder(resp, op->status, &op->u);
	xdr_commit_encode(xdr);

	/* nfsd4_check_resp_size guarantees enough room for error status */
	if (!op->status) {
		int space_needed = 0;
		if (!nfsd4_last_compound_op(rqstp))
			space_needed = COMPOUND_ERR_SLACK_SPACE;
		op->status = nfsd4_check_resp_size(resp, space_needed);
	}
	if (op->status == nfserr_resource && nfsd4_has_session(&resp->cstate)) {
		struct nfsd4_slot *slot = resp->cstate.slot;

		if (slot->sl_flags & NFSD4_SLOT_CACHETHIS)
			op->status = nfserr_rep_too_big_to_cache;
		else
			op->status = nfserr_rep_too_big;
	}
	if (op->status == nfserr_resource ||
	    op->status == nfserr_rep_too_big ||
	    op->status == nfserr_rep_too_big_to_cache) {
		/*
		 * The operation may have already been encoded or
		 * partially encoded.  No op returns anything additional
		 * in the case of one of these three errors, so we can
		 * just truncate back to after the status.  But it's a
		 * bug if we had to do this on a non-idempotent op:
		 */
		warn_on_nonidempotent_op(op);
		xdr_truncate_encode(xdr, post_err_offset);
	}
	if (so) {
		int len = xdr->buf->len - post_err_offset;

		so->so_replay.rp_status = op->status;
		so->so_replay.rp_buflen = len;
		read_bytes_from_xdr_buf(xdr->buf, post_err_offset,
						so->so_replay.rp_buf, len);
	}
status:
	/* Note that op->status is already in network byte order: */
	write_bytes_to_xdr_buf(xdr->buf, post_err_offset - 4, &op->status, 4);
}

/* 
 * Encode the reply stored in the stateowner reply cache 
 * 
 * XDR note: do not encode rp->rp_buflen: the buffer contains the
 * previously sent already encoded operation.
 */
void
nfsd4_encode_replay(struct xdr_stream *xdr, struct nfsd4_op *op)
{
	__be32 *p;
	struct nfs4_replay *rp = op->replay;

	BUG_ON(!rp);

	p = xdr_reserve_space(xdr, 8 + rp->rp_buflen);
	if (!p) {
		WARN_ON_ONCE(1);
		return;
	}
	*p++ = cpu_to_be32(op->opnum);
	*p++ = rp->rp_status;  /* already xdr'ed */

	p = xdr_encode_opaque_fixed(p, rp->rp_buf, rp->rp_buflen);
}

int
nfs4svc_encode_voidres(struct svc_rqst *rqstp, __be32 *p, void *dummy)
{
        return xdr_ressize_check(rqstp, p);
}

int nfsd4_release_compoundargs(void *rq, __be32 *p, void *resp)
{
	struct svc_rqst *rqstp = rq;
	struct nfsd4_compoundargs *args = rqstp->rq_argp;

	if (args->ops != args->iops) {
		kfree(args->ops);
		args->ops = args->iops;
	}
	kfree(args->tmpp);
	args->tmpp = NULL;
	while (args->to_free) {
		struct svcxdr_tmpbuf *tb = args->to_free;
		args->to_free = tb->next;
		kfree(tb);
	}
	return 1;
}

int
nfs4svc_decode_compoundargs(struct svc_rqst *rqstp, __be32 *p, struct nfsd4_compoundargs *args)
{
	if (rqstp->rq_arg.head[0].iov_len % 4) {
		/* client is nuts */
		dprintk("%s: compound not properly padded! (peeraddr=%pISc xid=0x%x)",
			__func__, svc_addr(rqstp), be32_to_cpu(rqstp->rq_xid));
		return 0;
	}
	args->p = p;
	args->end = rqstp->rq_arg.head[0].iov_base + rqstp->rq_arg.head[0].iov_len;
	args->pagelist = rqstp->rq_arg.pages;
	args->pagelen = rqstp->rq_arg.page_len;
	args->tmpp = NULL;
	args->to_free = NULL;
	args->ops = args->iops;
	args->rqstp = rqstp;

	return !nfsd4_decode_compound(args);
}

int
nfs4svc_encode_compoundres(struct svc_rqst *rqstp, __be32 *p, struct nfsd4_compoundres *resp)
{
	/*
	 * All that remains is to write the tag and operation count...
	 */
	struct xdr_buf *buf = resp->xdr.buf;

	WARN_ON_ONCE(buf->len != buf->head[0].iov_len + buf->page_len +
				 buf->tail[0].iov_len);

	rqstp->rq_next_page = resp->xdr.page_ptr + 1;

	p = resp->tagp;
	*p++ = htonl(resp->taglen);
	memcpy(p, resp->tag, resp->taglen);
	p += XDR_QUADLEN(resp->taglen);
	*p++ = htonl(resp->opcnt);

	nfsd4_sequence_done(resp);
	return 1;
}

/*
 * Local variables:
 *  c-basic-offset: 8
 * End:
 */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 * segment.c - NILFS segment constructor.
 *
 * Copyright (C) 2005-2008 Nippon Telegraph and Telephone Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Written by Ryusuke Konishi <ryusuke@osrg.net>
 *
 */

#include <linux/pagemap.h>
#include <linux/buffer_head.h>
#include <linux/writeback.h>
#include <linux/bitops.h>
#include <linux/bio.h>
#include <linux/completion.h>
#include <linux/blkdev.h>
#include <linux/backing-dev.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/crc32.h>
#include <linux/pagevec.h>
#include <linux/slab.h>
#include "nilfs.h"
#include "btnode.h"
#include "page.h"
#include "segment.h"
#include "sufile.h"
#include "cpfile.h"
#include "ifile.h"
#include "segbuf.h"


/*
 * Segment constructor
 */
#define SC_N_INODEVEC	16   /* Size of locally allocated inode vector */

#define SC_MAX_SEGDELTA 64   /* Upper limit of the number of segments
				appended in collection retry loop */

/* Construction mode */
enum {
	SC_LSEG_SR = 1,	/* Make a logical segment having a super root */
	SC_LSEG_DSYNC,	/* Flush data blocks of a given file and make
			   a logical segment without a super root */
	SC_FLUSH_FILE,	/* Flush data files, leads to segment writes without
			   creating a checkpoint */
	SC_FLUSH_DAT,	/* Flush DAT file. This also creates segments without
			   a checkpoint */
};

/* Stage numbers of dirty block collection */
enum {
	NILFS_ST_INIT = 0,
	NILFS_ST_GC,		/* Collecting dirty blocks for GC */
	NILFS_ST_FILE,
	NILFS_ST_IFILE,
	NILFS_ST_CPFILE,
	NILFS_ST_SUFILE,
	NILFS_ST_DAT,
	NILFS_ST_SR,		/* Super root */
	NILFS_ST_DSYNC,		/* Data sync blocks */
	NILFS_ST_DONE,
};

#define CREATE_TRACE_POINTS
#include <trace/events/nilfs2.h>

/*
 * nilfs_sc_cstage_inc(), nilfs_sc_cstage_set(), nilfs_sc_cstage_get() are
 * wrapper functions of stage count (nilfs_sc_info->sc_stage.scnt). Users of
 * the variable must use them because transition of stage count must involve
 * trace events (trace_nilfs2_collection_stage_transition).
 *
 * nilfs_sc_cstage_get() isn't required for the above purpose because it doesn't
 * produce tracepoint events. It is provided just for making the intention
 * clear.
 */
static inline void nilfs_sc_cstage_inc(struct nilfs_sc_info *sci)
{
	sci->sc_stage.scnt++;
	trace_nilfs2_collection_stage_transition(sci);
}

static inline void nilfs_sc_cstage_set(struct nilfs_sc_info *sci, int next_scnt)
{
	sci->sc_stage.scnt = next_scnt;
	trace_nilfs2_collection_stage_transition(sci);
}

static inline int nilfs_sc_cstage_get(struct nilfs_sc_info *sci)
{
	return sci->sc_stage.scnt;
}

/* State flags of collection */
#define NILFS_CF_NODE		0x0001	/* Collecting node blocks */
#define NILFS_CF_IFILE_STARTED	0x0002	/* IFILE stage has started */
#define NILFS_CF_SUFREED	0x0004	/* segment usages has been freed */
#define NILFS_CF_HISTORY_MASK	(NILFS_CF_IFILE_STARTED | NILFS_CF_SUFREED)

/* Operations depending on the construction mode and file type */
struct nilfs_sc_operations {
	int (*collect_data)(struct nilfs_sc_info *, struct buffer_head *,
			    struct inode *);
	int (*collect_node)(struct nilfs_sc_info *, struct buffer_head *,
			    struct inode *);
	int (*collect_bmap)(struct nilfs_sc_info *, struct buffer_head *,
			    struct inode *);
	void (*write_data_binfo)(struct nilfs_sc_info *,
				 struct nilfs_segsum_pointer *,
				 union nilfs_binfo *);
	void (*write_node_binfo)(struct nilfs_sc_info *,
				 struct nilfs_segsum_pointer *,
				 union nilfs_binfo *);
};

/*
 * Other definitions
 */
static void nilfs_segctor_start_timer(struct nilfs_sc_info *);
static void nilfs_segctor_do_flush(struct nilfs_sc_info *, int);
static void nilfs_segctor_do_immediate_flush(struct nilfs_sc_info *);
static void nilfs_dispose_list(struct the_nilfs *, struct list_head *, int);

#define nilfs_cnt32_gt(a, b)   \
	(typecheck(__u32, a) && typecheck(__u32, b) && \
	 ((__s32)(b) - (__s32)(a) < 0))
#define nilfs_cnt32_ge(a, b)   \
	(typecheck(__u32, a) && typecheck(__u32, b) && \
	 ((__s32)(a) - (__s32)(b) >= 0))
#define nilfs_cnt32_lt(a, b)  nilfs_cnt32_gt(b, a)
#define nilfs_cnt32_le(a, b)  nilfs_cnt32_ge(b, a)

static int nilfs_prepare_segment_lock(struct nilfs_transaction_info *ti)
{
	struct nilfs_transaction_info *cur_ti = current->journal_info;
	void *save = NULL;

	if (cur_ti) {
		if (cur_ti->ti_magic == NILFS_TI_MAGIC)
			return ++cur_ti->ti_count;
		else {
			/*
			 * If journal_info field is occupied by other FS,
			 * it is saved and will be restored on
			 * nilfs_transaction_commit().
			 */
			printk(KERN_WARNING
			       "NILFS warning: journal info from a different "
			       "FS\n");
			save = current->journal_info;
		}
	}
	if (!ti) {
		ti = kmem_cache_alloc(nilfs_transaction_cachep, GFP_NOFS);
		if (!ti)
			return -ENOMEM;
		ti->ti_flags = NILFS_TI_DYNAMIC_ALLOC;
	} else {
		ti->ti_flags = 0;
	}
	ti->ti_count = 0;
	ti->ti_save = save;
	ti->ti_magic = NILFS_TI_MAGIC;
	current->journal_info = ti;
	return 0;
}

/**
 * nilfs_transaction_begin - start indivisible file operations.
 * @sb: super block
 * @ti: nilfs_transaction_info
 * @vacancy_check: flags for vacancy rate checks
 *
 * nilfs_transaction_begin() acquires a reader/writer semaphore, called
 * the segment semaphore, to make a segment construction and write tasks
 * exclusive.  The function is used with nilfs_transaction_commit() in pairs.
 * The region enclosed by these two functions can be nested.  To avoid a
 * deadlock, the semaphore is only acquired or released in the outermost call.
 *
 * This function allocates a nilfs_transaction_info struct to keep context
 * information on it.  It is initialized and hooked onto the current task in
 * the outermost call.  If a pre-allocated struct is given to @ti, it is used
 * instead; otherwise a new struct is assigned from a slab.
 *
 * When @vacancy_check flag is set, this function will check the amount of
 * free space, and will wait for the GC to reclaim disk space if low capacity.
 *
 * Return Value: On success, 0 is returned. On error, one of the following
 * negative error code is returned.
 *
 * %-ENOMEM - Insufficient memory available.
 *
 * %-ENOSPC - No space left on device
 */
int nilfs_transaction_begin(struct super_block *sb,
			    struct nilfs_transaction_info *ti,
			    int vacancy_check)
{
	struct the_nilfs *nilfs;
	int ret = nilfs_prepare_segment_lock(ti);
	struct nilfs_transaction_info *trace_ti;

	if (unlikely(ret < 0))
		return ret;
	if (ret > 0) {
		trace_ti = current->journal_info;

		trace_nilfs2_transaction_transition(sb, trace_ti,
				    trace_ti->ti_count, trace_ti->ti_flags,
				    TRACE_NILFS2_TRANSACTION_BEGIN);
		return 0;
	}

	sb_start_intwrite(sb);

	nilfs = sb->s_fs_info;
	down_read(&nilfs->ns_segctor_sem);
	if (vacancy_check && nilfs_near_disk_full(nilfs)) {
		up_read(&nilfs->ns_segctor_sem);
		ret = -ENOSPC;
		goto failed;
	}

	trace_ti = current->journal_info;
	trace_nilfs2_transaction_transition(sb, trace_ti, trace_ti->ti_count,
					    trace_ti->ti_flags,
					    TRACE_NILFS2_TRANSACTION_BEGIN);
	return 0;

 failed:
	ti = current->journal_info;
	current->journal_info = ti->ti_save;
	if (ti->ti_flags & NILFS_TI_DYNAMIC_ALLOC)
		kmem_cache_free(nilfs_transaction_cachep, ti);
	sb_end_intwrite(sb);
	return ret;
}

/**
 * nilfs_transaction_commit - commit indivisible file operations.
 * @sb: super block
 *
 * nilfs_transaction_commit() releases the read semaphore which is
 * acquired by nilfs_transaction_begin(). This is only performed
 * in outermost call of this function.  If a commit flag is set,
 * nilfs_transaction_commit() sets a timer to start the segment
 * constructor.  If a sync flag is set, it starts construction
 * directly.
 */
int nilfs_transaction_commit(struct super_block *sb)
{
	struct nilfs_transaction_info *ti = current->journal_info;
	struct the_nilfs *nilfs = sb->s_fs_info;
	int err = 0;

	BUG_ON(ti == NULL || ti->ti_magic != NILFS_TI_MAGIC);
	ti->ti_flags |= NILFS_TI_COMMIT;
	if (ti->ti_count > 0) {
		ti->ti_count--;
		trace_nilfs2_transaction_transition(sb, ti, ti->ti_count,
			    ti->ti_flags, TRACE_NILFS2_TRANSACTION_COMMIT);
		return 0;
	}
	if (nilfs->ns_writer) {
		struct nilfs_sc_info *sci = nilfs->ns_writer;

		if (ti->ti_flags & NILFS_TI_COMMIT)
			nilfs_segctor_start_timer(sci);
		if (atomic_read(&nilfs->ns_ndirtyblks) > sci->sc_watermark)
			nilfs_segctor_do_flush(sci, 0);
	}
	up_read(&nilfs->ns_segctor_sem);
	trace_nilfs2_transaction_transition(sb, ti, ti->ti_count,
			    ti->ti_flags, TRACE_NILFS2_TRANSACTION_COMMIT);

	current->journal_info = ti->ti_save;

	if (ti->ti_flags & NILFS_TI_SYNC)
		err = nilfs_construct_segment(sb);
	if (ti->ti_flags & NILFS_TI_DYNAMIC_ALLOC)
		kmem_cache_free(nilfs_transaction_cachep, ti);
	sb_end_intwrite(sb);
	return err;
}

void nilfs_transaction_abort(struct super_block *sb)
{
	struct nilfs_transaction_info *ti = current->journal_info;
	struct the_nilfs *nilfs = sb->s_fs_info;

	BUG_ON(ti == NULL || ti->ti_magic != NILFS_TI_MAGIC);
	if (ti->ti_count > 0) {
		ti->ti_count--;
		trace_nilfs2_transaction_transition(sb, ti, ti->ti_count,
			    ti->ti_flags, TRACE_NILFS2_TRANSACTION_ABORT);
		return;
	}
	up_read(&nilfs->ns_segctor_sem);

	trace_nilfs2_transaction_transition(sb, ti, ti->ti_count,
		    ti->ti_flags, TRACE_NILFS2_TRANSACTION_ABORT);

	current->journal_info = ti->ti_save;
	if (ti->ti_flags & NILFS_TI_DYNAMIC_ALLOC)
		kmem_cache_free(nilfs_transaction_cachep, ti);
	sb_end_intwrite(sb);
}

void nilfs_relax_pressure_in_lock(struct super_block *sb)
{
	struct the_nilfs *nilfs = sb->s_fs_info;
	struct nilfs_sc_info *sci = nilfs->ns_writer;

	if (sb->s_flags & MS_RDONLY || unlikely(!sci) || !sci->sc_flush_request)
		return;

	set_bit(NILFS_SC_PRIOR_FLUSH, &sci->sc_flags);
	up_read(&nilfs->ns_segctor_sem);

	down_write(&nilfs->ns_segctor_sem);
	if (sci->sc_flush_request &&
	    test_bit(NILFS_SC_PRIOR_FLUSH, &sci->sc_flags)) {
		struct nilfs_transaction_info *ti = current->journal_info;

		ti->ti_flags |= NILFS_TI_WRITER;
		nilfs_segctor_do_immediate_flush(sci);
		ti->ti_flags &= ~NILFS_TI_WRITER;
	}
	downgrade_write(&nilfs->ns_segctor_sem);
}

static void nilfs_transaction_lock(struct super_block *sb,
				   struct nilfs_transaction_info *ti,
				   int gcflag)
{
	struct nilfs_transaction_info *cur_ti = current->journal_info;
	struct the_nilfs *nilfs = sb->s_fs_info;
	struct nilfs_sc_info *sci = nilfs->ns_writer;

	WARN_ON(cur_ti);
	ti->ti_flags = NILFS_TI_WRITER;
	ti->ti_count = 0;
	ti->ti_save = cur_ti;
	ti->ti_magic = NILFS_TI_MAGIC;
	current->journal_info = ti;

	for (;;) {
		trace_nilfs2_transaction_transition(sb, ti, ti->ti_count,
			    ti->ti_flags, TRACE_NILFS2_TRANSACTION_TRYLOCK);

		down_write(&nilfs->ns_segctor_sem);
		if (!test_bit(NILFS_SC_PRIOR_FLUSH, &sci->sc_flags))
			break;

		nilfs_segctor_do_immediate_flush(sci);

		up_write(&nilfs->ns_segctor_sem);
		yield();
	}
	if (gcflag)
		ti->ti_flags |= NILFS_TI_GC;

	trace_nilfs2_transaction_transition(sb, ti, ti->ti_count,
			    ti->ti_flags, TRACE_NILFS2_TRANSACTION_LOCK);
}

static void nilfs_transaction_unlock(struct super_block *sb)
{
	struct nilfs_transaction_info *ti = current->journal_info;
	struct the_nilfs *nilfs = sb->s_fs_info;

	BUG_ON(ti == NULL || ti->ti_magic != NILFS_TI_MAGIC);
	BUG_ON(ti->ti_count > 0);

	up_write(&nilfs->ns_segctor_sem);
	current->journal_info = ti->ti_save;

	trace_nilfs2_transaction_transition(sb, ti, ti->ti_count,
			    ti->ti_flags, TRACE_NILFS2_TRANSACTION_UNLOCK);
}

static void *nilfs_segctor_map_segsum_entry(struct nilfs_sc_info *sci,
					    struct nilfs_segsum_pointer *ssp,
					    unsigned bytes)
{
	struct nilfs_segment_buffer *segbuf = sci->sc_curseg;
	unsigned blocksize = sci->sc_super->s_blocksize;
	void *p;

	if (unlikely(ssp->offset + bytes > blocksize)) {
		ssp->offset = 0;
		BUG_ON(NILFS_SEGBUF_BH_IS_LAST(ssp->bh,
					       &segbuf->sb_segsum_buffers));
		ssp->bh = NILFS_SEGBUF_NEXT_BH(ssp->bh);
	}
	p = ssp->bh->b_data + ssp->offset;
	ssp->offset += bytes;
	return p;
}

/**
 * nilfs_segctor_reset_segment_buffer - reset the current segment buffer
 * @sci: nilfs_sc_info
 */
static int nilfs_segctor_reset_segment_buffer(struct nilfs_sc_info *sci)
{
	struct nilfs_segment_buffer *segbuf = sci->sc_curseg;
	struct buffer_head *sumbh;
	unsigned sumbytes;
	unsigned flags = 0;
	int err;

	if (nilfs_doing_gc())
		flags = NILFS_SS_GC;
	err = nilfs_segbuf_reset(segbuf, flags, sci->sc_seg_ctime, sci->sc_cno);
	if (unlikely(err))
		return err;

	sumbh = NILFS_SEGBUF_FIRST_BH(&segbuf->sb_segsum_buffers);
	sumbytes = segbuf->sb_sum.sumbytes;
	sci->sc_finfo_ptr.bh = sumbh;  sci->sc_finfo_ptr.offset = sumbytes;
	sci->sc_binfo_ptr.bh = sumbh;  sci->sc_binfo_ptr.offset = sumbytes;
	sci->sc_blk_cnt = sci->sc_datablk_cnt = 0;
	return 0;
}

/**
 * nilfs_segctor_zeropad_segsum - zero pad the rest of the segment summary area
 * @sci: segment constructor object
 *
 * nilfs_segctor_zeropad_segsum() zero-fills unallocated space at the end of
 * the current segment summary block.
 */
static void nilfs_segctor_zeropad_segsum(struct nilfs_sc_info *sci)
{
	struct nilfs_segsum_pointer *ssp;

	ssp = sci->sc_blk_cnt > 0 ? &sci->sc_binfo_ptr : &sci->sc_finfo_ptr;
	if (ssp->offset < ssp->bh->b_size)
		memset(ssp->bh->b_data + ssp->offset, 0,
		       ssp->bh->b_size - ssp->offset);
}

static int nilfs_segctor_feed_segment(struct nilfs_sc_info *sci)
{
	sci->sc_nblk_this_inc += sci->sc_curseg->sb_sum.nblocks;
	if (NILFS_SEGBUF_IS_LAST(sci->sc_curseg, &sci->sc_segbufs))
		return -E2BIG; /* The current segment is filled up
				  (internal code) */
	nilfs_segctor_zeropad_segsum(sci);
	sci->sc_curseg = NILFS_NEXT_SEGBUF(sci->sc_curseg);
	return nilfs_segctor_reset_segment_buffer(sci);
}

static int nilfs_segctor_add_super_root(struct nilfs_sc_info *sci)
{
	struct nilfs_segment_buffer *segbuf = sci->sc_curseg;
	int err;

	if (segbuf->sb_sum.nblocks >= segbuf->sb_rest_blocks) {
		err = nilfs_segctor_feed_segment(sci);
		if (err)
			return err;
		segbuf = sci->sc_curseg;
	}
	err = nilfs_segbuf_extend_payload(segbuf, &segbuf->sb_super_root);
	if (likely(!err))
		segbuf->sb_sum.flags |= NILFS_SS_SR;
	return err;
}

/*
 * Functions for making segment summary and payloads
 */
static int nilfs_segctor_segsum_block_required(
	struct nilfs_sc_info *sci, const struct nilfs_segsum_pointer *ssp,
	unsigned binfo_size)
{
	unsigned blocksize = sci->sc_super->s_blocksize;
	/* Size of finfo and binfo is enough small against blocksize */

	return ssp->offset + binfo_size +
		(!sci->sc_blk_cnt ? sizeof(struct nilfs_finfo) : 0) >
		blocksize;
}

static void nilfs_segctor_begin_finfo(struct nilfs_sc_info *sci,
				      struct inode *inode)
{
	sci->sc_curseg->sb_sum.nfinfo++;
	sci->sc_binfo_ptr = sci->sc_finfo_ptr;
	nilfs_segctor_map_segsum_entry(
		sci, &sci->sc_binfo_ptr, sizeof(struct nilfs_finfo));

	if (NILFS_I(inode)->i_root &&
	    !test_bit(NILFS_SC_HAVE_DELTA, &sci->sc_flags))
		set_bit(NILFS_SC_HAVE_DELTA, &sci->sc_flags);
	/* skip finfo */
}

static void nilfs_segctor_end_finfo(struct nilfs_sc_info *sci,
				    struct inode *inode)
{
	struct nilfs_finfo *finfo;
	struct nilfs_inode_info *ii;
	struct nilfs_segment_buffer *segbuf;
	__u64 cno;

	if (sci->sc_blk_cnt == 0)
		return;

	ii = NILFS_I(inode);

	if (test_bit(NILFS_I_GCINODE, &ii->i_state))
		cno = ii->i_cno;
	else if (NILFS_ROOT_METADATA_FILE(inode->i_ino))
		cno = 0;
	else
		cno = sci->sc_cno;

	finfo = nilfs_segctor_map_segsum_entry(sci, &sci->sc_finfo_ptr,
						 sizeof(*finfo));
	finfo->fi_ino = cpu_to_le64(inode->i_ino);
	finfo->fi_nblocks = cpu_to_le32(sci->sc_blk_cnt);
	finfo->fi_ndatablk = cpu_to_le32(sci->sc_datablk_cnt);
	finfo->fi_cno = cpu_to_le64(cno);

	segbuf = sci->sc_curseg;
	segbuf->sb_sum.sumbytes = sci->sc_binfo_ptr.offset +
		sci->sc_super->s_blocksize * (segbuf->sb_sum.nsumblk - 1);
	sci->sc_finfo_ptr = sci->sc_binfo_ptr;
	sci->sc_blk_cnt = sci->sc_datablk_cnt = 0;
}

static int nilfs_segctor_add_file_block(struct nilfs_sc_info *sci,
					struct buffer_head *bh,
					struct inode *inode,
					unsigned binfo_size)
{
	struct nilfs_segment_buffer *segbuf;
	int required, err = 0;

 retry:
	segbuf = sci->sc_curseg;
	required = nilfs_segctor_segsum_block_required(
		sci, &sci->sc_binfo_ptr, binfo_size);
	if (segbuf->sb_sum.nblocks + required + 1 > segbuf->sb_rest_blocks) {
		nilfs_segctor_end_finfo(sci, inode);
		err = nilfs_segctor_feed_segment(sci);
		if (err)
			return err;
		goto retry;
	}
	if (unlikely(required)) {
		nilfs_segctor_zeropad_segsum(sci);
		err = nilfs_segbuf_extend_segsum(segbuf);
		if (unlikely(err))
			goto failed;
	}
	if (sci->sc_blk_cnt == 0)
		nilfs_segctor_begin_finfo(sci, inode);

	nilfs_segctor_map_segsum_entry(sci, &sci->sc_binfo_ptr, binfo_size);
	/* Substitution to vblocknr is delayed until update_blocknr() */
	nilfs_segbuf_add_file_buffer(segbuf, bh);
	sci->sc_blk_cnt++;
 failed:
	return err;
}

/*
 * Callback functions that enumerate, mark, and collect dirty blocks
 */
static int nilfs_collect_file_data(struct nilfs_sc_info *sci,
				   struct buffer_head *bh, struct inode *inode)
{
	int err;

	err = nilfs_bmap_propagate(NILFS_I(inode)->i_bmap, bh);
	if (err < 0)
		return err;

	err = nilfs_segctor_add_file_block(sci, bh, inode,
					   sizeof(struct nilfs_binfo_v));
	if (!err)
		sci->sc_datablk_cnt++;
	return err;
}

static int nilfs_collect_file_node(struct nilfs_sc_info *sci,
				   struct buffer_head *bh,
				   struct inode *inode)
{
	return nilfs_bmap_propagate(NILFS_I(inode)->i_bmap, bh);
}

static int nilfs_collect_file_bmap(struct nilfs_sc_info *sci,
				   struct buffer_head *bh,
				   struct inode *inode)
{
	WARN_ON(!buffer_dirty(bh));
	return nilfs_segctor_add_file_block(sci, bh, inode, sizeof(__le64));
}

static void nilfs_write_file_data_binfo(struct nilfs_sc_info *sci,
					struct nilfs_segsum_pointer *ssp,
					union nilfs_binfo *binfo)
{
	struct nilfs_binfo_v *binfo_v = nilfs_segctor_map_segsum_entry(
		sci, ssp, sizeof(*binfo_v));
	*binfo_v = binfo->bi_v;
}

static void nilfs_write_file_node_binfo(struct nilfs_sc_info *sci,
					struct nilfs_segsum_pointer *ssp,
					union nilfs_binfo *binfo)
{
	__le64 *vblocknr = nilfs_segctor_map_segsum_entry(
		sci, ssp, sizeof(*vblocknr));
	*vblocknr = binfo->bi_v.bi_vblocknr;
}

static struct nilfs_sc_operations nilfs_sc_file_ops = {
	.collect_data = nilfs_collect_file_data,
	.collect_node = nilfs_collect_file_node,
	.collect_bmap = nilfs_collect_file_bmap,
	.write_data_binfo = nilfs_write_file_data_binfo,
	.write_node_binfo = nilfs_write_file_node_binfo,
};

static int nilfs_collect_dat_data(struct nilfs_sc_info *sci,
				  struct buffer_head *bh, struct inode *inode)
{
	int err;

	err = nilfs_bmap_propagate(NILFS_I(inode)->i_bmap, bh);
	if (err < 0)
		return err;

	err = nilfs_segctor_add_file_block(sci, bh, inode, sizeof(__le64));
	if (!err)
		sci->sc_datablk_cnt++;
	return err;
}

static int nilfs_collect_dat_bmap(struct nilfs_sc_info *sci,
				  struct buffer_head *bh, struct inode *inode)
{
	WARN_ON(!buffer_dirty(bh));
	return nilfs_segctor_add_file_block(sci, bh, inode,
					    sizeof(struct nilfs_binfo_dat));
}

static void nilfs_write_dat_data_binfo(struct nilfs_sc_info *sci,
				       struct nilfs_segsum_pointer *ssp,
				       union nilfs_binfo *binfo)
{
	__le64 *blkoff = nilfs_segctor_map_segsum_entry(sci, ssp,
							  sizeof(*blkoff));
	*blkoff = binfo->bi_dat.bi_blkoff;
}

static void nilfs_write_dat_node_binfo(struct nilfs_sc_info *sci,
				       struct nilfs_segsum_pointer *ssp,
				       union nilfs_binfo *binfo)
{
	struct nilfs_binfo_dat *binfo_dat =
		nilfs_segctor_map_segsum_entry(sci, ssp, sizeof(*binfo_dat));
	*binfo_dat = binfo->bi_dat;
}

static struct nilfs_sc_operations nilfs_sc_dat_ops = {
	.collect_data = nilfs_collect_dat_data,
	.collect_node = nilfs_collect_file_node,
	.collect_bmap = nilfs_collect_dat_bmap,
	.write_data_binfo = nilfs_write_dat_data_binfo,
	.write_node_binfo = nilfs_write_dat_node_binfo,
};

static struct nilfs_sc_operations nilfs_sc_dsync_ops = {
	.collect_data = nilfs_collect_file_data,
	.collect_node = NULL,
	.collect_bmap = NULL,
	.write_data_binfo = nilfs_write_file_data_binfo,
	.write_node_binfo = NULL,
};

static size_t nilfs_lookup_dirty_data_buffers(struct inode *inode,
					      struct list_head *listp,
					      size_t nlimit,
					      loff_t start, loff_t end)
{
	struct address_space *mapping = inode->i_mapping;
	struct pagevec pvec;
	pgoff_t index = 0, last = ULONG_MAX;
	size_t ndirties = 0;
	int i;

	if (unlikely(start != 0 || end != LLONG_MAX)) {
		/*
		 * A valid range is given for sync-ing data pages. The
		 * range is rounded to per-page; extra dirty buffers
		 * may be included if blocksize < pagesize.
		 */
		index = start >> PAGE_SHIFT;
		last = end >> PAGE_SHIFT;
	}
	pagevec_init(&pvec, 0);
 repeat:
	if (unlikely(index > last) ||
	    !pagevec_lookup_range_tag(&pvec, mapping, &index, last,
				PAGECACHE_TAG_DIRTY))
		return ndirties;

	for (i = 0; i < pagevec_count(&pvec); i++) {
		struct buffer_head *bh, *head;
		struct page *page = pvec.pages[i];

		lock_page(page);
		if (unlikely(page->mapping != mapping)) {
			/* Exclude pages removed from the address space */
			unlock_page(page);
			continue;
		}
		if (!page_has_buffers(page))
			create_empty_buffers(page, i_blocksize(inode), 0);
		unlock_page(page);

		bh = head = page_buffers(page);
		do {
			if (!buffer_dirty(bh) || buffer_async_write(bh))
				continue;
			get_bh(bh);
			list_add_tail(&bh->b_assoc_buffers, listp);
			ndirties++;
			if (unlikely(ndirties >= nlimit)) {
				pagevec_release(&pvec);
				cond_resched();
				return ndirties;
			}
		} while (bh = bh->b_this_page, bh != head);
	}
	pagevec_release(&pvec);
	cond_resched();
	goto repeat;
}

static void nilfs_lookup_dirty_node_buffers(struct inode *inode,
					    struct list_head *listp)
{
	struct nilfs_inode_info *ii = NILFS_I(inode);
	struct address_space *mapping = &ii->i_btnode_cache;
	struct pagevec pvec;
	struct buffer_head *bh, *head;
	unsigned int i;
	pgoff_t index = 0;

	pagevec_init(&pvec, 0);

	while (pagevec_lookup_tag(&pvec, mapping, &index,
					PAGECACHE_TAG_DIRTY)) {
		for (i = 0; i < pagevec_count(&pvec); i++) {
			bh = head = page_buffers(pvec.pages[i]);
			do {
				if (buffer_dirty(bh) &&
						!buffer_async_write(bh)) {
					get_bh(bh);
					list_add_tail(&bh->b_assoc_buffers,
						      listp);
				}
				bh = bh->b_this_page;
			} while (bh != head);
		}
		pagevec_release(&pvec);
		cond_resched();
	}
}

static void nilfs_dispose_list(struct the_nilfs *nilfs,
			       struct list_head *head, int force)
{
	struct nilfs_inode_info *ii, *n;
	struct nilfs_inode_info *ivec[SC_N_INODEVEC], **pii;
	unsigned nv = 0;

	while (!list_empty(head)) {
		spin_lock(&nilfs->ns_inode_lock);
		list_for_each_entry_safe(ii, n, head, i_dirty) {
			list_del_init(&ii->i_dirty);
			if (force) {
				if (unlikely(ii->i_bh)) {
					brelse(ii->i_bh);
					ii->i_bh = NULL;
				}
			} else if (test_bit(NILFS_I_DIRTY, &ii->i_state)) {
				set_bit(NILFS_I_QUEUED, &ii->i_state);
				list_add_tail(&ii->i_dirty,
					      &nilfs->ns_dirty_files);
				continue;
			}
			ivec[nv++] = ii;
			if (nv == SC_N_INODEVEC)
				break;
		}
		spin_unlock(&nilfs->ns_inode_lock);

		for (pii = ivec; nv > 0; pii++, nv--)
			iput(&(*pii)->vfs_inode);
	}
}

static void nilfs_iput_work_func(struct work_struct *work)
{
	struct nilfs_sc_info *sci = container_of(work, struct nilfs_sc_info,
						 sc_iput_work);
	struct the_nilfs *nilfs = sci->sc_super->s_fs_info;

	nilfs_dispose_list(nilfs, &sci->sc_iput_queue, 0);
}

static int nilfs_test_metadata_dirty(struct the_nilfs *nilfs,
				     struct nilfs_root *root)
{
	int ret = 0;

	if (nilfs_mdt_fetch_dirty(root->ifile))
		ret++;
	if (nilfs_mdt_fetch_dirty(nilfs->ns_cpfile))
		ret++;
	if (nilfs_mdt_fetch_dirty(nilfs->ns_sufile))
		ret++;
	if ((ret || nilfs_doing_gc()) && nilfs_mdt_fetch_dirty(nilfs->ns_dat))
		ret++;
	return ret;
}

static int nilfs_segctor_clean(struct nilfs_sc_info *sci)
{
	return list_empty(&sci->sc_dirty_files) &&
		!test_bit(NILFS_SC_DIRTY, &sci->sc_flags) &&
		sci->sc_nfreesegs == 0 &&
		(!nilfs_doing_gc() || list_empty(&sci->sc_gc_inodes));
}

static int nilfs_segctor_confirm(struct nilfs_sc_info *sci)
{
	struct the_nilfs *nilfs = sci->sc_super->s_fs_info;
	int ret = 0;

	if (nilfs_test_metadata_dirty(nilfs, sci->sc_root))
		set_bit(NILFS_SC_DIRTY, &sci->sc_flags);

	spin_lock(&nilfs->ns_inode_lock);
	if (list_empty(&nilfs->ns_dirty_files) && nilfs_segctor_clean(sci))
		ret++;

	spin_unlock(&nilfs->ns_inode_lock);
	return ret;
}

static void nilfs_segctor_clear_metadata_dirty(struct nilfs_sc_info *sci)
{
	struct the_nilfs *nilfs = sci->sc_super->s_fs_info;

	nilfs_mdt_clear_dirty(sci->sc_root->ifile);
	nilfs_mdt_clear_dirty(nilfs->ns_cpfile);
	nilfs_mdt_clear_dirty(nilfs->ns_sufile);
	nilfs_mdt_clear_dirty(nilfs->ns_dat);
}

static int nilfs_segctor_create_checkpoint(struct nilfs_sc_info *sci)
{
	struct the_nilfs *nilfs = sci->sc_super->s_fs_info;
	struct buffer_head *bh_cp;
	struct nilfs_checkpoint *raw_cp;
	int err;

	/* XXX: this interface will be changed */
	err = nilfs_cpfile_get_checkpoint(nilfs->ns_cpfile, nilfs->ns_cno, 1,
					  &raw_cp, &bh_cp);
	if (likely(!err)) {
		/* The following code is duplicated with cpfile.  But, it is
		   needed to collect the checkpoint even if it was not newly
		   created */
		mark_buffer_dirty(bh_cp);
		nilfs_mdt_mark_dirty(nilfs->ns_cpfile);
		nilfs_cpfile_put_checkpoint(
			nilfs->ns_cpfile, nilfs->ns_cno, bh_cp);
	} else if (err == -EINVAL || err == -ENOENT) {
		nilfs_error(sci->sc_super, __func__,
			    "checkpoint creation failed due to metadata corruption.");
		err = -EIO;
	}
	return err;
}

static int nilfs_segctor_fill_in_checkpoint(struct nilfs_sc_info *sci)
{
	struct the_nilfs *nilfs = sci->sc_super->s_fs_info;
	struct buffer_head *bh_cp;
	struct nilfs_checkpoint *raw_cp;
	int err;

	err = nilfs_cpfile_get_checkpoint(nilfs->ns_cpfile, nilfs->ns_cno, 0,
					  &raw_cp, &bh_cp);
	if (unlikely(err)) {
		if (err == -EINVAL || err == -ENOENT) {
			nilfs_error(sci->sc_super, __func__,
				    "checkpoint finalization failed due to metadata corruption.");
			err = -EIO;
		}
		goto failed_ibh;
	}
	raw_cp->cp_snapshot_list.ssl_next = 0;
	raw_cp->cp_snapshot_list.ssl_prev = 0;
	raw_cp->cp_inodes_count =
		cpu_to_le64(atomic64_read(&sci->sc_root->inodes_count));
	raw_cp->cp_blocks_count =
		cpu_to_le64(atomic64_read(&sci->sc_root->blocks_count));
	raw_cp->cp_nblk_inc =
		cpu_to_le64(sci->sc_nblk_inc + sci->sc_nblk_this_inc);
	raw_cp->cp_create = cpu_to_le64(sci->sc_seg_ctime);
	raw_cp->cp_cno = cpu_to_le64(nilfs->ns_cno);

	if (test_bit(NILFS_SC_HAVE_DELTA, &sci->sc_flags))
		nilfs_checkpoint_clear_minor(raw_cp);
	else
		nilfs_checkpoint_set_minor(raw_cp);

	nilfs_write_inode_common(sci->sc_root->ifile,
				 &raw_cp->cp_ifile_inode, 1);
	nilfs_cpfile_put_checkpoint(nilfs->ns_cpfile, nilfs->ns_cno, bh_cp);
	return 0;

 failed_ibh:
	return err;
}

static void nilfs_fill_in_file_bmap(struct inode *ifile,
				    struct nilfs_inode_info *ii)

{
	struct buffer_head *ibh;
	struct nilfs_inode *raw_inode;

	if (test_bit(NILFS_I_BMAP, &ii->i_state)) {
		ibh = ii->i_bh;
		BUG_ON(!ibh);
		raw_inode = nilfs_ifile_map_inode(ifile, ii->vfs_inode.i_ino,
						  ibh);
		nilfs_bmap_write(ii->i_bmap, raw_inode);
		nilfs_ifile_unmap_inode(ifile, ii->vfs_inode.i_ino, ibh);
	}
}

static void nilfs_segctor_fill_in_file_bmap(struct nilfs_sc_info *sci)
{
	struct nilfs_inode_info *ii;

	list_for_each_entry(ii, &sci->sc_dirty_files, i_dirty) {
		nilfs_fill_in_file_bmap(sci->sc_root->ifile, ii);
		set_bit(NILFS_I_COLLECTED, &ii->i_state);
	}
}

static void nilfs_segctor_fill_in_super_root(struct nilfs_sc_info *sci,
					     struct the_nilfs *nilfs)
{
	struct buffer_head *bh_sr;
	struct nilfs_super_root *raw_sr;
	unsigned isz, srsz;

	bh_sr = NILFS_LAST_SEGBUF(&sci->sc_segbufs)->sb_super_root;

	lock_buffer(bh_sr);
	raw_sr = (struct nilfs_super_root *)bh_sr->b_data;
	isz = nilfs->ns_inode_size;
	srsz = NILFS_SR_BYTES(isz);

	raw_sr->sr_sum = 0;  /* Ensure initialization within this update */
	raw_sr->sr_bytes = cpu_to_le16(srsz);
	raw_sr->sr_nongc_ctime
		= cpu_to_le64(nilfs_doing_gc() ?
			      nilfs->ns_nongc_ctime : sci->sc_seg_ctime);
	raw_sr->sr_flags = 0;

	nilfs_write_inode_common(nilfs->ns_dat, (void *)raw_sr +
				 NILFS_SR_DAT_OFFSET(isz), 1);
	nilfs_write_inode_common(nilfs->ns_cpfile, (void *)raw_sr +
				 NILFS_SR_CPFILE_OFFSET(isz), 1);
	nilfs_write_inode_common(nilfs->ns_sufile, (void *)raw_sr +
				 NILFS_SR_SUFILE_OFFSET(isz), 1);
	memset((void *)raw_sr + srsz, 0, nilfs->ns_blocksize - srsz);
	set_buffer_uptodate(bh_sr);
	unlock_buffer(bh_sr);
}

static void nilfs_redirty_inodes(struct list_head *head)
{
	struct nilfs_inode_info *ii;

	list_for_each_entry(ii, head, i_dirty) {
		if (test_bit(NILFS_I_COLLECTED, &ii->i_state))
			clear_bit(NILFS_I_COLLECTED, &ii->i_state);
	}
}

static void nilfs_drop_collected_inodes(struct list_head *head)
{
	struct nilfs_inode_info *ii;

	list_for_each_entry(ii, head, i_dirty) {
		if (!test_and_clear_bit(NILFS_I_COLLECTED, &ii->i_state))
			continue;

		clear_bit(NILFS_I_INODE_SYNC, &ii->i_state);
		set_bit(NILFS_I_UPDATED, &ii->i_state);
	}
}

static int nilfs_segctor_apply_buffers(struct nilfs_sc_info *sci,
				       struct inode *inode,
				       struct list_head *listp,
				       int (*collect)(struct nilfs_sc_info *,
						      struct buffer_head *,
						      struct inode *))
{
	struct buffer_head *bh, *n;
	int err = 0;

	if (collect) {
		list_for_each_entry_safe(bh, n, listp, b_assoc_buffers) {
			list_del_init(&bh->b_assoc_buffers);
			err = collect(sci, bh, inode);
			brelse(bh);
			if (unlikely(err))
				goto dispose_buffers;
		}
		return 0;
	}

 dispose_buffers:
	while (!list_empty(listp)) {
		bh = list_first_entry(listp, struct buffer_head,
				      b_assoc_buffers);
		list_del_init(&bh->b_assoc_buffers);
		brelse(bh);
	}
	return err;
}

static size_t nilfs_segctor_buffer_rest(struct nilfs_sc_info *sci)
{
	/* Remaining number of blocks within segment buffer */
	return sci->sc_segbuf_nblocks -
		(sci->sc_nblk_this_inc + sci->sc_curseg->sb_sum.nblocks);
}

static int nilfs_segctor_scan_file(struct nilfs_sc_info *sci,
				   struct inode *inode,
				   struct nilfs_sc_operations *sc_ops)
{
	LIST_HEAD(data_buffers);
	LIST_HEAD(node_buffers);
	int err;

	if (!(sci->sc_stage.flags & NILFS_CF_NODE)) {
		size_t n, rest = nilfs_segctor_buffer_rest(sci);

		n = nilfs_lookup_dirty_data_buffers(
			inode, &data_buffers, rest + 1, 0, LLONG_MAX);
		if (n > rest) {
			err = nilfs_segctor_apply_buffers(
				sci, inode, &data_buffers,
				sc_ops->collect_data);
			BUG_ON(!err); /* always receive -E2BIG or true error */
			goto break_or_fail;
		}
	}
	nilfs_lookup_dirty_node_buffers(inode, &node_buffers);

	if (!(sci->sc_stage.flags & NILFS_CF_NODE)) {
		err = nilfs_segctor_apply_buffers(
			sci, inode, &data_buffers, sc_ops->collect_data);
		if (unlikely(err)) {
			/* dispose node list */
			nilfs_segctor_apply_buffers(
				sci, inode, &node_buffers, NULL);
			goto break_or_fail;
		}
		sci->sc_stage.flags |= NILFS_CF_NODE;
	}
	/* Collect node */
	err = nilfs_segctor_apply_buffers(
		sci, inode, &node_buffers, sc_ops->collect_node);
	if (unlikely(err))
		goto break_or_fail;

	nilfs_bmap_lookup_dirty_buffers(NILFS_I(inode)->i_bmap, &node_buffers);
	err = nilfs_segctor_apply_buffers(
		sci, inode, &node_buffers, sc_ops->collect_bmap);
	if (unlikely(err))
		goto break_or_fail;

	nilfs_segctor_end_finfo(sci, inode);
	sci->sc_stage.flags &= ~NILFS_CF_NODE;

 break_or_fail:
	return err;
}

static int nilfs_segctor_scan_file_dsync(struct nilfs_sc_info *sci,
					 struct inode *inode)
{
	LIST_HEAD(data_buffers);
	size_t n, rest = nilfs_segctor_buffer_rest(sci);
	int err;

	n = nilfs_lookup_dirty_data_buffers(inode, &data_buffers, rest + 1,
					    sci->sc_dsync_start,
					    sci->sc_dsync_end);

	err = nilfs_segctor_apply_buffers(sci, inode, &data_buffers,
					  nilfs_collect_file_data);
	if (!err) {
		nilfs_segctor_end_finfo(sci, inode);
		BUG_ON(n > rest);
		/* always receive -E2BIG or true error if n > rest */
	}
	return err;
}

static int nilfs_segctor_collect_blocks(struct nilfs_sc_info *sci, int mode)
{
	struct the_nilfs *nilfs = sci->sc_super->s_fs_info;
	struct list_head *head;
	struct nilfs_inode_info *ii;
	size_t ndone;
	int err = 0;

	switch (nilfs_sc_cstage_get(sci)) {
	case NILFS_ST_INIT:
		/* Pre-processes */
		sci->sc_stage.flags = 0;

		if (!test_bit(NILFS_SC_UNCLOSED, &sci->sc_flags)) {
			sci->sc_nblk_inc = 0;
			sci->sc_curseg->sb_sum.flags = NILFS_SS_LOGBGN;
			if (mode == SC_LSEG_DSYNC) {
				nilfs_sc_cstage_set(sci, NILFS_ST_DSYNC);
				goto dsync_mode;
			}
		}

		sci->sc_stage.dirty_file_ptr = NULL;
		sci->sc_stage.gc_inode_ptr = NULL;
		if (mode == SC_FLUSH_DAT) {
			nilfs_sc_cstage_set(sci, NILFS_ST_DAT);
			goto dat_stage;
		}
		nilfs_sc_cstage_inc(sci);  /* Fall through */
	case NILFS_ST_GC:
		if (nilfs_doing_gc()) {
			head = &sci->sc_gc_inodes;
			ii = list_prepare_entry(sci->sc_stage.gc_inode_ptr,
						head, i_dirty);
			list_for_each_entry_continue(ii, head, i_dirty) {
				err = nilfs_segctor_scan_file(
					sci, &ii->vfs_inode,
					&nilfs_sc_file_ops);
				if (unlikely(err)) {
					sci->sc_stage.gc_inode_ptr = list_entry(
						ii->i_dirty.prev,
						struct nilfs_inode_info,
						i_dirty);
					goto break_or_fail;
				}
				set_bit(NILFS_I_COLLECTED, &ii->i_state);
			}
			sci->sc_stage.gc_inode_ptr = NULL;
		}
		nilfs_sc_cstage_inc(sci);  /* Fall through */
	case NILFS_ST_FILE:
		head = &sci->sc_dirty_files;
		ii = list_prepare_entry(sci->sc_stage.dirty_file_ptr, head,
					i_dirty);
		list_for_each_entry_continue(ii, head, i_dirty) {
			clear_bit(NILFS_I_DIRTY, &ii->i_state);

			err = nilfs_segctor_scan_file(sci, &ii->vfs_inode,
						      &nilfs_sc_file_ops);
			if (unlikely(err)) {
				sci->sc_stage.dirty_file_ptr =
					list_entry(ii->i_dirty.prev,
						   struct nilfs_inode_info,
						   i_dirty);
				goto break_or_fail;
			}
			/* sci->sc_stage.dirty_file_ptr = NILFS_I(inode); */
			/* XXX: required ? */
		}
		sci->sc_stage.dirty_file_ptr = NULL;
		if (mode == SC_FLUSH_FILE) {
			nilfs_sc_cstage_set(sci, NILFS_ST_DONE);
			return 0;
		}
		nilfs_sc_cstage_inc(sci);
		sci->sc_stage.flags |= NILFS_CF_IFILE_STARTED;
		/* Fall through */
	case NILFS_ST_IFILE:
		err = nilfs_segctor_scan_file(sci, sci->sc_root->ifile,
					      &nilfs_sc_file_ops);
		if (unlikely(err))
			break;
		nilfs_sc_cstage_inc(sci);
		/* Creating a checkpoint */
		err = nilfs_segctor_create_checkpoint(sci);
		if (unlikely(err))
			break;
		/* Fall through */
	case NILFS_ST_CPFILE:
		err = nilfs_segctor_scan_file(sci, nilfs->ns_cpfile,
					      &nilfs_sc_file_ops);
		if (unlikely(err))
			break;
		nilfs_sc_cstage_inc(sci);  /* Fall through */
	case NILFS_ST_SUFILE:
		err = nilfs_sufile_freev(nilfs->ns_sufile, sci->sc_freesegs,
					 sci->sc_nfreesegs, &ndone);
		if (unlikely(err)) {
			nilfs_sufile_cancel_freev(nilfs->ns_sufile,
						  sci->sc_freesegs, ndone,
						  NULL);
			break;
		}
		sci->sc_stage.flags |= NILFS_CF_SUFREED;

		err = nilfs_segctor_scan_file(sci, nilfs->ns_sufile,
					      &nilfs_sc_file_ops);
		if (unlikely(err))
			break;
		nilfs_sc_cstage_inc(sci);  /* Fall through */
	case NILFS_ST_DAT:
 dat_stage:
		err = nilfs_segctor_scan_file(sci, nilfs->ns_dat,
					      &nilfs_sc_dat_ops);
		if (unlikely(err))
			break;
		if (mode == SC_FLUSH_DAT) {
			nilfs_sc_cstage_set(sci, NILFS_ST_DONE);
			return 0;
		}
		nilfs_sc_cstage_inc(sci);  /* Fall through */
	case NILFS_ST_SR:
		if (mode == SC_LSEG_SR) {
			/* Appending a super root */
			err = nilfs_segctor_add_super_root(sci);
			if (unlikely(err))
				break;
		}
		/* End of a logical segment */
		sci->sc_curseg->sb_sum.flags |= NILFS_SS_LOGEND;
		nilfs_sc_cstage_set(sci, NILFS_ST_DONE);
		return 0;
	case NILFS_ST_DSYNC:
 dsync_mode:
		sci->sc_curseg->sb_sum.flags |= NILFS_SS_SYNDT;
		ii = sci->sc_dsync_inode;
		if (!test_bit(NILFS_I_BUSY, &ii->i_state))
			break;

		err = nilfs_segctor_scan_file_dsync(sci, &ii->vfs_inode);
		if (unlikely(err))
			break;
		sci->sc_curseg->sb_sum.flags |= NILFS_SS_LOGEND;
		nilfs_sc_cstage_set(sci, NILFS_ST_DONE);
		return 0;
	case NILFS_ST_DONE:
		return 0;
	default:
		BUG();
	}

 break_or_fail:
	return err;
}

/**
 * nilfs_segctor_begin_construction - setup segment buffer to make a new log
 * @sci: nilfs_sc_info
 * @nilfs: nilfs object
 */
static int nilfs_segctor_begin_construction(struct nilfs_sc_info *sci,
					    struct the_nilfs *nilfs)
{
	struct nilfs_segment_buffer *segbuf, *prev;
	__u64 nextnum;
	int err, alloc = 0;

	segbuf = nilfs_segbuf_new(sci->sc_super);
	if (unlikely(!segbuf))
		return -ENOMEM;

	if (list_empty(&sci->sc_write_logs)) {
		nilfs_segbuf_map(segbuf, nilfs->ns_segnum,
				 nilfs->ns_pseg_offset, nilfs);
		if (segbuf->sb_rest_blocks < NILFS_PSEG_MIN_BLOCKS) {
			nilfs_shift_to_next_segment(nilfs);
			nilfs_segbuf_map(segbuf, nilfs->ns_segnum, 0, nilfs);
		}

		segbuf->sb_sum.seg_seq = nilfs->ns_seg_seq;
		nextnum = nilfs->ns_nextnum;

		if (nilfs->ns_segnum == nilfs->ns_nextnum)
			/* Start from the head of a new full segment */
			alloc++;
	} else {
		/* Continue logs */
		prev = NILFS_LAST_SEGBUF(&sci->sc_write_logs);
		nilfs_segbuf_map_cont(segbuf, prev);
		segbuf->sb_sum.seg_seq = prev->sb_sum.seg_seq;
		nextnum = prev->sb_nextnum;

		if (segbuf->sb_rest_blocks < NILFS_PSEG_MIN_BLOCKS) {
			nilfs_segbuf_map(segbuf, prev->sb_nextnum, 0, nilfs);
			segbuf->sb_sum.seg_seq++;
			alloc++;
		}
	}

	err = nilfs_sufile_mark_dirty(nilfs->ns_sufile, segbuf->sb_segnum);
	if (err)
		goto failed;

	if (alloc) {
		err = nilfs_sufile_alloc(nilfs->ns_sufile, &nextnum);
		if (err)
			goto failed;
	}
	nilfs_segbuf_set_next_segnum(segbuf, nextnum, nilfs);

	BUG_ON(!list_empty(&sci->sc_segbufs));
	list_add_tail(&segbuf->sb_list, &sci->sc_segbufs);
	sci->sc_segbuf_nblocks = segbuf->sb_rest_blocks;
	return 0;

 failed:
	nilfs_segbuf_free(segbuf);
	return err;
}

static int nilfs_segctor_extend_segments(struct nilfs_sc_info *sci,
					 struct the_nilfs *nilfs, int nadd)
{
	struct nilfs_segment_buffer *segbuf, *prev;
	struct inode *sufile = nilfs->ns_sufile;
	__u64 nextnextnum;
	LIST_HEAD(list);
	int err, ret, i;

	prev = NILFS_LAST_SEGBUF(&sci->sc_segbufs);
	/*
	 * Since the segment specified with nextnum might be allocated during
	 * the previous construction, the buffer including its segusage may
	 * not be dirty.  The following call ensures that the buffer is dirty
	 * and will pin the buffer on memory until the sufile is written.
	 */
	err = nilfs_sufile_mark_dirty(sufile, prev->sb_nextnum);
	if (unlikely(err))
		return err;

	for (i = 0; i < nadd; i++) {
		/* extend segment info */
		err = -ENOMEM;
		segbuf = nilfs_segbuf_new(sci->sc_super);
		if (unlikely(!segbuf))
			goto failed;

		/* map this buffer to region of segment on-disk */
		nilfs_segbuf_map(segbuf, prev->sb_nextnum, 0, nilfs);
		sci->sc_segbuf_nblocks += segbuf->sb_rest_blocks;

		/* allocate the next next full segment */
		err = nilfs_sufile_alloc(sufile, &nextnextnum);
		if (unlikely(err))
			goto failed_segbuf;

		segbuf->sb_sum.seg_seq = prev->sb_sum.seg_seq + 1;
		nilfs_segbuf_set_next_segnum(segbuf, nextnextnum, nilfs);

		list_add_tail(&segbuf->sb_list, &list);
		prev = segbuf;
	}
	list_splice_tail(&list, &sci->sc_segbufs);
	return 0;

 failed_segbuf:
	nilfs_segbuf_free(segbuf);
 failed:
	list_for_each_entry(segbuf, &list, sb_list) {
		ret = nilfs_sufile_free(sufile, segbuf->sb_nextnum);
		WARN_ON(ret); /* never fails */
	}
	nilfs_destroy_logs(&list);
	return err;
}

static void nilfs_free_incomplete_logs(struct list_head *logs,
				       struct the_nilfs *nilfs)
{
	struct nilfs_segment_buffer *segbuf, *prev;
	struct inode *sufile = nilfs->ns_sufile;
	int ret;

	segbuf = NILFS_FIRST_SEGBUF(logs);
	if (nilfs->ns_nextnum != segbuf->sb_nextnum) {
		ret = nilfs_sufile_free(sufile, segbuf->sb_nextnum);
		WARN_ON(ret); /* never fails */
	}
	if (atomic_read(&segbuf->sb_err)) {
		/* Case 1: The first segment failed */
		if (segbuf->sb_pseg_start != segbuf->sb_fseg_start)
			/* Case 1a:  Partial segment appended into an existing
			   segment */
			nilfs_terminate_segment(nilfs, segbuf->sb_fseg_start,
						segbuf->sb_fseg_end);
		else /* Case 1b:  New full segment */
			set_nilfs_discontinued(nilfs);
	}

	prev = segbuf;
	list_for_each_entry_continue(segbuf, logs, sb_list) {
		if (prev->sb_nextnum != segbuf->sb_nextnum) {
			ret = nilfs_sufile_free(sufile, segbuf->sb_nextnum);
			WARN_ON(ret); /* never fails */
		}
		if (atomic_read(&segbuf->sb_err) &&
		    segbuf->sb_segnum != nilfs->ns_nextnum)
			/* Case 2: extended segment (!= next) failed */
			nilfs_sufile_set_error(sufile, segbuf->sb_segnum);
		prev = segbuf;
	}
}

static void nilfs_segctor_update_segusage(struct nilfs_sc_info *sci,
					  struct inode *sufile)
{
	struct nilfs_segment_buffer *segbuf;
	unsigned long live_blocks;
	int ret;

	list_for_each_entry(segbuf, &sci->sc_segbufs, sb_list) {
		live_blocks = segbuf->sb_sum.nblocks +
			(segbuf->sb_pseg_start - segbuf->sb_fseg_start);
		ret = nilfs_sufile_set_segment_usage(sufile, segbuf->sb_segnum,
						     live_blocks,
						     sci->sc_seg_ctime);
		WARN_ON(ret); /* always succeed because the segusage is dirty */
	}
}

static void nilfs_cancel_segusage(struct list_head *logs, struct inode *sufile)
{
	struct nilfs_segment_buffer *segbuf;
	int ret;

	segbuf = NILFS_FIRST_SEGBUF(logs);
	ret = nilfs_sufile_set_segment_usage(sufile, segbuf->sb_segnum,
					     segbuf->sb_pseg_start -
					     segbuf->sb_fseg_start, 0);
	WARN_ON(ret); /* always succeed because the segusage is dirty */

	list_for_each_entry_continue(segbuf, logs, sb_list) {
		ret = nilfs_sufile_set_segment_usage(sufile, segbuf->sb_segnum,
						     0, 0);
		WARN_ON(ret); /* always succeed */
	}
}

static void nilfs_segctor_truncate_segments(struct nilfs_sc_info *sci,
					    struct nilfs_segment_buffer *last,
					    struct inode *sufile)
{
	struct nilfs_segment_buffer *segbuf = last;
	int ret;

	list_for_each_entry_continue(segbuf, &sci->sc_segbufs, sb_list) {
		sci->sc_segbuf_nblocks -= segbuf->sb_rest_blocks;
		ret = nilfs_sufile_free(sufile, segbuf->sb_nextnum);
		WARN_ON(ret);
	}
	nilfs_truncate_logs(&sci->sc_segbufs, last);
}


static int nilfs_segctor_collect(struct nilfs_sc_info *sci,
				 struct the_nilfs *nilfs, int mode)
{
	struct nilfs_cstage prev_stage = sci->sc_stage;
	int err, nadd = 1;

	/* Collection retry loop */
	for (;;) {
		sci->sc_nblk_this_inc = 0;
		sci->sc_curseg = NILFS_FIRST_SEGBUF(&sci->sc_segbufs);

		err = nilfs_segctor_reset_segment_buffer(sci);
		if (unlikely(err))
			goto failed;

		err = nilfs_segctor_collect_blocks(sci, mode);
		sci->sc_nblk_this_inc += sci->sc_curseg->sb_sum.nblocks;
		if (!err)
			break;

		if (unlikely(err != -E2BIG))
			goto failed;

		/* The current segment is filled up */
		if (mode != SC_LSEG_SR ||
		    nilfs_sc_cstage_get(sci) < NILFS_ST_CPFILE)
			break;

		nilfs_clear_logs(&sci->sc_segbufs);

		if (sci->sc_stage.flags & NILFS_CF_SUFREED) {
			err = nilfs_sufile_cancel_freev(nilfs->ns_sufile,
							sci->sc_freesegs,
							sci->sc_nfreesegs,
							NULL);
			WARN_ON(err); /* do not happen */
			sci->sc_stage.flags &= ~NILFS_CF_SUFREED;
		}

		err = nilfs_segctor_extend_segments(sci, nilfs, nadd);
		if (unlikely(err))
			return err;

		nadd = min_t(int, nadd << 1, SC_MAX_SEGDELTA);
		sci->sc_stage = prev_stage;
	}
	nilfs_segctor_zeropad_segsum(sci);
	nilfs_segctor_truncate_segments(sci, sci->sc_curseg, nilfs->ns_sufile);
	return 0;

 failed:
	return err;
}

static void nilfs_list_replace_buffer(struct buffer_head *old_bh,
				      struct buffer_head *new_bh)
{
	BUG_ON(!list_empty(&new_bh->b_assoc_buffers));

	list_replace_init(&old_bh->b_assoc_buffers, &new_bh->b_assoc_buffers);
	/* The caller must release old_bh */
}

static int
nilfs_segctor_update_payload_blocknr(struct nilfs_sc_info *sci,
				     struct nilfs_segment_buffer *segbuf,
				     int mode)
{
	struct inode *inode = NULL;
	sector_t blocknr;
	unsigned long nfinfo = segbuf->sb_sum.nfinfo;
	unsigned long nblocks = 0, ndatablk = 0;
	struct nilfs_sc_operations *sc_op = NULL;
	struct nilfs_segsum_pointer ssp;
	struct nilfs_finfo *finfo = NULL;
	union nilfs_binfo binfo;
	struct buffer_head *bh, *bh_org;
	ino_t ino = 0;
	int err = 0;

	if (!nfinfo)
		goto out;

	blocknr = segbuf->sb_pseg_start + segbuf->sb_sum.nsumblk;
	ssp.bh = NILFS_SEGBUF_FIRST_BH(&segbuf->sb_segsum_buffers);
	ssp.offset = sizeof(struct nilfs_segment_summary);

	list_for_each_entry(bh, &segbuf->sb_payload_buffers, b_assoc_buffers) {
		if (bh == segbuf->sb_super_root)
			break;
		if (!finfo) {
			finfo =	nilfs_segctor_map_segsum_entry(
				sci, &ssp, sizeof(*finfo));
			ino = le64_to_cpu(finfo->fi_ino);
			nblocks = le32_to_cpu(finfo->fi_nblocks);
			ndatablk = le32_to_cpu(finfo->fi_ndatablk);

			inode = bh->b_page->mapping->host;

			if (mode == SC_LSEG_DSYNC)
				sc_op = &nilfs_sc_dsync_ops;
			else if (ino == NILFS_DAT_INO)
				sc_op = &nilfs_sc_dat_ops;
			else /* file blocks */
				sc_op = &nilfs_sc_file_ops;
		}
		bh_org = bh;
		get_bh(bh_org);
		err = nilfs_bmap_assign(NILFS_I(inode)->i_bmap, &bh, blocknr,
					&binfo);
		if (bh != bh_org)
			nilfs_list_replace_buffer(bh_org, bh);
		brelse(bh_org);
		if (unlikely(err))
			goto failed_bmap;

		if (ndatablk > 0)
			sc_op->write_data_binfo(sci, &ssp, &binfo);
		else
			sc_op->write_node_binfo(sci, &ssp, &binfo);

		blocknr++;
		if (--nblocks == 0) {
			finfo = NULL;
			if (--nfinfo == 0)
				break;
		} else if (ndatablk > 0)
			ndatablk--;
	}
 out:
	return 0;

 failed_bmap:
	return err;
}

static int nilfs_segctor_assign(struct nilfs_sc_info *sci, int mode)
{
	struct nilfs_segment_buffer *segbuf;
	int err;

	list_for_each_entry(segbuf, &sci->sc_segbufs, sb_list) {
		err = nilfs_segctor_update_payload_blocknr(sci, segbuf, mode);
		if (unlikely(err))
			return err;
		nilfs_segbuf_fill_in_segsum(segbuf);
	}
	return 0;
}

static void nilfs_begin_page_io(struct page *page)
{
	if (!page || PageWriteback(page))
		/* For split b-tree node pages, this function may be called
		   twice.  We ignore the 2nd or later calls by this check. */
		return;

	lock_page(page);
	clear_page_dirty_for_io(page);
	set_page_writeback(page);
	unlock_page(page);
}

static void nilfs_segctor_prepare_write(struct nilfs_sc_info *sci)
{
	struct nilfs_segment_buffer *segbuf;
	struct page *bd_page = NULL, *fs_page = NULL;

	list_for_each_entry(segbuf, &sci->sc_segbufs, sb_list) {
		struct buffer_head *bh;

		list_for_each_entry(bh, &segbuf->sb_segsum_buffers,
				    b_assoc_buffers) {
			if (bh->b_page != bd_page) {
				if (bd_page) {
					lock_page(bd_page);
					clear_page_dirty_for_io(bd_page);
					set_page_writeback(bd_page);
					unlock_page(bd_page);
				}
				bd_page = bh->b_page;
			}
		}

		list_for_each_entry(bh, &segbuf->sb_payload_buffers,
				    b_assoc_buffers) {
			set_buffer_async_write(bh);
			if (bh == segbuf->sb_super_root) {
				if (bh->b_page != bd_page) {
					lock_page(bd_page);
					clear_page_dirty_for_io(bd_page);
					set_page_writeback(bd_page);
					unlock_page(bd_page);
					bd_page = bh->b_page;
				}
				break;
			}
			if (bh->b_page != fs_page) {
				nilfs_begin_page_io(fs_page);
				fs_page = bh->b_page;
			}
		}
	}
	if (bd_page) {
		lock_page(bd_page);
		clear_page_dirty_for_io(bd_page);
		set_page_writeback(bd_page);
		unlock_page(bd_page);
	}
	nilfs_begin_page_io(fs_page);
}

static int nilfs_segctor_write(struct nilfs_sc_info *sci,
			       struct the_nilfs *nilfs)
{
	int ret;

	ret = nilfs_write_logs(&sci->sc_segbufs, nilfs);
	list_splice_tail_init(&sci->sc_segbufs, &sci->sc_write_logs);
	return ret;
}

static void nilfs_end_page_io(struct page *page, int err)
{
	if (!page)
		return;

	if (buffer_nilfs_node(page_buffers(page)) && !PageWriteback(page)) {
		/*
		 * For b-tree node pages, this function may be called twice
		 * or more because they might be split in a segment.
		 */
		if (PageDirty(page)) {
			/*
			 * For pages holding split b-tree node buffers, dirty
			 * flag on the buffers may be cleared discretely.
			 * In that case, the page is once redirtied for
			 * remaining buffers, and it must be cancelled if
			 * all the buffers get cleaned later.
			 */
			lock_page(page);
			if (nilfs_page_buffers_clean(page))
				__nilfs_clear_page_dirty(page);
			unlock_page(page);
		}
		return;
	}

	if (!err) {
		if (!nilfs_page_buffers_clean(page))
			__set_page_dirty_nobuffers(page);
		ClearPageError(page);
	} else {
		__set_page_dirty_nobuffers(page);
		SetPageError(page);
	}

	end_page_writeback(page);
}

static void nilfs_abort_logs(struct list_head *logs, int err)
{
	struct nilfs_segment_buffer *segbuf;
	struct page *bd_page = NULL, *fs_page = NULL;
	struct buffer_head *bh;

	if (list_empty(logs))
		return;

	list_for_each_entry(segbuf, logs, sb_list) {
		list_for_each_entry(bh, &segbuf->sb_segsum_buffers,
				    b_assoc_buffers) {
			clear_buffer_uptodate(bh);
			if (bh->b_page != bd_page) {
				if (bd_page)
					end_page_writeback(bd_page);
				bd_page = bh->b_page;
			}
		}

		list_for_each_entry(bh, &segbuf->sb_payload_buffers,
				    b_assoc_buffers) {
			clear_buffer_async_write(bh);
			if (bh == segbuf->sb_super_root) {
				clear_buffer_uptodate(bh);
				if (bh->b_page != bd_page) {
					end_page_writeback(bd_page);
					bd_page = bh->b_page;
				}
				break;
			}
			if (bh->b_page != fs_page) {
				nilfs_end_page_io(fs_page, err);
				fs_page = bh->b_page;
			}
		}
	}
	if (bd_page)
		end_page_writeback(bd_page);

	nilfs_end_page_io(fs_page, err);
}

static void nilfs_segctor_abort_construction(struct nilfs_sc_info *sci,
					     struct the_nilfs *nilfs, int err)
{
	LIST_HEAD(logs);
	int ret;

	list_splice_tail_init(&sci->sc_write_logs, &logs);
	ret = nilfs_wait_on_logs(&logs);
	nilfs_abort_logs(&logs, ret ? : err);

	list_splice_tail_init(&sci->sc_segbufs, &logs);
	nilfs_cancel_segusage(&logs, nilfs->ns_sufile);
	nilfs_free_incomplete_logs(&logs, nilfs);

	if (sci->sc_stage.flags & NILFS_CF_SUFREED) {
		ret = nilfs_sufile_cancel_freev(nilfs->ns_sufile,
						sci->sc_freesegs,
						sci->sc_nfreesegs,
						NULL);
		WARN_ON(ret); /* do not happen */
	}

	nilfs_destroy_logs(&logs);
}

static void nilfs_set_next_segment(struct the_nilfs *nilfs,
				   struct nilfs_segment_buffer *segbuf)
{
	nilfs->ns_segnum = segbuf->sb_segnum;
	nilfs->ns_nextnum = segbuf->sb_nextnum;
	nilfs->ns_pseg_offset = segbuf->sb_pseg_start - segbuf->sb_fseg_start
		+ segbuf->sb_sum.nblocks;
	nilfs->ns_seg_seq = segbuf->sb_sum.seg_seq;
	nilfs->ns_ctime = segbuf->sb_sum.ctime;
}

static void nilfs_segctor_complete_write(struct nilfs_sc_info *sci)
{
	struct nilfs_segment_buffer *segbuf;
	struct page *bd_page = NULL, *fs_page = NULL;
	struct the_nilfs *nilfs = sci->sc_super->s_fs_info;
	int update_sr = false;

	list_for_each_entry(segbuf, &sci->sc_write_logs, sb_list) {
		struct buffer_head *bh;

		list_for_each_entry(bh, &segbuf->sb_segsum_buffers,
				    b_assoc_buffers) {
			set_buffer_uptodate(bh);
			clear_buffer_dirty(bh);
			if (bh->b_page != bd_page) {
				if (bd_page)
					end_page_writeback(bd_page);
				bd_page = bh->b_page;
			}
		}
		/*
		 * We assume that the buffers which belong to the same page
		 * continue over the buffer list.
		 * Under this assumption, the last BHs of pages is
		 * identifiable by the discontinuity of bh->b_page
		 * (page != fs_page).
		 *
		 * For B-tree node blocks, however, this assumption is not
		 * guaranteed.  The cleanup code of B-tree node pages needs
		 * special care.
		 */
		list_for_each_entry(bh, &segbuf->sb_payload_buffers,
				    b_assoc_buffers) {
			const unsigned long set_bits = (1 << BH_Uptodate);
			const unsigned long clear_bits =
				(1 << BH_Dirty | 1 << BH_Async_Write |
				 1 << BH_Delay | 1 << BH_NILFS_Volatile |
				 1 << BH_NILFS_Redirected);

			set_mask_bits(&bh->b_state, clear_bits, set_bits);
			if (bh == segbuf->sb_super_root) {
				if (bh->b_page != bd_page) {
					end_page_writeback(bd_page);
					bd_page = bh->b_page;
				}
				update_sr = true;
				break;
			}
			if (bh->b_page != fs_page) {
				nilfs_end_page_io(fs_page, 0);
				fs_page = bh->b_page;
			}
		}

		if (!nilfs_segbuf_simplex(segbuf)) {
			if (segbuf->sb_sum.flags & NILFS_SS_LOGBGN) {
				set_bit(NILFS_SC_UNCLOSED, &sci->sc_flags);
				sci->sc_lseg_stime = jiffies;
			}
			if (segbuf->sb_sum.flags & NILFS_SS_LOGEND)
				clear_bit(NILFS_SC_UNCLOSED, &sci->sc_flags);
		}
	}
	/*
	 * Since pages may continue over multiple segment buffers,
	 * end of the last page must be checked outside of the loop.
	 */
	if (bd_page)
		end_page_writeback(bd_page);

	nilfs_end_page_io(fs_page, 0);

	nilfs_drop_collected_inodes(&sci->sc_dirty_files);

	if (nilfs_doing_gc())
		nilfs_drop_collected_inodes(&sci->sc_gc_inodes);
	else
		nilfs->ns_nongc_ctime = sci->sc_seg_ctime;

	sci->sc_nblk_inc += sci->sc_nblk_this_inc;

	segbuf = NILFS_LAST_SEGBUF(&sci->sc_write_logs);
	nilfs_set_next_segment(nilfs, segbuf);

	if (update_sr) {
		nilfs->ns_flushed_device = 0;
		nilfs_set_last_segment(nilfs, segbuf->sb_pseg_start,
				       segbuf->sb_sum.seg_seq, nilfs->ns_cno++);

		clear_bit(NILFS_SC_HAVE_DELTA, &sci->sc_flags);
		clear_bit(NILFS_SC_DIRTY, &sci->sc_flags);
		set_bit(NILFS_SC_SUPER_ROOT, &sci->sc_flags);
		nilfs_segctor_clear_metadata_dirty(sci);
	} else
		clear_bit(NILFS_SC_SUPER_ROOT, &sci->sc_flags);
}

static int nilfs_segctor_wait(struct nilfs_sc_info *sci)
{
	int ret;

	ret = nilfs_wait_on_logs(&sci->sc_write_logs);
	if (!ret) {
		nilfs_segctor_complete_write(sci);
		nilfs_destroy_logs(&sci->sc_write_logs);
	}
	return ret;
}

static int nilfs_segctor_collect_dirty_files(struct nilfs_sc_info *sci,
					     struct the_nilfs *nilfs)
{
	struct nilfs_inode_info *ii, *n;
	struct inode *ifile = sci->sc_root->ifile;

	spin_lock(&nilfs->ns_inode_lock);
 retry:
	list_for_each_entry_safe(ii, n, &nilfs->ns_dirty_files, i_dirty) {
		if (!ii->i_bh) {
			struct buffer_head *ibh;
			int err;

			spin_unlock(&nilfs->ns_inode_lock);
			err = nilfs_ifile_get_inode_block(
				ifile, ii->vfs_inode.i_ino, &ibh);
			if (unlikely(err)) {
				nilfs_warning(sci->sc_super, __func__,
					      "failed to get inode block.\n");
				return err;
			}
			spin_lock(&nilfs->ns_inode_lock);
			if (likely(!ii->i_bh))
				ii->i_bh = ibh;
			else
				brelse(ibh);
			goto retry;
		}

		// Always redirty the buffer to avoid race condition
		mark_buffer_dirty(ii->i_bh);
		nilfs_mdt_mark_dirty(ifile);

		clear_bit(NILFS_I_QUEUED, &ii->i_state);
		set_bit(NILFS_I_BUSY, &ii->i_state);
		list_move_tail(&ii->i_dirty, &sci->sc_dirty_files);
	}
	spin_unlock(&nilfs->ns_inode_lock);

	return 0;
}

static void nilfs_segctor_drop_written_files(struct nilfs_sc_info *sci,
					     struct the_nilfs *nilfs)
{
	struct nilfs_inode_info *ii, *n;
	int during_mount = !(sci->sc_super->s_flags & MS_ACTIVE);
	int defer_iput = false;

	spin_lock(&nilfs->ns_inode_lock);
	list_for_each_entry_safe(ii, n, &sci->sc_dirty_files, i_dirty) {
		if (!test_and_clear_bit(NILFS_I_UPDATED, &ii->i_state) ||
		    test_bit(NILFS_I_DIRTY, &ii->i_state))
			continue;

		clear_bit(NILFS_I_BUSY, &ii->i_state);
		brelse(ii->i_bh);
		ii->i_bh = NULL;
		list_del_init(&ii->i_dirty);
		if (!ii->vfs_inode.i_nlink || during_mount) {
			/*
			 * Defer calling iput() to avoid deadlocks if
			 * i_nlink == 0 or mount is not yet finished.
			 */
			list_add_tail(&ii->i_dirty, &sci->sc_iput_queue);
			defer_iput = true;
		} else {
			spin_unlock(&nilfs->ns_inode_lock);
			iput(&ii->vfs_inode);
			spin_lock(&nilfs->ns_inode_lock);
		}
	}
	spin_unlock(&nilfs->ns_inode_lock);

	if (defer_iput)
		schedule_work(&sci->sc_iput_work);
}

/*
 * Main procedure of segment constructor
 */
static int nilfs_segctor_do_construct(struct nilfs_sc_info *sci, int mode)
{
	struct the_nilfs *nilfs = sci->sc_super->s_fs_info;
	int err;

	if (sci->sc_super->s_flags & MS_RDONLY)
		return -EROFS;

	nilfs_sc_cstage_set(sci, NILFS_ST_INIT);
	sci->sc_cno = nilfs->ns_cno;

	err = nilfs_segctor_collect_dirty_files(sci, nilfs);
	if (unlikely(err))
		goto out;

	if (nilfs_test_metadata_dirty(nilfs, sci->sc_root))
		set_bit(NILFS_SC_DIRTY, &sci->sc_flags);

	if (nilfs_segctor_clean(sci))
		goto out;

	do {
		sci->sc_stage.flags &= ~NILFS_CF_HISTORY_MASK;

		err = nilfs_segctor_begin_construction(sci, nilfs);
		if (unlikely(err))
			goto out;

		/* Update time stamp */
		sci->sc_seg_ctime = get_seconds();

		err = nilfs_segctor_collect(sci, nilfs, mode);
		if (unlikely(err))
			goto failed;

		/* Avoid empty segment */
		if (nilfs_sc_cstage_get(sci) == NILFS_ST_DONE &&
		    nilfs_segbuf_empty(sci->sc_curseg)) {
			nilfs_segctor_abort_construction(sci, nilfs, 1);
			goto out;
		}

		err = nilfs_segctor_assign(sci, mode);
		if (unlikely(err))
			goto failed;

		if (sci->sc_stage.flags & NILFS_CF_IFILE_STARTED)
			nilfs_segctor_fill_in_file_bmap(sci);

		if (mode == SC_LSEG_SR &&
		    nilfs_sc_cstage_get(sci) >= NILFS_ST_CPFILE) {
			err = nilfs_segctor_fill_in_checkpoint(sci);
			if (unlikely(err))
				goto failed_to_write;

			nilfs_segctor_fill_in_super_root(sci, nilfs);
		}
		nilfs_segctor_update_segusage(sci, nilfs->ns_sufile);

		/* Write partial segments */
		nilfs_segctor_prepare_write(sci);

		nilfs_add_checksums_on_logs(&sci->sc_segbufs,
					    nilfs->ns_crc_seed);

		err = nilfs_segctor_write(sci, nilfs);
		if (unlikely(err))
			goto failed_to_write;

		if (nilfs_sc_cstage_get(sci) == NILFS_ST_DONE ||
		    nilfs->ns_blocksize_bits != PAGE_CACHE_SHIFT) {
			/*
			 * At this point, we avoid double buffering
			 * for blocksize < pagesize because page dirty
			 * flag is turned off during write and dirty
			 * buffers are not properly collected for
			 * pages crossing over segments.
			 */
			err = nilfs_segctor_wait(sci);
			if (err)
				goto failed_to_write;
		}
	} while (nilfs_sc_cstage_get(sci) != NILFS_ST_DONE);

 out:
	nilfs_segctor_drop_written_files(sci, nilfs);
	return err;

 failed_to_write:
	if (sci->sc_stage.flags & NILFS_CF_IFILE_STARTED)
		nilfs_redirty_inodes(&sci->sc_dirty_files);

 failed:
	if (nilfs_doing_gc())
		nilfs_redirty_inodes(&sci->sc_gc_inodes);
	nilfs_segctor_abort_construction(sci, nilfs, err);
	goto out;
}

/**
 * nilfs_segctor_start_timer - set timer of background write
 * @sci: nilfs_sc_info
 *
 * If the timer has already been set, it ignores the new request.
 * This function MUST be called within a section locking the segment
 * semaphore.
 */
static void nilfs_segctor_start_timer(struct nilfs_sc_info *sci)
{
	spin_lock(&sci->sc_state_lock);
	if (!(sci->sc_state & NILFS_SEGCTOR_COMMIT)) {
		sci->sc_timer.expires = jiffies + sci->sc_interval;
		add_timer(&sci->sc_timer);
		sci->sc_state |= NILFS_SEGCTOR_COMMIT;
	}
	spin_unlock(&sci->sc_state_lock);
}

static void nilfs_segctor_do_flush(struct nilfs_sc_info *sci, int bn)
{
	spin_lock(&sci->sc_state_lock);
	if (!(sci->sc_flush_request & (1 << bn))) {
		unsigned long prev_req = sci->sc_flush_request;

		sci->sc_flush_request |= (1 << bn);
		if (!prev_req)
			wake_up(&sci->sc_wait_daemon);
	}
	spin_unlock(&sci->sc_state_lock);
}

/**
 * nilfs_flush_segment - trigger a segment construction for resource control
 * @sb: super block
 * @ino: inode number of the file to be flushed out.
 */
void nilfs_flush_segment(struct super_block *sb, ino_t ino)
{
	struct the_nilfs *nilfs = sb->s_fs_info;
	struct nilfs_sc_info *sci = nilfs->ns_writer;

	if (!sci || nilfs_doing_construction())
		return;
	nilfs_segctor_do_flush(sci, NILFS_MDT_INODE(sb, ino) ? ino : 0);
					/* assign bit 0 to data files */
}

struct nilfs_segctor_wait_request {
	wait_queue_t	wq;
	__u32		seq;
	int		err;
	atomic_t	done;
};

static int nilfs_segctor_sync(struct nilfs_sc_info *sci)
{
	struct nilfs_segctor_wait_request wait_req;
	int err = 0;

	spin_lock(&sci->sc_state_lock);
	init_wait(&wait_req.wq);
	wait_req.err = 0;
	atomic_set(&wait_req.done, 0);
	wait_req.seq = ++sci->sc_seq_request;
	spin_unlock(&sci->sc_state_lock);

	init_waitqueue_entry(&wait_req.wq, current);
	add_wait_queue(&sci->sc_wait_request, &wait_req.wq);
	set_current_state(TASK_INTERRUPTIBLE);
	wake_up(&sci->sc_wait_daemon);

	for (;;) {
		if (atomic_read(&wait_req.done)) {
			err = wait_req.err;
			break;
		}
		if (!signal_pending(current)) {
			schedule();
			continue;
		}
		err = -ERESTARTSYS;
		break;
	}
	finish_wait(&sci->sc_wait_request, &wait_req.wq);
	return err;
}

static void nilfs_segctor_wakeup(struct nilfs_sc_info *sci, int err)
{
	struct nilfs_segctor_wait_request *wrq, *n;
	unsigned long flags;

	spin_lock_irqsave(&sci->sc_wait_request.lock, flags);
	list_for_each_entry_safe(wrq, n, &sci->sc_wait_request.task_list,
				 wq.task_list) {
		if (!atomic_read(&wrq->done) &&
		    nilfs_cnt32_ge(sci->sc_seq_done, wrq->seq)) {
			wrq->err = err;
			atomic_set(&wrq->done, 1);
		}
		if (atomic_read(&wrq->done)) {
			wrq->wq.func(&wrq->wq,
				     TASK_UNINTERRUPTIBLE | TASK_INTERRUPTIBLE,
				     0, NULL);
		}
	}
	spin_unlock_irqrestore(&sci->sc_wait_request.lock, flags);
}

/**
 * nilfs_construct_segment - construct a logical segment
 * @sb: super block
 *
 * Return Value: On success, 0 is retured. On errors, one of the following
 * negative error code is returned.
 *
 * %-EROFS - Read only filesystem.
 *
 * %-EIO - I/O error
 *
 * %-ENOSPC - No space left on device (only in a panic state).
 *
 * %-ERESTARTSYS - Interrupted.
 *
 * %-ENOMEM - Insufficient memory available.
 */
int nilfs_construct_segment(struct super_block *sb)
{
	struct the_nilfs *nilfs = sb->s_fs_info;
	struct nilfs_sc_info *sci = nilfs->ns_writer;
	struct nilfs_transaction_info *ti;
	int err;

	if (sb->s_flags & MS_RDONLY || unlikely(!sci))
		return -EROFS;

	/* A call inside transactions causes a deadlock. */
	BUG_ON((ti = current->journal_info) && ti->ti_magic == NILFS_TI_MAGIC);

	err = nilfs_segctor_sync(sci);
	return err;
}

/**
 * nilfs_construct_dsync_segment - construct a data-only logical segment
 * @sb: super block
 * @inode: inode whose data blocks should be written out
 * @start: start byte offset
 * @end: end byte offset (inclusive)
 *
 * Return Value: On success, 0 is retured. On errors, one of the following
 * negative error code is returned.
 *
 * %-EROFS - Read only filesystem.
 *
 * %-EIO - I/O error
 *
 * %-ENOSPC - No space left on device (only in a panic state).
 *
 * %-ERESTARTSYS - Interrupted.
 *
 * %-ENOMEM - Insufficient memory available.
 */
int nilfs_construct_dsync_segment(struct super_block *sb, struct inode *inode,
				  loff_t start, loff_t end)
{
	struct the_nilfs *nilfs = sb->s_fs_info;
	struct nilfs_sc_info *sci = nilfs->ns_writer;
	struct nilfs_inode_info *ii;
	struct nilfs_transaction_info ti;
	int err = 0;

	if (sb->s_flags & MS_RDONLY || unlikely(!sci))
		return -EROFS;

	nilfs_transaction_lock(sb, &ti, 0);

	ii = NILFS_I(inode);
	if (test_bit(NILFS_I_INODE_SYNC, &ii->i_state) ||
	    nilfs_test_opt(nilfs, STRICT_ORDER) ||
	    test_bit(NILFS_SC_UNCLOSED, &sci->sc_flags) ||
	    nilfs_discontinued(nilfs)) {
		nilfs_transaction_unlock(sb);
		err = nilfs_segctor_sync(sci);
		return err;
	}

	spin_lock(&nilfs->ns_inode_lock);
	if (!test_bit(NILFS_I_QUEUED, &ii->i_state) &&
	    !test_bit(NILFS_I_BUSY, &ii->i_state)) {
		spin_unlock(&nilfs->ns_inode_lock);
		nilfs_transaction_unlock(sb);
		return 0;
	}
	spin_unlock(&nilfs->ns_inode_lock);
	sci->sc_dsync_inode = ii;
	sci->sc_dsync_start = start;
	sci->sc_dsync_end = end;

	err = nilfs_segctor_do_construct(sci, SC_LSEG_DSYNC);
	if (!err)
		nilfs->ns_flushed_device = 0;

	nilfs_transaction_unlock(sb);
	return err;
}

#define FLUSH_FILE_BIT	(0x1) /* data file only */
#define FLUSH_DAT_BIT	(1 << NILFS_DAT_INO) /* DAT only */

/**
 * nilfs_segctor_accept - record accepted sequence count of log-write requests
 * @sci: segment constructor object
 */
static void nilfs_segctor_accept(struct nilfs_sc_info *sci)
{
	spin_lock(&sci->sc_state_lock);
	sci->sc_seq_accepted = sci->sc_seq_request;
	spin_unlock(&sci->sc_state_lock);
	del_timer_sync(&sci->sc_timer);
}

/**
 * nilfs_segctor_notify - notify the result of request to caller threads
 * @sci: segment constructor object
 * @mode: mode of log forming
 * @err: error code to be notified
 */
static void nilfs_segctor_notify(struct nilfs_sc_info *sci, int mode, int err)
{
	/* Clear requests (even when the construction failed) */
	spin_lock(&sci->sc_state_lock);

	if (mode == SC_LSEG_SR) {
		sci->sc_state &= ~NILFS_SEGCTOR_COMMIT;
		sci->sc_seq_done = sci->sc_seq_accepted;
		nilfs_segctor_wakeup(sci, err);
		sci->sc_flush_request = 0;
	} else {
		if (mode == SC_FLUSH_FILE)
			sci->sc_flush_request &= ~FLUSH_FILE_BIT;
		else if (mode == SC_FLUSH_DAT)
			sci->sc_flush_request &= ~FLUSH_DAT_BIT;

		/* re-enable timer if checkpoint creation was not done */
		if ((sci->sc_state & NILFS_SEGCTOR_COMMIT) &&
		    time_before(jiffies, sci->sc_timer.expires))
			add_timer(&sci->sc_timer);
	}
	spin_unlock(&sci->sc_state_lock);
}

/**
 * nilfs_segctor_construct - form logs and write them to disk
 * @sci: segment constructor object
 * @mode: mode of log forming
 */
static int nilfs_segctor_construct(struct nilfs_sc_info *sci, int mode)
{
	struct the_nilfs *nilfs = sci->sc_super->s_fs_info;
	struct nilfs_super_block **sbp;
	int err = 0;

	nilfs_segctor_accept(sci);

	if (nilfs_discontinued(nilfs))
		mode = SC_LSEG_SR;
	if (!nilfs_segctor_confirm(sci))
		err = nilfs_segctor_do_construct(sci, mode);

	if (likely(!err)) {
		if (mode != SC_FLUSH_DAT)
			atomic_set(&nilfs->ns_ndirtyblks, 0);
		if (test_bit(NILFS_SC_SUPER_ROOT, &sci->sc_flags) &&
		    nilfs_discontinued(nilfs)) {
			down_write(&nilfs->ns_sem);
			err = -EIO;
			sbp = nilfs_prepare_super(sci->sc_super,
						  nilfs_sb_will_flip(nilfs));
			if (likely(sbp)) {
				nilfs_set_log_cursor(sbp[0], nilfs);
				err = nilfs_commit_super(sci->sc_super,
							 NILFS_SB_COMMIT);
			}
			up_write(&nilfs->ns_sem);
		}
	}

	nilfs_segctor_notify(sci, mode, err);
	return err;
}

static void nilfs_construction_timeout(unsigned long data)
{
	struct task_struct *p = (struct task_struct *)data;
	wake_up_process(p);
}

static void
nilfs_remove_written_gcinodes(struct the_nilfs *nilfs, struct list_head *head)
{
	struct nilfs_inode_info *ii, *n;

	list_for_each_entry_safe(ii, n, head, i_dirty) {
		if (!test_bit(NILFS_I_UPDATED, &ii->i_state))
			continue;
		list_del_init(&ii->i_dirty);
		truncate_inode_pages(&ii->vfs_inode.i_data, 0);
		nilfs_btnode_cache_clear(&ii->i_btnode_cache);
		iput(&ii->vfs_inode);
	}
}

int nilfs_clean_segments(struct super_block *sb, struct nilfs_argv *argv,
			 void **kbufs)
{
	struct the_nilfs *nilfs = sb->s_fs_info;
	struct nilfs_sc_info *sci = nilfs->ns_writer;
	struct nilfs_transaction_info ti;
	int err;

	if (unlikely(!sci))
		return -EROFS;

	nilfs_transaction_lock(sb, &ti, 1);

	err = nilfs_mdt_save_to_shadow_map(nilfs->ns_dat);
	if (unlikely(err))
		goto out_unlock;

	err = nilfs_ioctl_prepare_clean_segments(nilfs, argv, kbufs);
	if (unlikely(err)) {
		nilfs_mdt_restore_from_shadow_map(nilfs->ns_dat);
		goto out_unlock;
	}

	sci->sc_freesegs = kbufs[4];
	sci->sc_nfreesegs = argv[4].v_nmembs;
	list_splice_tail_init(&nilfs->ns_gc_inodes, &sci->sc_gc_inodes);

	for (;;) {
		err = nilfs_segctor_construct(sci, SC_LSEG_SR);
		nilfs_remove_written_gcinodes(nilfs, &sci->sc_gc_inodes);

		if (likely(!err))
			break;

		nilfs_warning(sb, __func__,
			      "segment construction failed. (err=%d)", err);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(sci->sc_interval);
	}
	if (nilfs_test_opt(nilfs, DISCARD)) {
		int ret = nilfs_discard_segments(nilfs, sci->sc_freesegs,
						 sci->sc_nfreesegs);
		if (ret) {
			printk(KERN_WARNING
			       "NILFS warning: error %d on discard request, "
			       "turning discards off for the device\n", ret);
			nilfs_clear_opt(nilfs, DISCARD);
		}
	}

 out_unlock:
	sci->sc_freesegs = NULL;
	sci->sc_nfreesegs = 0;
	nilfs_mdt_clear_shadow_map(nilfs->ns_dat);
	nilfs_transaction_unlock(sb);
	return err;
}

static void nilfs_segctor_thread_construct(struct nilfs_sc_info *sci, int mode)
{
	struct nilfs_transaction_info ti;

	nilfs_transaction_lock(sci->sc_super, &ti, 0);
	nilfs_segctor_construct(sci, mode);

	/*
	 * Unclosed segment should be retried.  We do this using sc_timer.
	 * Timeout of sc_timer will invoke complete construction which leads
	 * to close the current logical segment.
	 */
	if (test_bit(NILFS_SC_UNCLOSED, &sci->sc_flags))
		nilfs_segctor_start_timer(sci);

	nilfs_transaction_unlock(sci->sc_super);
}

static void nilfs_segctor_do_immediate_flush(struct nilfs_sc_info *sci)
{
	int mode = 0;

	spin_lock(&sci->sc_state_lock);
	mode = (sci->sc_flush_request & FLUSH_DAT_BIT) ?
		SC_FLUSH_DAT : SC_FLUSH_FILE;
	spin_unlock(&sci->sc_state_lock);

	if (mode) {
		nilfs_segctor_do_construct(sci, mode);

		spin_lock(&sci->sc_state_lock);
		sci->sc_flush_request &= (mode == SC_FLUSH_FILE) ?
			~FLUSH_FILE_BIT : ~FLUSH_DAT_BIT;
		spin_unlock(&sci->sc_state_lock);
	}
	clear_bit(NILFS_SC_PRIOR_FLUSH, &sci->sc_flags);
}

static int nilfs_segctor_flush_mode(struct nilfs_sc_info *sci)
{
	if (!test_bit(NILFS_SC_UNCLOSED, &sci->sc_flags) ||
	    time_before(jiffies, sci->sc_lseg_stime + sci->sc_mjcp_freq)) {
		if (!(sci->sc_flush_request & ~FLUSH_FILE_BIT))
			return SC_FLUSH_FILE;
		else if (!(sci->sc_flush_request & ~FLUSH_DAT_BIT))
			return SC_FLUSH_DAT;
	}
	return SC_LSEG_SR;
}

/**
 * nilfs_segctor_thread - main loop of the segment constructor thread.
 * @arg: pointer to a struct nilfs_sc_info.
 *
 * nilfs_segctor_thread() initializes a timer and serves as a daemon
 * to execute segment constructions.
 */
static int nilfs_segctor_thread(void *arg)
{
	struct nilfs_sc_info *sci = (struct nilfs_sc_info *)arg;
	struct the_nilfs *nilfs = sci->sc_super->s_fs_info;
	int timeout = 0;

	sci->sc_timer.data = (unsigned long)current;
	sci->sc_timer.function = nilfs_construction_timeout;

	/* start sync. */
	sci->sc_task = current;
	wake_up(&sci->sc_wait_task); /* for nilfs_segctor_start_thread() */
	printk(KERN_INFO
	       "segctord starting. Construction interval = %lu seconds, "
	       "CP frequency < %lu seconds\n",
	       sci->sc_interval / HZ, sci->sc_mjcp_freq / HZ);

	spin_lock(&sci->sc_state_lock);
 loop:
	for (;;) {
		int mode;

		if (sci->sc_state & NILFS_SEGCTOR_QUIT)
			goto end_thread;

		if (timeout || sci->sc_seq_request != sci->sc_seq_done)
			mode = SC_LSEG_SR;
		else if (!sci->sc_flush_request)
			break;
		else
			mode = nilfs_segctor_flush_mode(sci);

		spin_unlock(&sci->sc_state_lock);
		nilfs_segctor_thread_construct(sci, mode);
		spin_lock(&sci->sc_state_lock);
		timeout = 0;
	}


	if (freezing(current)) {
		spin_unlock(&sci->sc_state_lock);
		try_to_freeze();
		spin_lock(&sci->sc_state_lock);
	} else {
		DEFINE_WAIT(wait);
		int should_sleep = 1;

		prepare_to_wait(&sci->sc_wait_daemon, &wait,
				TASK_INTERRUPTIBLE);

		if (sci->sc_seq_request != sci->sc_seq_done)
			should_sleep = 0;
		else if (sci->sc_flush_request)
			should_sleep = 0;
		else if (sci->sc_state & NILFS_SEGCTOR_COMMIT)
			should_sleep = time_before(jiffies,
					sci->sc_timer.expires);

		if (should_sleep) {
			spin_unlock(&sci->sc_state_lock);
			schedule();
			spin_lock(&sci->sc_state_lock);
		}
		finish_wait(&sci->sc_wait_daemon, &wait);
		timeout = ((sci->sc_state & NILFS_SEGCTOR_COMMIT) &&
			   time_after_eq(jiffies, sci->sc_timer.expires));

		if (nilfs_sb_dirty(nilfs) && nilfs_sb_need_update(nilfs))
			set_nilfs_discontinued(nilfs);
	}
	goto loop;

 end_thread:
	/* end sync. */
	sci->sc_task = NULL;
	wake_up(&sci->sc_wait_task); /* for nilfs_segctor_kill_thread() */
	spin_unlock(&sci->sc_state_lock);
	return 0;
}

static int nilfs_segctor_start_thread(struct nilfs_sc_info *sci)
{
	struct task_struct *t;

	t = kthread_run(nilfs_segctor_thread, sci, "segctord");
	if (IS_ERR(t)) {
		int err = PTR_ERR(t);

		printk(KERN_ERR "NILFS: error %d creating segctord thread\n",
		       err);
		return err;
	}
	wait_event(sci->sc_wait_task, sci->sc_task != NULL);
	return 0;
}

static void nilfs_segctor_kill_thread(struct nilfs_sc_info *sci)
	__acquires(&sci->sc_state_lock)
	__releases(&sci->sc_state_lock)
{
	sci->sc_state |= NILFS_SEGCTOR_QUIT;

	while (sci->sc_task) {
		wake_up(&sci->sc_wait_daemon);
		spin_unlock(&sci->sc_state_lock);
		wait_event(sci->sc_wait_task, sci->sc_task == NULL);
		spin_lock(&sci->sc_state_lock);
	}
}

/*
 * Setup & clean-up functions
 */
static struct nilfs_sc_info *nilfs_segctor_new(struct super_block *sb,
					       struct nilfs_root *root)
{
	struct the_nilfs *nilfs = sb->s_fs_info;
	struct nilfs_sc_info *sci;

	sci = kzalloc(sizeof(*sci), GFP_KERNEL);
	if (!sci)
		return NULL;

	sci->sc_super = sb;

	nilfs_get_root(root);
	sci->sc_root = root;

	init_waitqueue_head(&sci->sc_wait_request);
	init_waitqueue_head(&sci->sc_wait_daemon);
	init_waitqueue_head(&sci->sc_wait_task);
	spin_lock_init(&sci->sc_state_lock);
	INIT_LIST_HEAD(&sci->sc_dirty_files);
	INIT_LIST_HEAD(&sci->sc_segbufs);
	INIT_LIST_HEAD(&sci->sc_write_logs);
	INIT_LIST_HEAD(&sci->sc_gc_inodes);
	INIT_LIST_HEAD(&sci->sc_iput_queue);
	INIT_WORK(&sci->sc_iput_work, nilfs_iput_work_func);
	init_timer(&sci->sc_timer);

	sci->sc_interval = HZ * NILFS_SC_DEFAULT_TIMEOUT;
	sci->sc_mjcp_freq = HZ * NILFS_SC_DEFAULT_SR_FREQ;
	sci->sc_watermark = NILFS_SC_DEFAULT_WATERMARK;

	if (nilfs->ns_interval)
		sci->sc_interval = HZ * nilfs->ns_interval;
	if (nilfs->ns_watermark)
		sci->sc_watermark = nilfs->ns_watermark;
	return sci;
}

static void nilfs_segctor_write_out(struct nilfs_sc_info *sci)
{
	int ret, retrycount = NILFS_SC_CLEANUP_RETRY;

	/* The segctord thread was stopped and its timer was removed.
	   But some tasks remain. */
	do {
		struct nilfs_transaction_info ti;

		nilfs_transaction_lock(sci->sc_super, &ti, 0);
		ret = nilfs_segctor_construct(sci, SC_LSEG_SR);
		nilfs_transaction_unlock(sci->sc_super);

		flush_work(&sci->sc_iput_work);

	} while (ret && ret != -EROFS && retrycount-- > 0);
}

/**
 * nilfs_segctor_destroy - destroy the segment constructor.
 * @sci: nilfs_sc_info
 *
 * nilfs_segctor_destroy() kills the segctord thread and frees
 * the nilfs_sc_info struct.
 * Caller must hold the segment semaphore.
 */
static void nilfs_segctor_destroy(struct nilfs_sc_info *sci)
{
	struct the_nilfs *nilfs = sci->sc_super->s_fs_info;
	int flag;

	up_write(&nilfs->ns_segctor_sem);

	spin_lock(&sci->sc_state_lock);
	nilfs_segctor_kill_thread(sci);
	flag = ((sci->sc_state & NILFS_SEGCTOR_COMMIT) || sci->sc_flush_request
		|| sci->sc_seq_request != sci->sc_seq_done);
	spin_unlock(&sci->sc_state_lock);

	if (flush_work(&sci->sc_iput_work))
		flag = true;

	if (flag || !nilfs_segctor_confirm(sci))
		nilfs_segctor_write_out(sci);

	if (!list_empty(&sci->sc_dirty_files)) {
		nilfs_warning(sci->sc_super, __func__,
			      "dirty file(s) after the final construction\n");
		nilfs_dispose_list(nilfs, &sci->sc_dirty_files, 1);
	}

	if (!list_empty(&sci->sc_iput_queue)) {
		nilfs_warning(sci->sc_super, __func__,
			      "iput queue is not empty\n");
		nilfs_dispose_list(nilfs, &sci->sc_iput_queue, 1);
	}

	WARN_ON(!list_empty(&sci->sc_segbufs));
	WARN_ON(!list_empty(&sci->sc_write_logs));

	nilfs_put_root(sci->sc_root);

	down_write(&nilfs->ns_segctor_sem);

	del_timer_sync(&sci->sc_timer);
	kfree(sci);
}

/**
 * nilfs_attach_log_writer - attach log writer
 * @sb: super block instance
 * @root: root object of the current filesystem tree
 *
 * This allocates a log writer object, initializes it, and starts the
 * log writer.
 *
 * Return Value: On success, 0 is returned. On error, one of the following
 * negative error code is returned.
 *
 * %-ENOMEM - Insufficient memory available.
 */
int nilfs_attach_log_writer(struct super_block *sb, struct nilfs_root *root)
{
	struct the_nilfs *nilfs = sb->s_fs_info;
	int err;

	if (nilfs->ns_writer) {
		/*
		 * This happens if the filesystem is made read-only by
		 * __nilfs_error or nilfs_remount and then remounted
		 * read/write.  In these cases, reuse the existing
		 * writer.
		 */
		return 0;
	}

	nilfs->ns_writer = nilfs_segctor_new(sb, root);
	if (!nilfs->ns_writer)
		return -ENOMEM;

	inode_attach_wb(nilfs->ns_bdev->bd_inode, NULL);

	err = nilfs_segctor_start_thread(nilfs->ns_writer);
	if (unlikely(err))
		nilfs_detach_log_writer(sb);

	return err;
}

/**
 * nilfs_detach_log_writer - destroy log writer
 * @sb: super block instance
 *
 * This kills log writer daemon, frees the log writer object, and
 * destroys list of dirty files.
 */
void nilfs_detach_log_writer(struct super_block *sb)
{
	struct the_nilfs *nilfs = sb->s_fs_info;
	LIST_HEAD(garbage_list);

	down_write(&nilfs->ns_segctor_sem);
	if (nilfs->ns_writer) {
		nilfs_segctor_destroy(nilfs->ns_writer);
		nilfs->ns_writer = NULL;
	}
	set_nilfs_purging(nilfs);

	/* Force to free the list of dirty files */
	spin_lock(&nilfs->ns_inode_lock);
	if (!list_empty(&nilfs->ns_dirty_files)) {
		list_splice_init(&nilfs->ns_dirty_files, &garbage_list);
		nilfs_warning(sb, __func__,
			      "Hit dirty file after stopped log writer\n");
	}
	spin_unlock(&nilfs->ns_inode_lock);
	up_write(&nilfs->ns_segctor_sem);

	nilfs_dispose_list(nilfs, &garbage_list, 1);
	clear_nilfs_purging(nilfs);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /*
 * linux/fs/nls/nls_cp932.c
 *
 * Charset cp932 translation tables.
 * This translation table was generated automatically, the
 * original table can be download from the Microsoft website.
 * (http://www.microsoft.com/typography/unicode/unicodecp.htm)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/nls.h>
#include <linux/errno.h>

static const wchar_t c2u_81[256] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x00-0x07 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x08-0x0F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x10-0x17 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x18-0x1F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x20-0x27 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x28-0x2F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x30-0x37 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x38-0x3F */
	0x3000,0x3001,0x3002,0xFF0C,0xFF0E,0x30FB,0xFF1A,0xFF1B,/* 0x40-0x47 */
	0xFF1F,0xFF01,0x309B,0x309C,0x00B4,0xFF40,0x00A8,0xFF3E,/* 0x48-0x4F */
	0xFFE3,0xFF3F,0x30FD,0x30FE,0x309D,0x309E,0x3003,0x4EDD,/* 0x50-0x57 */
	0x3005,0x3006,0x3007,0x30FC,0x2015,0x2010,0xFF0F,0xFF3C,/* 0x58-0x5F */
	0xFF5E,0x2225,0xFF5C,0x2026,0x2025,0x2018,0x2019,0x201C,/* 0x60-0x67 */
	0x201D,0xFF08,0xFF09,0x3014,0x3015,0xFF3B,0xFF3D,0xFF5B,/* 0x68-0x6F */
	0xFF5D,0x3008,0x3009,0x300A,0x300B,0x300C,0x300D,0x300E,/* 0x70-0x77 */
	0x300F,0x3010,0x3011,0xFF0B,0xFF0D,0x00B1,0x00D7,0x0000,/* 0x78-0x7F */

	0x00F7,0xFF1D,0x2260,0xFF1C,0xFF1E,0x2266,0x2267,0x221E,/* 0x80-0x87 */
	0x2234,0x2642,0x2640,0x00B0,0x2032,0x2033,0x2103,0xFFE5,/* 0x88-0x8F */
	0xFF04,0xFFE0,0xFFE1,0xFF05,0xFF03,0xFF06,0xFF0A,0xFF20,/* 0x90-0x97 */
	0x00A7,0x2606,0x2605,0x25CB,0x25CF,0x25CE,0x25C7,0x25C6,/* 0x98-0x9F */
	0x25A1,0x25A0,0x25B3,0x25B2,0x25BD,0x25BC,0x203B,0x3012,/* 0xA0-0xA7 */
	0x2192,0x2190,0x2191,0x2193,0x3013,0x0000,0x0000,0x0000,/* 0xA8-0xAF */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0xB0-0xB7 */
	0x2208,0x220B,0x2286,0x2287,0x2282,0x2283,0x222A,0x2229,/* 0xB8-0xBF */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0xC0-0xC7 */
	0x2227,0x2228,0xFFE2,0x21D2,0x21D4,0x2200,0x2203,0x0000,/* 0xC8-0xCF */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0xD0-0xD7 */
	0x0000,0x0000,0x2220,0x22A5,0x2312,0x2202,0x2207,0x2261,/* 0xD8-0xDF */
	0x2252,0x226A,0x226B,0x221A,0x223D,0x221D,0x2235,0x222B,/* 0xE0-0xE7 */
	0x222C,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0xE8-0xEF */
	0x212B,0x2030,0x266F,0x266D,0x266A,0x2020,0x2021,0x00B6,/* 0xF0-0xF7 */
	0x0000,0x0000,0x0000,0x0000,0x25EF,0x0000,0x0000,0x0000,/* 0xF8-0xFF */
};

static const wchar_t c2u_82[256] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x00-0x07 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x08-0x0F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x10-0x17 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x18-0x1F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x20-0x27 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x28-0x2F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x30-0x37 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x38-0x3F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x40-0x47 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0xFF10,/* 0x48-0x4F */
	0xFF11,0xFF12,0xFF13,0xFF14,0xFF15,0xFF16,0xFF17,0xFF18,/* 0x50-0x57 */
	0xFF19,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x58-0x5F */
	0xFF21,0xFF22,0xFF23,0xFF24,0xFF25,0xFF26,0xFF27,0xFF28,/* 0x60-0x67 */
	0xFF29,0xFF2A,0xFF2B,0xFF2C,0xFF2D,0xFF2E,0xFF2F,0xFF30,/* 0x68-0x6F */
	0xFF31,0xFF32,0xFF33,0xFF34,0xFF35,0xFF36,0xFF37,0xFF38,/* 0x70-0x77 */
	0xFF39,0xFF3A,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x78-0x7F */

	0x0000,0xFF41,0xFF42,0xFF43,0xFF44,0xFF45,0xFF46,0xFF47,/* 0x80-0x87 */
	0xFF48,0xFF49,0xFF4A,0xFF4B,0xFF4C,0xFF4D,0xFF4E,0xFF4F,/* 0x88-0x8F */
	0xFF50,0xFF51,0xFF52,0xFF53,0xFF54,0xFF55,0xFF56,0xFF57,/* 0x90-0x97 */
	0xFF58,0xFF59,0xFF5A,0x0000,0x0000,0x0000,0x0000,0x3041,/* 0x98-0x9F */
	0x3042,0x3043,0x3044,0x3045,0x3046,0x3047,0x3048,0x3049,/* 0xA0-0xA7 */
	0x304A,0x304B,0x304C,0x304D,0x304E,0x304F,0x3050,0x3051,/* 0xA8-0xAF */
	0x3052,0x3053,0x3054,0x3055,0x3056,0x3057,0x3058,0x3059,/* 0xB0-0xB7 */
	0x305A,0x305B,0x305C,0x305D,0x305E,0x305F,0x3060,0x3061,/* 0xB8-0xBF */
	0x3062,0x3063,0x3064,0x3065,0x3066,0x3067,0x3068,0x3069,/* 0xC0-0xC7 */
	0x306A,0x306B,0x306C,0x306D,0x306E,0x306F,0x3070,0x3071,/* 0xC8-0xCF */
	0x3072,0x3073,0x3074,0x3075,0x3076,0x3077,0x3078,0x3079,/* 0xD0-0xD7 */
	0x307A,0x307B,0x307C,0x307D,0x307E,0x307F,0x3080,0x3081,/* 0xD8-0xDF */
	0x3082,0x3083,0x3084,0x3085,0x3086,0x3087,0x3088,0x3089,/* 0xE0-0xE7 */
	0x308A,0x308B,0x308C,0x308D,0x308E,0x308F,0x3090,0x3091,/* 0xE8-0xEF */
	0x3092,0x3093,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0xF0-0xF7 */
};

static const wchar_t c2u_83[256] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x00-0x07 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x08-0x0F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x10-0x17 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x18-0x1F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x20-0x27 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x28-0x2F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x30-0x37 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x38-0x3F */
	0x30A1,0x30A2,0x30A3,0x30A4,0x30A5,0x30A6,0x30A7,0x30A8,/* 0x40-0x47 */
	0x30A9,0x30AA,0x30AB,0x30AC,0x30AD,0x30AE,0x30AF,0x30B0,/* 0x48-0x4F */
	0x30B1,0x30B2,0x30B3,0x30B4,0x30B5,0x30B6,0x30B7,0x30B8,/* 0x50-0x57 */
	0x30B9,0x30BA,0x30BB,0x30BC,0x30BD,0x30BE,0x30BF,0x30C0,/* 0x58-0x5F */
	0x30C1,0x30C2,0x30C3,0x30C4,0x30C5,0x30C6,0x30C7,0x30C8,/* 0x60-0x67 */
	0x30C9,0x30CA,0x30CB,0x30CC,0x30CD,0x30CE,0x30CF,0x30D0,/* 0x68-0x6F */
	0x30D1,0x30D2,0x30D3,0x30D4,0x30D5,0x30D6,0x30D7,0x30D8,/* 0x70-0x77 */
	0x30D9,0x30DA,0x30DB,0x30DC,0x30DD,0x30DE,0x30DF,0x0000,/* 0x78-0x7F */

	0x30E0,0x30E1,0x30E2,0x30E3,0x30E4,0x30E5,0x30E6,0x30E7,/* 0x80-0x87 */
	0x30E8,0x30E9,0x30EA,0x30EB,0x30EC,0x30ED,0x30EE,0x30EF,/* 0x88-0x8F */
	0x30F0,0x30F1,0x30F2,0x30F3,0x30F4,0x30F5,0x30F6,0x0000,/* 0x90-0x97 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0391,/* 0x98-0x9F */
	0x0392,0x0393,0x0394,0x0395,0x0396,0x0397,0x0398,0x0399,/* 0xA0-0xA7 */
	0x039A,0x039B,0x039C,0x039D,0x039E,0x039F,0x03A0,0x03A1,/* 0xA8-0xAF */
	0x03A3,0x03A4,0x03A5,0x03A6,0x03A7,0x03A8,0x03A9,0x0000,/* 0xB0-0xB7 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x03B1,/* 0xB8-0xBF */
	0x03B2,0x03B3,0x03B4,0x03B5,0x03B6,0x03B7,0x03B8,0x03B9,/* 0xC0-0xC7 */
	0x03BA,0x03BB,0x03BC,0x03BD,0x03BE,0x03BF,0x03C0,0x03C1,/* 0xC8-0xCF */
	0x03C3,0x03C4,0x03C5,0x03C6,0x03C7,0x03C8,0x03C9,0x0000,/* 0xD0-0xD7 */
};

static const wchar_t c2u_84[256] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x00-0x07 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x08-0x0F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x10-0x17 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x18-0x1F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x20-0x27 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x28-0x2F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x30-0x37 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x38-0x3F */
	0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0401,0x0416,/* 0x40-0x47 */
	0x0417,0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,/* 0x48-0x4F */
	0x041F,0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,/* 0x50-0x57 */
	0x0427,0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,/* 0x58-0x5F */
	0x042F,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x60-0x67 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x68-0x6F */
	0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0451,0x0436,/* 0x70-0x77 */
	0x0437,0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x0000,/* 0x78-0x7F */

	0x043E,0x043F,0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,/* 0x80-0x87 */
	0x0446,0x0447,0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,/* 0x88-0x8F */
	0x044E,0x044F,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x90-0x97 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x2500,/* 0x98-0x9F */
	0x2502,0x250C,0x2510,0x2518,0x2514,0x251C,0x252C,0x2524,/* 0xA0-0xA7 */
	0x2534,0x253C,0x2501,0x2503,0x250F,0x2513,0x251B,0x2517,/* 0xA8-0xAF */
	0x2523,0x2533,0x252B,0x253B,0x254B,0x2520,0x252F,0x2528,/* 0xB0-0xB7 */
	0x2537,0x253F,0x251D,0x2530,0x2525,0x2538,0x2542,0x0000,/* 0xB8-0xBF */
};

static const wchar_t c2u_87[256] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x00-0x07 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x08-0x0F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x10-0x17 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x18-0x1F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x20-0x27 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x28-0x2F */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x30-0x37 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,/* 0x38-0x3F */
	0x2460,0x2461,0x2462,0x2463,0x2464,0x2465,0x2466,0x2467,/* 0x40-0x47 */
	0x2468,0x2469,0x246A,0x246B,0x246C,0x246D,0x246E,0x246F,/* 0x48-0x4F */
	0x2470,0x2471,0x2472,0x2473,0x2160,0x2161,0x2162,0x2163,/* 0x50-0x57 */
	0x2164,0x2165,0x2166,0x2167,0x2168,0x2169,0x0000,0x33