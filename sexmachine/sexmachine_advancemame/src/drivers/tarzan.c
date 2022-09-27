
/* Old IGS game called Tarzan, don't know much about it, could be gambling */
/* CPU might not be z80 */

#include "driver.h"

#if 0
static unsigned short *file1_data;
static int file1_size;

static unsigned short *result_data;

static int decrypt_tarzan()
{
  int fd;
  int i;
  char msg[512];

  if(argc != 3) {
    fprintf(stderr, "Usage:\n%s <file1.bin> <result.bin>\n", argv[0]);
    exit(1);
  }

  sprintf(msg, "Open %s", argv[1]);
  fd = open(argv[1], O_RDONLY);
  if(fd<0) {
    perror(msg);
    exit(2);
  }

  file1_size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  file1_data = malloc(file1_size);
  read(fd, file1_data, file1_size);
  close(fd);

  result_data = malloc(file1_size);
  for(i=0; i<file1_size/2; i++) {
    unsigned short x = file1_data[i];

    if((i & 0x10c0) == 0x0000)
      x ^= 0x0001;

    if((i & 0x0010) == 0x0010 || (i & 0x0130) == 0x0020)
      x ^= 0x0404;

    if((i & 0x00d0) != 0x0010)
      x ^= 0x1010;

    if(((i & 0x0008) == 0x0008)^((i & 0x10c0) == 0x0000))
      x ^= 0x0100;

    result_data[i] = x;
  }

  sprintf(msg, "Open %s", argv[2]);
  fd = open(argv[2], O_RDWR|O_CREAT|O_TRUNC, 0666);
  if(fd<0) {
    perror(msg);
    exit(2);
  }

  write(fd, result_data, file1_size);
  close(fd);

  return 0;
}
#endif


VIDEO_START(tarzan)
{
	return 0;
}

VIDEO_UPDATE(tarzan)
{

}

static ADDRESS_MAP_START( tarzan_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END


INPUT_PORTS_START( tarzan )
INPUT_PORTS_END


static MACHINE_DRIVER_START( tarzan )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000)		 /* ? */
	MDRV_CPU_PROGRAM_MAP(tarzan_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(tarzan)
	MDRV_VIDEO_UPDATE(tarzan)
MACHINE_DRIVER_END



ROM_START( tarzan )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )
	ROM_LOAD( "0228-u16.bin", 0x00000, 0x040000, CRC(e6c552a5) SHA1(f156de9459833474c85a1f5b35917881b390d34c)  )

	ROM_REGION( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "0228-u21.bin", 0x00000, 0x080000, CRC(80aaece4) SHA1(07cad92492c5de36c3915867ed4c6544b1a30c07)  )

	ROM_REGION( 0x080000, REGION_USER2, 0 )
	ROM_LOAD( "0228-u6.bin", 0x00000, 0x080000, CRC(55e94832) SHA1(b15409f4f1264b6d1218d5dc51c5bd1de2e40284)  )

ROM_END


GAME( 199?, tarzan, 0, tarzan, tarzan, 0, ROT0, "IGS", "Tarzan",GAME_NOT_WORKING|GAME_NO_SOUND )
