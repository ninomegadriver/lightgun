/* s16fd.c modified to support s24

this could get messy if games change their own code after initial loading as we'll have to invalidate caches etc.


*/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "machine/fd1094.h"

extern UINT16 *s24_mainram1;

#define S16_NUMCACHE 8

static unsigned char *s24_fd1094_key; // the memory region containing key
static UINT16 *s24_fd1094_cpuregion; // the CPU region with encrypted code
static UINT32  s24_fd1094_cpuregionsize; // the size of this region in bytes

static UINT16* s24_fd1094_userregion; // a user region where the current decrypted state is put and executed from
static UINT16* s24_fd1094_cacheregion[S16_NUMCACHE]; // a cache region where S16_NUMCACHE states are stored to improve performance
static int fd1094_cached_states[S16_NUMCACHE]; // array of cached state numbers
static int fd1094_current_cacheposition; // current position in cache array

/* this function checks the cache to see if the current state is cached,
   if it is then it copies the cached data to the user region where code is
   executed from, if its not cached then it gets decrypted to the current
   cache position using the functions in s24_fd1094.c */
void s24_fd1094_setstate_and_decrypt(int state)
{
	int i;
	UINT32 addr;

	cpunum_set_info_int(1, CPUINFO_INT_REGISTER + M68K_PREF_ADDR, 0x0010);	// force a flush of the prefetch cache

	/* set the s24_fd1094 state ready to decrypt.. */
	state = fd1094_set_state(s24_fd1094_key,state);

	/* first check the cache, if its cached we don't need to decrypt it, just copy */
	for (i=0;i<S16_NUMCACHE;i++)
	{
		if (fd1094_cached_states[i] == state)
		{
			/* copy cached state */
			s24_fd1094_userregion=s24_fd1094_cacheregion[i];
			memory_set_decrypted_region(1, 0, s24_fd1094_cpuregionsize - 1, s24_fd1094_userregion);
			m68k_set_encrypted_opcode_range(1,0,s24_fd1094_cpuregionsize);

			return;
		}
	}

// printf("new state %04x\n",state);

	/* mark it as cached (because it will be once we decrypt it) */
	fd1094_cached_states[fd1094_current_cacheposition]=state;

	for (addr=0;addr<s24_fd1094_cpuregionsize/2;addr++)
	{
		UINT16 dat;
		dat = fd1094_decode(addr,s24_fd1094_cpuregion[addr],s24_fd1094_key,0);
		s24_fd1094_cacheregion[fd1094_current_cacheposition][addr]=dat;
	}

	/* copy newly decrypted data to user region */
	s24_fd1094_userregion=s24_fd1094_cacheregion[fd1094_current_cacheposition];
	memory_set_decrypted_region(1, 0, s24_fd1094_cpuregionsize - 1, s24_fd1094_userregion);
	m68k_set_encrypted_opcode_range(1,0,s24_fd1094_cpuregionsize);

	fd1094_current_cacheposition++;

	if (fd1094_current_cacheposition>=S16_NUMCACHE)
	{
		printf("out of cache, performance may suffer, incrase S16_NUMCACHE!\n");
		fd1094_current_cacheposition=0;
	}
}

/* Callback for CMP.L instructions (state change) */
void s24_fd1094_cmp_callback(unsigned int val, int reg)
{
	if (reg == 0 && (val & 0x0000ffff) == 0x0000ffff) // ?
	{
		s24_fd1094_setstate_and_decrypt((val & 0xffff0000) >> 16);
	}
}

/* Callback when the s24_fd1094 enters interrupt code */
int s24_fd1094_int_callback (int irq)
{
	s24_fd1094_setstate_and_decrypt(FD1094_STATE_IRQ);
	return (0x60+irq*4)/4; // vector address
}

void s24_fd1094_rte_callback (void)
{
	s24_fd1094_setstate_and_decrypt(FD1094_STATE_RTE);
}


/* KLUDGE, set the initial PC / SP based on table as we can't decrypt them yet */
void s24_fd1094_kludge_reset_values(void)
{
	int i;

	for (i = 0;i < 4;i++)
		s24_fd1094_userregion[i] = fd1094_decode(i,s24_fd1094_cpuregion[i],s24_fd1094_key,1);
}


/* function, to be called from MACHINE_RESET (every reset) */
void s24_fd1094_machine_init(void)
{
	/* punt if no key; this allows us to be called even for non-s24_fd1094 games */
	if (!s24_fd1094_key)
		return;

	s24_fd1094_setstate_and_decrypt(FD1094_STATE_RESET);
	s24_fd1094_kludge_reset_values();

	cpunum_set_info_fct(1, CPUINFO_PTR_M68K_CMPILD_CALLBACK, (genf *)s24_fd1094_cmp_callback);
	cpunum_set_info_fct(1, CPUINFO_PTR_M68K_RTE_CALLBACK, (genf *)s24_fd1094_rte_callback);
	cpunum_set_irq_callback(1, s24_fd1094_int_callback);
}

/* startup function, to be called from DRIVER_INIT (once on startup) */
void s24_fd1094_driver_init(void)
{
	int i;

	s24_fd1094_cpuregion = (UINT16*)s24_mainram1;
	s24_fd1094_cpuregionsize = 0x40000;
	s24_fd1094_key = memory_region(REGION_USER3);

	/* punt if no key; this allows us to be called even for non-s24_fd1094 games */
	if (!s24_fd1094_key)
		return;

	for (i=0;i<S16_NUMCACHE;i++)
	{
		s24_fd1094_cacheregion[i]=auto_malloc(s24_fd1094_cpuregionsize);
	}

	/* flush the cached state array */
	for (i=0;i<S16_NUMCACHE;i++)
		fd1094_cached_states[i] = -1;

	fd1094_current_cacheposition = 0;
}
