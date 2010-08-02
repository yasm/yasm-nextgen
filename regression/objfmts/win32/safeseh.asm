; [oformat win32]
MyHandler:
	ret
[safeseh MyHandler]
[extern MyHandler3]
[safeseh MyHandler3]
