Nr�   r   z; {!r} is not a packager   �   zCannot set an attribute on z for child module )r�   r   r]   rD   r�   rk   �_ERR_MSGr.   �ModuleNotFoundErrorr�   r�   r   r�   r�   r�   )	r   �import_r�   r�   Zparent_moduler�   r`   ra   Zchildr
   r
   r   �_find_and_load_unlocked�  s4    







r�   c                 C   s�   t | ��> tj�| t�}|tu r8t| |�W  d  � S W d  � n1 sL0    Y  |du rtd�| �}t|| d��t| � |S )zFind and load the module.Nz(import of {} halted; None in sys.modulesr   )	r3   r   r]   r#   �_NEEDS_LOADINGr�   r.   r�   rB   )r   r�   ra   rL   r
   r
   r   �_find_and_