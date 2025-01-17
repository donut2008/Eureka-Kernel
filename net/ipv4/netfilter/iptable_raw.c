z$WindowsRegistryFinder._open_registryc                 C   s�   | j r| j}n| j}|j|dtjd d�  d�}z:| �|��}t�|d�}W d   � n1 s^0    Y  W n t	y~   Y d S 0 |S )Nz%d.%dr   )r�   Zsys_versionr(   )
�DEBUG_BUILD�REGISTRY_KEY_DEBUG�REGISTRY_KEYr>   r   �version_infor�   r�   Z
QueryValuer2   )r�   r�   Zregistry_keyr   Zhkey�filepathr   r   r   �_search_registry�  s    �.z&WindowsRegistryFinder._search_registryNc                 C   sx   | � |�}|d u rd S zt|� W n ty6   Y d S 0 t� D ]4\}}|�t|��r>tj||||�|d�}|  S q>d S )Nr�   )r�   r1   r2   r�   rn   ro   r�   �spec_from_loader)r�   r�   r,   �targetr�   r�   r�   r�   r   r   r   �	find_spec�  s    
�zWindowsRegistryFinder.find_specc                 C   s"   | � ||�}|dur|jS dS dS )zlFind module named in the registry.

        This method is deprecated.  Use exec_module() instead.

        N�r�   r�   �r�   r�   r,   r�   r   r   r   �find_module�  s    z!WindowsRegistryFinder.find_module)NN)N)r}   r|   r~   r   r�   r�   r�   �classmethodr�   r�   r�   r�   r   r   r   r   r�   �  s   ��

r�   c                   @   s0   e Zd ZdZdd� Zdd� Zdd� Zdd	� Zd
S )�_LoaderBasicszSBase class of common code needed by both SourceLoader and
    SourcelessFileLoader.c                 C   s@   t | �|��d }|�dd�d }|�d�d }|dko>|dkS )z�Concrete implementation of InspectLoader.is_package by checking if
        the path returned by get_filename has a filename of '__init__.py'.r'   rG   rI   r   �__init__)r/   r�   r+   r)   )rv   r�   r`   Zfilename_baseZ	tail_namer   r   r   r�     s    z_LoaderBasics.is_packagec                 C   s   dS �z*Use default semantics for module creation.Nr   �rv   r�   r   r   r   �create_module  s    z_LoaderBasics.create_modulec                 C   s8   | � |j�}|du r$td�|j���t�t||j� d