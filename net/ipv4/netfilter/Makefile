  C   s$   | j dkrtd��|  j d8  _ d S )Nr   r+   r'   )r   r,   r0   r
   r
   r   r)   �   s    
z_DummyModuleLock.releasec                 C   s   d� | jt| ��S )Nz_DummyModuleLock({!r}) at {}r-   r0   r
   r
   r   r1   �   s    z_DummyModuleLock.__repr__N)r   r    r   r   r   r(   r)   r1   r
   r
   r
   r   r2   �   s
   r2   c                   @   s$   e Zd Zdd� Zdd� Zdd� ZdS )�_ModuleLockManagerc                 C   s   || _ d | _d S r   )�_name�_lockr   r
   r
   r   r   �   s    z_ModuleLockManager.__init__c                 C   s   t | j�| _| j��  d S r   )�_get_module_lockr4   r5   r(   r0   r
   r
   r   �	__enter__�   s    z_ModuleLockManager.__enter__c                 O   s   | j ��  d S r   )r5   r)   )r   �argsZkwargsr
   r
   r   �__exit__�   s    z_ModuleLockManager.__exit__N)r   r    r   r   r7   r9   r
   r
   r
   r   r3   �   s   r3   c                 C   s�   t ��  zpzt|  � }W n ty.   d}Y n0 |du rntdu rJt| �}nt| �}| fdd�}t�||�t| < W t �	�  n
t �	�  0 |S )z�Get or create the module lock for a given module name.

    Acquire/release internally the global import lock to protect
    _module_locks.Nc                 S   s8   t ��  z t�|�| u rt|= W t ��  n
t ��  0 d S r   )�_imp�acquire_lock�_module_locksr#   �release_lock)�refr   r
   r
   r   �cb�   s
    z_get_module_lock.<locals>.cb)
r:   r;   r<   �KeyErrorr   r2   r   �_weakrefr>   r=   )r   r   r?   r
   r
   r   r6   �   s    

r6   c                 C   s4   t | �}z|��  W n ty&   Y n
0 |��  dS )z�Acquires then releases the module lock for a given module name.

    This is used to ensure a module is completely initialized, in the
    event it is being imported by another thread.
    N)r6   r(   r   r)   )r   r   r
   r
   r   �_lock_unlock_module�   s    rB   c                 O   s   | |i |��S )a.  remove_importlib_frames in import.c will always remove sequences
    of importlib frames that end with a call to this function

    Use it instead of a normal call in places where including the importlib
    frames introduces unwanted noise into the traceback (e.g. when executing
    module code)
   