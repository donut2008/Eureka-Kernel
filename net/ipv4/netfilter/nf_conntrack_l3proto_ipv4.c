this method allows for the writing of bytecode files.
        Nr   )rv   r,   r   r   r   r   r�   ?  s    zSourceLoader.set_datac              
   C   sT   | � |�}z| �|�}W n2 tyJ } ztd|d�|�W Y d}~n
d}~0 0 t|�S )z4Concrete implementation of InspectLoader.get_source.z'source not available through get_data()rs   N)r�   �get_datar2   ru   r�   )rv   r�   r,   r�   �excr   r   r   �
get_sourceF  s    
��zSourceLoader.get_sourcerh   )�	_optimizec                C   s   t jt||dd|d�S )z�Return the code object compiled from source.

        The 'data' argument can be any object type that compile() supports.
        r�   T)�dont_inheritrS   )r�   r�   �compile)rv   r   r,   r�   r   r   r   �source_to_codeP  s    �zSourceLoader.source_to_codec              	   C   s  | � |�}d}d}d}d}d}zt|�}W n tyB   d}Y �n*0 z| �|�}	W n tyf   Y �n0 t|	d �}z| �|�}
W n ty�   Y n�0 ||d�}z�t|
||�}t|
�dd� }|d@ dk}|�r|d	@ dk}t	j
d
k�r2|s�t	j
dk�r2| �|�}t	�t|�}t|
|||� nt|
||	d ||� W n ttf�yL   Y n 0 t�d||� t||||d�S |du �r�| �|�}| �||�}t�d|� tj�s|du�r|du�r|�r�|du �r�t	�|�}t|||�}
nt||t|��}
z| �|||
� W n t�y   Y n0 |S )z�Concrete implementation of InspectLoader.get_code.

        Reading of bytecode requires path_stats to be implemented. To write
        bytecode, set_data must also be implemented.

        NFTr�   r�   r�   r'   rI   r   ZneverZalways�sizez{} matches {})rt   rj   rk   zcode object from {})r�   ra   rR   r�   r2   r   r�   r�   �
memoryviewr�   Zcheck_hash_based_pycsr�   �_RAW_MAGIC_NUMBERr�   r�   ru   r�   r�   r�   r�   r�   r   �dont_write_bytecoder�   r�   r   r�   )rv   r�   rk   r�   r�   r�   Z
hash_basedZcheck_sourcerj   �str   r�   r   Z
bytes_dataZcode_objectr   r   r   r�   X  s�    
���
�����

�

�zSourceLoader.get_codeN)
r}   r|   r~   r�   r�   r�   r�   r�   r�   r�   r   r   r   r   r�     s   

r�   c                       s|   e Zd ZdZdd� Zdd� Zdd� Ze� fdd	��Zed
d� �Z	dd� Z
edd� �Zdd� Zdd� Zdd� Zdd� Z�  ZS )�
FileLoaderzgBase file loader class which implements the loader protocol methods that
    require file system usage.c                 C   s   || _ || _dS )zKCache the module name and the path to the file found by the
        finder.Nr�   )rv   r�   r,   r   r   r   r�   �  s    zFileLoader.__init__c                 C   s   | j |j ko| j|jkS rm   ��	__class__r�   �rv   Zotherr   r   r   �__eq__�  s    
�zFileLoader.__eq__c                 C   s   t | j�t | j�A S rm   ��hashrt   r,   �rv   r   r   r   �__hash__�  s    zFileLoader.__hash__c                    s   t t| ��|�S )zdLoad a module from a file.

        This method is deprecated.  Use exec_module() instead.

        )�superr�   r�   r�   �r�   r   r   r�   �  s    
zFileLoader.load_modulec                 C   s   | j S �z:Return the path to the source file as found by the finder.r0   r�   r   r   r   r�   �  s    zFileLoader.get_filenamec                 C   s~   t | ttf�rFt�t|���}|�� W  d  � S 1 s:0    Y  n4t�|d��}|�� W  d  � S 1 sp0    Y  dS )z'Return the data from path as raw bytes.N�r)r�   r�   �ExtensionFileLoaderr@   Z	open_coderT   ZreadrA   )rv   r,   rD   r   r   r   r�   �  s
    (zFileLoader.get_datac                 C   s   | � |�r| S d S rm   )r�   �rv   r�   r   r   r   �get_resource_reader�  s    
zFileLoader.get_resource_readerc                 C   s    t t| j�d |�}t�|d�S )NrI   r�   )r&   r/   r,   r@   rA   �rv   Zresourcer,   r   r   r   �open_resource�  s    zFileLoader.open_resourcec                 C   s&   | � |�st�tt| j�d |�}|S �NrI   )�is_resource�FileNotFoundErrorr&   r/   r,   r�   r   r   r   �resource_path�  s    
zFileLoader.resource_pathc                 C   s(   t |v rdS tt| j�d |�}t|�S )NFrI   )r#   r&   r/   r,   r6   �rv   rt   r,   r   r   r   r  �  s    zFileLoader.is_resourcec                 C   s   t t�t| j�d ��S r  )�iterr   �listdirr/   r,   r�   r   r   r   �contents�  s    zFileLoader.contents)r}   r|   r~   r   r�   r�   r�   r�   r�   r�   r�   r�   r   r  r  r  Z__classcell__r   r   r�   r   r�   �  s   

r�   c                   @   s.   e Zd ZdZdd� Zdd� Zdd�dd	�Zd
S )�SourceFileLoaderz>Concrete implementation of SourceLoader using the file system.c                 C   s   t |�}|j|jd�S )z!Return the metadata for the path.)r�   r�   )r1   �st_mtimeZst_size)rv   r,   r�   r   r   r   r�   �  s    zSourceFileLoader.path_statsc                 C   s   t |�}| j|||d�S )N��_mode)rr   r�   )rv   rk   rj   r   r4   r   r   r   r�   �  s    z SourceFileLoader._cache_bytecoder<   r  c          	      C   s�   t |�\}}g }|r4t|�s4t |�\}}|�|� qt|�D ]h}t||�}zt�|� W q< tyn   Y q<Y q< ty� } zt	�
d||� W Y d}~ dS d}~0 0 q<zt|||� t	�
d|� W n4 t� y� } zt	�
d||� W Y d}~n
d}~0 0 dS )zWrite bytes data to a file.zcould not create {!r}: {!r}Nzcreated {!r})r/   r8   r�   r*   r&   r   Zmkdir�FileExistsErrorr2   r�   r�   rE   )	rv   r,   r   r  �parentr`   r%   r!   r�   r   r   r   r�     s.    
��zSourceFileLoader.set_dataN)r}   r|   r~   r   r�   r�   r�   r   r   r   r   r	  �  s   r	  c                   @   s    e Zd ZdZdd� Zdd� ZdS )�SourcelessFileLoaderz-Loader which handles sourceless file imports.c                 C   sD   | � |�}| �|�}||d�}t|||� tt|�dd � ||d�S )Nr�   r�   )rt   rj   )r�   r�   r�   r�   r�   )rv   r�   r,   r   r�   r   r   r   r�   &  s    

��zSourcelessFileLoader.get_codec                 C   s   dS )z'Return None as there is no source code.Nr   r�   r   r   r   r�   6  s    zSourcelessFileLoader.get_sourceN)r}   r|   r~   r   r�   r�   r   r   r   r   r  "  s   r  c                   @   s\   e Zd ZdZdd� Zdd� Zdd� Zdd	� Zd
d� Zdd� Z	dd� Z
dd� Zedd� �ZdS )r�   z]Loader for extension modules.

    The constructor is designed to work with FileFinder.

    c                 C   s   || _ || _d S rm   r�   r  r   r   r   r�   G  s    zExtensionFileLoader.__init__c                 C   s   | j |j ko| j|jkS rm   r�   r�   r   r   r   r�   K  s    
�zExtensionFileLoader.__eq__c                 C   s   t | j�t | j�A S rm   r�   r�   r   r   r   r�   O  s    zExtensionFileLoader.__hash__c                 C   s$   t �tj|�}t �d|j| j� |S )z&Create an unitialized extension modulez&extension module {!r} loaded from {!r})r�   r�   r�   Zcreate_dynamicr�   rt   r,   )rv   r�   r�   r   r   r   r�   R  s    ��z!ExtensionFileLoader.create_modulec                 C   s$   t �tj|� t �d| j| j� dS )zInitialize an extension modulez(extension module {!r} executed from {!r}N)r�   r�   r�   Zexec_dynamicr�   rt   r,   r�   r   r   r   r�   Z  s    �zExtensionFileLoader.exec_modulec                    s$   t | j�d � t� fdd�tD ��S )z1Return True if the extension module is a package.r'   c                 3   s   | ]}� d | kV  qdS )r�   Nr   �r    �suffix�Z	file_namer   r   �	<genexpr>c  s   �z1ExtensionFileLoader.is_package.<locals>.<genexpr>)r/   r,   �any�EXTENSION_SUFFIXESr�   r   r  r   r�   `  s    �zExtensionFileLoader.is_packagec                 C   s   dS )z?Return None as an extension module cannot create a code object.Nr   r�   r   r   r   r�   f  s    zExtensionFileLoader.get_codec                 C   s   dS )z5Return None as extension modules have no source code.Nr   r�   r   r   r   r�   j  s    zExtensionFileLoader.get_sourcec                 C   s   | j S r�   r0   r�   r   r   r   r�   n  s    z ExtensionFileLoader.get_filenameN)r}   r|   r~   r   r�   r�   r�   r�   r�   r�   r�   r�   r�   r�   r   r   r   r   r�   ?  s   r�   c                   @   sh   e Zd ZdZdd� Zdd� Zdd� Zdd	� Zd
d� Zdd� Z	dd� Z
dd� Zdd� Zdd� Zdd� ZdS )�_NamespacePatha&  Represents a namespace package's path.  It uses the module name
    to find its parent module, and from there it looks up the parent's
    __path__.  When this changes, the module's own path is recomputed,
    using path_finder.  For top-level modules, the parent module's path
    is sys.path.c                 C   s$   || _ || _t| �� �| _|| _d S rm   )�_name�_pathro   �_get_parent_path�_last_parent_path�_path_finder�rv   rt   r,   Zpath_finderr   r   r   r�   {  s    z_NamespacePath.__init__c                 C   s&   | j �d�\}}}|dkrdS |dfS )z>Returns a tuple of (parent-module-name, parent-path-attr-name)rG   r(   )r   r,   Z__path__)r  r)   )rv   r  �dotZmer   r   r   �_find_parent_path_names�  s    z&_NamespacePath._find_parent_path_namesc                 C   s   | � � \}}ttj| |�S rm   )r  r�   r   �modules)rv   Zparent_module_nameZpath_attr_namer   r   r   r  �  s    z_NamespacePath._get_parent_pathc                 C   sP   t | �� �}|| jkrJ| �| j|�}|d urD|jd u rD|jrD|j| _|| _| jS rm   )ro   r  r  r  r  r�   r�   r  )rv   Zparent_pathr�   r   r   r   �_recalculate�  s    
z_NamespacePath._recalculatec                 C   s   t | �� �S rm   )r  r   r�   r   r   r   �__iter__�  s    z_NamespacePath.__iter__c                 C   s   | � � | S rm   �r   )rv   �indexr   r   r   �__getitem__�  s    z_NamespacePath.__getitem__c                 C   s   || j |< d S rm   )r  )rv   r#  r,   r   r   r   �__setitem__�  s    z_NamespacePath.__setitem__c                 C   s   t | �� �S rm   )r   r   r�   r   r   r   �__len__�  s    z_NamespacePath.__len__c                 C   s   d� | j�S )Nz_NamespacePath({!r}))r>   r  r�   r   r   r   �__repr__�  s    z_NamespacePath.__repr__c                 C   s   || � � v S rm   r"  �rv   �itemr   r   r   �__contains__�  s    z_NamespacePath.__contains__c                 C   s   | j �|� d S rm   )r  r�   r(  r   r   r   r�   �  s    z_NamespacePath.appendN)r}   r|   r~   r   r�   r  r  r   r!  r$  r%  r&  r'  r*  r�   r   r   r   r   r  t  s   
r  c                   @   sP   e Zd Zdd� Zedd� �Zdd� Zdd� Zd	d
� Zdd� Z	dd� Z
dd� ZdS )�_NamespaceLoaderc                 C   s   t |||�| _d S rm   )r  r  r  r   r   r   r�   �  s    z_NamespaceLoader.__init__c                 C   s   d� |j�S )zsReturn repr for the module.

        The method is deprecated.  The import machinery does the job itself.

        z<module {!r} (namespace)>)r>   r}   )r�   r�   r   r   r   �module_repr�  s    z_NamespaceLoader.module_reprc                 C   s   dS )NTr   r�   r   r   r   r�   �  s    z_NamespaceLoader.is_packagec                 C   s   dS )Nr(   r   r�   r   r   r   r�   �  s    z_NamespaceLoader.get_sourcec                 C   s   t ddddd�S )Nr(   z<string>r�   T)r�   )r�   r�   r   r   r   r�   �  s    z_NamespaceLoader.get_codec                 C   s   dS r�   r   r�   r   r   r   r�   �  s    z_NamespaceLoader.create_modulec                 C   s   d S rm   r   r�   r   r   r   r�   �  s    z_NamespaceLoader.exec_modulec                 C   s   t �d| j� t �| |�S )zbLoad a namespace module.

        This method is deprecated.  Use exec_module() instead.

        z&namespace module loaded with path {!r})r�   r�   r  r�   r�   r   r   r   r�   �  s    �z_NamespaceLoader.load_moduleN)r}   r|   r~   r�   r�   r,  r�   r�   r�   r�   r�   r�   r   r   r   r   r+  �  s   
r+  c                   @   sv   e Zd ZdZedd� �Zedd� �Zedd� �Zedd	� �Zeddd��Z	eddd��Z
eddd��Zedd� �Zd
S )�
PathFinderz>Meta path finder for sys.path and package __path__ attributes.c                 C   s@   t tj�� �D ],\}}|du r(tj|= qt|d�r|��  qdS )z}Call the invalidate_caches() method on all path entry finders
        stored in sys.path_importer_caches (where implemented).N�invalidate_caches)�listr   �path_importer_cache�itemsr�   r.  )r�   rt   �finderr   r   r   r.  �  s
    

zPathFinder.invalidate_cachesc              	   C   sR   t jdurt jst�dt� t jD ]*}z||�W   S  tyJ   Y q"Y q"0 q"dS )z.Search sys.path_hooks for a finder for 'path'.Nzsys.path_hooks is empty)r   �
path_hooksrK   rL   r�   ru   )r�   r,   Zhookr   r   r   �_path_hooks�  s    
zPathFinder._path_hooksc                 C   sd   |dkr*zt �� }W n ty(   Y dS 0 ztj| }W n& ty^   | �|�}|tj|< Y n0 |S )z�Get the finder for the path entry from sys.path_importer_cache.

        If the path entry is not in the cache, find the appropriate finder
        and cache it. If no finder is available, store None.

        r(   N)r   r7   r  r   r0  �KeyErrorr4  )r�   r,   r2  r   r   r   �_path_importer_cache�  s    
zPathFinder._path_importer_cachec                 C   sR   t |d�r|�|�\}}n|�|�}g }|d ur<t�||�S t�|d �}||_|S )Nr�   )r�   r�   r�   r�   r�   r�   r�   )r�   r�   r2  r�   r�   r�   r   r   r   �_legacy_get_spec  s    

zPathFinder._legacy_get_specNc           	      C   s�   g }|D ]�}t |ttf�sq| �|�}|durt|d�rF|�||�}n| �||�}|du r\q|jdurn|  S |j}|du r�t	d��|�
|� qt�|d�}||_|S )z?Find the loader or namespace_path 