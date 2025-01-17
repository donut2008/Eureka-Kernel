| dd �}t|d�r6z|�| �W S  ty4   Y n0 z
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
�zModuleSpec.__repr__c                 C   sj   | j }zH| j|jkoL| j|jkoL| j|jkoL||j koL| j|jkoL| j|jkW S  tyd   t Y S 0 d S r   )ru   r   rn   rr   �cached�has_locationrk   �NotImplemented)r