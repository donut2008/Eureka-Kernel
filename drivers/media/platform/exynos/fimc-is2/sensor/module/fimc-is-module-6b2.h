mporter.find_specc                 C   s   t �|�r| S dS )z]Find a frozen module.

        This method is deprecated.  Use find_spec() instead.

        N)r:   rY   )r�   rR   r�   r
   r
   r   r�   2  s    zFrozenImporter.find_modulec                 C   s   dS )z*Use default semantics for module creation.Nr
   )r�   r`   r
   r
   r   r�   ;  s    zFrozenImporter.create_modulec                 C   s@   | j j}t�|�s$td�|�|d��ttj|�}t|| j	� d S rX   )
rj   r   r:   rY   rP   r.   rD   �get_frozen_object�execr   )ra   r   �coder
   r
   r   r�   ?  