| |d�S |r8g nd}|| ||d�S |du r�t |d�r�z|�| �}W q� ty�   d}Y q�0 nd}t| |||d�S )z5Return a module spec based on various loader methods.Zget_filenameN)rn   )rn   ru   rt   Fr�   )r   r   r�   �spec_from_file_locationrt   rP   rq   )r   rn   rr   rt   r�   Zsearchr
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
|� W n0   ztj| j= W n ty�   Y n0 � Y n0 tj�| j�}|tj| j< td| j| j � W d| _nd| _0 |S )Nr�   Tr�   r   zimport {!r} # {!r}F)rn   r   r�   r�   Z_initializingr   r]   r   ru   rP   r�   r@   r�   rM   r�   r
   r
   r   �_load_unlocked�  s.    


r�   c                 C   s6   t | j�� t| �W  d  � S 1 s(0    Y  dS )z�Return a new module object, loaded by the spec's loader.

    The module is not added to its parent.

    If a module is already in sys.modules, that existing module gets
    clobbered.

    N)r3   r   r�   )r`   r
   r
   r   r_   �  s    	r_   c                   @   s�   e Zd ZdZdZedd� �Zeddd��Zeddd	��Z	ed
d� �Z
edd� �Zeedd� ��Zeedd� ��Zeedd� ��Zee�ZdS )�BuiltinImporterz�Meta path import for built-in modules.

    All methods are either class or static methods to avoid the need to
    instantiate the class.

    zbuilt-inc                 C   s   d| j �dtj� d�S )�sReturn repr for the module.

        The method is deprecated.  The import machinery does the job itself.

        z<module z (z)>)r   r�   r�   )ra   r
   r
   r   rd   �  s    zBuiltinImporter.module_reprNc                 C   s.   