V_OK) {
		spin_lock(&dev->pending_lock);
		if (list_empty(&qp->iowait)) {
			dev->