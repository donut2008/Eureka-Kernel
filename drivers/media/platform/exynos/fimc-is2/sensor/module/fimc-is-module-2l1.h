ace.

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
r�   c                 C   s&   t | |� tj�t� tj�t� dS 