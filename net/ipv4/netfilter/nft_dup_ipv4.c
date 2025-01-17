 H  s    zFrozenImporter.load_modulec                 C   s
   t �|�S )z-Return the code object for the frozen module.)r:   r�   r�   r
   r
   r   r�   Q  s    zFrozenImporter.get_codec                 C   s   dS )z6Return None as frozen modules do not have source code.Nr
   r�   r
   r
   r   r�   W  s    zFrozenImporter.get_sourcec                 C   s
   t �|�S )z.Return True if the frozen module is a package.)r:   Zis_frozen_packager�   r
   r
   r   rt   ]  s    zFrozenImporter.is_package)NN)N)r   r    r   r   r�   r�   rd   r�   r�   r�   r�   r�   r�   r[   r�   r�   rt   r
   r
   r
   r   r�     s.   



r�   c                   @   s    e Zd ZdZdd� Zdd� ZdS )�_ImportLockContextz$Context manager for the import lock.c                 C   s   t ��  dS )zAcquire the import lock.N)r:   r;   r0   r
   r
   r   r7   j  s    z_ImportLockContext.__enter__c                 C   s   t ��  dS )z<Release the import lock regardless of any raised exceptions.N)r:   r=   )r   �exc_type�	exc_value�exc_tracebackr
   r
   r   r9   n  s    z_ImportLockContext.__exit__N)r   r    r   r   r7   r9   r
   r
   r
   r   r�   f  s   r�   c                 C   s@   |� d|d �}t|�|k r$td��|d }| r<d�|| �S |S )z2Resolve a relative module name to an absolute one.r�   r'   z2attempted relative import beyond top-level packager   �{}.{})�rsplit�lenrP   r.   )r   �package�levelZbitsZbaser
   r
   r   �_resolve_names  s
    r�   c                 C   s"   | � ||�}|d u rd S t||�S r   )r�   r\   )�finderr   r�   rn   r
   r
   r   �_find_spec_legacy|  s    r�   c           
   
   C   s   t j}|du rtd��|s&t�dt� | t jv }|D ]�}t� �^ z
|j}W n6 t	y�   t
|| |�}|du r|Y W d  � q4Y n0 || ||�}W d  � n1 s�0    Y  |dur4|�s| t jv �rt j|  }z
|j}	W n t	y�   | Y   S 0 |	du �r|  S |	  S q4|  S q4dS )zFind a module's spec.Nz5sys.meta_path is None, Python is likely shutting downzsys.meta_path is empty)r   �	meta_pathrP   �	_warnings�warn�ImportWarningr]   r�   r�   rk   r�   rj   )
r   r�   r�   r�   Z	is_reloadr�   r�   r`   ra   rj   r
   r
   r   �
_find_spec�  s6    

*




r�   c                 C   sl   t | t�std�t| ����|dk r,td��|dkrTt |t�sHtd��n|sTtd��| sh|dkrhtd��dS )zVerify arguments are "sane".zmodule name must be str, not {}r   zlevel must be >= 0z__package__ not set to a stringz6attempted relative import with no known parent packagezEmpty module nameN)�
isinstance�str�	TypeErrorr.   r   �
ValueErrorrP   �r   r�   r�   r
   r
   r   �_sanity_check�  s    


r�   zNo module named z{!r}c           	      C   s  d }| � d�d }|r�|tjvr*t||� | tjv r>tj