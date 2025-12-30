#include "cmn.h"
#include "eip130_drv.h"
static int trng_cmd[] = {
0x04044f65, 0x00000000, 0x00000010, 0x15e3e620, 0x00000000
};
static int trng_test_len[]= {
0x10,0x20,0x190,0x404,5,6,7,8
};
static unsigned int trng_cfg_cmd[]={
0x1402e6f5 ,0x4f5a3647 ,0x00000003 ,0x00030801
};
static int result_token[64];
static int data[0x1000] = {
};

static void mem_set(int *data, int size, int val)
{
	int i;
	int cnt =  (size + 3)/ 4;
	for (i = 0; i < cnt; i++) {
		data[i] = val;
	}
}

static int random_get(int *data, int size)
{
	long data_addr;
	data_addr = (long)data;
	mem_set(data, size, 0);
	trng_cmd[2] = size;
	trng_cmd[3] = data_addr & 0xffffffff;
	trng_cmd[4] = data_addr >> 32;
	eip130_physical_token_exchange((unsigned int *)trng_cmd, (unsigned int *)result_token, 1);
	if(result_token[0] & (1 << 31)){
		return -1;
	}
	return 0;
}
static int trng_init()
{
        int ret = 0;
        ret = eip130_physical_token_exchange((unsigned int *)trng_cfg_cmd, (unsigned int *)result_token, 1);
        return ret;
}

int trng_test()
{
	int ret = 0;
	int i;
	trng_init();
	for(i = 0; i < (sizeof(trng_test_len) / sizeof(trng_test_len[0])); i++) {
		ret = random_get(data, trng_test_len[i]);
		if(ret < 0) {
			while(1);
		}
	}
	return ret;
}
