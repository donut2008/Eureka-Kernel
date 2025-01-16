ddlZddlmZmZ ddlZddlZddlZddl	Z	ddl
Z
ddlZddgZejZejdd� ZG dd� de�Zi Zee
�ZdZd	Zd
ZG dd� d�Zed ddfed ddfddfZdd� Zdd� Zdd� Zdd� ZdZdadd� Z dd� Z!dd � Z"d!d"� Z#ee#j$�Z%d#d$� Z&d%d&� Z'd'd(� Z(d)d*� Z)d+d,� Z*d-d.� Z+G d/d0� d0�Z,dS )1aP  zipimport provides support for importing Python modules from Zip archives.

This module exports three objects:
- zipimporter: a class; its constructor takes a path to a Zip archive.
- ZipImportError: exception raised by zipimporter objects. It's a
  subclass of ImportError, so it can be caught as ImportError, too.
- _zip_directory_cache: a dict, mapping archive paths to zip directory
  info dicts, as used in zipimporter._files.

It is usually not needed to use the zipimport module explicitly; it is
used by the builtin import mechanism for sys.path items that are paths
to Zip archives.
�    N)�_unpack_uint16�_unpack_uint32�ZipImportError�zipimporter�   c                   @   s   e Zd ZdS )r   N)�__name__�
__module__�__qualname__� r	   r	   �<frozen zipimport>r   !   s   �   s   PKi��  c                   @   sl   e Zd ZdZdd� Zddd�Zddd�Zd	d
� Zdd� Zdd� Z	dd� Z
dd� Zdd� Zdd� Zdd� ZdS )r   a�  zipimporter(archivepath) -> zipimporter object

    Create a new zipimporter instance. 'archivepath' must be a path to
    a zipfile, or to a specific path inside a zipfile. For example, it can be
    '/tmp/myimport.zip', or '/tmp/myimport.zip/mydirectory', if mydirectory is a
    valid directory inside the archive.

    'ZipImportError is raised if 'archivepath' doesn't point to a valid Zip
    archive.

    The 'archive' attribute of zipimporter objects contains the name of the
    zipfile targeted.
    c              	   C   s   t |t�sdd l}|�|�}|s,td|d��tr<|�tt�}g }zt�	|�}W nF t
tfy�   t�|�\}}||kr�td|d��|}|�|� Y q@0 |jd@ dkr�td|d��q�q@zt| }W n" ty�   t|�}|t|< Y n0 || _|| _tj|d d d� � | _| j�r|  jt7  _d S )Nr    zarchive path is empty��pathznot a Zip filei �  i �  �����)�
isinstance�str�osZfsdecoder   �alt_path_sep�replace�path_sep�_bootstrap_externalZ
_path_stat�OSError�
ValueErrorZ_path_split�appendZst_mode�_zip_directory_cache�KeyError�_read_directory�_files�archive�
_path_join�prefix)�selfr   r   r   ZstZdirnameZbasename�filesr	   r	   r
   �__init__?   s:    

zzipimporter.__init__Nc                 C   sN   t | |�}|dur| g fS t| |�}t| |�rFd| j� t� |� �gfS dg fS )a�  find_loader(fullname, path=None) -> self, str or None.

        Search for a module specified by 'fullname'. 'fullname' must be the
        fully qualified (dotted) module name. It returns the zipimporter
        instance itself if the module was found, a string containing the
        full path name if it's possibly a portion of a namespace package,
        or None otherwise. The optional 'path' argument is ignored -- it's
        there for compatibility with the importer protocol.
        N)�_get_module_info�_get_module_path�_is_dirr   r   )r    �fullnamer   �mi�modpathr	   r	   r
   �find_loaderm   s    



zzipimporter.find_loaderc                 C   s   | � ||�d S )a�  find_module(fullname, path=None) -> self or None.

        Search for a module specified by 'fullname'. 'fullname' must be the
        fully qualified (dotted) module name. It returns the zipimporter
        instance itself if the module was found, or None if it wasn't.
        The optional 'path' argument is ignored -- it's there for compatibility
        with the importer protocol.
        r    )r)   )r    r&   r   r	   r	   r
   �find_module�   s    	zzipimporter.find_modulec                 C   s   t | |�\}}}|S )z�get_code(fullname) -> code object.

        Return the code object for the specified module. Raise ZipImportError
        if the module couldn't be found.
        ��_get_module_code�r    r&   �code�	ispackager(   r	   r	   r
   �get_code�   s    zzipimporter.get_codec                 C   st   t r|�t t�}|}|�| jt �r:|t| jt �d� }z| j| }W n tyf   tdd|��Y n0 t	| j|�S )z�get_data(pathname) -> string with file data.

        Return the data associated with 'pathname'. Raise OSError if
        the file wasn't found.
        Nr    � )
r   r   r   �
startswithr   �lenr   r   r   �	_get_data)r    �pathnameZkey�	toc_entryr	   r	   r
   �get_data�   s    zzipimporter.get_datac                 C   s   t | |�\}}}|S )zjget_filename(fullname) -> filename string.

        Return the filename for the specified module.
        r+   r-   r	   r	   r
   �get_filename�   s    zzipimporter.get_filenamec                 C   s~   t | |�}|du r$td|��|d��t| |�}|r@t�|d�}n
|� d�}z| j| }W n tyl   Y dS 0 t| j|��	� S )z�get_source(fullname) -> source string.

        Return the source code for the specified module. Raise ZipImportError
        if the module couldn't be found, return None if the archive does
        contain the module, but has no source for it.
        N�can't find module ��name�__init__.py�.py)
r#   r   r$   r   r   r   r   r4   r   �decode)r    r&   r'   r   �fullpathr6   r	   r	   r
   �
get_source�   s    


zzipimporter.get_sourcec                 C   s(   t | |�}|du r$td|��|d��|S )z�is_package(fullname) -> bool.

        Return True if the module specified by fullname is a package.
        Raise ZipImportError if the module couldn't be found.
        Nr9   r:   )r#   r   )r    r&   r'   r	   r	   r
   �
is_package�   s    
zzipimporter.is_packagec                 C   s�   t | |�\}}}tj�|�}|du s.t|t�s@t|�}|tj|< | |_zT|rlt| |�}t�	| j
|�}|g|_t|d�s|t|_t�|j||� t||j� W n   tj|= � Y n0 ztj| }W n" ty�   td|�d���Y n0 t�d||� |S )z�load_module(fullname) -> module.

        Load the module specified by 'fullname'. 'fullname' must be the
        fully qualified (dotted) module name. It returns the imported
        module, or raises ZipImportError if it wasn't found.
        N�__builtins__zLoaded module z not found in sys.moduleszimport {} # loaded from Zip {})r,   �sys�modules�getr   �_module_type�
__loader__r$   r   r   r   Z__path__�hasattrrB   Z_fix_up_module�__dict__�execr   �ImportError�
_bootstrap�_verbose_message)r    r&   r.   r/   r(   Zmodr   r?   r	   r	   r
   �load_module�   s0    


zzipimporter.load_modulec                 C   sV   z| � |�sW dS W n ty(   Y dS 0 tjsLddlm} |�t� dt_t| |�S )z�Return the ResourceReader for a package in a zip file.

        If 'fullname' is a package within the zip file, return the
        'ResourceReader' object for the package.  Otherwise return None.
        Nr    )�ResourceReaderT)rA   r   �_ZipImportResourceReader�_registeredZimportlib.abcrO   Zregister)r    r&   rO   r	   r	   r
   �get_resource_reader  s    


zzipimporter.get_resource_readerc                 C   s   d| j � t� | j� d�S )Nz<zipimporter object "z">)r   r   r   )r    r	   r	   r
   �__repr__"  s    zzipimporter.__repr__)N)N)r   r   r   �__doc__r"   r)   r*   r0   r7   r8   r@   rA   rN   rR   rS   r	   r	   r	   r
   r   -   s   .
 

&z__init__.pycTr<   F)z.pycTF)r=   FFc                 C   s   | j |�d�d  S )N�.�   )r   �
rpartition)r    r&   r	   r	   r
   r$   4  s    r$   c                 C   s   |t  }|| jv S �N)r   r   )r    r   Zdirpathr	   r	   r
   r%   8  s    r%   c                 C   s8   t | |�}tD ]$\}}}|| }|| jv r|  S qd S rX   )r$   �_zip_searchorderr   )r    r&   r   �suffix�
isbytecoder/   r?   r	   r	   r
   r#   A  s    


r#   c              	   C   s  zt �| �}W n$ ty2   td| ��| d��Y n0 |��� z$|�t d� |�� }|�t�}W n$ ty�   td| ��| d��Y n0 t|�tkr�td| ��| d��|d d� t	k�r�z|�dd� |�� }W n$ ty�   td| ��| d��Y n0 t
|t t d�}z|�|� |�� }W n& t�yB   td| ��| d��Y n0 |�t	�}|dk �rjtd| ��| d��|||t � }t|�tk�r�td| ��| d��|t|� | }t|d	d
� �}t|d
d� �}	||k �r�td| ��| d��||	k �rtd| ��| d��||8 }||	 }
|
dk �r.td| ��| d��i }d}z|�|� W n& t�yj   td| ��| d��Y n0 |�d�}t|�dk �r�td��|d d� dk�r��q�t|�dk�r�td��t|dd� �}t|dd	� �}t|d	d� �}t|dd
� �}t|d
d� �}t|dd� �}t|dd� �}t|dd� �}t|dd� �}t|dd� �}t|dd� �}|| | }||	k�r�td| ��| d��||
7 }z|�|�}W n& t�y�   td| ��| d��Y n0 t|�|k�r�td| ��| d��z2t|�|| ��|| k�rtd| ��| d��W n& t�yF   td| ��| d��Y n0 |d@ �r\|�� }n4z|�d�}W n$ t�y�   |�d��t�}Y n0 |�dt�}t�| |�}||||||||f}|||< |d 7 }�qlW d   � n1 �s�0    Y  t�d!|| � |S )"Nzcan't open Zip file: r   rV   �can't read Zip file: �   r    znot a Zip file: zcorrupt Zip file: �   �   �   zbad central directory size: zbad central directory offset: z&bad central directory size or offset: �.   �EOF read where not expecteds   PK�   �
   �   �   �   �   �    �"   �*   zbad local header offset: i   �asciiZlatin1�/r   z!zipimport: found {} names in {!r})�_io�	open_coder   r   �seek�END_CENTRAL_DIR_SIZEZtell�readr3   �STRING_END_ARCHIVE�max�MAX_COMMENT_LEN�rfindr   �EOFErrorr   r>   �UnicodeDecodeError�	translate�cp437_tabler   r   r   r   rL   rM   )r   �fpZheader_position�buffer�	file_sizeZmax_comment_start�dataZpos�header_sizeZheader_offsetZ
arc_offsetr!   �count�flags�compress�time�date�crc�	data_size�	name_size�
extra_sizeZcomment_size�file_offsetr;   r   �tr	   r	   r
   r   `  s�    
���

�


�
�






,r   u�   	
 !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒáíóúñÑªº¿⌐¬½¼¡«»░▒▓│┤╡╢╖╕╣║╗╝╜╛┐└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■ c                  C   sn   t rt�d� td��da z>zddlm}  W n$ tyP   t�d� td��Y n0 W da nda 0 t�d� | S )Nzzipimport: zlib UNAVAILABLE�)can't decompress data; zlib not availableTr    ��
decompressFzzipimport: zlib available)�_importing_zlibrL   rM   r   Zzlibr�   �	Exceptionr�   r	   r	   r
   �_get_decompress_func�  s    


r�   c              	   C   s�  |\}}}}}}}}	|dk r$t d��t�| ���}
z|
�|� W n$ tyd   t d| ��| d��Y n0 |
�d�}t|�dkr�td��|d d� dkr�t d	| ��| d��t|d
d� �}t|dd� �}d| | }||7 }z|
�|� W n& t�y   t d| ��| d��Y n0 |
�|�}t|�|k�r0td��W d   � n1 �sF0    Y  |dk�r^|S z
t	� }W n t
�y�   t d��Y n0 ||d�S )Nr    znegative data sizer\   r   rh   rb   r]   s   PKzbad local file header: �   rg   zzipimport: can't read datar�   i����)r   rn   ro   rp   r   rr   r3   rw   r   r�   r�   )r   r6   Zdatapathr�   r�   r}   r�   r�   r�   r�   r{   r|   r�   r�   r   Zraw_datar�   r	   r	   r
   r4     s>    

(

r4   c                 C   s   t | | �dkS )Nr   )�abs)Zt1Zt2r	   r	   r
   �	_eq_mtimeA  s    r�   c                 C   s8  ||d�}zt �|||�}W n ty0   Y d S 0 |d@ dk}|r�|d@ dk}tjdkr�|sftjdkr�t| |�}	|	d ur�t�t j|	�}
zt �||
||� W n ty�   Y d S 0 nTt	| |�\}}|�rt
t|dd� �|�r�t|dd	� �|k�rt�d
|��� d S t�|d	d � �}t|t��s4td|�d���|S )N)r;   r   r   r    rV   ZneverZalwaysrc   r^   r_   zbytecode is stale for zcompiled module z is not a code object)r   Z_classify_pycrK   �_impZcheck_hash_based_pycs�_get_pyc_source�source_hashZ_RAW_MAGIC_NUMBERZ_validate_hash_pyc�_get_mtime_and_size_of_sourcer�   r   rL   rM   �marshalZloadsr   �
_code_type�	TypeError)r    r5   r?   r&   r~   Zexc_detailsr�   Z
hash_basedZcheck_sourceZsource_bytesr�   Zsource_mtimeZsource_sizer.   r	   r	   r
   �_unmarshal_codeK  sR    �
��
��
���r�   c                 C   s   | � dd�} | � dd�} | S )Ns   
�   
�   )r   )�sourcer	   r	   r
   �_normalize_line_endings~  s    r�   c                 C   s   t |�}t|| ddd�S )NrJ   T)Zdont_inherit)r�   �compile)r5   r�   r	   r	   r
   �_compile_source�  s    r�   c                 C   sD   t �| d? d | d? d@ | d@ |d? |d? d@ |d@ d d	d	d	f	�S )
N�	   i�  �   �   �   �   �?   rV   r   )r�   Zmktime)�dr�   r	   r	   r
   �_parse_dostime�  s    



�r�   c              
   C   sr   zR|dd � dv sJ �|d d� }| j | }|d }|d }|d }t||�|fW S  tttfyl   Y dS 0 d S )Nr   ��c�or�   �   �   )r    r    )r   r�   r   �
IndexErrorr�   )r    r   r6   r�   r�   Zuncompressed_sizer	   r	   r
   r�   �  s    
r�   c                 C   sT   |dd � dv sJ �|d d� }z| j | }W n tyB   Y d S 0 t| j|�S d S )Nr   r�   )r   r   r4   r   )r    r   r6   r	   r	   r
   r�   �  s    r�   c              	   C   s�   t | |�}tD ]�\}}}|| }tjd| jt|dd� z| j| }W n tyV   Y q0 |d }t| j|�}	|r�t	| ||||	�}
n
t
||	�}
|
d u r�q|d }|
||f  S qtd|��|d��d S )Nztrying {}{}{}rV   )Z	verbosityr    r9   r:   )r$   rY   rL   rM   r   r   r   r   r4   r�   r�   r   )r    r&   r   rZ   r[   r/   r?   r6   r(   r~   r.   r	   r	   r
   r,   �  s$    

r,   c                   @   s<   e Zd ZdZdZdd� Zdd� Zdd� Zd	d
� Zdd� Z	dS )rP   z�Private class used to support ZipImport.get_resource_reader().

    This class is allowed to reference all the innards and private parts of
    the zipimporter.
    Fc                 C   s   || _ || _d S rX   )r   r&   )r    r   r&   r	   r	   r
   r"   �  s    z!_ZipImportResourceReader.__init__c                 C   sZ   | j �dd�}|� d|� �}ddlm} z|| j�|��W S  tyT   t|��Y n0 d S )NrU   rm   r    )�BytesIO)r&   r   Zior�   r   r7   r   �FileNotFoundError)r    �resource�fullname_as_pathr   r�   r	   r	   r
   �open_resource�  s    z&_ZipImportResourceReader.open_resourcec                 C   s   t �d S rX   )r�   )r    r�   r	   r	   r
   �resource_path�  s    z&_ZipImportResourceReader.resource_pathc                 C   sF   | j �dd�}|� d|� �}z| j�|� W n ty@   Y dS 0 dS )NrU   rm   FT)r&   r   r   r7   r   )r    r;   r�   r   r	   r	   r
   �is_resource�  s    z$_ZipImportResourceReader.is_resourcec           	   	   c   s�   ddl m} || j�| j��}|�| jj�}|jdks:J �|j}t	� }| jj
D ]d}z||��|�}W n tyz   Y qNY n0 |jj}t|�dkr�|jV  qN||vrN|�|� |V  qNd S )Nr    )�Pathr<   )Zpathlibr�   r   r8   r&   Zrelative_tor   r;   Zparent�setr   r   r3   �add)	r    r�   Zfullname_pathZrelative_pathZpackage_pathZsubdirs_seen�filenameZrelativeZparent_namer	   r	   r
   �contents�  s"    


z!_ZipImportResourceReader.contentsN)
r   r   r   rT   rQ   r"   r�   r�   r�   r�   r	   r	   r	   r
   rP   �  s   	rP   )-rT   Z_frozen_importlib_externalr   r   r   Z_frozen_importlibrL   r�   rn   r�   rC   r�   Z__all__r   Zpath_separatorsr   rK   r   r   �typerF   rq   rs   ru   r   rY   r$   r%   r#   r   rz   r�   r�   r4   r�   r�   �__code__r�   r�   r�   r�   r�   r�   r,   rP   r	   r	   r	   r
   �<module>   sX     �		~�.
.
          j> j> j> 4j> j> j> j> j> j> =j> Fj> j> j> Nj> j> Wj> j> `j> ij> j> j> j> j> j> j> j> j> j> j> j> j> j> j> j> j> rj> |j> �j> j> �j> j> �j> �j> j> j> �j> j> �j> j> j> �j> �v> �v> �v> Qu> �v> �v> �v> �v> �v> Qu> Qu> �v> �v> Qu> �v> �v> �v> �v> w> �v> �v> �v> �v> �v> �v> �v> �v> �v> �v> �v> �v> �v> �v> �v> �v> �u> `v> v> �v> v> �v> �u> �u> �v> �v> �u> �v> 1w> �v> �v> Tw> �~> �> �> �~> �> �> �> �> �> �~> 	> �> �> �~> �> �~> �> �~> �~> �> �> �> �> �> �> �> �> �> �> �> �> �> > �> �> �~> �~> �> �> �> �> �~> 	> �> �> �~> �> �~> �> �> �~> *�> 0�> 0�> *�> 0�> 0�> 0�> 0�> 0�> F�> $�> 0�> 0�> �> 0�> �> 0�> �> �> 0�> 0�> 0�> 0�> 0�> 0�> 0�> 0�> 0�> 0�> 0�> 0�> 0�> 0�> 0�> 0�> *�> *�> �> 0�> $�> 0�> F�> $�> 0�> 0�> �> 0�> �> 0�> 0�> �> k�> ��> ��> ��> ��> ��> ��> ��> ��> ��> ˏ> ��> ��> �> ��> ��> ��> �> '�> ��> ��> ��> ��> ��> ��> ��> ��> ��> ��> ��> ��> ��> ��> ��> ��> >�> U�> l�> ��> ��> ��> ��> ��> ��> ��> ��> ��> А> ��> ��> �>     memoryview(object)
--

Create a new memoryview object which references the given object.        release($self, /)
--

Release the underlying buffer exposed by the memoryview object.           tobytes($self, /, order=None)
--

Return the data in the buffer as a byte string. Order can be {'C', 'F', 'A'}.
When order is 'C' or 'F', the data of the original array is converted to C or
Fortran order. For contiguous views, 'A' returns an exact copy of the physical
memory. In particular, in-memory Fortran order is preserved. For non-contiguous
views, the data is converted to C first. order=None is the same as order='C'.      hex($self, /, sep=<unrepresentable>, bytes_per_sep=1)
--

Return the data in the buffer as a str of hexadecimal numbers.

  sep
    An optional single character or byte to separate hex bytes.
  bytes_per_sep
    How many bytes between separators.  Positive values count from the
    right, negative values count from the left.

Example:
>>> value = memoryview(b'\xb9\x01\xef')
>>> value.hex()
'b901ef'
>>> value.hex(':')
'b9:01:ef'
>>> value.hex(':', 2)
'b9:01ef'
>>> value.hex(':', -2)
'b901:ef'                tolist($self, /)
--

Return the data in the buffer as a list of elements.       cast($self, /, format, *, shape)
--

Cast a memoryview to a new format or