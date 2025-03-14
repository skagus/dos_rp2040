
MEMORY {
	FLASH(rx) : ORIGIN = 0x10000000, LENGTH = 128K
	
	/*
	RAM(rwx) : ORIGIN =  0x20000000, LENGTH = 256K 
	*/
	FS_AREA(rwx) : ORIGIN = 0x10020000, LENGTH = 1M

	RAM(rwx) : ORIGIN =  0x20000000, LENGTH = 128K 
	RAM_GUEST(rwx) : ORIGIN =  0x20020000, LENGTH = 128K 
}

/* LittleFS 전용 영역 할당 */
PROVIDE(__flash_fs_start = ORIGIN(FS_AREA));  /* 64KB 예약 */
PROVIDE(__flash_fs_size = LENGTH(FS_AREA));                  /* 256KB 파일 시스템 */
