 raised when the data is found to be truncated.

    Nr   zbad magic number in z: �{}�   z(reached EOF while reading pyc header of �   �����zinvalid flags z in )�MAGIC_NUMBERr�   �_verbose_messageru   r   �EOFErrorr   )r   rt   �exc_detailsZmagicr\   r   r   r   r   �_classify_pyc�  s    
r�   c                 C   sx   t | dd� �|d@ kr>d|��}t�d|� t|fi |���|durtt | dd� �|d@ krttd|��fi |���dS )a  Validate a pyc against the source last-modified time.

    *data* is the contents of the pyc file. (Only the first 16 bytes are
    required.)

    *source_mtime* is the last modified timestamp of the source file.

    *source_size* is None or the size of the source file in bytes.

    *name* is the name of the module being imported. It is used for logging.

    *exc_details* is a dictionary passed to ImportError if it raised for
    improved debugging.

    An ImportError is raised if the bytecode is stale.

    r�   �   r   zbytecode is stale for r�   Nr�   )r   r�   r�   ru   )r   �source_mtime�source_sizert   r�   r\   r   r   r   �_validate_timestamp_pyc  s    
�r�   c                 C   s*   | dd� |kr&t d|��fi |���dS )a�  Validate a hash-based pyc by checking the real source hash against the one in
    the pyc header.

    *data* is the contents of the pyc file. (Only the first 16 bytes are
    required.)

    *source_hash* is the importlib.util.source_hash() of the source file.

    *name* is the name of the module being imported. It is used for logging.

    *exc_details* is a dictionary passed to ImportError if it raised for
    improved debugging.

    An ImportError is raised if the bytecode is stale.

    r�   r�   z.hash in bytecode doesn't match hash of source N)ru   )r   �source_hashrt   r�   r   r   r   �_validate_hash_pyc1  s    ��r�   c                 C   sP   t �| �}t|t�r8t�d|� |dur4t�||� |S td�	|�||d��dS )z#Compile bytecode as found in a pyc.zcode object from {!r}NzNon-code object in {!r}�rt   r,   )
�marshalZloads�
isinstance�
_code_typer�   r�   �_impZ_fix_co_filenameru   r>   )r   rt   rj   rk   �coder   r   r   �_compile_bytecodeI  s    


�r�   rI   c                 C   sF   t t�}|�td�� |�t|�� |�t|�� |�t�| �� |S )z+Produce the data for a timestamp-based pyc.rI   )�	bytearrayr�   �extendr   r�   �dumps)r�   �mtimer�   r   r   r   r   �_code_to_timestamp_pycV  s    r�   Tc                 C   sP   t t�}d|d> B }|�t|�� t|�dks2J �|�|� |�t�| �� |S )z&Produce the data for a hash-based pyc.r'   r�   )r�   r�   r�   r   r   r�   r�   )r�   r�   Zcheckedr   r   r   r   r   �_code_to_hash_pyc`  s    
r�   c                 C   s>   ddl }t�| �j}|�|�}t�dd�}|�| �|d ��S )zyDecode bytes representing source code and return the string.

    Universal newline support is used in the decoding.
    rI   NT)�tokenizer@   ZBytesIOZreadlineZdetect_encodingZIncrementalNewlineDecoder�decode)�source_bytesr�   Zsource_bytes_readline�encodingZnewline_decoderr   r   r   �decode_sourcek  s
    
r�   �r�   �submodule_search_locationsc          	      C   s  |du r:d}t |d�rDz|�| �}W qD ty6   Y qD0 n
t�|�}tj| ||d�}d|_|du r�t� D ]*\}}|�	t
|��rh|| |�}||_ q�qhdS |tu r�t |d�r�z|�| �}W n ty�   Y q�0 |r�g |_n||_|jg k�r|�rt|�d }|j�|� |S )a=  Return a module spec based on a file location.

    To indicate that the module is a package, set
    submodule_search_locations to a list of directory paths.  An
    empty list is sufficient, though its not otherwise useful to the
    import system.

    The loader must take a spec as its only __init__() arg.

    Nz	<unknown>�get_filename��originT�
is_packagerI   )r�   r�   ru   r   rO   r�   �
ModuleSpecZ_set_fileattr�_get_supported_file_loadersrn   ro   r�   �	_POPULATEr�   r�   r/   �append)	rt   Zlocationr�   r�   �spec�loader_class�suffixesr�   Zdirnamer   r   r   �spec_from_file_location|  s>    



r�   c                   @   sP   e Zd ZdZdZdZdZedd� �Zedd� �Z	edd
d��Z
eddd��Zd	S )�WindowsRegistryFinderz>Meta path finder for modules declared in the Windows registry.z;Software\Python\PythonCore\{sys_version}\Modules\{fullname}zASoftware\Python\PythonCore\{sys_version}\Modules\{fullname}\DebugFc                 C   s6   zt �t j|�W S  ty0   t �t j|� Y S 0 d S rm   )�winregZOpenKeyZHKEY_CURRENT_USERr2   ZHKEY_LOCAL_MACHINE)�clsr   r   r   r   �_open_registry�  s    z$WindowsRegistryFinder._open_registryc                 C   s�   | j r| j}n| j}|j|dtjd d�  d�}z:| �|��}t�|d�}W d   � n1 s^0    Y  W n t	y~   Y d S 0 |S )Nz%d.%dr   )r�   Zsys_versionr(   )
�DEBUG_BUILD�REGISTRY_KEY_DEBUG�REGISTRY_KEYr>   r   �version_infor�   r�   Z
QueryValuer2   )r�   r�   Zregistry_keyr   Zhkey�filepathr   r   r   �_search_registry�  s    �.z&WindowsRegistryFinder._search_registryNc                 C   sx   | � |�}|d u rd S zt|� W n ty6   Y d S 0 t� D ]4\}}|�t|��r>tj||||�|d�}|  S q>d S )Nr�   )r�   r1   r2   r�   rn   ro   r�   �spec_from_loader)r�   r�   r,   �targetr�   r�   r�   r�   r   r   r   �	find_spec�  s    
�zWindowsRegistryFinder.find_specc                 C   s"   | � ||�}|dur|jS dS dS )zlFind module named in the registry.

        This method is deprecated.  Use exec_module() instead.

        N�r�   r�   �r�   r�   r,   r�   r   r   r   �find_module�  s    z!WindowsRegistryFinder.find_module)NN)N)r}   r|   r~   r   r�   r�   r�   �classmethodr�   r�   r�   r�   r   r   r   r   r�   �  s   ��

r�   c                   @   s0   e Zd ZdZdd� Zdd� Zdd� Zdd	� Zd
S )�_LoaderBasicszSBase class of common code needed by both SourceLoader and
    SourcelessFileLoader.c                 C   s@   t | �|��d }|�dd�d }|�d�d }|dko>|dkS )z�Concrete implementation of InspectLoader.is_package by checking if
        the path returned by get_filename has a filename of '__init__.py'.r'   rG   rI   r   �__init__)r/   r�   r+   r)   )rv   r�   r`   Zfilename_baseZ	tail_namer   r   r   r�     s    z_LoaderBasics.is_packagec                 C   s   dS �z*Use default semantics for module creation.Nr   �rv   r�   r   r   r   �create_module  s    z_LoaderBasics.create_modulec                 C   s8   | � |j�}|du r$td�|j���t�t||j� dS )zExecute the module.Nz4cannot load module {!r} when get_code() returns None)�get_coder}   ru   r>   r�   �_call_with_frames_removed�execr�   )rv   �moduler�   r   r   r   �exec_module  s    �z_LoaderBasics.exec_modulec                 C   s   t �| |�S )zThis module is deprecated.)r�   �_load_module_shim�rv   r�   r   r   r   �load_module  s    z_LoaderBasics.load_moduleN)r}   r|   r~   r   r�   r�   r�   r�   r   r   r   r   r�      s
   r�   c                   @   sJ   e Zd Zdd� Zdd� Zdd� Zdd� Zd	d
� Zdd�dd�Zdd� Z	dS )�SourceLoaderc                 C   s   t �dS )z�Optional method that returns the modification time (an int) for the
        specified path (a str).

        Raises OSError when the path cannot be handled.
        N)r2   �rv   r,   r   r   r   �
path_mtime  s    zSourceLoader.path_mtimec                 C   s   d| � |�iS )a�  Optional method returning a metadata dict for the specified
        path (a str).

        Possible keys:
        - 'mtime' (mandatory) is the numeric timestamp of last source
          code modification;
        - 'size' (optional) is the size in bytes of the source code.

        Implementing this method allows the loader to read bytecode files.
        Raises OSError when the path cannot be handled.
        r�   )r�   r�   r   r   r   �
path_stats'  s    zSourceLoader.path_statsc                 C   s   | � ||�S )z�Optional method which writes data (bytes) to a file path (a str).

        Implementing this method allows for the writing of bytecode files.

        The source path is needed in order to correctly transfer permissions
        )�set_data)rv   rk   Z
cache_pathr   r   r   r   �_cache_bytecode5  s    zSourceLoader._cache_bytecodec                 C   s   dS )z�Optional method which writes data (bytes) to a file path (a str).

        Implementing this method allows for the writing of bytecode files.
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
|� qt�|d�}||_|S )z?Find the loader or namespace_path for this module/package name.Nr�   zspec missing loader)r�   rT   �bytesr6  r�   r�   r7  r�   r�   ru   r�   r�   r�   )	r�   r�   r,   r�   �namespace_pathZentryr2  r�   r�   r   r   r   �	_get_spec  s(    


zPathFinder._get_specc                 C   sd   |du rt j}| �|||�}|du r(dS |jdu r\|j}|rVd|_t||| j�|_|S dS n|S dS )z�Try to find a spec for 'fullname' on sys.path or 'path'.

        The search is based on sys.path_hooks and sys.path_importer_cache.
        N)r   r,   r:  r�   r�   r�   r  )r�   r�   r,   r�   r�   r9  r   r   r   r�   =  s    
zPathFinder.find_specc                 C   s   | � ||�}|du rdS |jS )z�find the module on sys.path or 'path' based on sys.path_hooks and
        sys.path_importer_cache.

        This method is deprecated.  Use find_spec() instead.

        Nr�   r�   r   r   r   r�   U  s    zPathFinder.find_modulec                 O   s   ddl m} |j|i |��S )a   
        Find distributions.

        Return an iterable of all Distribution instances capable of
        loading the metadata for packages matching ``context.name``
        (or all names if ``None`` indicated) along the paths in the list
        of directories ``context.path``.
        rI   )�MetadataPathFinder)Zimportlib.metadatar;  �find_distributions)r�   rw   rx   r;  r   r   r   r<  b  s    
zPathFinder.find_distributions)N)NN)N)r}   r|   r~   r   r�   r.  r4  r6  r7  r:  r�   r�   r<  r   r   r   r   r-  �  s"   
	


r-  c                   @   sZ   e Zd ZdZdd� Zdd� ZeZdd� Zdd	� Z	ddd�Z
dd� Zedd� �Zdd� Zd
S )�
FileFinderz�File-based finder.

    Interactions with the file system are cached for performance, being
    refreshed when the directory the finder is handling has been modified.

    c                    sT   g }|D ] \� }|� � fdd�|D �� q|| _|p6d| _d| _t� | _t� | _dS )z�Initialize with the path to search on and a variable number of
        2-tuples containing the loader and the file suffixes the loader
        recognizes.c                 3   s   | ]}|� fV  qd S rm   r   r  �r�   r   r   r    �    z&FileFinder.__init__.<locals>.<genexpr>rG   rh   N)r�   �_loadersr,   �_path_mtime�set�_path_cache�_relaxed_path_cache)rv   r,   �loader_detailsZloadersr�   r   r>  r   r�   y  s    
zFileFinder.__init__c                 C   s
   d| _ dS )zInvalidate the directory mtime.rh   N)rA  r�   r   r   r   r.  �  s    zFileFinder.invalidate_cachesc                 C   s*   | � |�}|du rdg fS |j|jp&g fS )z�Try to find a loader for the specified module, or the namespace
        package portions. Returns (loader, list-of-portions).

        This method is deprecated.  Use find_spec() instead.

        N)r�   r�   r�   )rv   r�   r�   r   r   r   r�   �  s    
zFileFinder.find_loaderc                 C   s   |||�}t ||||d�S )Nr�   )r�   )rv   r�   r�   r,   Zsmslr�   r�   r   r   r   r:  �  s    
�zFileFinder._get_specNc                 C   s`  d}|� d�d }zt| jp"t�� �j}W n ty@   d}Y n0 || jkrZ| ��  || _t	� rp| j
}|�� }n
| j}|}||v r�t| j|�}| jD ]:\}	}
d|	 }t||�}t|�r�| �|
|||g|�  S q�t|�}| jD ]R\}	}
t| j||	 �}tjd|dd� ||	 |v r�t|�r�| �|
||d|�  S q�|�r\t�d	|� t�|d�}|g|_|S dS )
zoTry to find a spec for the specified module.

        Returns the matching spec, or None if not found.
        FrG   r   rh   r�   z	trying {})Z	verbosityNzpossible namespace for {})r)   r1   r,   r   r7   r
  r2   rA  �_fill_cacher	   rD  ri   rC  r&   r@  r6   r:  r8   r�   r�   r�   r�   )rv   r�   r�   Zis_namespaceZtail_moduler�   ZcacheZcache_moduleZ	base_pathr  r�   Zinit_filenameZ	full_pathr�   r   r   r   r�   �  sH    




�
zFileFinder.find_specc           	   
   C   s�   | j }zt�|pt�� �}W n tttfy8   g }Y n0 tj�	d�sRt
|�| _nJt
� }|D ]8}|�d�\}}}|r�d�||�� �}n|}|�|� q\|| _tj�	t�r�dd� |D �| _dS )zDFill the cache of potential modules and packages for this directory.r    rG   r=   c                 S   s   h | ]}|� � �qS r   )ri   )r    Zfnr   r   r   �	<setcomp>�  r?  z)FileFinder._fill_cache.<locals>.<setcomp>N)r,   r   r  r7   r  �PermissionError�NotADirectoryErrorr   r
   r   rB  rC  rd   r>   ri   �addr   rD  )	rv   r,   r  Zlower_suffix_contentsr)  rt   r  r  Znew_namer   r   r   rF  �  s"    
zFileFinder._fill_cachec                    s   � �fdd�}|S )a  A class method which returns a closure to use on sys.path_hook
        which will return an instance using the specified loaders and the path
        called on the closure.

        If the path called on the closure is not a directory, ImportError is
        raised.

        c                    s$   t | �std| d��� | g��R � S )z-Path hook for importlib.machinery.FileFinder.zonly directories are supportedr0   )r8   ru   r0   �r�   rE  r   r   �path_hook_for_FileFinder�  s    z6FileFinder.path_hook.<locals>.