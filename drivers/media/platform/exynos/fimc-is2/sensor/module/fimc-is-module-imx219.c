re in this set but not the others.)           Remove all elements of another set from this set.               Return the intersection of two sets as a new set.

(i.e. all elements that are in both sets.)   Update a set with the intersection of itself and another.       Return True if two sets have a null intersection.               Report whether another set contains this set.   Report whether this set contains another set.   Remove and return an arbitrary set element.
Raises KeyError if the set is empty.                Remove an element from a set; it must be a member.

If the element is not a member, raise a KeyError.           S.__sizeof__() -> size of S in memory, in bytes Return the symmetric difference of two sets as a new set.

(i.e. all elements that are in exactly one of the sets.)             Update a set with the symmetric difference of itself and another.               Return the union of sets as a new set.

(i.e. all elements that are in either set.)             Update a set with the union of itself and others.               c                   @   s�  d Z dadd� Zdd� Zi Zi ZG dd� de�ZG dd	� d	�ZG d
d� d�Z	G dd� d�Z
dd� Zdd� Zdd� Zdd�dd�Zdd� Zdd� Zdd� Zdd� ZG d d!� d!�Zddd"�d#d$�Zd^d%d&�Zd'd(�d)d*�Zd+d,� Zd-d.� Zd/d0� Zd1d2� Zd3d4� Zd5d6� ZG d7d8� d8�ZG d9d:� d:�ZG d;d<� d<�Zd=d>� Z d?d@� Z!d_dAdB�Z"dCdD� Z#dEZ$e$dF Z%dGdH� Z&e'� Z(dIdJ� Z)d`dLdM�Z*d'dN�dOdP�Z+dQdR� Z,dadTdU�Z-dVdW� Z.dXdY� Z/dZd[� Z0d\d]� Z1dS )baS  Core implementation of import.

This module is NOT meant to be directly imported! It has been designed such
that it can be bootstrapped into Python as the implementation of import. As
such it requires the injection of specific modules and attributes in order to
work. One should use importlib as the public-facing version of this module.

Nc                 C   s8   dD ] }t ||�rt| |t||�� q| j�|j� dS )z/Simple substitute for functools.update_wrapper.)�
__module__�__name__�__qualname__�__doc__N)�hasattr�setattr�getattr�__dict__�update)ZnewZold�replace� r
   �<frozen importlib._bootstrap>�_wrap   s    
r   c                 C   s   t t�| �S �N)�type�sys��namer
   r
   r   �_new_module#   s    r   c                   @   s   e Zd ZdS )�_DeadlockErrorN)r   r    r   r
   r
   r
   r   r   0   s   r   c                   @   s8   e Zd ZdZdd� Zdd� Zdd� Zdd	� Zd
d� ZdS )�_ModuleLockz�A recursive lock implementation which is able to detect deadlocks
    (e.g. thread 1 trying to take locks A then B, and thread 2 trying to
    take locks B then A).
    c                 C   s0   t �� | _t �� | _|| _d | _d| _d| _d S �N�    )�_threadZallocate_lock�lock�wakeupr   �owner�count�waiters��selfr   r
   r
   r   �__init__:   s    

z_ModuleLock.__init__c                 C   sX   t �� }| j}t� }t�|�}|d u r*dS |j}||kr<dS ||v rHdS |�|� qd S )NFT)r   �	get_identr   �set�_blocking_on�get�add)r   Zme�tidZseenr   r
   r
   r   �has_deadlockB   s    
z_ModuleLock.has_deadlockc                 C   s�   t �� }| t|< z�| j�~ | jdks.| j|krZ|| _|  jd7  _W d  � W t|= dS | �� rntd|  ��| j�	d�r�|  j
d7  _
W d  � n1 s�0    Y  | j�	�  | j��  qW t|= nt|= 0 dS )z�
        Acquire the module lock.  If a potential deadlock is detected,
        a _DeadlockError is raised.
        Otherwise, the lock is always acquired and True is returned.
        r   �   NTzdeadlock detected by %rF)r   r    r"   r   r   r   r&   r   r   �acquirer   �release�r   r%   r
   r
   r   r(   W   s"    	�,
z_ModuleLock.acquirec                 C   s�   t �� }| j�l | j|kr"td��| jdks0J �|  jd8  _| jdkrld | _| jrl|  jd8  _| j��  W d   � n1 s�0    Y  d S )N�cannot release un-acquired lockr   r'   )	r   r    r   r   �RuntimeErrorr   r   r   r)   r*   r
   r
   r   r)   p   s    

z_ModuleLock.releasec                 C   s   d� | jt| ��S )Nz_ModuleLock({!r}) at {}��formatr   �id�r   r
   r
   r   �__repr__}   s    z_ModuleLock.__repr__N)	r   r    r   r   r   r&   r(   r)   r1   r
   r
   r
   r   r   4   s   r   c                   @   s0   e Zd ZdZdd� Zdd� Zdd� Zdd	� Zd
S )�_DummyModuleLockzVA simple _ModuleLock equivalent for Python builds without
    multi-threading support.c                 C   s   || _ d| _d S r   )r   r   r   r
   r
   r   r   �   s    z_DummyModuleLock.__init__c                 C   s   |  j d7  _ dS )Nr'   T)r   r0   r
   r
   r   r(   �   s    z_DummyModuleLock.acquirec                 C   s$   | j dkrtd��|  j d8  _ d S )Nr   r+   r'   )r   r,   r0   r
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
    r
   )�fr8   Zkwdsr
   r
   r   �_call_with_frames_removed�   s    rD   r'   )�	verbosityc                G   s6   t jj|kr2| �d�sd|  } t| j|� t jd� dS )z=Print the message to stderr if -v/PYTHONVERBOSE is turned on.)�#zimport z# )ZfileN)r   �flags�verbose�
startswith�printr.   �stderr)�messagerE   r8   r
   r
   r   �_verbose_message�   s    
rM   c                    s   � fdd�}t |� � |S )z1Decorator to verify the named module is built-in.c                    s&   |t jvrtd�|�|d��� | |�S )N�{!r} is not a built-in moduler   )r   �builtin_module_names�ImportErrorr.   �r   �fullname��fxnr
   r   �_requires_builtin_wrapper�   s
    

�z4_requires_builtin.<locals>._requires_builtin_wrapper�r   )rT   rU   r
   rS   r   �_requires_builtin�   s    
rW   c                    s   � fdd�}t |� � |S )z/Decorator to verify the named module is frozen.c                    s&   t �|�std�|�|d��� | |�S �Nz{!r} is not a frozen moduler   )r:   �	is_frozenrP   r.   rQ   rS   r
   r   �_requires_frozen_wrapper�   s
    

�z2_requires_frozen.<locals>._requires_frozen_wrapperrV   )rT   rZ   r
   rS   r   �_requires_frozen�   s    
r[   c                 C   s>   t || �}|tjv r2tj| }t||� tj| S t|�S dS )z�Load the specified module into sys.modules and return it.

    This method is deprecated.  Use loader.exec_module instead.

    N)�spec_from_loaderr   �modules�_exec�_load)r   rR   �spec�moduler
   r
   r   �_load_module_shim  s    




rb   c                 C   s�   t | dd �}t|d�r6z|�| �W S  ty4   Y n0 z
| j}W n tyR   Y n0 |d urdt|�S z
| j}W n ty�   d}Y n0 z
| j}W n8 ty�   |d u r�d�	|� Y S d�	||� Y S Y n0 d�	||�S d S )N�
__loader__�module_repr�?�<module {!r}>�<module {!r} ({!r})>�<module {!r} from {!r}>)
r   r   rd   �	Exception�__spec__�AttributeError�_module_repr_from_specr   �__file__r.   )ra   �loaderr`   r   �filenamer
   r
   r   �_module_repr  s.    




rp   c                   @   sr   e Zd ZdZdddd�dd�Zdd� Zdd	� Zed
d� �Zej	dd� �Zedd� �Z
edd� �Zej	dd� �ZdS )�
ModuleSpeca�  The specification for a module, used for loading.

    A module's spec is the source for information about the module.  For
    data associated with the module, including source, use the spec's
    loader.

    `name` is the absolute name of the module.  `loader` is the loader
    to use when loading the module.  `parent` is the name of the
    package the module is in.  The parent is derived from the name.

    `is_package` determines if the module is considered a package or
    not.  On modules this is reflected by the `__path__` attribute.

    `origin` is the specific location used by the loader from which to
    load the module, if that information is available.  When filename is
    set, origin will match.

    `has_location` indicates that a spec's "origin" reflects a location.
    When this is True, `__file__` attribute of the module is set.

    `cached` is the location of the cached bytecode file, if any.  It
    corresponds to the `__cached__` attribute.

    `submodule_search_locations` is the sequence of path entries to
    search when importing submodules.  If set, is_package should be
    True--and False otherwise.

    Packages are simply modules that (may) have submodules.  If a spec
    has a non-None value in `submodule_search_locations`, the import
    system will consider modules loaded from the spec as packages.

    Only finders (see importlib.abc.MetaPathFinder and
    importlib.abc.PathEntryFinder) should modify ModuleSpec instances.

    N)�origin�loader_state�
is_packagec                C   s6   || _ || _|| _|| _|r g nd | _d| _d | _d S )NF)r   rn   rr   rs   �submodule_search_locations�_set_fileattr�_cached)r   r   rn   rr   rs   rt   r
   r
   r   r   _  s    zModuleSpec.__init__c                 C   sf   d� | j�d� | j�g}| jd ur4|�d� | j�� | jd urP|�d� | j�� d� | jjd�|��S )Nz	name={!r}zloader={!r}zorigin={!r}zsubmodule_search_locations={}z{}({})z, )	r.   r   rn   rr   �appendru   �	__class__r   �join)r   r8   r
   r
   r   r1   k  s    

�

�zModuleSpec.__repr__c                 C   sj   | j }zH| j|jkoL| j|jkoL| j|jkoL||j koL| j|jkoL| j|jkW S  tyd   t Y S 0 d S r   )ru   r   rn   rr   �cached�has_locationrk   �NotImplemented)r   ZotherZsmslr
   r
   r   �__eq__u  s    
�
��
�
�zModuleSpec.__eq__c                 C   s:   | j d u r4| jd ur4| jr4td u r&t�t�| j�| _ | j S r   )rw   rr   rv   �_bootstrap_external�NotImplementedErrorZ_get_cachedr0   r
   r
   r   r{   �  s    
zModuleSpec.cachedc                 C   s
   || _ d S r   )rw   )r   r{   r
   r
   r   r{   �  s    c                 C   s$   | j du r| j�d�d S | jS dS )z The name of the module's parent.N�.r   )ru   r   �
rpartitionr0   r
   r
   r   �parent�  s    
zModuleSpec.parentc                 C   s   | j S r   )rv   r0   r
   r
   r   r|   �  s    zModuleSpec.has_locationc                 C   s   t |�| _d S r   )�boolrv   )r   �valuer
   r
   r   r|   �  s    )r   r    r   r   r   r1   r~   �propertyr{   �setterr�   r|   r
   r
   r
   r   rq   :  s    $�




rq   �rr   rt   c                C   s�   t |d�rJtdu rt�tj}|du r0|| |d�S |r8g nd}|| ||d�S |du r�t |d�r�z|�| �}W q� ty�   d}Y q�0 nd}t| |||d�S )z5Return a module spec based on various loader methods.Zget_filenameN)rn   )rn   ru   rt   Fr�   )r   r   r�   �spec_from_file_locationrt   rP   rq   )r   rn   rr   rt   r�   Zsearchr
   r
   r   r\   �  s$    
�
r\   c                 C   s*  z
| j }W n ty   Y n0 |d ur*|S | j}|d u rVz
| j}W n tyT   Y n0 z
| j}W n tyv   d }Y n0 |d u r�|d u r�z
|j}W q� ty�   d }Y q�0 n|}z
| j}W n ty�   d }Y n0 zt| j�}W n ty�   d }Y n0 t	|||d�}|d u �rdnd|_
||_||_|S )N�rr   FT)rj   rk   r   rc   rm   �_ORIGIN�
__cached__�list�__path__rq   rv   r{   ru   )ra   rn   rr   r`   r   Zlocationr{   ru   r
   r
   r   �_spec_from_module�  sH    







r�   F��overridec                C   s�  |st |dd �d u r4z| j|_W n ty2   Y n0 |sHt |dd �d u r�| j}|d u r�| jd ur�td u rlt�tj}|�	|�}| j|_
|| _d |_z
||_W n ty�   Y n0 |s�t |dd �d u r�z| j|_W n ty�   Y n0 z
| |_W n ty�   Y n0 |�st |dd �d u �rF| jd u�rFz| j|_W n t�yD   Y n0 | j�r�|�sft |dd �d u �r�z| j|_W n t�y�   Y n0 |�s�t |dd �d u �r�| jd u�r�z| j|_W n t�y�   Y n0 |S )Nr   rc   �__package__r�   rm   r�   )r   r   r   rk   rn   ru   r   r�   �_NamespaceLoader�__new__Z_pathrm   rc   r�   r�   rj   r�   r|   rr   r{   r�   )r`   ra   r�   rn   r�   r
   r
   r   �_init_module_attrs�  s`    



r�   c                 C   sR   d}t | jd�r| j�| �}nt | jd�r2td��|du rDt| j�}t| |� |S )z+Create a module based on the provided spec.N�create_module�exec_modulezBloaders that define exec_module() must also define create_module())r   rn   r�   rP   r   r   r�   �r`   ra   r
   r
   r   �module_from_spec.  s    

r�   c                 C   sj   | j du rdn| j }| jdu rB| jdu r2d�|�S d�|| j�S n$| jrVd�|| j�S d�| j | j�S dS )z&Return the repr to use for the module.Nre   rf   rg   rh   �<module {!r} ({})>)r   rr   rn   r.   r|   )r`   r   r
   r
   r   rl   ?  s    


rl   c              
   C   s�   | j }t|��� tj�|�|ur6d�|�}t||d��z�| jdu rj| jdu rZtd| j d��t	| |dd� n4t	| |dd� t
| jd�s�| j�|� n| j�|� W tj�| j �}|tj| j < ntj�| j �}|tj| j < 0 W d  � n1 s�0    Y  |S )zFExecute the spec's specified module in an existing module's namespace.zmodule {!r} not in sys.modulesr   N�missing loaderTr�   r�   )r   r3   r   r]   r#   r.   rP   rn   ru   r�   r   �load_moduler�   �pop)r`   ra   r   �msgr
   r
   r   r^   P  s&    



�,r^   c                 C   s  z| j �| j� W n4   | jtjv r@tj�| j�}|tj| j< � Y n0 tj�| j�}|tj| j< t|dd �d u r�z| j |_W n ty�   Y n0 t|dd �d u r�z(|j	|_
t|d�s�| j�d�d |_
W n ty�   Y n0 t|dd �d u �rz
| |_W n t�y   Y n0 |S )Nrc   r�   r�   r�   r   rj   )rn   r�   r   r   r]   r�   r   rc   rk   r   r�   r   r�   rj   r�   r
   r
   r   �_load_backward_compatiblen  s6    

r�   c                 C   s�   | j d urt| j d�st| �S t| �}d| _z�|tj| j< z4| j d u r`| jd u rlt	d| jd��n| j �
|