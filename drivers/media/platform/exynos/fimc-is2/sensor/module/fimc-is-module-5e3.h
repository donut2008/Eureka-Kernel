 )r   r   �
rpartitionr#   �reversed�rsplit)�pathZfront�_�tailr   r   r   r   �_path_splitD   s    r/   c                 C   s
   t �| �S )z~Stat the path.

    Made a separate function to make it easier to override in experiments
    (e.g. cache stat results).

    )r   Zstat�r,   r   r   r   �
_path_statP   s    r1   c                 C   s0   zt | �}W n ty    Y dS 0 |jd@ |kS )z1Test whether the path is the specified mode type.Fi �  )r1   �OSError�st_mode)r,   �modeZ	stat_infor   r   r   �_path_is_mode_typeZ   s
    r5   c   