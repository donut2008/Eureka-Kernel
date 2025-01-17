 and return the module based on its name, the package the call is
    being made from, and the level adjustment.

    This function represents the greatest common denominator of functionality
    between import_module and __import__. This includes setting __package__ if
    the loader did not.

    r   )r�   r�   r�   �_gcd_importr�   r
   r
   r   r�   �  s    	r�   ��	recursivec                C   s�   |D ]�}t |t�sB|r"| jd }nd}td|� dt|�j� ���q|dkrl|s�t| d�r�t| | j|dd� qt| |�sd	�| j|�}zt	||� W q t
y� } z6|j|kr�tj�|t�d
ur�W Y d
}~q� W Y d
}~qd
}~0 0 q| S )z�Figure out what __import__ should return.

    The import_ parameter is a callable which takes the name of module to
    import. It is required to decouple the function from assuming importlib's
    import implementation is desired.

    z.__all__z``from list''zItem in z must be str, not �*�__all__Tr�   r�   N)r�   r�   r   r�   r   r   �_handle_fromlistr�   r.   rD   r�   r   r   r]   r#   r�   )ra   �fromlistr�   r�   �xZwhereZ	from_nameZexcr
   r
   r   r�   	  s0    


�

�

�r�   c                 C   s�   | � d�}| � d�}|durR|durN||jkrNtjd|�d|j�d�tdd� |S |dur`|jS tjd	tdd� | d
 }d| vr�|�d�d }|S )z�Calculate what __package__ should be.

    __package__ is not guaranteed to be defined or could be set to None
    to represent that its proper value is unknown.

    r�   rj   Nz __package__ != __spec__.parent (z != �)�   )Z
stacklevelzYcan't resolve package from __spec__ or __package__, falling back on __name__ and __path__r   r�   r�   r   )r#   r�   r�   r�   r�   r�   )�globalsr�   r`   r
   r
   r   �_calc___package__.  s*    

����r�   r
   c           	      C   s�   |dkrt | �}n$|dur|ni }t|�}t | ||�}|s�|dkrTt | �d�d �S | s\|S t| �t| �d�d � }tj|jdt|j�| �  S nt|d�r�t||t �S |S dS )a�  Import a module.

    The 'globals' argument is used to infer where the import is occurring from
    to handle relative imports. The 'locals' argument is ignored. The
    'fromlist' argument specifies what should exist as attributes on the module
    being imported (e.g. ``from module import <fromlist>``).  The 'level'
    argument represents the package location to import from in a relative
    import (e.g. ``from ..pkg import mod`` would have a 'level' of 2).

    r   Nr�   r�   )	r�   r�   �	partitionr�   r   r]   r   r   r�   )	r   r�   �localsr�   r�   ra   Zglobals_r�   Zcut_offr
   r
   r   �
__import__I  s    
 
r�   c                 C   s&   t �| �}|d u rtd|  ��t|�S )Nzno built-in module named )r�   r�   rP   r�   )r   r`   r
   r
   r   �_builtin_from_namen  s    
r�   c           
      C   s�   |a | att�}tj�� D ]H\}}t||�r|tjv r<t}nt �|�rt	}nqt
||�}t||� qtjt }dD ].}|tjvr�t|�}	n
tj| }	t|||	� qrdS )z�Setup importlib by importing needed built-in modules and injecting them
    into the global namespace.

    As sys is needed for sys.modules access and _imp is needed to load built-in
    modules, those two modules must be explicitly passed in.

    )r   r�   rA   N)r:   r   r   r]   �itemsr�   rO   r�   rY   r�   r�   r�   r   r�   r   )
�
sys_module�_imp_moduleZmodule_typer   ra   rn   r`   Zself_moduleZbuiltin_nameZbuiltin_moduler
   r
   r   �_setupu  s$    	







r�   c                 C   s&   t | |� tj�t� tj�t� dS )z0Install importers for builtin and frozen modulesN)r�   r   r�   rx   r�   r�   )r�   r�   r
   r
   r   �_install�  s    
r�   c                  C   s    ddl } | a| �tjt � dS )z9Install importers that require external filesystem accessr   N)�_frozen_importlib_externalr   r�   r   r]   r   )r�   r
   r
   r   �_install_external_importers�  s    r�   )NN)N)Nr   )NNr
   r   )2r   r   r   r   r<   r"   r,   r   r   r2   r3   r6   rB   rD   rM   rW   r[   rb   rp   rq   r\   r�   r�   r�   rl   r^   r�   r�   r_   r�   r�   r�   r�   r�   r�   r�   Z_ERR_MSG_PREFIXr�   r�   �objectr�   r�   r�   r�   r�   r�   r�   r�   r�   r�   r
   r
   r
   r   �<module>   s^   M%$e
-H%*KO		
/ 
%
%#       c                   @   s   d Z dZdZee Zdd� Zdd� Zdd� Zd	d
� Zdd� Zdd� Z	dd� Z
dd� Zdd� Zdd� Zdd� Zdedd�Zeej�Zd�dd�d Ze�ed�Zd Zd!Zd"gZd#gZe ZZdfd$d%�d&d'�Zd(d)� Zd*d+� Z d,d-� Z!d.d/� Z"d0d1� Z#d2d3� Z$d4d5� Z%d6d7� Z&d8d9� Z'dgd:d;�Z(dhd=d>�Z)did@dA�Z*dBdC� Z+e,� Z-djd$e-dD�dEdF�Z.G dGdH� dH�Z/G dIdJ� dJ�Z0G dKdL� dLe0�Z1G dMdN� dN�Z2G dOdP� dPe2e1�Z3G dQdR� dRe2e0�Z4g Z5G dSdT� dTe2e0�Z6G dUdV� dV�Z7G dWdX� dX�Z8G dYdZ� dZ�Z9G d[d\� d\�Z:dkd]d^�Z;d_d`� Z<dadb� Z=dcdd� Z>d$S )la^  Core implementation of path-based import.

This module is NOT meant to be directly imported! It has been designed such
that it can be bootstrapped into Python as the implementation of import. As
such it requires the injection of specific modules and attributes in order to
work. One should use importlib as the public-facing version of this module.

)�win)ZcygwinZdarwinc                     s<   t j�t�r0t j�t�rd� nd� � fdd�} ndd� } | S )NZPYTHONCASEOKs   PYTHONCASEOKc                      s   t jj o� tjv S )z^True if filenames must be checked case-insensitively and ignore environment flags are not set.)�sys�flags�ignore_environment�_osZenviron� ��keyr   �&<frozen importlib._bootstrap_external>�_relax_case$   s    z%_make_relax_case.<locals>._relax_casec                   S   s   dS )z5True if filenames must be checked case-insensitively.Fr   r   r   r   r   r	   (   s    )r   �platform�
startswith�_CASE_INSENSITIVE_PLATFORMS�#_CASE_INSENSITIVE_PLATFORMS_STR_KEY)r	   r   r   r   �_make_relax_case   s    r   c                 C   s   t | �d@ �dd�S )z*Convert a 32-bit integer to little-endian.�   �� �   �little)�int�to_bytes)�xr   r   r   �_pack_uint32.   s    r   c                 C   s   t | �dksJ �t�| d�S )z/Convert 4 bytes in little-endian to an integer.r   r   ��lenr   �
from_bytes��datar   r   r   �_unpack_uint323   s    r   c                 C   s   t | �dksJ �t�| d�S )z/Convert 2 bytes in little-endian to an integer.�   r   r   r   r   r   r   �_unpack_uint168   s    r   c                  G   s   t �dd� | D ��S )zReplacement for os.path.join().c                 S   s   g | ]}|r|� t��qS r   )�rstrip�path_separators)�.0�partr   r   r   �
<listcomp>@   s   �z_path_join.<locals>.<listcomp>)�path_sep�join)�
path_partsr   r   r   �
_path_join>   s    
�r&   c                 C   s`   t t�dkr$| �t�\}}}||fS t| �D ]*}|tv r,| j|dd�\}}||f  S q,d| fS )z Replacement for os.path.split().�   )Zmaxsplit� )r   r   �
rpartitionr#   �reversed�rsplit)�pathZfront�_�tailr   r   r   r   �_path_splitD   s    r/   c                 C   s
   t �| �S )z~Stat the path.

    Made a separate function to make it easier to override in experiments
    (e.g. cache stat results).

    )r   Zstat�r,   r   r   r   �
_path_statP   s    r1   c                 C   s0   zt | �}W n ty    Y dS 0 |jd@ |kS )z1Test whether the path is the specified mode type.Fi �  )r1   �OSError�st_mode)r,   �modeZ	stat_infor   r   r   �_path_is_mode_typeZ   s
    r5   c                 C   s
   t | d�S )zReplacement for os.path.isfile.i �  )r5   r0   r   r   r   �_path_isfilec   s    r6   c                 C   s   | st �� } t| d�S )zReplacement for os.path.isdir.i @  )r   �getcwdr5   r0   r   r   r   �_path_isdirh   s    r8   c                 C   s   | � t�p| dd� tv S )z�Replacement for os.path.isabs.

    Considers a Windows drive-relative path (no drive, but starts with slash) to
    still be "absolute".
    r'   �   )r   r   �_pathseps_with_colonr0   r   r   r   �_path_isabso   s    r;   �  c                 C   s�   d� | t| ��}t�|tjtjB tjB |d@ �}zFt�|d��}|�	|� W d  � n1 s^0    Y  t�
|| � W n6 ty�   zt�|� W n ty�   Y n0 � Y n0 dS )z�Best-effort function to write data to a path atomically.
    Be prepared to handle a FileExistsError if concurrent writing of the
    temporary file is attempted.�{}.{}r<   ZwbN)�format�idr   ZopenZO_EXCLZO_CREATZO_WRONLY�_io�FileIO�write�replacer2   Zunlink)r,   r   r4   Zpath_tmpZfd�filer   r   r   �_write_atomicx   s    �(rE   ia  r   r   s   
Z__pycache__zopt-z.pyz.pycN)�optimizationc                C   sX  |dur4t �dt� |dur(d}t|��|r0dnd}t�| �} t| �\}}|�d�\}}}tj	j
}	|	du rrtd��d�|r~|n|||	g�}
|du r�tjjdkr�d}ntjj}t|�}|dkr�|�� s�td	�|���d
�|
t|�}
|
td  }tjdu�rLt|��stt�� |�}|d dk�r8|d tv�r8|dd� }ttj|�t�|�S t|t|�S )a�  Given the path to a .py file, return the path to its .pyc file.

    The .py file does not need to exist; this simply returns the path to the
    .pyc file calculated as if the .py file were imported.

    The 'optimization' parameter controls the presumed optimization level of
    the bytecode file. If 'optimization' is not None, the string representation
    of the argument is taken and verified to be alphanumeric (else ValueError
    is raised).

    The debug_override parameter is deprecated. If debug_override is not None,
    a True value is the same as setting 'optimization' to the empty string
    while a False value is equivalent to setting 'optimization' to '1'.

    If sys.implementation.cache_tag is None then NotImplementedError is raised.

    NzFthe debug_override parameter is deprecated; use 'optimization' insteadz2debug_override or optimization must be set to Noner(   r'   �.�$sys.implementation.cache_tag is None�    z{!r} is not alphanumericz{}.{}{}�:r   )�	_warnings�warn�DeprecationWarning�	TypeErrorr   �fspathr/   r)   r   �implementation�	cache_tag�NotImplementedErrorr$   r   �optimize�str�isalnum�
ValueErrorr>   �_OPT�BYTECODE_SUFFIXES�pycache_prefixr;   r&   r7   r   �lstrip�_PYCACHE)r,   Zdebug_overriderF   �message�headr.   Zbase�sep�restZtagZalmost_filename�filenamer   r   r   �cache_from_source-  sH    �
	
�ra   c           
      C   s.  t jjdu rtd��t�| �} t| �\}}d}t jdurft j�t	�}|�
|t �rf|t|�d� }d}|s�t|�\}}|tkr�tt� d| ����|�d�}|dvr�td|����n\|d	k�r|�dd
�d }|�
t�s�tdt����|tt�d� }|�� �std|�d���|�d�d }	t||	td  �S )an  Given the path to a .pyc. file, return the path to its .py file.

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

    ImportError is raised when the magic number is incorrect or when the flags
    field is invalid. EOFError is raised when the data is found to be truncated.

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
r�   c                   @   sP   e Zd ZdZdZdZdZedd� �Z