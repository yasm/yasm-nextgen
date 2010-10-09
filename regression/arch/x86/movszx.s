.code64
movsbw %cl, %cx		# out: 66 0f be c9
movsbl %cl, %ecx	# out: 0f be c9
movswl %cx, %ecx	# out: 0f bf c9
movsbq %cl, %rcx	# out: 48 0f be c9
movswq %cx, %rcx	# out: 48 0f bf c9
movslq %ecx, %rcx	# out: 48 63 c9

movsx %cl, %cx		# out: 66 0f be c9
movsx %cl, %ecx		# out: 0f be c9
movsx %cx, %ecx		# out: 0f bf c9
movsx %cl, %rcx		# out: 48 0f be c9
movsx %cx, %rcx		# out: 48 0f bf c9
movsx %ecx, %rcx	# out: 48 63 c9

movzbw %cl, %cx		# out: 66 0f b6 c9
movzbl %cl, %ecx	# out: 0f b6 c9
movzwl %cx, %ecx	# out: 0f b7 c9
movzbq %cl, %rcx	# out: 48 0f b6 c9
movzwq %cx, %rcx	# out: 48 0f b7 c9

movzx %cl, %cx		# out: 66 0f b6 c9
movzx %cl, %ecx		# out: 0f b6 c9
movzx %cx, %ecx		# out: 0f b7 c9
movzx %cl, %rcx		# out: 48 0f b6 c9
movzx %cx, %rcx		# out: 48 0f b7 c9
