rn the path to its .py file.

    The .pyc file does not need to exist; this simply returns the path to
    the .py file calculated to correspond to the .pyc file.  If path does
    not conform to PEP 3147/488 format, ValueError will be raised. If
    sys.implementation.cache_tag is None then NotImplementedError is raised.

    NrH   FTz not bottom-level directory in rG   >   r   r9   zexpected only 2 or 3 dots in r9   r   �����z5optimization portion of filename does not start with zoptimization level z is not an alphanumeric valuerI   )r   rP   rQ   rR   r   rO   r/   rY   r   r   r   r#   r   r[   rV   �countr+   rW   rU   �	partitionr&   �SOURCE_SUFFIXES)
r,   r]   Zpycache_filenameZfound_in_pycache_prefixZstripped_pathZpycacheZ	dot_countrF   Z	opt_levelZbase_filenamer   r   r   �source_from_cachet  s<    	

�


�
rf   c              	   C   s|   t | �dkrdS | �d�\}}}|r8|�� dd� dkr<| S zt| �}W n" ttfyj   | dd� }Y n0 t|�rx|S | S )z�Convert a bytecode file path to a source path (if possible).

    This function exists purely for backwards-compatibility for
    PyImport_ExecCodeModuleWithFilenames() in the C API.

    rI   NrG   ����������Zpy)r   r)   �lowerrf   rR   rV   r6   )�bytecode_pathr_   r-   Z	extension�source_pathr   r   r   �_get_sourcefile�  s    rl   c                 C   sH   | � tt��r.z
t| �W S  ty*   Y qD0 n| � tt��r@| S d S d S �N)�endswith�tuplere   ra   rR   rX   )r`   r   r   r   �_get_cached�  s    
rp   c                 C   s2   zt | �j}W n ty$   d}Y n0 |dO }|S )z3Calculate the mode permissions for a bytecode file.r<   �   )r1   r3   r2   )r,   r4   r   r   r   �
_calc_mode�  s    
rr   c                    sB   d� fdd�	}z
t j}W n ty2   dd� }Y n0 ||� � |S )z�Decorator to verify that the module being requested matches the one the
    loader can handle.

    The first argument (self) must define _name which the second argument is
    compared against. If the comparison fails then ImportError is raised.

    Nc                    sH   |d u r| j }n | j |kr0td| j |f |d��� | |g|�R i |��S )Nzloader for %s cannot handle %s��name)rt   �ImportError)�selfrt   �args�kwargs��methodr   r   �_check_name_wrapper�  s    
��z(_check_name.<locals>._check_name_wrapperc                 S   s8   dD ] }t ||�rt| |t||�� q| j�|j� d S )N)�
__module__�__name__�__qualname__�__doc__)�hasattr�setattr�getattr�__dict__�update)ZnewZoldrC   r   r   r   �_wrap�  s    
z_check_name.<locals>._wrap)N)�
_bootstrapr�   �	NameError)rz   r{   r�   r   ry   r   �_check_name�  s    

r�   c                 C   s<   | � |�\}}|du r8t|�r8d}t�|�|d �t� |S )z�Try to find a loader for the specified module by delegating to
    self.find_loader().

    This method is deprecated in favor of finder.find_spec().

    Nz,Not importing directory {}: missing __init__rI   )�find_loaderr   rK   rL   r>   �ImportWarning)rv   �fullname�loader�portions�msgr   r   r   �_find_module_shim�  s
    
r�   c                 C   s�   | dd� }|t kr@d|�d|��}t�d|� t|fi |���t| �dk rjd|��}t�d|� t|��t| dd� �}|d	@ r�d
|�d|��}t|fi |���|S )aT  Perform basic validity checking of a pyc header and return the flags field,
    which determines how the pyc should be further validated against the source.

    *data* is the contents of the pyc file. (Only the first 16 bytes are
    required, though.)

    *name* is the name of the module being imported. It is used for logging.

    *exc_details* is a dictionary passed to ImportError if it raised for
    improved debugging.

    ImportError is raised when the magi